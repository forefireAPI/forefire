import {project,HOUR,wrap} from './core.js?v=overlay-1';
export const DANGER={low:{dead:.16,live:1.15},moderate:{dead:.12,live:1.05},high:{dead:.10,live:1},veryhigh:{dead:.07,live:.9},extreme:{dead:.05,live:.8}};
export function moistureAt(danger,time,epoch,cycle=false,amplitude=.012,presets=DANGER){const p=presets[danger]??DANGER[danger]??DANGER.high;const hour=new Date(epoch+time*1000).getUTCHours()+new Date(epoch+time*1000).getUTCMinutes()/60;return {dead:Math.max(.02,Math.min(1,p.dead+(cycle?amplitude*Math.cos((hour-4)*Math.PI/12):0))),live:p.live};}
export function validateEvents(events=[]){if(!Array.isArray(events)||events.length>1000)throw Error('Scenario supports at most 1,000 actions.');const ids=new Set();return events.map((e,i)=>{if(!['point','polygon','break','fuel','water','wind','danger'].includes(e.type)||!Number.isFinite(e.time)||e.time<0||e.time>7*86400)throw Error('Invalid scenario action or time.');const v=structuredClone(e);v.id=String(e.id??i);if(ids.has(v.id))throw Error('Duplicate scenario action ID.');ids.add(v.id);
 if(['point','polygon','break','fuel','water'].includes(e.type)){const points=e.points;if(!Array.isArray(points)||points.length<(e.type==='point'||e.type==='water'?1:e.type==='break'?2:3)||points.length>10000||points.some(p=>!Array.isArray(p)||p.length!==2||!p.every(Number.isFinite)||Math.abs(p[0])>180||Math.abs(p[1])>=89))throw Error('Invalid action geometry.');}
 if(e.type==='wind'&&(!Array.isArray(e.vector)||e.vector.length!==2||!e.vector.every(Number.isFinite)||Math.hypot(...e.vector)>100))throw Error('Invalid wind action.');
 if(e.type==='danger'&&e.moisture&&(!Number.isFinite(e.moisture.dead)||e.moisture.dead<0||e.moisture.dead>1||!Number.isFinite(e.moisture.live)||e.moisture.live<0||e.moisture.live>3))throw Error('Invalid fuel moisture.');
 if(e.type==='water'&&(!Number.isFinite(e.length)||e.length<5||e.length>2000||!Number.isFinite(e.bearing)))throw Error('Invalid water drop dimensions.');
 if(e.type==='danger'&&!DANGER[e.danger])throw Error('Invalid danger preset.');
 if(e.type==='fuel'&&(!Number.isInteger(e.fuel)||e.fuel<0||e.fuel>255))throw Error('Fuel ID must be 0–255.');
 if(['water','break'].includes(e.type)&&(!Number.isFinite(e.width)||e.width<5||e.width>1000))throw Error('Width must be 5–1,000 m.');return v;}).sort((a,b)=>a.time-b.time);}
export function footprints(e){if(e.type==='fuel')return [e.points];if(e.type==='water'){const [lon,lat]=e.points[0],w=e.width/2,h=(e.length??e.width*4)/2,angle=(e.bearing??0)*Math.PI/180;return [Array.from({length:32},(_,i)=>{const t=i*Math.PI/16,x=w*Math.cos(t),y=h*Math.sin(t);return [lon+(x*Math.cos(angle)+y*Math.sin(angle))/(111195*Math.cos(lat*Math.PI/180)),lat+(-x*Math.sin(angle)+y*Math.cos(angle))/111195];})];}
 if(e.type==='break'){const result=[];for(let i=1;i<e.points.length;i++){const a=e.points[i-1],b=e.points[i],lat=(a[1]+b[1])/2,s=111195*Math.cos(lat*Math.PI/180),dx=(b[0]-a[0])*s,dy=(b[1]-a[1])*111195,l=Math.hypot(dx,dy);if(!l)continue;const nx=-dy/l*e.width/2/s,ny=dx/l*e.width/2/111195;result.push([[a[0]+nx,a[1]+ny],[b[0]+nx,b[1]+ny],[b[0]-nx,b[1]-ny],[a[0]-nx,a[1]-ny]]);}
 for(const [lon,lat]of e.points)result.push(Array.from({length:16},(_,i)=>[lon+Math.cos(i*Math.PI/8)*e.width/2/(111195*Math.cos(lat*Math.PI/180)),lat+Math.sin(i*Math.PI/8)*e.width/2/111195]));return result;}return [];}
