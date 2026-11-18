import {interpolate,wrap} from './core.js?v=overlay-1';
import {ramp,WIND_COLORS,samplePreview} from './visuals.js?v=overlay-1';
export class WindOverlay {
 constructor(viewer,field,flow){this.viewer=viewer;this.field=field;this.flow=flow;this.ctx=field.getContext('2d');this.trails=flow.getContext('2d');this.enabled=false;this.dirty=true;this.particles=[];this.last=0;this.lastField=-Infinity;
  viewer.camera.percentageChanged=.01;viewer.camera.changed.addEventListener(()=>{this.dirty=true;this.clearTrails();});window.addEventListener('resize',()=>{this.dirty=true;this.clearTrails();});this.animate=this.animate.bind(this);
 }
 clearTrails(){this.trails.clearRect(0,0,this.flow.width,this.flow.height);}
 setEnabled(enabled){if(enabled===this.enabled)return;this.enabled=enabled;if(enabled){this.dirty=true;this.raf=requestAnimationFrame(this.animate);}else{cancelAnimationFrame(this.raf);this.ctx.clearRect(0,0,this.field.width,this.field.height);this.clearTrails();}}
 setWindow(window){if(this.sourceWindow===window&&this.window)return;this.sourceWindow=window;const source=window?.previewGeometry??window?.geometry;if(!source||!window.samples?.length)return;const g={...source,nx:Math.max(2,source.nx),ny:Math.max(2,source.ny)};this.window={geometry:g,samples:window.samples.map(s=>{const input=window.previewGeometry?s.preview:s;if(!input)return null;const u=[],v=[];for(let j=0;j<g.ny;j++)for(let i=0;i<g.nx;i++){const at=Math.min(j,source.ny-1)*source.nx+Math.min(i,source.nx-1);u.push(input.u[at]);v.push(input.v[at]);}return {ms:s.ms,u,v};}).filter(Boolean)};this.dirty=true;this.particles=[];}
 setTime(ms){if(this.ms!==ms){this.ms=ms;this.dirty=true;}}
 vectors(){if(!this.window)return null;try{const pairs=interpolate(this.window.samples,this.ms);return {u:pairs.map(p=>p[0]),v:pairs.map(p=>p[1])};}catch{return null;}}
 project(lon,lat){const C=globalThis.Cesium,p=C.Cartesian3.fromDegrees(lon,lat);return C.SceneTransforms.wgs84ToWindowCoordinates(this.viewer.scene,p);}
 resize(){const ratio=Math.min(devicePixelRatio||1,2),w=Math.round(innerWidth*ratio),h=Math.round(innerHeight*ratio);for(const canvas of [this.field,this.flow])if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;this.dirty=true;}this.ctx.setTransform(ratio,0,0,ratio,0,0);this.trails.setTransform(ratio,0,0,ratio,0,0);}
 drawField(vectors){const ctx=this.ctx;ctx.clearRect(0,0,this.field.width,this.field.height);if(!vectors)return;const g=this.window.geometry,nodes=[];
  for(let j=0;j<g.ny;j++)for(let i=0;i<g.nx;i++){const lon=g.west+(g.east-g.west)*i/(g.nx-1),lat=g.south+(g.north-g.south)*j/(g.ny-1),k=j*g.nx+i;nodes.push({p:this.project(lon,lat),lon,lat,u:vectors.u[k],v:vectors.v[k]});}
  for(let j=0;j<g.ny-1;j++)for(let i=0;i<g.nx-1;i++){const k=j*g.nx+i,q=[nodes[k],nodes[k+1],nodes[k+1+g.nx],nodes[k+g.nx]];if(q.some(n=>!n.p||!Number.isFinite(n.u)||!Number.isFinite(n.v)))continue;const speed=q.reduce((s,n)=>s+Math.hypot(n.u,n.v),0)/4,rgb=ramp(WIND_COLORS,speed/20);ctx.fillStyle=`rgba(${rgb.join(',')},.075)`;ctx.beginPath();q.forEach((n,a)=>a?ctx.lineTo(n.p.x,n.p.y):ctx.moveTo(n.p.x,n.p.y));ctx.closePath();ctx.fill();}
  // A screen-space lattice keeps arrows readable at every zoom level.
  // Geographic finite differences provide the local east/north→view Jacobian.
  const spacing=Math.max(48,Math.sqrt(innerWidth*innerHeight/192)),arrows=[];
  for(let sy=spacing/2;sy<innerHeight;sy+=spacing)for(let sx=spacing/2;sx<innerWidth;sx+=spacing){
   const C=globalThis.Cesium,ray=this.viewer.camera.getPickRay(new C.Cartesian2(sx,sy)),p=ray&&this.viewer.scene.globe.pick(ray,this.viewer.scene);if(!p)continue;
   const q=C.Cartographic.fromCartesian(p),lon=C.Math.toDegrees(q.longitude),lat=C.Math.toDegrees(q.latitude),w=samplePreview(g,vectors.u,vectors.v,lon,lat);if(!w)continue;
   const speed=Math.hypot(...w);if(speed<.2)continue;
   const h=20,e=this.project(lon+h/(111195*Math.cos(lat*Math.PI/180)),lat),n=this.project(lon,lat+h/111195),o=this.project(lon,lat);if(!e||!n||!o)continue;
   const dx=(e.x-o.x)*w[0]+(n.x-o.x)*w[1],dy=(e.y-o.y)*w[0]+(n.y-o.y)*w[1];if(Math.hypot(dx,dy)<1e-8)continue;
   arrows.push({x:sx,y:sy,angle:Math.atan2(dy,dx),length:17+Math.min(speed,20)*.6,color:ramp(WIND_COLORS,speed/20)});
  }
  ctx.lineCap='round';ctx.lineJoin='round';ctx.lineWidth=2.3;
  for(const a of arrows){const dx=Math.cos(a.angle)*a.length/2,dy=Math.sin(a.angle)*a.length/2;ctx.beginPath();ctx.moveTo(a.x-dx,a.y-dy);ctx.lineTo(a.x+dx,a.y+dy);for(const t of [-.55,.55]){ctx.moveTo(a.x+dx,a.y+dy);ctx.lineTo(a.x+dx-Math.cos(a.angle+t)*6,a.y+dy-Math.sin(a.angle+t)*6);}ctx.strokeStyle=`rgb(${a.color.join(',')})`;ctx.stroke();}

 }
 seed(p){const g=this.window.geometry,C=globalThis.Cesium;
  for(let attempt=0;attempt<4;attempt++){const screen=new C.Cartesian2(Math.random()*innerWidth,Math.random()*innerHeight),ray=this.viewer.camera.getPickRay(screen),point=ray&&this.viewer.scene.globe.pick(ray,this.viewer.scene);if(!point)continue;const q=C.Cartographic.fromCartesian(point),lon=C.Math.toDegrees(q.longitude),lat=C.Math.toDegrees(q.latitude),x=g.west+wrap(lon-g.west);if(x>=g.west&&x<=g.east&&lat>=g.south&&lat<=g.north){p.lon=lon;p.lat=lat;p.age=Math.random()*100;return;}}
  p.lon=g.west+Math.random()*(g.east-g.west);p.lat=g.south+Math.random()*(g.north-g.south);p.age=Math.random()*100;
 }
 animate(now){if(!this.enabled)return;this.raf=requestAnimationFrame(this.animate);if(now-this.last<32)return;const dt=Math.min(.06,(now-this.last)/1000||.03);this.last=now;this.resize();const vectors=this.vectors();if(this.dirty&&now-this.lastField>120){this.drawField(vectors);this.lastField=now;this.dirty=false;}
  if(!vectors){this.clearTrails();return;}if(matchMedia('(prefers-reduced-motion: reduce)').matches){this.clearTrails();return;}
  const ctx=this.trails;ctx.globalCompositeOperation='destination-in';ctx.fillStyle='rgba(0,0,0,.92)';ctx.fillRect(0,0,innerWidth,innerHeight);ctx.globalCompositeOperation='source-over';ctx.lineWidth=1.45;ctx.lineCap='round';
  const count=Math.max(500,Math.min(1800,Math.round(innerWidth*innerHeight/1100)));if(this.particles.length!==count)this.particles=Array.from({length:count},()=>{const p={};this.seed(p);return p;});
  const paths=Array.from({length:WIND_COLORS.length},()=>[]);
  for(const p of this.particles){const w=samplePreview(this.window.geometry,vectors.u,vectors.v,p.lon,p.lat);if(!w||p.age++>110){this.seed(p);continue;}const a=this.project(p.lon,p.lat);if(!a||a.x<0||a.y<0||a.x>innerWidth||a.y>innerHeight){this.seed(p);continue;}
   const lat=p.lat+w[1]*dt*350/111195,lon=p.lon+w[0]*dt*350/(111195*Math.cos(p.lat*Math.PI/180));let b=this.project(lon,lat);if(!b){this.seed(p);continue;}const distance=Math.hypot(b.x-a.x,b.y-a.y),factor=Math.min(1,36/Math.max(1,distance));p.lon+=wrap(lon-p.lon)*factor;p.lat+=(lat-p.lat)*factor;if(factor<1)b=this.project(p.lon,p.lat);if(!b)continue;
   const color=Math.min(WIND_COLORS.length-1,Math.floor(Math.hypot(...w)/20*(WIND_COLORS.length-1)));paths[color].push([a,b]);
  }
  for(let i=0;i<paths.length;i++){if(!paths[i].length)continue;ctx.beginPath();ctx.strokeStyle=WIND_COLORS[i]+'d9';for(const [a,b]of paths[i]){ctx.moveTo(a.x,a.y);ctx.lineTo(b.x,b.y);}ctx.stroke();}
 }
}
