/** Exercise the actual demo worker under Node without a browser or DOM.
 * node bindings/wasm/tiles-demo/smoke.mjs http://127.0.0.1:8765
 * The loopback server must be running and serve the converted Corsican tiles.
 */
import assert from 'node:assert/strict';
import {Worker} from 'node:worker_threads';
import {readFile,mkdtemp,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const base=process.argv[2]??'http://127.0.0.1:8765';
const root=resolve(new URL('../../..',import.meta.url).pathname);
const createURL=pathToFileURL(join(root,'bin/forefire.mjs')).href;
const serviceURL=pathToFileURL(join(root,'bindings/wasm/tile-service.mjs')).href;
const dir=await mkdtemp(join(tmpdir(),'forefire-tiles-worker-'));
const source=(await readFile(new URL('./worker.mjs',import.meta.url),'utf8'))
 .replace("'/wasm/forefire.mjs'",JSON.stringify(createURL))
 .replace("'/tile-service.mjs'",JSON.stringify(serviceURL));
const shim=`import {parentPort} from 'node:worker_threads';
const nativeFetch=fetch;globalThis.fetch=(url,options)=>nativeFetch(new URL(url,${JSON.stringify(base)}),options);
globalThis.self={postMessage:data=>parentPort.postMessage(data)};
${source}
parentPort.on('message',data=>self.onmessage({data}));\n`;
await writeFile(join(dir,'worker.mjs'),shim);
const worker=new Worker(pathToFileURL(join(dir,'worker.mjs')));
let loaded,frames=[],tiles=0,ignitions=0,windChanged=false,changed=false,paused=false,resumed=false,secondIgnition,previewChecked=false;
try {
 await new Promise((resolve,reject)=>{
  const timeout=setTimeout(()=>reject(new Error('WASM worker test timed out')),60000);
  worker.on('error',reject);
  worker.on('message',data=>{
   try {
    if(data.type==='error')throw new Error(data.message);
    if(data.type==='tiles')tiles++;
    if(data.type==='loaded'){
     loaded=data;assert.equal(data.available,2);assert.equal(data.missing,0);
     assert.equal(data.fuel.width,384);assert.equal(data.altitude.height,384);
     assert(data.fuel.data.some(v=>v===104));assert(data.altitude.data.some(v=>v>0));
     const pixel=data.fuel.data.findIndex((v,i)=>data.burnableFuels.includes(v)&&i%384>210&&i%384<230&&Math.floor(i/384)>190);
     worker.postMessage({type:'preview',id:7,x:data.domain.width/4,y:data.domain.height/4,width:data.domain.width/2,height:data.domain.height/2});
     assert(pixel>=0);secondIgnition=[(pixel%384+.5)*data.domain.width/384,(Math.floor(pixel/384)+.5)*data.domain.height/384];
     worker.postMessage({type:'run',ignition:[data.domain.width/2,data.domain.height/2],wind:[6,6],duration:600,pace:0});
    }
    if(data.type==='preview'){assert.equal(data.id,7);assert.equal(data.fuel.width,512);assert.equal(data.altitude.data.length,512*512);assert(data.fuel.data.every(Number.isInteger));previewChecked=true;}
    if(data.type==='ignited')ignitions++;
    if(data.type==='wind'&&data.time>0)windChanged=true;
    if(data.type==='notice')throw new Error(data.message);
    if(data.type==='frame'){
     if(data.frame.time>=120&&!changed){changed=true;worker.postMessage({type:'ignite',ignition:secondIgnition});worker.postMessage({type:'wind',wind:[-6,3]});}
     if(data.frame.time>=300&&!paused){paused=true;worker.postMessage({type:'cancel'});}
     assert(data.frame.fronts.every(p=>p.every(xy=>xy.every(Number.isFinite))));frames.push(data.frame);}
    if(data.type==='done'){
     if(!resumed){assert(data.reason.includes('paused'));resumed=true;worker.postMessage({type:'run',wind:[-6,3],duration:300,pace:0});return;}
     assert(previewChecked);assert(frames.some(f=>f.time>120&&f.fronts.length>=2),'second fire must appear in solver geometry');assert.equal(ignitions,2);assert(windChanged);assert(frames.at(-1).time>=600);
     assert(loaded);assert.equal(tiles,2);assert(frames.length>=10);
     assert(frames.at(-1).area>frames[0].area*2);
     console.log(JSON.stringify({availableTiles:loaded.available,fetchedBytes:loaded.bytes,
      ignitions,windChanged,resumed,previewChecked,frameCount:frames.length,simulatedSeconds:frames.at(-1).time,
      initialArea:frames[0].area,finalArea:frames.at(-1).area,cachedTiles:data.domain.cachedTiles},null,2));
     clearTimeout(timeout);resolve();
    }
   }catch(error){clearTimeout(timeout);reject(error);}
  });
  worker.postMessage({type:'init',config:{provider:process.argv[3]??new URL('/tiles/',base).href,center:[8.9,42.01],size:[.08,.04]}});
 });
}finally{await worker.terminate();await rm(dir,{recursive:true,force:true});}
