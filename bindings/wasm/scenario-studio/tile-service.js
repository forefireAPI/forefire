/** Browser/Node transport for the same fixed-domain .fftile loadData contract.
 * Fetch compressed files asynchronously, stage them in MEMFS, then let the
 * C++ tile provider decode and page them. No JS image decoder or NetCDF.
 */
export const METERS = 111195;
const radians = Math.PI / 180;
export const wrapLongitude = lon => ((lon + 180) % 360 + 360) % 360 - 180;
export function tileServiceGeometry({center, size = [2, 2]}) {
  const [lon, lat] = center ?? [], [w, h] = size;
  if (![lon, lat, w, h].every(Number.isFinite) || lon < -180 || lon >= 180 ||
      w <= 0 || w > 2 || h <= 0 || h > 2 || lat-h/2 <= -90 || lat+h/2 >= 90)
    throw new Error('Choose a valid center and a domain size between 0 and 2 degrees.');
  const width = w * METERS * Math.min(Math.cos((lat-h/2)*radians), Math.cos((lat+h/2)*radians));
  const height = h * METERS;
  const geographic = (x, y) => {
    const latitude = lat + (y-height/2)/METERS;
    return [wrapLongitude(lon + (x-width/2)/(METERS*Math.cos(latitude*radians))), latitude];
  };
  let west = Infinity, east = -Infinity;
  for (const x of [0, width]) for (const y of [0, height]) {
    const [longitude] = geographic(x,y);
    const unwrapped = lon + wrapLongitude(longitude-lon);
    west = Math.min(west,unwrapped); east = Math.max(east,unwrapped);
  }
  const tiles = [];
  const firstLat = Math.floor((lat-h/2+90)*2), lastLat = Math.ceil((lat+h/2+90)*2)-1;
  const firstLon = Math.floor((west+180)*2), lastLon = Math.ceil((east+180)*2)-1;
  for (let li=firstLat; li<=lastLat; li++) for (let lj=firstLon; lj<=lastLon; lj++) {
    const wrapped = (lj%720+720)%720;
    tiles.push({latIndex:li,lonIndex:wrapped,name:`${li/2-90}_${wrapped/2-180}.fftile`});
  }
  return {width,height,center:[lon,lat],size:[w,h],tiles,geographic};
}
function validate(bytes,tile) {
  if (bytes.length<256 || new TextDecoder().decode(bytes.subarray(0,8))!=='FFTILE01')
    throw new Error(`${tile.name}: invalid .fftile header`);
  const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
  if (view.getInt16(8,true)!==tile.latIndex || view.getInt16(10,true)!==tile.lonIndex)
    throw new Error(`${tile.name}: service returned a different tile`);
  const flags=view.getUint16(12,true);
  if (!(flags&1) || (flags&~3)) throw new Error(`${tile.name}: unsupported tile flags`);
  let end=256;
  for(let i=0;i<4;i++) {
    const at=view.getUint32(160+i*8,true),size=view.getUint32(164+i*8,true);
    if(at!==end || !size || at+size>bytes.length)throw new Error(`${tile.name}: invalid payload bounds`);
    end=at+size;
  }
  if(end!==bytes.length)throw new Error(`${tile.name}: unexpected trailing data`);
  const expected=view.getUint32(200,true);
  if(expected) {
    let crc=0xffffffff;
    for(let i=0;i<200;i++){crc^=bytes[i];for(let b=0;b<8;b++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
    if(((crc^0xffffffff)>>>0)!==expected)throw new Error(`${tile.name}: header CRC mismatch`);
  }
}
async function download(url, fetcher) {
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),30000);
  try {
    const response=await fetcher(url,{signal:controller.signal});
    if(response.status===404 || response.status===410)return null;
    if(!response.ok)throw new Error(`Tile service returned HTTP ${response.status}`);
    const limit=64*1024*1024;
    if(Number(response.headers.get('content-length'))>limit)throw new Error('Tile exceeds 64 MiB');
    if(!response.body)throw new Error('Tile service returned an empty response');
    const reader=response.body.getReader(),chunks=[];let length=0;
    while(true){const {done,value}=await reader.read();if(done)break;length+=value.byteLength;
      if(length>limit){await reader.cancel();throw new Error('Tile exceeds 64 MiB');}chunks.push(value);}
    const bytes=new Uint8Array(length);let at=0;
    for(const chunk of chunks){bytes.set(chunk,at);at+=chunk.length;}return bytes;
  } finally {clearTimeout(timer);}
}
export function terrainRegions(polygons=[],points=[],center=[0,0],margin=5000){
 const result=[],dy=margin/METERS;
 const add=ring=>{if(!ring.length)return;let west=Infinity,east=-Infinity,south=Infinity,north=-Infinity;for(const [lon,lat]of ring){const x=center[0]+wrapLongitude(lon-center[0]);west=Math.min(west,x);east=Math.max(east,x);south=Math.min(south,lat);north=Math.max(north,lat);}const dx=margin/(METERS*Math.max(.01,Math.min(Math.cos(south*radians),Math.cos(north*radians))));result.push({west:west-dx,east:east+dx,south:south-dy,north:north+dy});};
 for(const polygon of polygons)add(polygon[0]??[]);for(const p of points)add([p]);return result;
}
export async function loadTileService(Module, ff, {provider,center,size=[2,2],onProgress=()=>{},onLoadStart=()=>{},fetcher=fetch,regions=null,restore=null}) {
 const url=new URL(provider);
 if(!['http:','https:'].includes(url.protocol)||url.search||url.hash)throw Error('Use an HTTP(S) tile-directory URL without a query or fragment.');
 if(ff.tileDomain())throw Error('Create a fresh WASM instance before loading another service.');
 const geometry=tileServiceGeometry({center,size}),root='/tile-service';Module.FS.mkdirTree(root);
 const attempted=new Set(),results=[];let available=0,missing=0,totalBytes=0,queue=Promise.resolve();
 function select(regions){return geometry.tiles.filter(tile=>{const south=tile.latIndex/2-90,west=center[0]+wrapLongitude(tile.lonIndex/2-180-center[0]);return regions===null||regions.some(r=>r.east>=west-1e-10&&r.west<=west+.5+1e-10&&r.north>=south-1e-10&&r.south<=south+.5+1e-10);});}
 async function stage(regions){const pending=select(regions).filter(t=>!attempted.has(t.name));if(!pending.length)return false;
  onLoadStart({names:pending.map(t=>t.name)});let next=0;
  await Promise.all(Array.from({length:Math.min(3,pending.length)},async()=>{while(next<pending.length){const tile=pending[next++],tileURL=new URL(`${url.href.replace(/\/+$/,'')}/${tile.name}`),bytes=await download(tileURL.href,fetcher);
   if(bytes){validate(bytes,tile);Module.FS.writeFile(`${root}/${tile.name}`,bytes);available++;totalBytes+=bytes.length;}else missing++;
   attempted.add(tile.name);const entry={name:tile.name,available:!!bytes,bytes:bytes?.length??0};results.push(entry);
   onProgress({available,missing,bytes:totalBytes,total:geometry.tiles.length,tile:entry});
  }}));
  if(ff.tileDomain())ff.refreshTiles();return true;
 }
 const ensureRegions=regions=>{const job=queue.then(()=>stage(regions));queue=job.catch(()=>{});return job;};
 await ensureRegions(regions);
 const [lon,lat]=center,[w,h]=size;
 const output=ff.execute(restore?`loadState[file=${restore};provider=${root}]`:`loadData[provider=${root};center=(${lon},${lat});size=(${w},${h});prefetch=0]`),domain=ff.tileDomain();
 if(!domain)throw Error(output||'The tile domain could not be created.');
 return {domain,provider:url.href,ensureRegions,get available(){return available;},get missing(){return missing;},get bytes(){return totalBytes;},get tiles(){return results.slice();},total:geometry.tiles.length};
}
