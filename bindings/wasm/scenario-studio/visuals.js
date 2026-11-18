import {wrap} from './core.js?v=overlay-1';
export const FIRE_COLORS=['#d82b1f','#ef531b','#f28e19','#eec52c','#b8d52b','#76c84d','#29b98e','#08bcb9'];
export const WIND_COLORS=['#69b7c2','#9ed3b4','#d3df8a','#efd16c','#e99b55','#d96348'];
export function ramp(colors,t){const x=Math.max(0,Math.min(1,t))*(colors.length-1),i=Math.floor(x),f=x-i;const rgb=s=>[1,3,5].map(k=>parseInt(s.slice(k,k+2),16));const a=rgb(colors[i]),b=rgb(colors[Math.min(i+1,colors.length-1)]);return a.map((v,j)=>Math.round(v*(1-f)+b[j]*f));}
export function fireColor(t){return 'rgb('+ramp(FIRE_COLORS,t).join(',')+')';}
export function buildBands(snapshots,clip,center=0){
 const ordered=snapshots.slice().sort((a,b)=>a.time-b.time),first=ordered.find(f=>f.polygons?.[0]?.[0]?.length);
 if(!first)return [];
 const latitude=first.polygons[0][0][0][1],meters=111195,scale=meters*Math.cos(latitude*Math.PI/180);
 const color=i=>i===0?'#343c47':fireColor(ordered.length<=2?0:Math.max(0,(i-1)/(ordered.length-2)));
 // Work in local metres, with an explicit sub-metre grid. Raw geographic
 // doubles near coincident fire lines can break the sweep's ring assembly.
 for(const precision of [.01,.1,.5,1]){
  try{
   const bands=[];let cumulative=[];
   const ring=r=>{
    const out=[];for(const [lon,lat]of r){const p=[Math.round(wrap(lon-center)*scale/precision),Math.round((lat-latitude)*meters/precision)];if(!out.length||p.some((x,i)=>x!==out.at(-1)[i]))out.push(p);}
    if(out.length>1&&out[0].every((x,i)=>x===out.at(-1)[i]))out.pop();if(out.length<3)return null;
    const a=out[0],b=out[1];if(!out.some(p=>(b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])!==0))return null;
    out.push(out[0].slice());return out;
   };
   const restore=polygons=>polygons.map(p=>p.map(r=>r.map(([x,y])=>[center+x*precision/scale,latitude+y*precision/meters])));
   for(let i=0;i<ordered.length;i++){
    const f=ordered[i],polygons=[];
    for(const p of f.polygons){const outer=ring(p[0]);if(!outer)continue;polygons.push([outer,...p.slice(1).map(ring).filter(Boolean)]);}
    if(!polygons.length)continue;
    const next=cumulative.length?clip.union(cumulative,polygons):clip.union(polygons),geometry=cumulative.length?clip.difference(next,cumulative):next;
    bands.push({time:f.time,polygons:restore(geometry),color:color(i),initial:i===0,precisionMeters:precision,fill:true});cumulative=next;
   }
   if(!bands.length)throw Error('No filled contour geometry');
   return bands;
  }catch{/* Retry only the display geometry, never the solver's coordinates. */}
 }
 // Keep every original hourly boundary visible even if all filled-band
 // attempts fail. No synthetic repair or change to simulated perimeters.
 return ordered.map((f,i)=>({time:f.time,polygons:f.polygons.map(p=>p.map(r=>{const q=r.map(([lon,lat])=>[center+wrap(lon-center),lat]);if(q.length)q.push(q[0].slice());return q;})),color:color(i),initial:i===0,fill:false}));
}
// Preview arrays have geographic control nodes, south-first, with x fastest.
export function samplePreview(g,u,v,lon,lat){
 const x=(g.west+wrap(lon-g.west)-g.west)/(g.east-g.west)*(g.nx-1),y=(lat-g.south)/(g.north-g.south)*(g.ny-1);
 if(x<0||y<0||x>g.nx-1||y>g.ny-1)return null;
 const i=Math.min(Math.floor(x),g.nx-2),j=Math.min(Math.floor(y),g.ny-2),fx=x-i,fy=y-j;let east=0,north=0;
 for(const [di,dj,w]of [[0,0,(1-fx)*(1-fy)],[1,0,fx*(1-fy)],[0,1,(1-fx)*fy],[1,1,fx*fy]]){if(w<1e-12)continue;const k=(j+dj)*g.nx+i+di;if(!Number.isFinite(u[k])||!Number.isFinite(v[k]))return null;east+=u[k]*w;north+=v[k]*w;}
 return [east,north];
}
