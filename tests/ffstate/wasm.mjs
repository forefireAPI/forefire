import assert from 'node:assert/strict';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';

const root=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
const [modulePath=resolve(root,'bin/forefire.mjs'),casePath=resolve(root,'local/ffstate/smoke/grid-256'),tiles=resolve(root,'test_output_tiles')]=process.argv.slice(2);
const {default:create}=await import(pathToFileURL(resolve(modulePath)).href);
function chunks(data){
 const v=new DataView(data.buffer,data.byteOffset,data.byteLength);let pos=16;const out={};
 assert.equal(new TextDecoder().decode(data.slice(0,8)),'FFSTATE1');
 for(let i=0;i<v.getUint32(12,true);++i){const n=v.getUint32(pos,true),size=Number(v.getBigUint64(pos+4,true));pos+=16;const key=new TextDecoder().decode(data.slice(pos,pos+n));pos+=n;out[key]=data.slice(pos,pos+size);pos+=size;}
 assert.equal(pos,data.length);return out;
}
function command(ff,text){const result=ff.execute(text);assert(!result.includes('ffstate:'),result);return result;}
const tile=await readFile(resolve(tiles,'42_8.5.fftile'));
async function engine(state){
 const logs=[],M=await create({print:t=>logs.push(t),printErr:t=>logs.push(t)});
 M.FS.mkdirTree(resolve(tiles));M.FS.writeFile(resolve(tiles,'42_8.5.fftile'),tile);M.FS.writeFile('/input.ffstate',state);
 const ff=new M.ForeFire();ff.setString('caseDirectory','/');ff.setString('ForeFireDataDirectory','');command(ff,'loadState[file=/input.ffstate]');
 assert(!logs.some(t=>t.includes('ffstate:')),logs.join('\n'));
 return {M,ff,logs};
}
const original=await readFile(resolve(casePath,'seed.ffstate'));
const {M,ff,logs}=await engine(original);
assert.equal(ff.getTime(),120.25);command(ff,'saveState[file=/roundtrip.ffstate]');
const before=chunks(original),after=chunks(M.FS.readFile('/roundtrip.ffstate'));
for(const name of ['objects.ff','events','arrival'])assert.deepEqual(after[name],new Uint8Array(before[name]),name);
command(ff,'step[dt=300]');assert.equal(ff.getTime(),420.25);
const u=Array(25).fill(6).concat(Array(25).fill(8)),v=Array(25).fill(2).concat(Array(25).fill(3));u[0]=v[0]=NaN;
ff.setWindGrid({nx:5,ny:5,west:8.5,east:9.1,south:41.85,north:42.45,times:[0,3600],u,v});
command(ff,'saveState[file=/gridded.ffstate]');
assert(!logs.some(t=>t.includes('ffstate:')),logs.join('\n'));
const gridded=M.FS.readFile('/gridded.ffstate');
assert(chunks(gridded).wind,'rolling wind window missing');
const second=await engine(gridded);assert.equal(second.ff.getTime(),420.25);
command(second.ff,'step[dt=60]');assert.equal(second.ff.getTime(),480.25);
assert(!second.logs.some(t=>t.includes('ffstate:')),second.logs.join('\n'));
await mkdir(resolve(root,'local/ffstate'),{recursive:true});
await writeFile(resolve(root,'local/ffstate/wasm-gridded.ffstate'),gridded);
second.ff.execute('clear[]');second.ff.delete();ff.execute('clear[]');ff.delete();
console.log('Native → WASM node/event/arrival round trip exact; resumed propagation and rolling-wind checkpoint passed.');
