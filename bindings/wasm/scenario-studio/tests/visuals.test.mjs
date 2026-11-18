import {test} from 'node:test';import assert from 'node:assert/strict';import vm from 'node:vm';import {readFile} from 'node:fs/promises';
import {AvencRangeModel} from '../vendor/avenc/avenc_range_model.js';import {zlibSync} from '../vendor/fflate.js';
import {buildBands,fireColor,samplePreview,ramp,WIND_COLORS} from '../visuals.js';import {polygonArea} from '../core.js';import {Weather} from '../weather.js';
const context={};vm.runInNewContext(await readFile(new URL('../vendor/polygon-clipping.umd.min.js',import.meta.url),'utf8'),context);const clip=context.polygonClipping;
const square=(x0,y0,x1,y1)=>[[[x0,y0],[x1,y0],[x1,y1],[x0,y1],[x0,y0]]];
test('hourly fills are disjoint, preserve holes, and progress from red to turquoise',()=>{
 const snapshots=[{time:0,polygons:[square(0,0,1,1)]},{time:3600,polygons:[square(-.5,-.5,1.5,1.5)]},{time:7200,polygons:[square(-1,-1,2,2)]}];
 const bands=buildBands(snapshots,clip);assert.equal(bands.length,3);assert.equal(bands[0].color,'#343c47');assert(Math.abs(polygonArea(bands[0].polygons)-1)<2e-6);assert(Math.abs(polygonArea(bands[1].polygons)-3)<2e-6);assert(Math.abs(polygonArea(bands[2].polygons)-5)<2e-6);assert.equal(clip.intersection(bands[1].polygons,bands[2].polygons).length,0);assert.equal(bands[1].color,fireColor(0));assert.equal(bands[2].color,fireColor(1));
 const outer=square(0,0,3,3),hole=square(1,1,2,2)[0];const b=buildBands([{time:0,polygons:[[outer[0],hole]]}],clip);assert.equal(b[0].polygons[0].length,2);assert(Math.abs(polygonArea(b[0].polygons)-8)<2e-6);
});
test('display union retains past burned extent without modifying solver snapshots, including the dateline',()=>{const a={time:0,polygons:[square(179.5,0,-179.5,1)]},b={time:3600,polygons:[square(179.6,0,-179.4,1)]};const before=JSON.stringify([a,b]);const bands=buildBands([a,b],clip,180);assert(Math.abs(bands.reduce((s,b)=>s+polygonArea(b.polygons),0)-1.1)<2e-6);assert.equal(JSON.stringify([a,b]),before);});
test('preview vectors interpolate in space and preserve missing data',()=>{const g={west:8,east:9,south:41,north:42,nx:2,ny:2};assert.deepEqual(samplePreview(g,[0,2,4,6],[6,4,2,0],8.5,41.5),[3,3]);assert.equal(samplePreview(g,[NaN,2,4,6],[6,4,2,0],8.5,41.5),null);assert.equal(samplePreview(g,[0,2,4,6],[6,4,2,0],9.1,41.5),null);});
test('ERA preview uses the decoded source grids independently from the solver control grid',async()=>{const weather=new Weather({center:[8.5,41.5],size:[.08,.08]});const ms=Date.parse('2011-08-05T12:00:00Z'),g={nx:2,ny:2,lon0:8,lat0:41,dx:1,dy:1,cyclic:false};const U={id:1},V={id:2};weather.year=async()=>({frames:[{ms,group:U,frame:0,v:{group:V,frame:0}}],model:{getGrid:async group=>group.id===1?[0,2,4,6]:[6,4,2,0],geometryForGroup:()=>g}});const s=await weather.sample(ms);assert.equal(s.u.length,25);assert.equal(s.preview.u.length,33*33);assert(Math.abs(s.preview.u[544]-3)<2e-6);assert(Math.abs(s.preview.v[544]-3)<2e-6);assert(Math.abs(s.u[12]-3)<2e-6);});

test('precision retries recover band assembly and preserve outlines if all filled attempts fail',()=>{
 const snapshots=[{time:0,polygons:[square(2.60,43,2.61,43.01)]},{time:3600,polygons:[square(2.59999999999999,42.99999999999999,2.62,43.02)]}];
 let calls=0;const fragile={union(...args){if(calls++===0)throw Error('Unable to complete output ring');return clip.union(...args);},difference:(...args)=>clip.difference(...args)};
 const before=JSON.stringify(snapshots),recovered=buildBands(snapshots,fragile,2.66);assert(recovered.every(b=>b.fill&&b.precisionMeters===.1));assert.equal(JSON.stringify(snapshots),before);
 const fallback=buildBands(snapshots,{union(){throw Error('Unable to complete output ring');}},2.66);assert.equal(fallback.length,2);assert(fallback.every(b=>b.fill===false&&b.polygons.length));assert.equal(JSON.stringify(snapshots),before);
});

