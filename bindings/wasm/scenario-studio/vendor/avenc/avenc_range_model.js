import { AvencModel } from './avenc_model.js';
import { decompressSync } from '../fflate.js';
import { decodeValue, streamGeometry, lumaStorageCode } from './avenc_met_core.js';

// Strict byte-range transport. Never fall back to a whole-file HTTP 200.
export class ByteRanges {
  constructor(url,onProgress=()=>{}){this.url=url;this.bytes=0;this.requests=0;this.size=0;this.onProgress=onProgress;}
  async read(start,end,signal){
    signal?.throwIfAborted();
    const r=await fetch(this.url,{headers:{Range:`bytes=${start}-${end}`},signal});
    const m=(r.headers.get('Content-Range')||'').match(/^bytes (\d+)-(\d+)\/(\d+)$/i);
    if(r.status!==206||!m||+m[1]!==start||+m[2]!==end){await r.body?.cancel();throw new Error('The MP4 host must return HTTP 206 byte ranges. Use python3 www/serve.py locally; for remote files expose Content-Range through CORS.');}
    const length=end-start+1;
    if(length>64*1024*1024){await r.body?.cancel();throw new Error('A single requested range exceeds the 64 MB safety limit.');}
    const out=new Uint8Array(length);let offset=0;
    const reader=r.body.getReader();
    try {while(true){const {done,value}=await reader.read();if(done)break;if(offset+value.length>length)throw new Error('Oversized range response');out.set(value,offset);offset+=value.length;}}
    catch(e){await reader.cancel().catch(()=>{});throw e;}
    if(offset!==length)throw new Error('Truncated MP4 byte range');
    this.size=+m[3];this.bytes+=offset;this.requests++;this.onProgress(this);
    return out;
  }
  async index(signal){
    let pos=0,ftyp=null,moov=null;
    for(let i=0;i<100;i++){
      const h=await this.read(pos,pos+15,signal),d=new DataView(h.buffer);
      let n=d.getUint32(0);const type=new TextDecoder().decode(h.subarray(4,8));
      if(n===1)n=Number(d.getBigUint64(8));if(n===0)n=this.size-pos;
      if(!Number.isSafeInteger(n)||n<8||pos+n>this.size)throw new Error('Invalid MP4 top-level box');
      if(type==='moof')throw new Error('Fragmented MP4 is not supported by the map viewer.');
      if(type==='ftyp'||type==='moov'){
        const b=await this.read(pos,pos+n-1,signal);
        if(type==='ftyp')ftyp=b;else moov=b;
      }
      if(ftyp&&moov){const b=new Uint8Array(ftyp.length+moov.length);b.set(ftyp);b.set(moov,ftyp.length);return {index:b.buffer,metadata:moov};}
      pos+=n;if(pos>=this.size)break;
    }
    throw new Error('No standard MP4 movie index found');
  }
}

