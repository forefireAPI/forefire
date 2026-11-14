import {eraProvider} from './config.js';
import {AvencRangeModel} from './vendor/avenc/avenc_range_model.js?v=overlay-1';
import {buildInventory,pairWind} from './vendor/avenc/avenc_met_core.js';
import {sampleVelocity} from './vendor/avenc/avenc_trajectories.js';
import {HOUR,wrap} from './core.js?v=overlay-1';
export class Weather {
 constructor({center,size,spatial='grid',onStatus=()=>{}}){this.center=center;this.size=size;this.onStatus=onStatus;this.models=new Map();this.samples=new Map();this.abort=new AbortController();this.nx=spatial==='center'?1:Math.max(5,Math.ceil(size[0]/.1)+1);this.ny=spatial==='center'?1:Math.max(5,Math.ceil(size[1]/.1)+1);
  const [lon,lat]=center,[w,h]=size;this.geometry={west:lon-w/2-.02,south:lat-h/2-.02,east:lon+w/2+.02,north:lat+h/2+.02,nx:this.nx,ny:this.ny};
  const pw=Math.max(.6,w*1.6),ph=Math.max(.6,h*1.6);
  this.previewGeometry={west:lon-pw/2,east:lon+pw/2,south:Math.max(-89.9,lat-ph/2),north:Math.min(89.9,lat+ph/2),nx:33,ny:33};
 }
 close(){this.abort.abort();for(const {model} of this.models.values())model.close();}
 async year(year){if(this.models.has(year))return this.models.get(year);this.onStatus('Opening ERA '+year+'…');
  // The archive retains damped ocean values; preserve its quantization but do not mask those values.
  const model=new AvencRangeModel({applyBitmap:false});await model.open(new URL(`ERA_LAND_${year}_wind.mp4`,eraProvider).href,this.abort.signal);
  const inv=buildInventory(model.groups),u=inv.find(v=>['10u','u10'].includes(v.name)),v=inv.find(v=>['10v','v10'].includes(v.name));
  const frames=pairWind(u,v);if(!frames.length)throw Error('ERA archive has no matching 10 m U/V fields.');
  const entry={model,frames};this.models.set(year,entry);while(this.models.size>2){const key=this.models.keys().next().value;this.models.get(key).model.close();this.models.delete(key);}return entry;
 }
 async sample(ms){if(this.samples.has(ms))return this.samples.get(ms);const {model,frames}=await this.year(new Date(ms).getUTCFullYear());
  const f=frames.find(f=>f.ms===ms);if(!f)throw Error('No hourly ERA wind at '+new Date(ms).toISOString());
  const u=await model.getGrid(f.group,f.frame,this.abort.signal),v=await model.getGrid(f.v.group,f.v.frame,this.abort.signal);
  const gu=model.geometryForGroup(f.group),gv=model.geometryForGroup(f.v.group);if(JSON.stringify(gu)!==JSON.stringify(gv))throw Error('U/V grid geometry differs.');
  const east=[],north=[],g=this.geometry;
  for(let j=0;j<this.ny;j++)for(let i=0;i<this.nx;i++){const lon=this.nx===1?this.center[0]:g.west+(g.east-g.west)*i/(this.nx-1),lat=this.ny===1?this.center[1]:g.south+(g.north-g.south)*j/(this.ny-1);
   const w=sampleVelocity(gu,u,v,lat,wrap(lon));east.push(w?.[0]??NaN);north.push(w?.[1]??NaN);}
  const pg=this.previewGeometry,pu=[],pv=[];
  for(let j=0;j<pg.ny;j++)for(let i=0;i<pg.nx;i++){
   const lon=pg.west+(pg.east-pg.west)*i/(pg.nx-1),lat=pg.south+(pg.north-pg.south)*j/(pg.ny-1);
   const vector=sampleVelocity(gu,u,v,lat,wrap(lon));pu.push(vector?.[0]??NaN);pv.push(vector?.[1]??NaN);
  }
  const sample={ms,u:east,v:north,preview:{u:pu,v:pv}};this.samples.set(ms,sample);return sample;
 }
 async preload(ms,hours=6){if(!Number.isFinite(hours)||hours<1||hours>168)throw Error("Weather horizon must be 1–168 hours.");const start=Math.floor(ms/HOUR)*HOUR,end=Math.ceil((ms+hours*HOUR)/HOUR)*HOUR;for(let at=start;at<=end;at+=HOUR){this.onStatus('ERA · '+new Date(at).toISOString().slice(11,16)+' UTC');await this.sample(at);}while(this.samples.size>400)this.samples.delete(this.samples.keys().next().value);return this.window(ms);}
 window(ms){const samples=[...this.samples.values()].sort((a,b)=>a.ms-b.ms);if(!samples.length||samples[0].ms>ms||samples.at(-1).ms<ms+HOUR)return null;return {geometry:this.geometry,samples,previewGeometry:this.previewGeometry};}
}
