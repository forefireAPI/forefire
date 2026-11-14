// Paper-based subset: Allaire, Filippi & Mallet (2020), section 2.2 / Table 2.
// Global biases are held constant through each member; no fitted site calibration.
export function perturbations(count=15,seed=19073,{directionSigma=30,speedMax=1.5,moistureSigma=.15}={}){
 if(!Number.isInteger(count)||count<3||count>15||(!Number.isInteger(seed)||seed<0||seed>4294967295)||!Number.isFinite(directionSigma)||directionSigma<0||directionSigma>90||!Number.isFinite(speedMax)||speedMax<1||speedMax>3||!Number.isFinite(moistureSigma)||moistureSigma<0||moistureSigma>.3)throw Error('Invalid ensemble settings.');
 let state=seed>>>0;const random=()=>{state+=0x6D2B79F5;let t=state;t=Math.imul(t^t>>>15,t|1);t^=t+Math.imul(t^t>>>7,t|61);return ((t^t>>>14)>>>0)/4294967296;};
 const normal=()=>{for(;;){const z=Math.sqrt(-2*Math.log(Math.max(1e-12,random())))*Math.cos(2*Math.PI*random());if(Math.abs(z)<=2)return z;}};
 return Array.from({length:count},(_,i)=>({member:i+1,direction:normal()*directionSigma,speed:Math.exp(normal()*.5*Math.log(speedMax)),moisture:1+normal()*moistureSigma}));
}
export function perturbVector(u,v,p){const a=p.direction*Math.PI/180,c=Math.cos(a),s=Math.sin(a);return [(u*c+v*s)*p.speed,(-u*s+v*c)*p.speed];}
const unwrap=(x,center)=>center+((x-center+180)%360+360)%360-180;
export function ensembleGrid(members,center,width=384){
 let west=Infinity,east=-Infinity,south=Infinity,north=-Infinity;
 for(const member of members)for(const f of member.frames)for(const p of f.polygons)for(const r of p)for(const [x,y]of r){const lon=unwrap(x,center);west=Math.min(west,lon);east=Math.max(east,lon);south=Math.min(south,y);north=Math.max(north,y);}
 if(!Number.isFinite(west))throw Error('No ensemble perimeters.');
 const dx=Math.max(.0002,(east-west)*.025),dy=Math.max(.0002,(north-south)*.025);west-=dx;east+=dx;south-=dy;north+=dy;
 return {west,east,south,north,width,height:width,center};
}
// Scanline fill, even/odd holes, union over times: burned areas stay burned.
export function burnCounts(members,time,g){
 const counts=new Uint8Array(g.width*g.height),dx=(g.east-g.west)/g.width,dy=(g.north-g.south)/g.height;
 for(const member of members){const mask=new Uint8Array(counts.length);
  for(const f of member.frames){if(f.time>time+1e-7)continue;
   for(const poly of f.polygons){const rings=poly.map(r=>r.map(([x,y])=>[unwrap(x,g.center),y]));let ymin=Infinity,ymax=-Infinity;for(const r of rings)for(const p of r){ymin=Math.min(ymin,p[1]);ymax=Math.max(ymax,p[1]);}const lo=Math.max(0,Math.floor((g.north-ymax)/dy)),hi=Math.min(g.height-1,Math.floor((g.north-ymin)/dy));
    for(let row=lo;row<=hi;row++){const y=g.north-(row+.5)*dy,xs=[];
     for(const r of rings)for(let i=0,j=r.length-1;i<r.length;j=i++){const a=r[i],b=r[j];if((a[1]>y)!==(b[1]>y))xs.push(a[0]+(y-a[1])*(b[0]-a[0])/(b[1]-a[1]));}
     xs.sort((a,b)=>a-b);for(let k=0;k+1<xs.length;k+=2){const first=Math.max(0,Math.ceil((xs[k]-g.west)/dx-.5)),last=Math.min(g.width-1,Math.floor((xs[k+1]-g.west)/dx-.5));if(last>=first)mask.fill(1,row*g.width+first,row*g.width+last+1);}
    }
   }
  }
  for(let i=0;i<mask.length;i++)counts[i]+=mask[i];
 }
 return counts;
}
export function probabilityPixels(counts,n,quantile=0,initialCounts=null){
 const pixels=new Uint8ClampedArray(counts.length*4),colors=[[46,198,214],[85,211,160],[247,198,82],[255,122,66],[224,61,86]];
 const threshold=quantile?Math.ceil(n*quantile-1e-10):1;
 for(let i=0;i<counts.length;i++){if(counts[i]<threshold)continue;const p=counts[i]/n,c=initialCounts?.[i]===n?[113,124,137]:colors[counts[i]===n?4:Math.min(3,Math.floor(p*4))];pixels.set([...c,quantile?165:Math.round(65+p*145)],i*4);}return pixels;
}
export function runMember(config,{onProgress=()=>{},signal,makeWorker=(url,opts)=>new Worker(url,opts)}={}){
 return new Promise((resolve,reject)=>{
  const worker=makeWorker(new URL('./worker.js?v=ensemble-2',import.meta.url),{type:'module'}),weather=makeWorker(new URL('./weather-worker.js?v=ensemble-2',import.meta.url)),frames=[];
  let timer,last;const cleanup=()=>{clearTimeout(timer);worker.terminate();weather.terminate();signal?.removeEventListener('abort',abort);};
  const abort=()=>{cleanup();reject(new DOMException('Ensemble cancelled','AbortError'));};if(signal?.aborted){abort();return;}signal?.addEventListener('abort',abort,{once:true});
  const fail=e=>{cleanup();reject(Error(e.message||String(e)));};worker.onerror=weather.onerror=fail;
  timer=setTimeout(()=>fail(Error('Ensemble member timed out after 20 minutes.')),1200000);
  weather.postMessage({type:'init',config:{center:config.center,size:config.size,spatial:config.wind.spatial??'grid'}});
  weather.onmessage=({data:d})=>{if(d.type==='weather')worker.postMessage(d);};
  const remember=f=>{if(frames.at(-1)?.time===f.time)frames[frames.length-1]=f;else frames.push(f);};
  worker.onmessage=({data:d})=>{
   if(d.type==='error'){fail(Error(d.message));return;}
   if(d.type==='weather-needed')weather.postMessage({type:'preload',id:d.id,ms:d.ms,hours:Math.min(168,Math.max(1,(config.epoch+config.horizon*1000-d.ms)/3600000))});
   if(d.type==='loaded'){remember(d.frame);const duration=config.horizon-d.frame.time;if(duration<=0){cleanup();resolve({frames,perturbation:config.perturbation});return;}worker.postMessage({type:'run',duration,pace:0});}
   if(d.type==='frame'){last=d.frame;if(d.frame.time<=3600||d.frame.time%3600===0||d.frame.time>=config.horizon)remember(d.frame);onProgress(d.frame.time);}
   if(d.type==='paused'){if(last)remember(last);cleanup();resolve({frames,perturbation:config.perturbation});}
  };
  worker.postMessage({type:'init',config});
 });
}
