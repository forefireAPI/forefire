// Pure meteorological helpers, shared by the browser and Node tests.
export const RAW_NAMES = {'0/6/192':'tcc','0/1/198':'sf','0/2/62':'ewss','0/2/63':'nsss','0/3/22':'slor','0/19/192':'asn','10/2/1':'sithick','10/3/1':'zos','10/3/14':'sve','10/3/15':'svn'};
export function rawParameter(template) {
  if (!template) return '';
  const b=Uint8Array.from(atob(template),c=>c.charCodeAt(0));
  if(b.length<16 || b[7]!==2) return '';
  const d=new DataView(b.buffer); let p=16;
  while(p+11<b.length){const n=d.getUint32(p);if(n<5||p+n>b.length)break;if(b[p+4]===4)return `${b[6]}/${b[p+9]}/${b[p+10]}`;p+=n;}
  return '';
}
export function rawName(template) {return RAW_NAMES[rawParameter(template)]||'';}
export function resolveVariable(name,raw,catalogue={}) {
  const entries=catalogue.variables||{};
  if(name!=='unknown'&&entries[name])return {name,entry:entries[name]};
  const alias=Object.entries(entries).find(([,v])=>v.aliases?.includes(name));
  if(alias)return {name:alias[0],entry:alias[1]};
  const matches=Object.entries(entries).filter(([,v])=>v.match&&[v.match.discipline,v.match.parameterCategory,v.match.parameterNumber].join('/')===raw);
  // Raw temperature codes, for example, may have different statistical meanings.
  // Resolve only unique matches; never guess units from data values.
  return matches.length===1?{name:matches[0][0],entry:matches[0][1]}:{name:name==='unknown'?(RAW_NAMES[raw]||name):name,entry:null};
}
export function buildInventory(groups,catalogue={}) {
  const views=new Map();
  for(const g of groups){
    const raw=rawParameter(g.options?.template_b64);
    for(const r of g.records||[]){
      const resolved=resolveVariable(r.shortName||g.short_name,raw,catalogue),name=resolved.name;
      const kind=r.typeOfLevel||g.type_of_level,level=String(r.level??g.level??'');
      const member=String(r.perturbationNumber||r.number||'');
      const key=[name,kind,level,member].join('|');
      if(!views.has(key))views.set(key,{key,name,kind,level,member,rawCode:raw,variable:resolved.entry,frames:[]});
      const ms=Date.parse(r.timestamp);
      if(Number.isFinite(ms))views.get(key).frames.push({group:g,frame:Number(r.frame_index),record:r,ms});
    }
  }
  for(const view of views.values())view.frames.sort((a,b)=>a.ms-b.ms||a.frame-b.frame);
  return [...views.values()];
}
export function displayConfiguration(view){
  const v=view?.variable;
  if(!v)return {title:view?.name||'Field',unit:'unit unknown',factor:1,offset:0,range:[0,1],palette:'wind',auto:true,decimals:3};
  return {title:v.title||v.name,description:v.name,sourceUnit:v.units,range:[0,1],...v.display};
}
export function formatReading(value,config){
  if(!Number.isFinite(value))return 'No data';
  const decimals=config.decimals??1,abs=Math.abs(value);
  return abs>=10000||(abs>0&&abs<.001)?value.toExponential(2):value.toFixed(decimals);
}
export function pairWind(u,v){
  if(!u||!v||u.kind!==v.kind||u.level!==v.level||u.member!==v.member)return [];
  const other=new Map(v.frames.map(f=>[f.ms,f]));
  return u.frames.filter(f=>other.has(f.ms)).map(f=>({...f,v:other.get(f.ms)}));
}
export function decodeValue(raw,bits,bounds,opts,hasBitmap,missing=false){
  if(missing||(!opts.bitmap_payload&&hasBitmap&&raw===0))return NaN;
  let q=Number(raw),shift=Number(opts.storage_shift||0);
  if(shift>0)q=Math.floor((q+2**(shift-1))/2**shift);
  const max=2**bits-1;q=Math.min(max,q);
  if(hasBitmap)q=Math.max(0,q-1);
  const norm=q/(hasBitmap?max-1:max);
  let value=bounds.min+norm*(bounds.max-bounds.min);
  if(opts.apply_shifted_log){const a=Number(opts.transform_scale??1);value=Number(opts.transform_origin??0)+Number(opts.transform_span??1)*Math.expm1(value*Math.log1p(a))/a;}
  else if(opts.apply_asinh)value=Number(opts.transform_scale??1)*Math.sinh(value);
  else if(opts.apply_log2)value=2**value-Number(opts.log_offset??1);
  return value;
}
export function makeGeometry(lat,lon,nx,ny,{gaussian=false,gaussianN}={}){
  if(lat.length!==ny||lon.length!==nx||nx<2||ny<2)throw new Error('This map needs a regular latitude/longitude grid with embedded coordinate axes.');
  const dy=(lat[ny-1]-lat[0])/(ny-1),dx=(lon[nx-1]-lon[0])/(nx-1);
  if(!dy||!dx||(!gaussian&&lat.some((v,j)=>Math.abs(v-lat[0]-j*dy)>.002))||lon.some((v,i)=>Math.abs(v-lon[0]-i*dx)>.002))throw new Error('Only regular latitude/longitude grids are supported.');
  const cyclic=Math.abs(Math.abs(dx)*nx-360)<.01;
  const geo={nx,ny,lat0:lat[0],lon0:lon[0],dx,dy,cyclic,kind:gaussian?'gaussian':'regular'};
  if(gaussian){
    if(lat.some((v,j)=>!Number.isFinite(v)||(j&&((v-lat[j-1])*dy<=0))))throw Error('Invalid Gaussian latitude order');
    geo.latitudes=Array.from(lat);geo.gaussianN=gaussianN;geo.rowAtLatitude=value=>latitudeRow(geo,value);
  }
  return geo;
}
export function latitudeRow(geo,lat){
  const a=geo.latitudes;if(!a)return (lat-geo.lat0)/geo.dy;
  if(!Number.isFinite(lat)||Math.abs(lat)>90)return NaN;
  const sign=a[a.length-1]>a[0]?1:-1,t=lat*sign;
  if(t<=a[0]*sign)return 0;if(t>=a[a.length-1]*sign)return a.length-1;
  let lo=0,hi=a.length-1;while(hi-lo>1){const mid=(lo+hi)>>1;if(a[mid]*sign<=t)lo=mid;else hi=mid;}
  return lo+(lat-a[lo])/(a[hi]-a[lo]);
}
export function gridIndex(geo,lat,lon){
  if(!Number.isFinite(lat)||!Number.isFinite(lon))return -1;
  const j=Math.round(latitudeRow(geo,lat));
  let x=(lon-geo.lon0)/geo.dx;
  if(geo.cyclic)x=((x%geo.nx)+geo.nx)%geo.nx;
  const i=Math.round(x)% (geo.cyclic?geo.nx:Infinity);
  return !Number.isFinite(j)||j<0||j>=geo.ny||i<0||i>=geo.nx?-1:j*geo.nx+i;
}
export function vectorAt(geo,u,v,lat,lon){const k=gridIndex(geo,lat,lon);return k<0?null:{u:u[k],v:v[k],speed:Math.hypot(u[k],v[k])};}
// Gaussian parallels are roots of P_(2N)(sin(latitude)), not an evenly spaced axis.
const gaussianCache=new Map();
export function gaussianLatitudes(count){
  if(!Number.isInteger(count)||count<2||count>8192||count%2)throw Error('Invalid full Gaussian latitude count');
  if(gaussianCache.has(count))return gaussianCache.get(count).slice();
  const lat=new Array(count),half=count/2;
  for(let i=0;i<half;i++){
    let z=Math.cos(Math.PI*(i+.75)/(count+.5)),converged=false;
    for(let iteration=0;iteration<16;iteration++){
      let p=1,previous=0;
      for(let k=1;k<=count;k++){const next=((2*k-1)*z*p-(k-1)*previous)/k;previous=p;p=next;}
      const derivative=count*(z*p-previous)/(z*z-1),next=z-p/derivative;
      if(Math.abs(next-z)<2e-15){z=next;converged=true;break;}z=next;
    }
    if(!converged)throw Error('Gaussian latitude calculation did not converge');
    lat[i]=Math.asin(z)*180/Math.PI;lat[count-1-i]=-lat[i];
  }
  if(gaussianCache.size>=4)gaussianCache.delete(gaussianCache.keys().next().value);
  gaussianCache.set(count,lat);return lat.slice();
}
function gaussianAxes(nx,ny,N,lat0,latEnd,lon0,lonEnd,di,scan,unit){
  if(nx<2||ny!==2*N||nx===65535||nx===0xffffffff||scan&48)return null;
  let lat=gaussianLatitudes(ny);if(scan&64)lat.reverse();
  if(Math.abs(lat[0]-lat0)>Math.max(4*unit,2e-6)||Math.abs(lat[ny-1]-latEnd)>Math.max(4*unit,2e-6))return null;
  const sign=scan&128?-1:1,delta=(lonEnd-lon0)*sign,span=((delta%360)+360)%360;
  const exact=360/nx,error=((lon0+sign*(nx-1)*exact-lonEnd+180)%360+360)%360-180;
  const dx=Math.abs(error)<=Math.max(4*unit,2e-6)?sign*exact:sign*di;
  if(!Number.isFinite(dx)||!dx||span<1)return null;
  return {lat,lon:Array.from({length:nx},(_,i)=>lon0+i*dx),gaussian:true,gaussianN:N};
}
// GRIB1 GDS representation 0: regular, unrotated latitude/longitude.
// https://codes.ecmwf.int/grib/format/grib1/grids/0/
function grib1Axes(b,d){
  const u24=p=>b[p]*65536+b[p+1]*256+b[p+2];
  if(b.length<16)return null;
  const pds=u24(8),p=8+pds;
  if(pds<8||p+32>b.length||!(b[15]&128))return null;
  const length=u24(p);if(length<32||p+length>b.length||![0,4].includes(b[p+5]))return null;
  const nx=d.getUint16(p+6),ny=d.getUint16(p+8),scan=b[p+27];
  // Reduced grids and unusual scan layouts must retain the native-raster fallback.
  if(nx<2||ny<2||nx===65535||ny===65535)return null;
  if(scan&48)throw new Error('Alternating or column-consecutive GRIB scanning is not supported by this map.');
  if(b[p+5]===4){
    const signed=off=>{const v=u24(p+off);return ((v&0x800000)?-(v&0x7fffff):v)/1000;};
    return gaussianAxes(nx,ny,d.getUint16(p+25),signed(10),signed(17),signed(13),signed(20),d.getUint16(p+23)/1000,scan,.001);
  }
  const di=d.getUint16(p+23),dj=d.getUint16(p+25);
  if(!(b[p+16]&128)||!di||!dj||di===65535||dj===65535)return null;
  const signed=off=>{const v=u24(p+off);return ((v&0x800000)?-(v&0x7fffff):v)/1000;};
  const lat0=signed(10),lon0=signed(13),dx=di/1000*((scan&128)?-1:1),dy=dj/1000*((scan&64)?1:-1);
  const latEnd=lat0+(ny-1)*dy,lonEnd=lon0+(nx-1)*dx;
  const lonDifference=((lonEnd-signed(20)+180)%360+360)%360-180;
  if(Math.abs(lat0)>90||Math.abs(latEnd)>90.002||Math.abs(latEnd-signed(17))>.002||Math.abs(lonDifference)>.002)return null;
  return {lat:Array.from({length:ny},(_,j)=>lat0+j*dy),lon:Array.from({length:nx},(_,i)=>lon0+i*dx)};
}
// Read GRIB2 template 3.0 rather than interpolating across a wrapped endpoint.
// A global grid may start at 180 E and finish at 179.75 E: its true dx is +.25.
export function gribAxes(template){
  if(!template)return null;
  const b=Uint8Array.from(atob(template),c=>c.charCodeAt(0)),d=new DataView(b.buffer);
  if(b.length<8)return null;
  if(b[7]===1)return grib1Axes(b,d);
  if(b.length<16||b[7]!==2)return null;
  let p=16;
  while(p+14<=b.length){
    const n=d.getUint32(p);if(n<5||p+n>b.length)break;
    if(b[p+4]===3){
      const type=d.getUint16(p+12);if(![0,40].includes(type)||n<72)return null;
      const signed=off=>{const v=d.getUint32(p+off);return (v&0x80000000)?-(v&0x7fffffff):v;};
      const nx=d.getUint32(p+30),ny=d.getUint32(p+34),basic=d.getUint32(p+38),sub=d.getUint32(p+42);
      const unit=basic&&basic!==0xffffffff&&sub&&sub!==0xffffffff?basic/sub:1e-6;
      const scan=b[p+71];if(scan&48)throw new Error('Alternating or column-consecutive GRIB scanning is not supported by this map.');
      const lat0=signed(46)*unit,lon0=signed(50)*unit;
      if(type===40)return gaussianAxes(nx,ny,d.getUint32(p+67),lat0,signed(55)*unit,lon0,signed(59)*unit,d.getUint32(p+63)*unit,scan,unit);
      const dx=d.getUint32(p+63)*unit*((scan&128)?-1:1),dy=d.getUint32(p+67)*unit*((scan&64)?1:-1);
      return {lat:Array.from({length:ny},(_,j)=>lat0+j*dy),lon:Array.from({length:nx},(_,i)=>lon0+i*dx)};
    }p+=n;
  }return null;
}
// Match FFmpeg's full-range 10/12-bit -> gray16 bit replication before
// applying the metadata's storage shift (not an RGB color conversion).
export function lumaStorageCode(raw,nativeBits,storedBits,fullRange=true){
  // WebCodecs returns native Y codes, not FFmpeg's full-range gray output.
  // Expand studio-range Y (16..235 at 8 bits) before scientific dequantization.
  if(fullRange===false){
    const factor=2**(nativeBits-8),fraction=(raw-16*factor)/(219*factor);
    return Math.round(Math.max(0,Math.min(1,fraction))*(2**storedBits-1));
  }
  const shift=storedBits-nativeBits;
  if(shift===0)return raw;
  if(shift<0)return Math.floor(raw/2**(-shift));
  return raw*2**shift+Math.floor(raw/2**(nativeBits-shift));
}

