import createForeFire from '/wasm/forefire.mjs';
import {loadTileService} from '/tile-service.mjs';
let Module,ff,domain,fuels,cancelled=false,running=false;
const send=(type,data={})=>self.postMessage({type,...data});
function fronts(text){
  const out=[];let current;
  for(const line of text.split('\n')){
    if(line.includes('FireFront[')){current=[];out.push(current);}
    else if(current && line.includes('FireNode[')){
      const match=/loc=\(([-\d.eE+]+),([-\d.eE+]+),/.exec(line);
      if(match)current.push([Number(match[1]),Number(match[2])]);
    }
  }
  return out.filter(p=>p.length>=3);
}
function area(polygons){let a=0;for(const p of polygons)for(let i=0;i<p.length;i++){
  const q=p[(i+1)%p.length];a+=p[i][0]*q[1]-q[0]*p[i][1];}return Math.abs(a)/2;}
function frame(){const polygons=fronts(ff.execute('print[]'));return {time:ff.getTime(),fronts:polygons,area:area(polygons)};}
async function initialize(config){
  const diagnostics=[];
  Module=await createForeFire({print:text=>diagnostics.push(text),printErr:text=>diagnostics.push(text)});
  ff=new Module.ForeFire();
  const response=await fetch('/fuels.csv');if(!response.ok)throw new Error('Cannot read the demo fuel table.');
  const table=await response.text();fuels=new Map();
  const rows=table.trim().split(/\r?\n/),keys=rows.shift().split(';');
  for(const row of rows){const v=row.split(';');fuels.set(Number(v[0]),Number(v[keys.indexOf('e')]));}
  ff.setString('fuelsTable',table);ff.setString('propagationModel',config.model??'Rothermel');
  ff.setString('caseDirectory','/');ff.setString('ForeFireDataDirectory','');
  for(const [k,v] of Object.entries({spatialIncrement:3,perimeterResolution:12,
    minimalPropagativeFrontDepth:10,initialFrontDepth:.1,relax:.5,smoothing:0,
    minSpeed:0,bmapLayer:0,windReductionFactor:.4,propagationSpeedAdjustmentFactor:.6,
    fftileCacheSize:6}))ff.setDouble(k,v);
  // The public static host currently has no CORS headers. The local demo
  // forwards only this fixed host's canonical tile paths.
  const fetcher=(url,options)=>fetch(String(url).startsWith('https://forefire.univ-corse.fr/fftiles/')
    ? '/public-tiles/'+new URL(url).pathname.split('/').pop() : url,options);
  const loaded=await loadTileService(Module,ff,{...config,fetcher,onProgress:p=>send('tiles',p)});
  domain=loaded.domain;
  send('status',{message:'Sampling terrain in WebAssembly…'});
  const fuel=ff.sampleLayer('fuel',384,384),altitude=ff.sampleLayer('altitude',384,384);
  send('loaded',{...loaded,fuel,altitude,burnableFuels:[...fuels].filter(([,depth])=>depth>0).map(([id])=>id),version:Module.version()});
}
function setWind(wind){
  if(!Array.isArray(wind)||wind.length!==2||!wind.every(Number.isFinite))throw new Error('Invalid wind.');
  ff.execute(`trigger[wind;vel=(${wind[0]},${wind[1]},0);t=${ff.getTime()}]`);
  send('wind',{wind,time:ff.getTime()});
}
function ignite(ignition){
  if(!Array.isArray(ignition)||ignition.length!==2||!ignition.every(Number.isFinite))throw new Error('Invalid ignition.');
  const [x,y]=ignition;
  if(x<domain.x0||y<domain.y0||x>domain.x0+domain.width||y>domain.y0+domain.height)throw new Error('Ignition is outside the domain.');
  const id=ff.sampleAt('fuel',x,y);
  if(!id || !(fuels.get(id)>0)){send('notice',{message:`Fuel ${id} has no burnable bed here. Choose a green or brown area.`});return false;}
  ff.execute(`startFire[loc=(${x},${y},0);t=${ff.getTime()}]`);
  send('ignited',{ignition,time:ff.getTime()});send('frame',{frame:frame()});return true;
}
async function simulate({ignition,wind,duration=1800,pace=500}){
  if(!ff || running)return;
  if(!Number.isFinite(duration)||duration<=0||duration>86400)throw new Error('Invalid duration.');
  if(wind)setWind(wind);
  if(ignition&&!ignite(ignition))return;
  running=true;cancelled=false;send('running');
  try {
    const end=ff.getTime()+duration;
    while(ff.getTime()<end && !cancelled){
      ff.execute(`step[dt=${Math.min(30,end-ff.getTime())}]`);
      send('frame',{frame:frame()});
      // Yield between solver steps so ignitions, pause and wind messages apply
      // at the current simulation time. Pace also leaves time to interact.
      await new Promise(resolve=>setTimeout(resolve,Math.max(0,Math.min(1000,pace))));
    }
    send('done',{reason:cancelled?'Simulation paused.':'Interval complete.',domain:ff.tileDomain()});
  } finally {running=false;}
}
self.onmessage=async({data})=>{
  try {
    if(data.type==='init')await initialize(data.config);
    else if(data.type==='run')await simulate(data);
    else if(data.type==='ignite'&&ff)ignite(data.ignition);
    else if(data.type==='wind'&&ff)setWind(data.wind);
    else if(data.type==='preview'&&ff){
      const {x,y,width,height,id}=data;
      const fuel=ff.sampleWindow('fuel',x,y,width,height,512,512);
      const altitude=ff.sampleWindow('altitude',x,y,width,height,512,512);
      send('preview',{id,x,y,width,height,fuel,altitude});
    }
    else if(data.type==='cancel')cancelled=true;
  }catch(error){send('error',{message:error?.message??String(error)});}
};