test('ERA bitmap bypass keeps the encoded damped values and preserves bitmap-aware quantization',async()=>{
 const bytes=zlibSync(Uint8Array.from([0,1,0,1])),bitmap=Buffer.from(bytes).toString('base64');
 async function decode(applyBitmap){const model=new AvencRangeModel({applyBitmap}),group={id:1,options:{crop_width:2,crop_height:2,bits:8,bitmap_payload:bitmap}};
  model.geometryForGroup=()=>({nx:2,ny:2});model.selectGroup=async()=>{model.currentGroup=group;};model._getFrameRecord=()=>({bitmapPresent:true});model._getFrameQuantBounds=()=>({min:0,max:10});model._ensureDecodedFrame=async()=>({frame:{format:'I420',colorSpace:{fullRange:true},allocationSize:()=>4,copyTo:async out=>{out.set([128,128,255,0]);return [{offset:0,stride:2}];}}});
  return model.getGrid(group,0,new AbortController().signal);
 }
 const normal=await decode(true),damped=await decode(false);assert(Number.isNaN(normal[1]));assert(Number.isNaN(normal[3]));assert.deepEqual([...damped],[5,5,10,0]);assert.equal(damped[0],normal[0]);assert.equal(damped[2],normal[2]);
});

test('screen-space wind arrows use a single velocity-coloured stroke without outlines',async()=>{
 const {WindOverlay}=await import('../wind-overlay.js');const saved={window:globalThis.window,Cesium:globalThis.Cesium,innerWidth:globalThis.innerWidth,innerHeight:globalThis.innerHeight};
 let moves=0;const strokes=[];const ctx={clearRect(){},beginPath(){moves=0;},moveTo(){moves++;},lineTo(){},closePath(){},fill(){},stroke(){strokes.push({width:this.lineWidth,arrows:moves/3,color:this.strokeStyle});}};
 try{globalThis.window={addEventListener(){}};globalThis.innerWidth=1280;globalThis.innerHeight=720;globalThis.Cesium={Cartesian2:class{constructor(x,y){this.x=x;this.y=y;}},Cartographic:{fromCartesian:p=>p},Math:{toDegrees:x=>x}};
 const viewer={camera:{changed:{addEventListener(){}},getPickRay:p=>p},scene:{globe:{pick:p=>({longitude:p.x/640,latitude:2-p.y/360})}}};
 const canvas={getContext:()=>ctx,width:1280,height:720},overlay=new WindOverlay(viewer,canvas,canvas);overlay.project=(lon,lat)=>({x:lon*640,y:(2-lat)*360});overlay.window={geometry:{west:0,east:2,south:0,north:2,nx:3,ny:3}};overlay.drawField({u:Array(9).fill(4),v:Array(9).fill(1)});
 assert(strokes.length>=150&&strokes.length<=220);assert(strokes.every(s=>s.width===2.3));assert(strokes.every(s=>s.arrows===1));assert(strokes.every(s=>s.color==='rgb('+ramp(WIND_COLORS,Math.hypot(4,1)/20).join(',')+')'));
 overlay.window.geometry={west:-50,east:50,south:-50,north:50,nx:3,ny:3};for(let i=0;i<20;i++){const p={};overlay.seed(p);assert(p.lon>=0&&p.lon<=2&&p.lat>=0&&p.lat<=2,'particles should seed in the viewport rather than across the weather domain');}
 }finally{Object.assign(globalThis,saved);}
});

test('dominant hourly advances are capped at two and ignore disconnected new ignitions',async()=>{const {spreadVectors}=await import('../scenario.js');const frame=(time,polygons)=>({time,polygons});const old=frame(0,[square(8.9,42,8.91,42.01)]),current=frame(3600,[square(8.898,41.998,8.93,42.015),square(9.2,42.2,9.21,42.21)]);const arrows=spreadVectors(old,current,100);assert(arrows.length>=1&&arrows.length<=2);assert(arrows.every(v=>v.to[0]<9));assert(arrows.every(v=>v.speed>0&&v.speed===v.distance/3600));assert.equal(spreadVectors(old,{...old,time:3600}).length,0);assert.equal(spreadVectors(null,current).length,0);});