export class AvencRangeModel extends AvencModel {
  constructor({assumeGlobal=false,applyBitmap=true}={}){super([]);this.assumeGlobal=assumeGlobal;this.applyBitmap=applyBitmap;this.transport=null;this.gridCache=new Map();this.packetCache=new Map();this.packetBytes=0;this.queue=Promise.resolve();}
  async open(url,signal,onProgress){
    this.close();this.gridCache.clear();this.packetCache.clear();this.packetBytes=0;
    this.transport=new ByteRanges(url,onProgress);this.currentDataset=url;
    const {index,metadata}=await this.transport.index(signal);
    this.metadata=this._extractAvencMetadata(metadata);
    if(this.metadata.schema!=='avenc.pack.v2.binary')throw new Error('The weather explorer currently requires native AVENC v2 metadata. Use viewer.html for other formats.');
    this.recon=this._extractReconPayload(this.metadata);
    this.groups=this.metadata.groups;this._attachRecordsFallback();this._decodeCoords();
    const info=await this._parseMp4Info(index);signal?.throwIfAborted();
    // MP4Box omits unrecognized FFV1 entries from info.videoTracks. AVENC
    // stream indices still include them: use every vide handler, preserving order.
    this.videoTracks=info.tracks.filter(t=>this._mp4File.getTrackById(t.id)?.mdia?.hdlr?.handler==='vide').sort((a,b)=>a.id-b.id);
    this.fileSizeBytes=this.transport.size;
    this.geometry=this.geometryForGroup(this.groups[0]);
    // No media payload has been downloaded at this point.
    return this;
  }
  geometryForGroup(group){return group._mapGeometry ||= streamGeometry(group,{assumeGlobal:this.assumeGlobal});}
  async supported(group){
    const track=this.videoTracks[Number(group.stream_index)];
    if(!globalThis.VideoDecoder||!track)return false;
    const config={codec:track.codec,codedWidth:track.video?.width||group.options.encoded_width,codedHeight:track.video?.height||group.options.encoded_height};
    const description=this._getDecoderDescription(this._mp4File,track.id);if(description)config.description=description;
    try{return (await VideoDecoder.isConfigSupported(config)).supported;}catch{return false;}
  }
  sampleWindow(group,frame){
    const track=this.videoTracks[Number(group.stream_index)];
    const samples=this._mp4File.getTrackById(track.id).samples;
    const display=samples.slice().sort((a,b)=>a.cts-b.cts||a.number-b.number);
    const target=display[frame];if(!target)throw new Error('Missing frame in MP4 sample table');
    let first=target.number;while(first>0&&!samples[first].is_sync)first--;
    let end=target.number+1;while(end<samples.length&&!samples[end].is_sync)end++;
    return {samples,target,first,end};
  }
  async packets(group,frame,signal){
    const w=this.sampleWindow(group,frame),key=`${group.id}:${w.first}:${w.end}`;
    if(this.packetCache.has(key))return {...w,packets:this.packetCache.get(key)};
    // Merge only adjacent samples from this stream; never span interleaved tracks.
    const chunks=w.samples.slice(w.first,w.end).slice().sort((a,b)=>a.offset-b.offset),runs=[];
    for(const s of chunks){const prev=runs.at(-1);if(prev&&prev.end===s.offset){prev.end+=s.size;prev.samples.push(s);}else runs.push({start:s.offset,end:s.offset+s.size,samples:[s]});}
    const total=chunks.reduce((n,s)=>n+s.size,0);if(total>64*1024*1024)throw new Error('This GOP exceeds the 64 MB media budget.');
    const packets=new Map();
    for(const run of runs){const b=await this.transport.read(run.start,run.end-1,signal);for(const s of run.samples)packets.set(s.number,b.subarray(s.offset-run.start,s.offset-run.start+s.size));}
    signal?.throwIfAborted();
    while(this.packetBytes+total>32*1024*1024&&this.packetCache.size){const k=this.packetCache.keys().next().value;const old=this.packetCache.get(k);this.packetBytes-=[...old.values()].reduce((n,b)=>n+b.byteLength,0);this.packetCache.delete(k);}
    if(total<=32*1024*1024){this.packetCache.set(key,packets);this.packetBytes+=total;}
    return {...w,packets};
  }
  async _decodeFrame(frameIndex,signal){
    const group=this.currentGroup,track=this.videoTracks[this.decodedStreamIndex];
    if(!await this.supported(group))throw new Error(`This browser cannot decode ${track.codec}. FFV1 and some 12/16-bit profiles need native AVENC tools; choose a supported AV1/HEVC field.`);
    const w=await this.packets(group,frameIndex,signal);signal.throwIfAborted();
    const saved={samples:this._samples,display:this._displaySamples,buffer:this.arrayBuffer};
    const mini=w.samples.slice(w.first,w.end).map((s,i)=>({...s,number:i,data:w.packets.get(s.number)}));
    // AV1 WebCodecs ignores description. MP4 stores the sequence header in
    // av1C; later keyframes can omit it. Restore that header at a range seek.
    if(String(track.codec).startsWith('av01')){
      const obu=this._mp4File.getTrackById(track.id)?.mdia?.minf?.stbl?.stsd?.entries?.[0]?.av1C?.configOBUs;
      if(obu?.length){const data=new Uint8Array(obu.length+mini[0].data.length);data.set(obu);data.set(mini[0].data,obu.length);mini[0].data=data;mini[0].size=data.length;}
    }
    const total=mini.reduce((n,s)=>n+s.size,0),buffer=new Uint8Array(total);let off=0;
    for(const s of mini){buffer.set(s.data,off);s.offset=off;off+=s.size;}
    this._samples=mini;this._displaySamples=mini.slice().sort((a,b)=>a.cts-b.cts||a.number-b.number);this.arrayBuffer=buffer.buffer;
    const local=this._displaySamples.findIndex(s=>s.number===w.target.number-w.first);
    try{
      try{
        const result=await super._decodeFrame(local,signal),format=result.frame.format||'';
        const nativeBits=Number(format.match(/P(\d+)/)?.[1]||8);
        const readable=format==='NV12'||/^I(420|422|444)(P(10|12|16))?$/.test(format);
        if((!readable||nativeBits<Number(group.options.bits))&&this.decodeHardwareAcceleration!=='prefer-software'){
          // Some GPU decoders return opaque surfaces (format=null). Obtain
          // readable native-depth planes via software, never an RGB guess.
          result.frame.close();this.decodeHardwareAcceleration='prefer-software';
          return await super._decodeFrame(local,signal);
        }
        return result;
      }
      catch(e){signal.throwIfAborted();if(this.decodeHardwareAcceleration==='prefer-software')throw e;this.decodeHardwareAcceleration='prefer-software';return await super._decodeFrame(local,signal);}
    }
    finally{this._samples=saved.samples;this._displaySamples=saved.display;this.arrayBuffer=saved.buffer;}
  }
  getGrid(group,frame,signal){
    const key=`${group.id}:${frame}`;
    const geometry=this.geometryForGroup(group);
    if(this.gridCache.has(key))return Promise.resolve(this.gridCache.get(key));
    const work=async()=>{
      signal?.throwIfAborted();if(this.gridCache.has(key))return this.gridCache.get(key);
      await this.selectGroup(group.id);
      const abort=()=>this._decodeRequest?.controller.abort();signal?.addEventListener('abort',abort,{once:true});
      try{
        const fr=await this._ensureDecodedFrame(frame);signal?.throwIfAborted();
        const format=fr.frame.format||'';if(format!=='NV12'&&!/^I(420|422|444)(P(10|12|16))?$/.test(format))throw new Error(`Raw luma format ${format||'unavailable'} cannot be reconstructed safely. No RGB approximation is used.`);
        const nativeBits=Number(format.match(/P(\d+)/)?.[1]||8),targetBits=Number(group.options.bits);
        if(nativeBits<targetBits)throw new Error(`Decoder exposed only ${nativeBits} of ${targetBits} scientific bits.`);
        const bytes=new Uint8Array(fr.frame.allocationSize());const layout=await fr.frame.copyTo(bytes);
        const data=new DataView(bytes.buffer),o=group.options,nx=Number(o.crop_width),ny=Number(o.crop_height),values=new Float32Array(nx*ny);
        if(nx!==geometry.nx||ny!==geometry.ny)throw new Error('This stream has inconsistent grid dimensions.');
        let mask=null;
        if(o.bitmap_payload){mask=group._mapBitmap ||=decompressSync(Uint8Array.from(atob(o.bitmap_payload),c=>c.charCodeAt(0)));if(mask.length!==nx*ny)throw new Error('Invalid shared bitmap size');}
        const rec=this._getFrameRecord(frame),hasBitmap=Boolean(mask||rec.bitmapPresent),bounds=this._getFrameQuantBounds(frame);
        for(let j=0;j<ny;j++)for(let i=0;i<nx;i++){
          const k=j*nx+i,p=layout[0].offset+j*layout[0].stride+i*(nativeBits>8?2:1);
          const raw=nativeBits>8?data.getUint16(p,true):data.getUint8(p);
          // Native decoder reads gray16 for shifted legacy streams. WebCodecs
          // exposes 10/12-bit right-aligned codes instead; restore that storage
          // representation before applying AVENC's recorded storage shift.
          const stored=lumaStorageCode(raw,nativeBits,targetBits+Number(o.storage_shift||0),fr.frame.colorSpace?.fullRange);
          values[k]=decodeValue(stored,targetBits,bounds,o,hasBitmap,this.applyBitmap&&Boolean(mask?.[k]));
        }
        while(this.gridCache.size>=6)this.gridCache.delete(this.gridCache.keys().next().value);
        this.gridCache.set(key,values);return values;
      }finally{signal?.removeEventListener('abort',abort);}
    };
    const job=this.queue.then(work,work);this.queue=job.catch(()=>{});return job;
  }
}
