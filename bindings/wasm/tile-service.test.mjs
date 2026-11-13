import test from 'node:test';
import assert from 'node:assert/strict';
import {tileServiceGeometry,loadTileService} from './tile-service.mjs';
function packet(tile){
  const bytes=new Uint8Array(260),v=new DataView(bytes.buffer);bytes.set(new TextEncoder().encode('FFTILE01'));
  v.setInt16(8,tile.latIndex,true);v.setInt16(10,tile.lonIndex,true);v.setUint16(12,1,true);
  for(let i=0;i<4;i++){v.setUint32(160+i*8,256+i,true);v.setUint32(164+i*8,1,true);}
  return bytes; // Transport-only fixture; C++ codec validation has its own tests.
}
function host(){let domain=null;const files=new Map();return {
 Module:{FS:{mkdirTree(){},writeFile:(path,bytes)=>files.set(path,bytes)}},
 ff:{tileDomain:()=>domain,execute:command=>{assert.match(command,/loadData\[provider=\/tile-service;center=/);domain={width:100,height:100};return '';}},files};}
test('the Corsican demo footprint intersects two canonical tiles',()=>{
 const g=tileServiceGeometry({center:[8.9,42.01],size:[.08,.04]});
 assert.deepEqual(g.tiles.map(t=>t.name),['41.5_8.5.fftile','42_8.5.fftile']);
 const [lon,lat]=g.geographic(g.width/2,g.height/2);assert(Math.abs(lon-8.9)<1e-12);assert(Math.abs(lat-42.01)<1e-12);
});
test('date-line crossings wrap tile indices and reject polar/invalid domains',()=>{
 const g=tileServiceGeometry({center:[179.99,0],size:[.1,.1]});
 assert.equal(g.tiles.length,4);assert(g.tiles.some(t=>t.lonIndex===719));assert(g.tiles.some(t=>t.lonIndex===0));
 for(const options of [{center:[0,90]},{center:[0,0],size:[3,1]},{center:[NaN,0]},{center:[0,0],size:[0,1]}])
  assert.throws(()=>tileServiceGeometry(options));
});
test('the adapter stages unchanged compressed bytes and treats 404/410 as missing',async()=>{
 const h=host(),options={provider:'http://tiles.example/tiles',center:[8.5,42],size:[.002,.002]};
 const expected=tileServiceGeometry(options).tiles;let n=0;
 const result=await loadTileService(h.Module,h.ff,{...options,fetcher:async url=>{
  const tile=expected.find(t=>url.endsWith('/'+t.name));const i=n++;
  return i<2?new Response(packet(tile),{status:200}):new Response(null,{status:i===2?404:410});
 }});
 assert.equal(result.available,2);assert.equal(result.missing,2);assert.equal(h.files.size,2);
 assert.equal(result.bytes,520);assert(h.ff.tileDomain());
});
test('service errors and malformed/wrong-address data never create a domain',async()=>{
 for(const kind of ['server','wrong-tile','truncated','too-large']){
  const h=host();
  await assert.rejects(loadTileService(h.Module,h.ff,{
   provider:'http://tiles.example/tiles',center:[8.9,42.02],size:[.01,.01],
   fetcher:async()=>{
    if(kind==='server')return new Response(null,{status:500});
    if(kind==='truncated')return new Response(new Uint8Array(5));
    if(kind==='too-large')return new Response(new Uint8Array(1),{headers:{'content-length':String(65*1024*1024)}});
    return new Response(packet({latIndex:0,lonIndex:0}));
   }}));
  assert.equal(h.ff.tileDomain(),null);assert.equal(h.files.size,0);
 }
});