export function modifierCommands(e){return footprints(e).map(r=>`trigger[tileFuel;fuelType=${e.type==='fuel'?e.fuel:0};points=${r.map(p=>p.join(',')).join('|')}]`);}
export function scenarioFile(config,events){return {format:'forefire-scenario',version:1,initial:{geojson:config.geojson,timestamp:config.timestamp,size:config.size,wind:config.wind,numerics:config.numerics,model:config.model,windAdjustment:config.windAdjustment,fuelsTable:config.fuelsTable,source:config.source},danger:config.danger??'high',diurnal:!!config.diurnal,amplitude:config.amplitude??.012,horizon:config.horizon??21600,dangerPresets:config.dangerPresets??DANGER,minSpeed:config.minSpeed??.001,consumption:config.consumption??.8,events:validateEvents(events)};}
// At most two well-separated dominant advances per interval, in local metres.
// These are contour correspondences, not tracked fire-node trajectories.
export function spreadVectors(previous,current,spacing=400){
 if(!previous||!current||current.time<=previous.time)return [];
 const candidates=[],latitude=current.polygons[0]?.[0]?.[0]?.[1]??0,scale=111195*Math.cos(latitude*Math.PI/180),origin=current.polygons[0]?.[0]?.[0]?.[0]??0;
 const xy=p=>[wrap(p[0]-origin)*scale,p[1]*111195],geo=p=>[wrap(origin+p[0]/scale),p[1]/111195];
 const contains=(ring,q)=>{let inside=false;for(let i=0,j=ring.length-1;i<ring.length;j=i++)if((ring[i][1]>q[1])!==(ring[j][1]>q[1])&&q[0]<(ring[j][0]-ring[i][0])*(q[1]-ring[i][1])/(ring[j][1]-ring[i][1])+ring[i][0])inside=!inside;return inside;};
 for(const polygon of current.polygons){const ring=polygon[0].map(xy);if(ring.length<3)continue;
  const old=previous.polygons.map(p=>p[0].map(xy)).filter(r=>r.some(p=>contains(ring,p)));if(!old.length)continue;
  const perimeter=ring.reduce((n,p,i)=>n+Math.hypot(p[0]-ring[(i+1)%ring.length][0],p[1]-ring[(i+1)%ring.length][1]),0),stride=Math.min(spacing,perimeter/24);let travel=stride,last=ring.at(-1);
  for(const q of ring){travel+=Math.hypot(q[0]-last[0],q[1]-last[1]);last=q;if(travel<stride)continue;travel=0;let best,dist=Infinity;
   for(const r of old)for(let i=0,j=r.length-1;i<r.length;j=i++){const a=r[j],b=r[i],dx=b[0]-a[0],dy=b[1]-a[1],f=Math.max(0,Math.min(1,((q[0]-a[0])*dx+(q[1]-a[1])*dy)/(dx*dx+dy*dy||1))),p=[a[0]+f*dx,a[1]+f*dy],d=Math.hypot(q[0]-p[0],q[1]-p[1]);if(d<dist){dist=d;best=p;}}
   if(dist>10&&!old.some(r=>contains(r,q)))candidates.push({from:geo(best),to:geo(q),speed:dist/(current.time-previous.time),distance:dist});
  }
 }
 candidates.sort((a,b)=>b.distance-a.distance);const first=candidates[0];if(!first)return [];
 const second=candidates.slice(1).find(v=>v.distance>=first.distance*.55&&Math.hypot(...xy(v.to).map((x,i)=>x-xy(first.to)[i]))>=Math.max(spacing,first.distance*.5)&&Math.hypot(...xy(v.from).map((x,i)=>x-xy(first.from)[i]))>=spacing*.5);
 return second?[first,second]:[first];
}
