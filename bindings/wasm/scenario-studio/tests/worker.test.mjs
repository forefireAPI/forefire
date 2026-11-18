import {test} from 'node:test';import assert from 'node:assert/strict';import {Worker} from 'node:worker_threads';import {mkdtemp,writeFile,rm} from 'node:fs/promises';import {tmpdir} from 'node:os';import {join} from 'node:path';import {pathToFileURL} from 'node:url';
const fixtures=process.env.FFTILE_FIXTURES;
test('actual WASM worker resumes a MultiPolygon with a hole under gridded wind, then accepts another ignition', {timeout:60000,skip:!fixtures},async()=>{
 const dir=await mkdtemp(join(tmpdir(),'forefire-studio-test-'));
 const target=new URL('../worker.js',import.meta.url).href;
 await writeFile(join(dir,'shim.js'),`import {parentPort} from 'node:worker_threads';import {readFile} from 'node:fs/promises';
 globalThis.self={postMessage:data=>parentPort.postMessage(data),location:{href:'https://forefire.univ-corse.fr/forefire/'}};
 globalThis.fetch=async(url)=>{url=String(url);let p;if(url.startsWith('file:'))p=new URL(url);else if(url.includes('/forefire/tiles/'))p=${JSON.stringify(fixtures)}+'/'+decodeURIComponent(new URL(url).pathname.split('/').pop());else throw Error('Unexpected fetch: '+url);try{return new Response(await readFile(p));}catch(e){if(e.code==='ENOENT')return new Response('',{status:404});throw e;}};
 await import(${JSON.stringify(target)});parentPort.on('message',data=>self.onmessage({data}));`);
 const worker=new Worker(pathToFileURL(join(dir,'shim.js')));const frames=[];let loaded=false,paused=false;
 const outer=[[8.898,42.008],[8.902,42.008],[8.902,42.012],[8.898,42.012],[8.898,42.008]],hole=[[8.8997,42.0097],[8.9003,42.0097],[8.9003,42.0103],[8.8997,42.0103],[8.8997,42.0097]],other=[[8.905,42.009],[8.906,42.009],[8.906,42.010],[8.905,42.010],[8.905,42.009]];
 try{await new Promise((resolve,reject)=>{worker.on('error',reject);worker.on('message',d=>{try{if(d.type==='error')throw Error(d.message);if(d.type==='loaded'){loaded=true;assert.equal(d.settings.rosMultiplier,1);assert.equal(d.settings.windAdjustment,.7);assert.equal(d.settings.arrivalGrid.subdivisions,256);assert.equal(d.settings.arrivalGrid.blockDegrees,.1);assert.deepEqual(d.numerics,{perimeterResolution:24,spatialIncrement:6});assert.equal(d.frame.polygons.length,2);assert.equal(d.frame.polygons[0].length,2);frames.push(d.frame);worker.postMessage({type:'run',duration:180,pace:0});}if(d.type==='frame'){frames.push(d.frame);assert(d.frame.polygons.flat(2).every(p=>p.every(Number.isFinite)));if(d.frame.time===60){worker.postMessage({type:'wind',mode:'manual',vector:[-3,4]});worker.postMessage({type:'ignite',point:[8.907,42.011]});}}if(d.type==='paused'){if(!paused){paused=true;worker.postMessage({type:'run',duration:60,pace:0});}else{assert(loaded);assert.equal(frames.at(-1).time,240);assert(frames.at(-1).area>frames[0].area);assert(frames.some(f=>f.polygons.length>=3));console.log(JSON.stringify({initialArea:frames[0].area,finalArea:frames.at(-1).area,frames:frames.length,holes:frames[0].polygons[0].length-1,resumed:paused}));resolve();}}}catch(e){reject(e);}});worker.postMessage({type:'init',config:{epoch:Date.parse('2011-08-05T12:00:00Z'),center:[8.9,42.01],size:[.08,.06],numerics:'fast',polygons:[[outer,hole],[other]],points:[],wind:{mode:'custom',geometry:{nx:5,ny:5,west:8.85,east:8.95,south:41.97,north:42.05},samples:[{ms:Date.parse('2011-08-05T11:00:00Z'),u:Array.from({length:25},(_,i)=>i===24?NaN:3+i%5),v:Array.from({length:25},(_,i)=>i===24?NaN:2+Math.floor(i/5)/4)},{ms:Date.parse('2011-08-05T20:00:00Z'),u:Array.from({length:25},(_,i)=>i===24?NaN:8-i%5),v:Array.from({length:25},(_,i)=>i===24?NaN:3-Math.floor(i/5)/4)}]}}});});}finally{await worker.terminate();await rm(dir,{recursive:true,force:true});}
});

