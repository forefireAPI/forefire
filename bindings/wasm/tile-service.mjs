/** Browser/Node transport for the same fixed-domain .fftile loadData contract.
 * Fetch compressed files asynchronously, stage them in MEMFS, then let the
 * C++ tile provider decode and page them. No JS image decoder or NetCDF.
 */
export const METERS = 111195;
const radians = Math.PI / 180;
export const wrapLongitude = lon => ((lon + 180) % 360 + 360) % 360 - 180;
export function tileServiceGeometry({center, size = [2, 2]}) {
  const [lon, lat] = center ?? [], [w, h] = size;
  if (![lon, lat, w, h].every(Number.isFinite) || lon < -180 || lon >= 180 ||
      w <= 0 || w > 2 || h <= 0 || h > 2 || lat-h/2 <= -90 || lat+h/2 >= 90)
    throw new Error('Choose a valid center and a domain size between 0 and 2 degrees.');
  const width = w * METERS * Math.min(Math.cos((lat-h/2)*radians), Math.cos((lat+h/2)*radians));
  const height = h * METERS;
  const geographic = (x, y) => {
    const latitude = lat + (y-height/2)/METERS;
    return [wrapLongitude(lon + (x-width/2)/(METERS*Math.cos(latitude*radians))), latitude];
  };
  let west = Infinity, east = -Infinity;
  for (const x of [0, width]) for (const y of [0, height]) {
    const [longitude] = geographic(x,y);
    const unwrapped = lon + wrapLongitude(longitude-lon);
    west = Math.min(west,unwrapped); east = Math.max(east,unwrapped);
  }
  const tiles = [];
  const firstLat = Math.floor((lat-h/2+90)*2), lastLat = Math.ceil((lat+h/2+90)*2)-1;
  const firstLon = Math.floor((west+180)*2), lastLon = Math.ceil((east+180)*2)-1;
  for (let li=firstLat; li<=lastLat; li++) for (let lj=firstLon; lj<=lastLon; lj++) {
    const wrapped = (lj%720+720)%720;
    tiles.push({latIndex:li,lonIndex:wrapped,name:`${li/2-90}_${wrapped/2-180}.fftile`});
  }
  return {width,height,center:[lon,lat],size:[w,h],tiles,geographic};
}
function validate(bytes,tile) {
  if (bytes.length<256 || new TextDecoder().decode(bytes.subarray(0,8))!=='FFTILE01')
    throw new Error(`${tile.name}: invalid .fftile header`);
  const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
  if (view.getInt16(8,true)!==tile.latIndex || view.getInt16(10,true)!==tile.lonIndex)
    throw new Error(`${tile.name}: service returned a different tile`);
  const flags=view.getUint16(12,true);
  if (!(flags&1) || (flags&~3)) throw new Error(`${tile.name}: unsupported tile flags`);
  let end=256;
  for(let i=0;i<4;i++) {
    const at=view.getUint32(160+i*8,true),size=view.getUint32(164+i*8,true);
    if(at!==end || !size || at+size>bytes.length)throw new Error(`${tile.name}: invalid payload bounds`);
    end=at+size;
  }
  if(end!==bytes.length)throw new Error(`${tile.name}: unexpected trailing data`);
  const expected=view.getUint32(200,true);
  if(expected) {
    let crc=0xffffffff;
    for(let i=0;i<200;i++){crc^=bytes[i];for(let b=0;b<8;b++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
    if(((crc^0xffffffff)>>>0)!==expected)throw new Error(`${tile.name}: header CRC mismatch`);
  }
}
async function download(url, fetcher) {
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),30000);
  try {
    const response=await fetcher(url,{signal:controller.signal});
    if(response.status===404 || response.status===410)return null;
    if(!response.ok)throw new Error(`Tile service returned HTTP ${response.status}`);
    const limit=64*1024*1024;
    if(Number(response.headers.get('content-length'))>limit)throw new Error('Tile exceeds 64 MiB');
    if(!response.body)throw new Error('Tile service returned an empty response');
    const reader=response.body.getReader(),chunks=[];let length=0;
    while(true){const {done,value}=await reader.read();if(done)break;length+=value.byteLength;
      if(length>limit){await reader.cancel();throw new Error('Tile exceeds 64 MiB');}chunks.push(value);}
    const bytes=new Uint8Array(length);let at=0;
    for(const chunk of chunks){bytes.set(chunk,at);at+=chunk.length;}return bytes;
  } finally {clearTimeout(timer);}
}
export async function loadTileService(Module, ff, {provider,center,size=[2,2],onProgress=()=>{},fetcher=fetch}) {
  const url=new URL(provider);
  if(!['http:','https:'].includes(url.protocol) || url.search || url.hash)
    throw new Error('Use an HTTP(S) tile-directory URL without a query or fragment.');
  if(ff.tileDomain())throw new Error('Create a fresh WASM instance before loading another service.');
  const geometry=tileServiceGeometry({center,size});
  const root='/tile-service';Module.FS.mkdirTree(root);
  // One module per landscape. Its MEMFS is the compressed cache for that run.
  let next=0,available=0,missing=0,totalBytes=0;
  const results=[];
  await Promise.all(Array.from({length:Math.min(3,geometry.tiles.length)},async()=>{
    while(next<geometry.tiles.length) {
      const tile=geometry.tiles[next++];
      const tileURL=new URL(`${url.href.replace(/\/+$/,'')}/${tile.name}`);
      const bytes=await download(tileURL.href,fetcher);
      if(bytes){validate(bytes,tile);Module.FS.writeFile(`${root}/${tile.name}`,bytes);available++;totalBytes+=bytes.length;}
      else missing++;
      const entry={name:tile.name,available:!!bytes,bytes:bytes?.length??0};results.push(entry);
      onProgress({available,missing,bytes:totalBytes,total:geometry.tiles.length,tile:entry});
    }
  }));
  const [lon,lat]=center,[w,h]=size;
  const output=ff.execute(`loadData[provider=${root};center=(${lon},${lat});size=(${w},${h});prefetch=0]`);
  const domain=ff.tileDomain();
  if(!domain)throw new Error(output || 'The tile domain could not be created.');
  return {domain,available,missing,bytes:totalBytes,tiles:results,provider:url.href};
}
