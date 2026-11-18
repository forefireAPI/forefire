// Horizontal preview trajectories. Velocities are m/s; elapsed time is seconds.
// Optional isotropic Brownian diffusion K (m²/s); no decorative speed multiplier.
// Backward diffusion is a possible-origin illustration, not inverse physical mixing.
const RAD=Math.PI/180,R=6371000;
export const wrapLon=x=>((x+180)%360+360)%360-180;
export function sampleVelocity(g,u,v,lat,lon){
 if(g.native||!Number.isFinite(lat)||!Number.isFinite(lon))return null;
 let x=(lon-g.lon0)/g.dx,y=g.rowAtLatitude?g.rowAtLatitude(lat):(lat-g.lat0)/g.dy;
 if(g.cyclic)x=((x%g.nx)+g.nx)%g.nx;
 if(x<0||y<0||x>g.nx-1&&!g.cyclic||y>g.ny-1)return null;
 const i=Math.floor(x),j=Math.floor(y),fx=x-i,fy=y-j;
 let east=0,north=0;
 for(const [di,dj,w]of [[0,0,(1-fx)*(1-fy)],[1,0,fx*(1-fy)],[0,1,(1-fx)*fy],[1,1,fx*fy]]){
  if(w<1e-10)continue;
  const col=g.cyclic?(i+di)%g.nx:i+di,row=j+dj;if(col>=g.nx||row>=g.ny)return null;
  const k=row*g.nx+col;if(!Number.isFinite(u[k])||!Number.isFinite(v[k]))return null;
  east+=u[k]*w;north+=v[k]*w;
 }
 return [east,north];
}
export function displace(lat,lon,u,v,seconds){
 const speed=Math.hypot(u,v);if(!speed)return {lat,lon:wrapLon(lon)};
 const distance=speed*seconds/R,bearing=Math.atan2(u,v),a=lat*RAD,b=lon*RAD;
 const c=Math.asin(Math.max(-1,Math.min(1,Math.sin(a)*Math.cos(distance)+Math.cos(a)*Math.sin(distance)*Math.cos(bearing))));
 const d=b+Math.atan2(Math.sin(bearing)*Math.sin(distance)*Math.cos(a),Math.cos(distance)-Math.sin(a)*Math.sin(c));
 return {lat:c/RAD,lon:wrapLon(d/RAD)};
}
function velocity(g,a,b,lat,lon,f){
 const x=sampleVelocity(g,a.u,a.v,lat,lon),y=sampleVelocity(g,b.u,b.v,lat,lon);
 return x&&y?[x[0]*(1-f)+y[0]*f,x[1]*(1-f)+y[1]*f]:null;
}
// Midpoint integration, bilinear spatial and linear temporal interpolation.
// A quarter-cell travel limit reduces the risk of stepping across missing cells.
function gaussian(seed,step){
 // Counter-based deterministic uniforms: repeatable under caching and seek/replay.
 const hash=x=>{x=Math.imul(x^(x>>>16),0x7feb352d);x=Math.imul(x^(x>>>15),0x846ca68b);return (x^(x>>>16))>>>0;};
 const u=(hash(seed^Math.imul(step+1,0x9e3779b9))+.5)/4294967296;
 const v=(hash(seed^Math.imul(step+1,0x85ebca6b)^0xc2b2ae35)+.5)/4294967296;
 const r=Math.sqrt(-2*Math.log(u)),t=2*Math.PI*v;return [r*Math.cos(t),r*Math.sin(t)];
}
export function advanceParticle(p,g,a,b,seconds,startFraction=0,{diffusion=0,seed=1}={}){
 const total=Math.abs(seconds),sign=Math.sign(seconds);let elapsed=total*startFraction,step=0;
 if(!p.active)return p;
 if(startFraction===1){if(!sampleVelocity(g,b.u,b.v,p.lat,p.lon))p.active=false;return p;}
 while(elapsed<total-1e-8){
  const f=elapsed/total,w=velocity(g,a,b,p.lat,p.lon,f);
  if(!w){p.active=false;break;}
  const cell=Math.max(1,R*RAD*Math.min(Math.abs(g.dy),Math.abs(g.dx)*Math.max(.015,Math.cos(p.lat*RAD))));
  const dt=Math.min(600,total-elapsed,.25*cell/Math.max(.01,Math.hypot(...w)),diffusion>0?cell*cell/(64*diffusion):Infinity);
  const middle=displace(p.lat,p.lon,...w,sign*dt/2),mid=velocity(g,a,b,middle.lat,middle.lon,(elapsed+dt/2)/total);
  if(!mid){p.active=false;break;}
  let next=displace(p.lat,p.lon,...mid,sign*dt);
  if(diffusion>0){const z=gaussian(seed,step++),sigma=Math.sqrt(2*diffusion*dt);next=displace(next.lat,next.lon,z[0]*sigma,z[1]*sigma,1);}
  if(!velocity(g,a,b,next.lat,next.lon,(elapsed+dt)/total)){p.active=false;break;}
  p.lat=next.lat;p.lon=next.lon;elapsed+=dt;
 }
 return p;
}
const clone=s=>({index:s.index,ms:s.ms,particles:s.particles.map(p=>({...p,track:p.track.map(x=>({...x}))}))});
export class EmitterSimulation{
 constructor(){this.sources=[];this.serial=0;this.diffusion=0;this.perInterval=16;this.lifetimeHours=Infinity;}
 setEmission(perInterval,lifetimeHours){
  if(![8,16,32].includes(perInterval)||![168,Infinity].includes(lifetimeHours))throw Error('Invalid plume density or lifetime.');
  if(perInterval===this.perInterval&&lifetimeHours===this.lifetimeHours)return;
  this.perInterval=perInterval;this.lifetimeHours=lifetimeHours;for(const s of this.sources){s.cache.clear();s.state=null;}
 }
 setDiffusion(value){
  if(!Number.isFinite(value)||value<0||value>100000)throw Error('Diffusion must be between 0 and 100,000 m²/s.');
  if(value===this.diffusion)return;this.diffusion=value;for(const s of this.sources){s.cache.clear();s.state=null;}
 }
 clear(){this.sources=[];}
 add(lat,lon,index,ms,color){
  if(this.sources.length>=6)throw Error('Six sources maximum. Remove a source to place another.');
  const source={id:++this.serial,lat,lon:wrapLon(lon),index,ms,color,cache:new Map(),state:null};
  this.sources.push(source);return source;
 }
 remove(id){this.sources=this.sources.filter(s=>s.id!==id);}
 async evaluate(target,{frames,geometry,loadPair,signal,onProgress=()=>{}}){
  const updates=[];
  for(const source of [...this.sources]){
   signal?.throwIfAborted();const direction=Math.sign(target-source.index);
   let state={index:source.index,ms:source.ms,particles:[{id:0,bornMs:source.ms,lat:source.lat,lon:source.lon,active:true,track:[{lat:source.lat,lon:source.lon}]}]};
   for(const cached of source.cache.values())if((cached.index-source.index)*direction>=0&&(target-cached.index)*direction>=0&&Math.abs(target-cached.index)<Math.abs(target-state.index))state=cached;
   state=clone(state);
   while(state.index!==target){
    signal?.throwIfAborted();const next=state.index+direction;
    onProgress(source,next,target);
    const a=await loadPair(state.index),b=await loadPair(next);signal?.throwIfAborted();
    const seconds=(frames[next].ms-frames[state.index].ms)/1000;
    if(!Number.isFinite(seconds)||seconds*direction<=0)throw Error('Trajectory fields must have strictly increasing valid times.');
    const options=p=>({diffusion:this.diffusion,seed:(Math.imul(source.id,104729)^Math.imul(p.id+1,13007)^Math.imul(state.index+1,7919)^Math.imul(next+1,65537))>>>0});
    for(const p of state.particles){advanceParticle(p,geometry,a,b,seconds,0,options(p));p.track.push({lat:p.lat,lon:p.lon});if(p.track.length>24)p.track.shift();}
    // Continuous staggered releases; capped density is a visual sampling choice.
    for(let n=1;n<=this.perInterval;n++){
     const p={id:direction*(Math.abs(state.index-source.index)*this.perInterval+n),bornMs:frames[state.index].ms+(frames[next].ms-frames[state.index].ms)*n/this.perInterval,lat:source.lat,lon:source.lon,active:true,track:[{lat:source.lat,lon:source.lon}]};
     advanceParticle(p,geometry,a,b,seconds,n/this.perInterval,options(p));p.track.push({lat:p.lat,lon:p.lon});state.particles.push(p);
    }
    // Lifetime follows forecast time, including reverse branches, never wall time.
    state.particles=state.particles.filter(p=>Math.abs(frames[next].ms-p.bornMs)<this.lifetimeHours*3600000);
    if(state.particles.length>768)state.particles.splice(0,state.particles.length-768);
    state.index=next;state.ms=frames[next].ms;
   }
   signal?.throwIfAborted();if(!this.sources.includes(source))continue;
   updates.push({source,state});
  }
  // Publish together: all colours must match the displayed forecast time.
  signal?.throwIfAborted();
  for(const {source,state}of updates){
   if(!this.sources.includes(source))continue;source.state=state;source.cache.delete(target);source.cache.set(target,state);
   while(source.cache.size>8)source.cache.delete(source.cache.keys().next().value);
  }
 }
}
