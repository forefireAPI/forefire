import {Worker,isMainThread,parentPort,workerData} from 'node:worker_threads';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';

if(!isMainThread){
 const {app,tiles,mode,center,radius,duration,kind}=workerData;
 const base=pathToFileURL(app+'/').href;
 const fetcher=async url=>{url=String(url);const file=url.startsWith('file:')?new URL(url):join(tiles,decodeURIComponent(new URL(url).pathname.split('/').pop()));try{return new Response(await readFile(file));}catch(e){if(e.code==='ENOENT')return new Response('',{status:404});throw e;}};
 globalThis.fetch=fetcher;
 if(kind==='core'){
  const {default:create}=await import(base+'assets/forefire.js');const {loadTileService}=await import(base+'tile-service.js');const {project,parseFronts,polygonArea,solverSettings,arrivalSettings}=await import(base+'core.js');
  const M=await create({print(){},printErr(){}}),ff=new M.ForeFire();ff.setString('fuelsTable',await readFile(app+'/assets/fuels.csv','utf8'));ff.setString('propagationModel','Rothermel');ff.setString('caseDirectory','/');ff.setString('ForeFireDataDirectory','');
  for(const[k,v]of Object.entries({...solverSettings(mode),...arrivalSettings(mode),...(mode==='super'?{perimeterResolution:24,spatialIncrement:6}:{}),minimalPropagativeFrontDepth:10,initialFrontDepth:.1,relax:.5,smoothing:0,minSpeed:0,bmapLayer:0,windReductionFactor:.7,propagationSpeedAdjustmentFactor:1,fftileCacheSize:6}))ff.setDouble(k,v);
  const loaded=await loadTileService(M,ff,{provider:'https://example.test/fftiles/',center,size:[.3,.3],fetcher});const d=loaded.domain,c=project(d,center);
  ff.setWindGrid({nx:1,ny:1,west:center[0]-.2,east:center[0]+.2,south:center[1]-.2,north:center[1]+.2,times:[0,86400],u:[6,6],v:[2,2]});
  const ring=Array.from({length:32},(_,i)=>[c[0]+radius*Math.cos(-2*Math.PI*i/32),c[1]+radius*Math.sin(-2*Math.PI*i/32)]);ff.importPerimeters([[ring]]);
  const coarse=mode==='super'?ff.setNumerics(solverSettings(mode).perimeterResolution,solverSettings(mode).spatialIncrement,true):false;ff.execute('step[dt=0]');const initialArea=polygonArea(parseFronts(ff.execute('print[]')));
  const start=performance.now();ff.execute(`step[dt=${duration}]`);const ms=performance.now()-start,text=ff.execute('print[]'),polygons=parseFronts(text);
  if(!polygons.length||!Number.isFinite(polygonArea(polygons)))throw Error('No usable final fire');
  parentPort.postMessage({kind,mode,center,radius,duration,ms,coarse,initialArea,area:polygonArea(polygons),nodes:[...text.matchAll(/FireNode\[/g)].length,arrivalGrid:ff.tileDomain().arrivalGrid});ff.execute('clear[]');ff.delete();parentPort.close();
 }else{
  globalThis.self={location:{href:'https://forefire.univ-corse.fr/forefire/'},postMessage:d=>{
   if(d.type==='error'){parentPort.postMessage({error:d.message});process.exitCode=1;parentPort.close();}
   if(d.type==='loaded'){globalThis.start=performance.now();globalThis.initialArea=d.frame.area;self.onmessage({data:{type:'run',duration,pace:0}});}
   if(d.type==='frame'){globalThis.last=d.frame;globalThis.count=(globalThis.count??0)+1;}
   if(d.type==='paused'){parentPort.postMessage({kind,mode,center,radius,duration,ms:performance.now()-globalThis.start,initialArea:globalThis.initialArea,area:globalThis.last.area,frames:globalThis.count,coarse:globalThis.last.coarseActive,throughput:globalThis.last.performance});parentPort.close();}
  }};
  await import(base+'worker.js');const meters=111195,ring=Array.from({length:33},(_,i)=>{const a=-2*Math.PI*(i%32)/32;return[center[0]+radius*Math.cos(a)/(meters*Math.cos(center[1]*Math.PI/180)),center[1]+radius*Math.sin(a)/meters];});
  await self.onmessage({data:{type:'init',config:{model:'Rothermel',numerics:mode,epoch:Date.parse('2025-08-05T12:00:00Z'),center,size:[.3,.3],polygons:[[ring]],points:[],windAdjustment:.7,wind:{mode:'manual',vector:[6,2]}}}});
 }
}else{
 const options=Object.fromEntries(process.argv.slice(2).reduce((a,x,i,all)=>i%2===0?[...a,[x.replace(/^--/,''),all[i+1]]]:a,[]));
 if(!options.app||!options.tiles||!options.out)throw Error('Usage: --app APP_DIRECTORY --tiles TILE_DIRECTORY --out RESULTS_JSON');
 const app=resolve(options.app),tiles=resolve(options.tiles),duration=Number(options.duration??3600),reps=Number(options.reps??3),results=[];
 const cases=[[8.640619357168653,42.3128837125316],[8.789048392921632,42.380055976886965],[8.765213073311665,42.24679487179487]];
 const run=data=>new Promise((resolve,reject)=>{const w=new Worker(new URL(import.meta.url),{workerData:data});w.on('error',reject);w.on('message',async r=>{await w.terminate();r.error?reject(Error(r.error)):resolve(r);});});
 for(const kind of ['core','worker'])for(const center of cases)for(let rep=0;rep<reps;rep++)for(const mode of rep%2?['super','fast']:['fast','super']){
  const r=await run({app,tiles,mode,center,radius:900,duration,kind});r.rep=rep;results.push(r);console.log(JSON.stringify(r));
 }
 const median=a=>a.sort((x,y)=>x-y)[Math.floor(a.length/2)];const summary=[];
 for(const kind of ['core','worker'])for(const center of cases){const rows=results.filter(r=>r.kind===kind&&r.center[0]===center[0]);const standardMs=median(rows.filter(r=>r.mode==='fast').map(r=>r.ms)),superMs=median(rows.filter(r=>r.mode==='super').map(r=>r.ms));summary.push({kind,center,standardMs,superMs,speedup:standardMs/superMs});}
 const out=resolve(options.out);await mkdir(resolve(out,'..'),{recursive:true});await writeFile(out,JSON.stringify({scope:'Actual WASM under Node, fresh worker/instance per member, fixed manual wind, excludes initialization; worker timing includes snapshots/messages/yields but not browser rendering or ERA downloads',duration,reps,results,summary},null,2)+'\n');console.log(JSON.stringify(summary));
}