test('Super fast protects small ignitions, then switches to coarse numerics and protects a new ignition', {timeout:60000,skip:!fixtures},async()=>{
 const dir=await mkdtemp(join(tmpdir(),'forefire-super-test-')),target=new URL('../worker.js',import.meta.url).href;
 await writeFile(join(dir,'shim.mjs'),`import {parentPort} from 'node:worker_threads';import {readFile} from 'node:fs/promises';globalThis.self={postMessage:d=>parentPort.postMessage(d),location:{href:'https://forefire.univ-corse.fr/forefire/'}};globalThis.fetch=async url=>{url=String(url);const p=url.startsWith('file:')?new URL(url):${JSON.stringify(fixtures)}+'/'+new URL(url).pathname.split('/').pop();try{return new Response(await readFile(p));}catch(e){if(e.code==='ENOENT')return new Response('',{status:404});throw e;}};await import(${JSON.stringify(target)});parentPort.on('message',data=>self.onmessage({data}));`);
 const worker=new Worker(pathToFileURL(join(dir,'shim.mjs'))),center=[8.8,42.15],ring=Array.from({length:33},(_,i)=>{const a=-2*Math.PI*(i%32)/32;return[center[0]+120*Math.cos(a)/(111195*Math.cos(center[1]*Math.PI/180)),center[1]+120*Math.sin(a)/111195];});
 let initialArea,phase='first',fineSeen=false,coarseSeen=false;
 try{await new Promise((resolve,reject)=>{worker.on('error',reject);worker.on('message',d=>{try{
  if(d.type==='error')throw Error(d.message);
  if(d.type==='loaded'){initialArea=d.frame.area;assert.deepEqual(d.numerics,{perimeterResolution:128,spatialIncrement:32});assert.equal(d.settings.arrivalGrid.subdivisions,128);worker.postMessage({type:'run',duration:1800,pace:0});}
  if(d.type==='frame'){
   assert(d.frame.area>0);assert(d.frame.polygons.flat(2).every(p=>p.every(Number.isFinite)));
   if(phase==='first'){fineSeen||=!d.frame.coarseActive;coarseSeen||=!!d.frame.coarseActive;assert(d.frame.performance.wallSeconds>0);}
   else if(phase==='ignite'){phase='resume';assert(d.frame.polygons.length>=2);worker.postMessage({type:'run',duration:300,pace:0});}
   else if(phase==='resume')assert.equal(d.frame.coarseActive,false,'new small ignition must restore fine stepping');
  }
  if(d.type==='paused'){
   if(phase==='first'){assert(fineSeen);assert(coarseSeen);phase='ignite';worker.postMessage({type:'ignite',point:[8.907,42.011]});}
   else if(phase==='resume')resolve();
  }
 }catch(e){reject(e);}});worker.postMessage({type:'init',config:{epoch:Date.parse('2025-08-05T12:00:00Z'),center,size:[.3,.3],model:'Rothermel',numerics:'super',polygons:[[ring]],points:[],windAdjustment:.7,wind:{mode:'manual',vector:[6,2]}}});});}finally{await worker.terminate();await rm(dir,{recursive:true,force:true});}
});
