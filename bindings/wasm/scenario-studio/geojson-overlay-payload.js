/* Lossless compact GeoJSON transport. Shared by Pyroscope and ForeFire. */
(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.GeoJSONOverlayPayload=api;})(globalThis,function(){
 'use strict';
 const MAX_WIRE=8*1024*1024,MAX_JSON=32*1024*1024,FORMAT='pyroscope-overlay',VERSION=1;
 function base64(bytes){let text='';for(let i=0;i<bytes.length;i+=16384)text+=String.fromCharCode(...bytes.subarray(i,i+16384));return btoa(text).replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,'');}
 function unbase64(text){if(typeof text!=='string'||text.length>MAX_WIRE||!/^[A-Za-z0-9_-]*={0,2}$/.test(text))throw Error('Invalid or oversized compact overlay.');const raw=atob(text.replace(/-/g,'+').replace(/_/g,'/'));return Uint8Array.from(raw,c=>c.charCodeAt(0));}
 async function collect(stream,limit){const reader=stream.getReader(),chunks=[];let size=0;try{while(true){const{done,value}=await reader.read();if(done)break;size+=value.length;if(size>limit)throw Error('GeoJSON overlay exceeds the size limit.');chunks.push(value);}}catch(e){await reader.cancel().catch(()=>{});throw e;}const bytes=new Uint8Array(size);let at=0;for(const b of chunks){bytes.set(b,at);at+=b.length;}return bytes;}
 async function pack(collection,{id,label='Forecast isochrones',kind='forecast'}={}){const raw=new TextEncoder().encode(JSON.stringify(collection));if(raw.length>MAX_JSON)throw Error('GeoJSON overlay exceeds 32 MiB.');let envelope;
  if(typeof CompressionStream==='function'){const bytes=await collect(new Blob([raw]).stream().pipeThrough(new CompressionStream('gzip')),MAX_WIRE);envelope={format:FORMAT,version:VERSION,encoding:'gzip-base64url',data:base64(bytes)};}
  else envelope={format:FORMAT,version:VERSION,encoding:'json',data:collection};
  envelope.id=id;envelope.label=String(label).slice(0,100);envelope.kind=String(kind).slice(0,40);if(new TextEncoder().encode(JSON.stringify(envelope)).length>MAX_WIRE)throw Error('Compact overlay exceeds 8 MiB. Download and import a smaller forecast.');return envelope;
 }
 async function unpack(input){let envelope=input;
  if(input instanceof ArrayBuffer||ArrayBuffer.isView(input)){const bytes=input instanceof ArrayBuffer?new Uint8Array(input):new Uint8Array(input.buffer,input.byteOffset,input.byteLength);if(bytes.length>MAX_WIRE)throw Error('Overlay file exceeds 8 MiB.');let decoded=bytes;if(bytes[0]===31&&bytes[1]===139){if(typeof DecompressionStream!=='function')throw Error('This browser cannot decode gzip overlays. Import plain GeoJSON.');decoded=await collect(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip')),MAX_JSON);}envelope=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(decoded));}
  else if(typeof input==='string'){if(new TextEncoder().encode(input).length>MAX_WIRE)throw Error('Overlay payload exceeds 8 MiB.');envelope=JSON.parse(input);}
  if(!envelope||typeof envelope!=='object')throw Error('Provide GeoJSON or a compact overlay envelope.');
  if(envelope.format!==FORMAT)return {geojson:envelope,options:{}};
  if(envelope.version!==VERSION)throw Error('Unsupported overlay version.');let geojson;
  if(envelope.encoding==='json')geojson=envelope.data;
  else if(envelope.encoding==='gzip-base64url'){if(typeof DecompressionStream!=='function')throw Error('This browser cannot decode gzip overlays. Import plain GeoJSON.');const bytes=unbase64(envelope.data),raw=await collect(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip')),MAX_JSON);geojson=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(raw));}
  else throw Error('Unsupported overlay compression. Use gzip-base64url or json.');
  return {geojson,options:{id:envelope.id,label:envelope.label,kind:envelope.kind}};
 }
 return {pack,unpack,base64,unbase64,MAX_WIRE,MAX_JSON,FORMAT,VERSION};
});
