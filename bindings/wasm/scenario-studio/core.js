// GeoJSON contract and the tile projection. Pure functions, shared with tests.
export const METERS=111195, HOUR=3600000;
export const wrap=x=>((x+180)%360+360)%360-180;
export function timestamp(value){
 if(typeof value!=='string'||!/(Z|[+-]\d\d:\d\d)$/.test(value)||!Number.isFinite(Date.parse(value)))throw Error('Timestamp must be ISO 8601 with a timezone.');
 return Date.parse(value);
}
export function normalize(input){
 const payload=input.geojson?input:{geojson:input};
 const gj=payload.geojson,features=gj.type==='FeatureCollection'?gj.features:gj.type==='Feature'?[gj]:[{type:'Feature',properties:{},geometry:gj}];
 if(!Array.isArray(features)||!features.length)throw Error('Empty GeoJSON.');
 // An isochrone collection is a time series, not simultaneous ignitions.
 const time=timestamp(payload.timestamp??features.at(-1)?.properties?.datetime);
 const dated=features.filter(f=>f.properties?.datetime||Number.isFinite(f.properties?.time));
 let selected=features;
 if(dated.length===features.length){const at=f=>f.properties.datetime?timestamp(f.properties.datetime):f.properties.time;const eligible=dated.filter(f=>at(f)<=time);if(!eligible.length)throw Error('No perimeter exists at or before the requested time.');const latest=Math.max(...eligible.map(at));selected=dated.filter(f=>at(f)===latest);}
 let count=0;const positions=[];
 const point=p=>{if(!Array.isArray(p)||p.length<2||!p.slice(0,2).every(Number.isFinite)||p[0]<-180||p[0]>180||Math.abs(p[1])>=89)throw Error('Invalid longitude/latitude.');if(++count>100000)throw Error('Perimeter exceeds 100,000 vertices.');positions.push(p);return p.slice(0,2);};
 const polygons=[],points=[];
 for(const f of selected){if(f.properties?.active===false||f.properties?.passive===true)throw Error('Passive sections are reserved but not yet simulated.');const g=f.geometry;
  if(g?.type==='Point')points.push(point(g.coordinates));
  else if(g?.type==='Polygon'||g?.type==='MultiPolygon')for(const polygon of g.type==='Polygon'?[g.coordinates]:g.coordinates){if(!polygon.length)throw Error('Empty polygon.');polygons.push(polygon.map(r=>{if(r.length<4)throw Error('A ring needs at least four coordinates.');const p=r.map(point);if(p[0][0]!==p.at(-1)[0]||p[0][1]!==p.at(-1)[1])throw Error('GeoJSON rings must be closed.');return p;}));}
  else throw Error('Use Point, Polygon or MultiPolygon geometry.');
 }
 const origin=positions[0][0],lons=positions.map(p=>origin+wrap(p[0]-origin)),lats=positions.map(p=>p[1]);
 const west=Math.min(...lons),east=Math.max(...lons),south=Math.min(...lats),north=Math.max(...lats);
 const center=[wrap((west+east)/2),(south+north)/2];
 const size=payload.size??[Math.max(.08,(east-west)*1.7+.04),Math.max(.06,(north-south)*1.7+.04)];
 if(size.length!==2||!size.every(x=>Number.isFinite(x)&&x>0&&x<=2))throw Error('The fixed domain must fit within 2° × 2°.');
 if(east-west>=size[0]||north-south>=size[1])throw Error('Perimeter does not fit inside the requested domain.');
 if(payload.horizon!=null&&(!Number.isFinite(payload.horizon)||payload.horizon<1||payload.horizon>604800))throw Error('Choose a planning horizon up to 7 days.');
 if(payload.minSpeed!=null&&(!Number.isFinite(payload.minSpeed)||payload.minSpeed<0||payload.minSpeed>1/60))throw Error('Inactive-node threshold must be 0–1 m/min.');
 if(payload.dangerPresets)for(const p of Object.values(payload.dangerPresets))if(!p||!Number.isFinite(p.dead)||p.dead<0||p.dead>1||!Number.isFinite(p.live)||p.live<0||p.live>3)throw Error('Invalid moisture preset.');
 if(payload.danger!=null&&!['low','moderate','high','veryhigh','extreme'].includes(payload.danger))throw Error('Choose a fire danger preset.');
 const numerics=payload.numerics??selected[0]?.properties?.numerics??'super';solverSettings(numerics);
 const savedModel=String(selected[0]?.properties?.model??'').replace(/^ForeFire\//,'');const model=payload.model??(['Rothermel','Balbi2015','Farsite'].includes(savedModel)?savedModel:'Rothermel');if(!['Rothermel','Balbi2015','Farsite'].includes(model))throw Error('Choose Rothermel, Balbi 2015 or Farsite.');
 const windAdjustment=payload.windAdjustment??.7;if(!Number.isFinite(windAdjustment)||windAdjustment<0||windAdjustment>2)throw Error('Wind adjustment must be between 0 and 2.');
 const fuelsTable=payload.fuelsTable;if(fuelsTable!=null&&(typeof fuelsTable!=='string'||fuelsTable.length>1024*1024))throw Error('Fuel table must be CSV under 1 MB.');
 if(model==='Farsite'&&fuelsTable&&(!['Index','h1','h10','h100','lh','lw','dynamic','sav1','savlh','savlw','depth','xmext','heatd','heatl'].every(k=>fuelsTable.trim().split(/\r?\n/)[0].split(';').includes(k))))throw Error('Upload a Farsite fuel table indexed to the tile fuel IDs before restarting.');
 const source=sourceLinks(payload.source??gj.metadata?.source??selected[0]?.properties?.source);
 return {dangerPresets:payload.dangerPresets,minSpeed:payload.minSpeed??.001,danger:payload.danger??'high',diurnal:!!payload.diurnal,amplitude:payload.amplitude??.012,horizon:payload.horizon??21600,events:payload.events??[],model,windAdjustment,fuelsTable,numerics,source,initialBounds:{west,east,south,north},timestamp:new Date(time).toISOString(),epoch:time,center,size,polygons,points,wind:payload.wind??{mode:'manual',vector:[6,0]},geojson:{type:'FeatureCollection',features:selected},metadata:gj.metadata??{}};
}
export function project(domain,[lon,lat]){return [domain.originX+wrap(lon-domain.longitude)*METERS*Math.cos(lat*Math.PI/180),domain.originY+(lat-domain.latitude)*METERS];}
export function geographic(domain,[x,y]){const lat=domain.latitude+(y-domain.originY)/METERS;return [wrap(domain.longitude+(x-domain.originX)/(METERS*Math.cos(lat*Math.PI/180))),lat];}
export function signedArea(r){let a=0;for(let i=0;i<r.length;i++){const q=r[(i+1)%r.length];a+=r[i][0]*q[1]-q[0]*r[i][1];}return a/2;}
export function perimeterCommands(polygons,domain,time=0){
 const lines=[];
 for(const p of polygons)for(let h=0;h<p.length;h++){
  let r=p[h].slice(0,-1).map(q=>project(domain,q));
  if(r.some(([x,y])=>x<=domain.x0||y<=domain.y0||x>=domain.x0+domain.width||y>=domain.y0+domain.height))throw Error('Perimeter crosses the fixed domain boundary.');
  if(Math.abs(signedArea(r))<1)throw Error('Degenerate perimeter.');
  // ForeFire normals point left: outer boundaries clockwise, holes anticlockwise.
  if((signedArea(r)>0)!==(h>0))r.reverse();
  const indent='\t'.repeat(h?2:1);lines.push(`${indent}FireFront[t=${time}]`);
  for(const [x,y] of r)lines.push(`${indent}\tFireNode[loc=(${x},${y},0);vel=(0,0,0);t=${time};state=init]`);
 }
 return lines.join('\n');
}
export function parseFronts(text){
 const polygons=[],stack=[];let ring;
 for(const line of text.split('\n')){
  if(line.includes('FireFront[')){
   const depth=Math.max(1,line.match(/^\s*/)[0].replace(/    /g,'\t').length);ring=[];
   let polygon;if(depth%2===1){polygon=[ring];polygons.push(polygon);}else{polygon=stack[depth-1]?.polygon;if(polygon)polygon.push(ring);}
   stack.length=depth+1;stack[depth]={polygon};
  }else if(ring&&line.includes('FireNode[')){const m=/loc=\(([-\d.eE+]+),([-\d.eE+]+),/.exec(line);if(m)ring.push([+m[1],+m[2]]);}
 }
 return polygons.filter(p=>p[0].length>=3).map(p=>[p[0],...p.slice(1).filter(r=>r.length>=3)]);
}
export const NUMERICS={fast:{perimeterResolution:24,spatialIncrement:6},fine:{perimeterResolution:12,spatialIncrement:3},super:{perimeterResolution:128,spatialIncrement:32}};
export function solverSettings(mode='super'){if(!Object.hasOwn(NUMERICS,mode))throw Error('Choose standard, fine or ultrafast resolution.');return {...NUMERICS[mode]};}
export function arrivalSettings(mode='super'){solverSettings(mode);return {arrivalMapSubdivisions:mode==='fine'?512:mode==='super'?128:256};}
export function sourceLinks(source){
 if(!source||typeof source!=='object')return null;
 const valid=value=>{if(typeof value!=='string'||value.length>12000)return null;try{const u=new URL(value);if(u.origin!=='https://pyroscope.ecmwf-pyroscope.compute.cci2.ecmwf.int'||u.username||u.password||!u.pathname.endsWith('/event.html'))return null;return u.href;}catch{return null;}};
 const url=valid(source.url),shareUrl=valid(source.shareUrl);return url?{url,shareUrl:shareUrl??url,eventId:typeof source.eventId==='string'?source.eventId.slice(0,250):''}:null;
}
export function polygonArea(polygons){return polygons.reduce((a,p)=>a+Math.abs(signedArea(p[0]))-p.slice(1).reduce((b,r)=>b+Math.abs(signedArea(r)),0),0);}
export function interpolate(samples,ms){if(!samples.length||ms<samples[0].ms||ms>samples.at(-1).ms)throw Error('Wind coverage does not include this time.');let i=0;while(i<samples.length-2&&samples[i+1].ms<ms)i++;const a=samples[i],b=samples[Math.min(i+1,samples.length-1)],f=b.ms===a.ms?0:(ms-a.ms)/(b.ms-a.ms);return a.u.map((u,j)=>[u*(1-f)+b.u[j]*f,a.v[j]*(1-f)+b.v[j]*f]);}