// Missing geometry can be previewed as an explicitly assumed global raster.
// Recognized projected/unsupported-scan grids retain native mode unless forced.
export function streamGeometry(group,{assumeGlobal=false}={}){
  const nx=Number(group.options.crop_width),ny=Number(group.options.crop_height);
  if(!Number.isSafeInteger(nx)||!Number.isSafeInteger(ny)||nx<1||ny<1)throw new Error('Invalid native grid dimensions');
  try{const axes=gribAxes(group.options.template_b64);if(axes)return {...makeGeometry(axes.lat,axes.lon,nx,ny,axes),native:false};}catch{/* Explicit fallback below. */}
  let recognizable=false;
  try{const b=Uint8Array.from(atob(group.options.template_b64||''),c=>c.charCodeAt(0));recognizable=b.length>=8&&[1,2].includes(b[7]);}catch{}
  const plausibleGlobal=nx>=360&&ny>=180&&Math.abs(nx/ny-2)<.08;
  if(assumeGlobal||(!recognizable&&plausibleGlobal))return {nx,ny,native:false,assumed:true,kind:'assumed-global',lat0:90,lon0:0,dx:360/nx,dy:-180/(ny-1),cyclic:true};
  return {nx,ny,native:true,lat0:0,lon0:0,dx:1,dy:-1,cyclic:false};
}
export function compatibleGeometry(a,b){return a.native===b.native&&Boolean(a.assumed)===Boolean(b.assumed)&&(a.kind||'regular')===(b.kind||'regular')&&['nx','ny','lat0','lon0','dx','dy'].every(k=>Math.abs(a[k]-b[k])<1e-6)&&(!a.latitudes&&!b.latitudes||a.latitudes?.length===b.latitudes?.length&&a.latitudes.every((v,i)=>Math.abs(v-b.latitudes[i])<1e-8));}
