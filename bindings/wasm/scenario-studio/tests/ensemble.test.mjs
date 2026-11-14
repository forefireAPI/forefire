import {test} from 'node:test';import assert from 'node:assert/strict';
import {perturbations,perturbVector,burnCounts,probabilityPixels,runMember} from '../ensemble.js';
import {normalize} from '../core.js';
import {Worker} from 'node:worker_threads';import {mkdtemp,writeFile,rm} from 'node:fs/promises';import {tmpdir} from 'node:os';import {join} from 'node:path';import {pathToFileURL} from 'node:url';
test('paper distributions are bounded, reproducible and clockwise meteorological rotation preserves speed',()=>{
 const a=perturbations(15,19073);assert.deepEqual(a,perturbations(15,19073));assert.notDeepEqual(a,perturbations(15,7));
 for(let seed=0;seed<100;seed++)for(const p of perturbations(15,seed)){assert(Math.abs(p.direction)<=60);assert(p.speed>=2/3&&p.speed<=1.5);assert(p.moisture>=.7&&p.moisture<=1.3);}
 const v=perturbVector(0,-10,{direction:90,speed:1.5});assert(Math.abs(v[0]+15)<1e-9);assert(Math.abs(v[1])<1e-9);
 assert.throws(()=>perturbations(16));assert.throws(()=>perturbations(3,1,{speedMax:NaN}));
});
test('burn frequencies preserve holes and burned history; quantiles use unconditional member counts',()=>{
 const box=(a,b,c,d)=>[[a,b],[c,b],[c,d],[a,d]],g={west:0,east:4,south:0,north:4,width:4,height:4,center:2};
 const full={time:0,polygons:[[box(0,0,4,4),box(1,1,3,3)]]},small={time:3600,polygons:[[box(0,0,1,1)]]};
 const members=[{frames:[full,small]},{frames:[small]},{frames:[small]}];
 assert.equal(burnCounts(members,0,g)[0],1);assert.equal(burnCounts(members,0,g)[5],0);
 const counts=burnCounts(members,3600,g);assert.equal(counts[12],3);assert.equal(counts[0],1);assert.equal(counts[5],0);
 for(const q of [.1,.5,.9]){const rgba=probabilityPixels(counts,3,q);assert(rgba[12*4+3]>0);assert.equal(rgba[5*4+3],0);assert.equal(rgba[3]>0,q===.1);}
});
const fixtures=process.env.FFTILE_FIXTURES;
test('15 real WASM members complete with separate workers, scenario retention and paper perturbations',{timeout:180000,skip:!fixtures},async()=>{
 const dir=await mkdtemp(join(tmpdir(),'ff-ensemble-')),target=new URL('../worker.js',import.meta.url).href;
 await writeFile(join(dir,'shim.mjs'),`import {parentPort} from 'node:worker_threads';import {readFile} from 'node:fs/promises';globalThis.self={postMessage:d=>parentPort.postMessage(d),location:{href:'https://forefire.univ-corse.fr/forefire/'}};globalThis.fetch=async url=>{url=String(url);const p=url.startsWith('file:')?new URL(url):${JSON.stringify(fixtures)}+'/'+new URL(url).pathname.split('/').pop();try{return new Response(await readFile(p));}catch(e){if(e.code==='ENOENT')return new Response('',{status:404});throw e;}};await import(${JSON.stringify(target)});parentPort.on('message',data=>self.onmessage({data}));`);
 let live=0,peak=0;const makeWorker=url=>{if(String(url).includes('weather-worker'))return {postMessage(){},terminate(){}};const w=new Worker(pathToFileURL(join(dir,'shim.mjs')));live++;peak=Math.max(peak,live);let ended=false;const adapter={postMessage:d=>w.postMessage(d),terminate:()=>{if(!ended){ended=true;live--;w.terminate();}}};w.on('message',data=>adapter.onmessage?.({data}));w.on('error',e=>adapter.onerror?.(e));return adapter;};
 const config={...normalize({geojson:{type:'Point',coordinates:[8.8,42.15]},timestamp:'2025-08-05T12:00:00Z',size:[.08,.08],wind:{mode:'manual',vector:[6,0]},numerics:'super'}),horizon:600,events:[{id:'wind-1',type:'wind',time:300,vector:[3,4]}]};
 const areas=[];try{
 const control=await runMember({...config,events:[],perturbation:{direction:0,speed:1,moisture:1}},{makeWorker});
 const slow=await runMember({...config,events:[],perturbation:{direction:0,speed:.9,moisture:1}},{makeWorker});
 const fast=await runMember({...config,events:[],perturbation:{direction:0,speed:1.1,moisture:1}},{makeWorker});
 assert(slow.frames.at(-1).area<control.frames.at(-1).area);assert(fast.frames.at(-1).area>control.frames.at(-1).area);
 const g={west:8.795,east:8.805,south:42.145,north:42.155,width:128,height:128,center:8.8};
 assert.deepEqual(burnCounts([control],60,g),burnCounts([{frames:[control.frames[0]]}],60,g),'First minute cannot contain future perimeters');
 for(const p of perturbations(15,19073)){const r=await runMember({...config,perturbation:p},{makeWorker});assert.equal(r.frames.at(-1).time,600);assert(r.frames.at(-1).area>0);areas.push(r.frames.at(-1).area);}assert.equal(live,0);assert.equal(peak,1);assert(Math.max(...areas)>Math.min(...areas)*1.01);console.log('15-member areas m²:',Math.min(...areas).toFixed(0),'–',Math.max(...areas).toFixed(0));}finally{await rm(dir,{recursive:true,force:true});}
});

test('known initial burn is grey and red is reserved for unanimous new burn',()=>{const counts=new Uint8Array([15,14,15,0]),initial=new Uint8Array([15,0,0,0]);const pixels=probabilityPixels(counts,15,0,initial);assert.deepEqual([...pixels.slice(0,3)],[113,124,137]);assert.notDeepEqual([...pixels.slice(4,7)],[224,61,86]);assert.deepEqual([...pixels.slice(8,11)],[224,61,86]);assert.equal(pixels[15],0);});
