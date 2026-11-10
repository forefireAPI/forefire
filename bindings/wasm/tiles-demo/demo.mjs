const $=id=>document.getElementById(id);
const canvas=$('map'),ctx=canvas.getContext('2d'),terrain=document.createElement('canvas');
let worker,domain,fields,config,ignition,frames=[],index=0,playing=false,lastTick=0,view='fuel',running=false,ready=false,pendingRun=false;
let plot={x:0,y:0,w:1,h:1},cssW=1,cssH=1,zoom=1,panX=0,panY=0,drag;
$('provider').value='https://forefire.univ-corse.fr/fftiles/';
let burnable=new Set(),detail,previewTimer,previewId=0;
const detailTerrain=document.createElement("canvas");
const colors={0:[25,46,53],62:[117,127,119],73:[166,161,98],75:[143,148,92],82:[82,108,64],83:[62,89,65],102:[146,145,91],103:[126,132,71],104:[150,147,94],105:[129,137,82],106:[114,131,79],121:[40,72,78],162:[129,136,123]};
function status(text,error=false){$('status').textContent=text;$('status').classList.toggle('error',error);}
function busy(show,text){$('busy').classList.toggle('hidden',!show);if(text)$('busy-label').textContent=text;}
function setControls(){ $('run').disabled=!ready||running||!ignition;$('load').disabled=running;$('stop').classList.toggle('hidden',!running);$('add-fire').disabled=!ready||!ignition;$('reset').disabled=!ready;$('run').textContent=frames.length?'Continue simulation →':'Ignite & run →'; }
function readConfig(){return {provider:$('provider').value,center:[Number($('longitude').value),Number($('latitude').value)],size:$('size').value.split(',').map(Number)};}
function wind(){const speed=Number($('wind-speed').value),angle=Number($('wind-direction').value)*Math.PI/180;return [speed*Math.sin(angle),speed*Math.cos(angle)];}
function updateWind(){const deg=Number($('wind-direction').value),names=['N','NE','E','SE','S','SW','W','NW'];$('wind-speed-value').value=`${$('wind-speed').value} m/s`;$('wind-direction-value').value=`${names[Math.round(deg/45)%8]} · ${deg}°`;$('wind-arrow').style.transform=`rotate(${deg}deg)`;}
function geographic(x,y){const lat=domain.latitude+(y-domain.originY)/111195;return [domain.longitude+(x-domain.originX)/(111195*Math.cos(lat*Math.PI/180)),lat];}
function project(x,y){return [plot.x+(x-domain.x0)/domain.width*plot.w,plot.y+plot.h-(y-domain.y0)/domain.height*plot.h];}
function formatTime(seconds){return `${String(Math.floor(seconds/60)).padStart(2,'0')}:${String(Math.floor(seconds%60)).padStart(2,'0')}`;}
function updateMetrics(){const f=frames[index];$('time').textContent=formatTime(f?.time??0);$('area').innerHTML=f?`${(f.area/10000).toFixed(1)}<small> ha</small>`:'—<small> ha</small>';$('nodes').textContent=f?f.fronts.reduce((n,p)=>n+p.length,0):'—';$('timeline').max=Math.max(0,frames.length-1);$('timeline').value=index;$('timeline').disabled=frames.length<2;$('play').disabled=frames.length<2;$('play').textContent=playing?'Ⅱ':'▶';}
function renderTerrain(source=fields,target=terrain,width=domain?.width,height=domain?.height){
  if(!source)return;
  const {fuel,altitude}=source,w=fuel.width,h=fuel.height;target.width=w;target.height=h;
  const c=target.getContext('2d'),image=c.createImageData(w,h),z=altitude.data;
  let low=Infinity,high=-Infinity;for(const v of z){low=Math.min(low,v);high=Math.max(high,v);}
  const dx=width/w,dy=height/h;
  for(let y=0;y<h;y++)for(let x=0;x<w;x++){
    const i=y*w+x,id=fuel.data[i],p=((h-1-y)*w+x)*4;
    const gx=(z[y*w+Math.min(w-1,x+1)]-z[y*w+Math.max(0,x-1)])/(2*dx);
    const gy=(z[Math.min(h-1,y+1)*w+x]-z[Math.max(0,y-1)*w+x])/(2*dy);
    const shade=Math.max(.4,Math.min(1.2,.86+(-gx*.55+gy*.45)/Math.sqrt(1+gx*gx+gy*gy)));
    let rgb=colors[id]??[112+(id*7)%45,120+(id*3)%35,74+(id*11)%40];
    if(!burnable.has(id)&&id!==0&&id!==121)rgb=[126,135,124];
    if(view==='height'){const t=(z[i]-low)/Math.max(1,high-low);rgb=[55+160*t,82+108*t,66+76*t];}
    for(let k=0;k<3;k++)image.data[p+k]=Math.min(255,Math.round(rgb[k]*shade));image.data[p+3]=255;
  }
  c.putImageData(image,0,0);draw();
}
function draw(){
  ctx.clearRect(0,0,cssW,cssH);ctx.fillStyle='#18241e';ctx.fillRect(0,0,cssW,cssH);
  ctx.strokeStyle='#26392b';ctx.lineWidth=.6;for(let x=0;x<cssW;x+=36){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,cssH);ctx.stroke();}for(let y=0;y<cssH;y+=36){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(cssW,y);ctx.stroke();}
  if(!domain||!fields)return;
  const scale=Math.min((cssW-46)/domain.width,(cssH-40)/domain.height)*zoom;
  plot={w:domain.width*scale,h:domain.height*scale};plot.x=(cssW-plot.w)/2+panX;plot.y=(cssH-plot.h)/2+panY;
  ctx.save();ctx.beginPath();ctx.rect(plot.x,plot.y,plot.w,plot.h);ctx.clip();
  ctx.imageSmoothingEnabled=false;ctx.drawImage(terrain,plot.x,plot.y,plot.w,plot.h);
  if(detail){const [left,top]=project(detail.x,detail.y+detail.height);
    ctx.drawImage(detailTerrain,left,top,detail.width/domain.width*plot.w,detail.height/domain.height*plot.h);}
  // Half-degree tile boundaries are geographical curves in the Cartesian view.
  ctx.strokeStyle='#e1e8cb88';ctx.lineWidth=1;ctx.setLineDash([5,6]);
  const south=geographic(0,domain.y0)[1],north=geographic(0,domain.y0+domain.height)[1];
  for(let lat=Math.ceil(south*2)/2;lat<north;lat+=.5){const y=domain.originY+(lat-domain.latitude)*111195;const a=project(domain.x0,y),b=project(domain.x0+domain.width,y);ctx.beginPath();ctx.moveTo(...a);ctx.lineTo(...b);ctx.stroke();}
  const west=geographic(domain.x0,domain.originY)[0],east=geographic(domain.x0+domain.width,domain.originY)[0];
  for(let lon=Math.ceil(west*2)/2;lon<east;lon+=.5){ctx.beginPath();for(let i=0;i<=20;i++){const y=domain.y0+domain.height*i/20,lat=geographic(0,y)[1],x=domain.originX+(lon-domain.longitude)*111195*Math.cos(lat*Math.PI/180);const p=project(x,y);if(i)ctx.lineTo(...p);else ctx.moveTo(...p);}ctx.stroke();}
  ctx.setLineDash([]);
  const paint=(f,fill,stroke,width)=>{ctx.fillStyle=fill;ctx.strokeStyle=stroke;ctx.lineWidth=width;ctx.lineJoin='round';for(const polygon of f?.fronts??[]){ctx.beginPath();polygon.forEach(([x,y],i)=>{const p=project(x,y);if(i)ctx.lineTo(...p);else ctx.moveTo(...p);});ctx.closePath();ctx.fill();ctx.stroke();}};
  for(let i=0;i<index;i+=8)paint(frames[i],'#ed984109','#ffb55b33',.8);
  paint(frames[index],'#ff862137','#ffae65',2);
  if(ignition){const [x,y]=project(...ignition);ctx.strokeStyle='#ffd6a7';ctx.lineWidth=1.4;ctx.beginPath();ctx.arc(x,y,7,0,Math.PI*2);ctx.stroke();ctx.beginPath();ctx.moveTo(x-12,y);ctx.lineTo(x+12,y);ctx.moveTo(x,y-12);ctx.lineTo(x,y+12);ctx.stroke();ctx.fillStyle='#ff9d53';ctx.beginPath();ctx.arc(x,y,2.5,0,Math.PI*2);ctx.fill();}
  ctx.restore();ctx.strokeStyle='#76856588';ctx.lineWidth=1;ctx.strokeRect(plot.x,plot.y,plot.w,plot.h);
  const nominal=80/scale,steps=[50,100,200,500,1000,2000,5000,10000,20000,50000],meters=steps.reduce((best,v)=>Math.abs(v-nominal)<Math.abs(best-nominal)?v:best,1000);
  $('scale-label').textContent=meters>=1000?`${meters/1000} km`:`${meters} m`;document.querySelector('.scale i').style.width=`${meters*scale}px`;
}
function resize(){const r=canvas.getBoundingClientRect(),dpr=Math.min(2,devicePixelRatio||1);cssW=r.width;cssH=r.height;canvas.width=Math.round(cssW*dpr);canvas.height=Math.round(cssH*dpr);ctx.setTransform(dpr,0,0,dpr,0,0);draw();}
function chooseIgnition(){
  const {fuel}=fields,cx=fuel.width/2,cy=fuel.height/2;let chosen=null,distance=Infinity;
  for(let y=0;y<fuel.height;y++)for(let x=0;x<fuel.width;x++)if(burnable.has(fuel.data[y*fuel.width+x])){
    const d=(x-cx)**2+(y-cy)**2;if(d<distance){distance=d;chosen=[domain.x0+(x+.5)*domain.width/fuel.width,domain.y0+(y+.5)*domain.height/fuel.height];}}
  return chosen;
}
function load(nextConfig,autoRun=false){
  worker?.terminate();ready=false;running=false;playing=false;frames=[];index=0;pendingRun=autoRun;config=nextConfig;domain=null;fields=null;detail=null;clearTimeout(previewTimer);previewId++;zoom=1;panX=panY=0;draw();
  $('activity').replaceChildren();$('activity-count').textContent='0';updateMetrics();setControls();busy(true,'Fetching the landscape…');status('Connecting to the tile service.');
  worker=new Worker('/worker.mjs',{type:'module'});
  worker.onmessage=({data})=>{
    if(data.type==='tiles'){
      const li=document.createElement('li'),name=document.createElement('span'),result=document.createElement('span');name.textContent=data.tile.name;result.textContent=data.tile.available?`${(data.tile.bytes/1e6).toFixed(2)} MB`:'no data';li.append(name,result);if(!data.tile.available)li.className='missing';$('activity').append(li);$('activity-count').textContent=String(data.available+data.missing);$('tile-count').textContent=`${data.available} / ${data.total}`;$('bytes').innerHTML=`${(data.bytes/1e6).toFixed(1)}<small> MB</small>`;busy(true,`Fetching tiles · ${data.available+data.missing} of ${data.total}`);
    }else if(data.type==='status')busy(true,data.message);
    else if(data.type==='loaded'){
      domain=data.domain;fields={fuel:data.fuel,altitude:data.altitude};burnable=new Set(data.burnableFuels);ready=true;$('engine').textContent=`WASM ${data.version}`;renderTerrain();
      if(!pendingRun)ignition=chooseIgnition();draw();busy(false);setControls();
      const [lon,lat]=config.center;$('location').textContent=`${lat.toFixed(4)}° N  ${lon.toFixed(4)}° E`;$('subtitle').textContent=`${(domain.width/1000).toFixed(1)} × ${(domain.height/1000).toFixed(1)} km · ${data.available} tiles available · Rothermel`;
      status(ignition?'Ready. Scroll to zoom, drag to pan, click to select a fire, then Ignite & run.':'No burnable ground is available in this view. Try another center.');
      if(pendingRun){pendingRun=false;startRun();}
    }else if(data.type==='frame'){
      frames.push(data.frame);index=frames.length-1;updateMetrics();draw();status(`Simulating · ${(data.frame.time/60).toFixed(1)} min. Add fires or change wind at any time.`);
    }else if(data.type==='done'){
      running=false;setControls();status(`${data.reason} Continue, add fires, or replay the timeline.`);playing=false;updateMetrics();draw();
    }else if(data.type==='preview'){if(data.id===previewId){detail=data;renderTerrain(data,detailTerrain,data.width,data.height);}}else if(data.type==='running'){running=true;setControls();
    }else if(data.type==='notice'){status(data.message,true);
    }else if(data.type==='ignited'){status(`Fire added at ${(data.time/60).toFixed(1)} min.`);
    }else if(data.type==='wind'){status(`Wind applied at ${(data.time/60).toFixed(1)} min.`);
    }else if(data.type==='error'){
      running=false;pendingRun=false;busy(false);setControls();status(data.message,true);
    }
  };
  worker.onerror=error=>{busy(false);running=false;ready=false;setControls();status(`The browser could not start the WASM demo: ${error.message}`,true);};
  worker.postMessage({type:'init',config});
}
function startRun(){if(!ready||!ignition)return;playing=false;worker.postMessage({type:'run',ignition:frames.length?null:ignition,wind:wind(),duration:Number($('duration').value),pace:Number($('pace').value)});status('Starting simulation…');}
$('load-form').addEventListener('submit',event=>{event.preventDefault();ignition=null;load(readConfig());});
$('run').addEventListener('click',()=>startRun());
$('add-fire').addEventListener('click',()=>{playing=false;worker?.postMessage({type:'ignite',ignition});});
$('reset').addEventListener('click',()=>load(config));
$('stop').addEventListener('click',()=>worker?.postMessage({type:'cancel'}));
$('play').addEventListener('click',()=>{if(index>=frames.length-1)index=0;playing=!playing;lastTick=0;updateMetrics();});
$('timeline').addEventListener('input',()=>{playing=false;index=Number($('timeline').value);updateMetrics();draw();});
function applyWind(){updateWind();if(ready)worker.postMessage({type:'wind',wind:wind()});}
$('wind-speed').addEventListener('input',applyWind);$('wind-direction').addEventListener('input',applyWind);
$('fuel-view').addEventListener('click',()=>setView('fuel'));$('height-view').addEventListener('click',()=>setView('height'));
function setView(next){view=next;for(const [id,key] of [['fuel-view','fuel'],['height-view','height']]){$(id).classList.toggle('active',view===key);$(id).setAttribute('aria-pressed',String(view===key));}renderTerrain();if(detail)renderTerrain(detail,detailTerrain,detail.width,detail.height);}
function selectIgnition(event){if(!ready)return;const box=canvas.getBoundingClientRect(),x=event.clientX-box.left,y=event.clientY-box.top;if(x<plot.x||x>plot.x+plot.w||y<plot.y||y>plot.y+plot.h)return;ignition=[domain.x0+(x-plot.x)/plot.w*domain.width,domain.y0+(plot.y+plot.h-y)/plot.h*domain.height];draw();setControls();const [lon,lat]=geographic(...ignition);status(`Ignition selected · ${lat.toFixed(5)}° N, ${lon.toFixed(5)}° E`);}
canvas.addEventListener('mousemove',event=>{if(!domain)return;const box=canvas.getBoundingClientRect(),x=event.clientX-box.left,y=event.clientY-box.top;if(x<plot.x||x>plot.x+plot.w||y<plot.y||y>plot.y+plot.h)return;const [lon,lat]=geographic(domain.x0+(x-plot.x)/plot.w*domain.width,domain.y0+(plot.y+plot.h-y)/plot.h*domain.height);$('location').textContent=`${lat.toFixed(4)}° N  ${lon.toFixed(4)}° E`;});
function requestPreview(){
  clearTimeout(previewTimer);const id=++previewId;
  if(!ready||zoom<=1)return;
  previewTimer=setTimeout(()=>{
    const left=Math.max(0,plot.x),right=Math.min(cssW,plot.x+plot.w),top=Math.max(0,plot.y),bottom=Math.min(cssH,plot.y+plot.h);
    if(right<=left||bottom<=top)return;
    worker.postMessage({type:'preview',id,x:domain.x0+(left-plot.x)/plot.w*domain.width,
      y:domain.y0+(plot.y+plot.h-bottom)/plot.h*domain.height,
      width:(right-left)/plot.w*domain.width,height:(bottom-top)/plot.h*domain.height});
  },180);
}
function zoomAt(factor,x=cssW/2,y=cssH/2){
  const next=Math.max(1,Math.min(64,zoom*factor)),ratio=next/zoom;
  panX=(panX+cssW/2-x)*ratio+x-cssW/2;panY=(panY+cssH/2-y)*ratio+y-cssH/2;zoom=next;
  if(zoom===1){panX=panY=0;detail=null;}draw();requestPreview();
}
$('zoom-in').onclick=()=>zoomAt(1.5);$('zoom-out').onclick=()=>zoomAt(1/1.5);
$('fit').onclick=()=>{zoom=1;panX=panY=0;detail=null;previewId++;clearTimeout(previewTimer);draw();};
canvas.addEventListener('wheel',e=>{e.preventDefault();const r=canvas.getBoundingClientRect();zoomAt(Math.exp(-e.deltaY*.002),e.clientX-r.left,e.clientY-r.top);},{passive:false});
const pointers=new Map();let pinch;
canvas.addEventListener('pointerdown',e=>{canvas.setPointerCapture(e.pointerId);pointers.set(e.pointerId,[e.clientX,e.clientY]);drag={x:e.clientX,y:e.clientY,moved:false};if(pointers.size===2){pinch=[...pointers.values()];drag.moved=true;}});
canvas.addEventListener('pointermove',e=>{
  if(!pointers.has(e.pointerId))return;
  const old=pointers.get(e.pointerId);pointers.set(e.pointerId,[e.clientX,e.clientY]);
  if(pointers.size===2){const now=[...pointers.values()],distance=p=>Math.hypot(p[0][0]-p[1][0],p[0][1]-p[1][1]);
    if(pinch){const r=canvas.getBoundingClientRect();zoomAt(distance(now)/Math.max(1,distance(pinch)),(now[0][0]+now[1][0])/2-r.left,(now[0][1]+now[1][1])/2-r.top);}pinch=now;drag.moved=true;
  }else if(drag&&(drag.moved||Math.hypot(e.clientX-drag.x,e.clientY-drag.y)>4)){drag.moved=true;panX+=e.clientX-old[0];panY+=e.clientY-old[1];draw();requestPreview();}
});
function endPointer(e){const click=drag&&!drag.moved&&pointers.size===1;pointers.delete(e.pointerId);pinch=null;if(!pointers.size)drag=null;else if(drag)drag.moved=true;if(click&&e.type==='pointerup')selectIgnition(e);}
canvas.addEventListener('pointerup',endPointer);canvas.addEventListener('pointercancel',endPointer);
function animate(time){if(playing&&frames.length>1){if(!lastTick)lastTick=time;if(time-lastTick>100){index++;lastTick=time;if(index>=frames.length-1){index=frames.length-1;playing=false;}updateMetrics();draw();}}requestAnimationFrame(animate);}
new ResizeObserver(resize).observe(canvas);updateWind();requestAnimationFrame(animate);load(readConfig());
