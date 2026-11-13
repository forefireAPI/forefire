/** DOM-unit checks for the production map's coordinate transforms and controls.
 * This does not replace testing rendering and gestures in a browser.
 */
import {test} from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import {readFile} from 'node:fs/promises';
const source=await readFile(new URL('./demo.mjs',import.meta.url),'utf8');
test('zoom preserves cursor coordinates; panning keeps ignition selection accurate; controls retain the worker',()=>{
 const elements=new Map(),messages=[];
 const paint=new Proxy({createImageData:(w,h)=>({data:new Uint8ClampedArray(w*h*4)})},{get:(t,k)=>t[k]??(()=>{})});
 function element(){return {value:'',style:{},listeners:{},classList:{toggle(){}},getContext:()=>paint,
  getBoundingClientRect:()=>({left:0,top:0,width:1000,height:600}),setPointerCapture(){},setAttribute(){},replaceChildren(){},
  addEventListener(name,callback){this.listeners[name]=callback;}};}
 const document={getElementById(id){if(!elements.has(id))elements.set(id,element());return elements.get(id);},createElement:element,querySelector:()=>element()};
 for(const [id,value] of Object.entries({longitude:'8.9',latitude:'42.01',size:'.08,.04','wind-speed':'8','wind-direction':'45',duration:'1800',pace:'500'}))document.getElementById(id).value=value;
 let worker;
 const context=vm.createContext({document,URL,location:{href:'http://localhost:8765'},devicePixelRatio:1,Uint8ClampedArray,Set,Map,console,
  setTimeout:()=>1,clearTimeout(){},requestAnimationFrame(){},ResizeObserver:class{constructor(cb){this.cb=cb;}observe(){this.cb();}},
  Worker:class{constructor(){worker=this;}postMessage(data){messages.push(data);}terminate(){throw new Error('Interaction discarded the simulation');}}});
 vm.runInContext(source,context);
 worker.onmessage({data:{type:'loaded',domain:{width:10000,height:5000,x0:0,y0:0,originX:5000,originY:2500,longitude:8.9,latitude:42.01},
  fuel:{width:4,height:4,data:Array(16).fill(104)},altitude:{width:4,height:4,data:Array(16).fill(100)},burnableFuels:[104],available:2,version:'test'}});
 const evalJS=code=>vm.runInContext(code,context),map=elements.get('map');
 const before=evalJS('[(700-plot.x)/plot.w,(plot.y+plot.h-300)/plot.h]');
 map.listeners.wheel({deltaY:-500,clientX:700,clientY:300,preventDefault(){}});
 const after=evalJS('[(700-plot.x)/plot.w,(plot.y+plot.h-300)/plot.h]');
 assert(Math.abs(before[0]-after[0])<1e-12);assert(Math.abs(before[1]-after[1])<1e-12);
 const original=evalJS('JSON.stringify(ignition)');
 map.listeners.pointerdown({pointerId:1,clientX:500,clientY:300});
 map.listeners.pointermove({pointerId:1,clientX:600,clientY:350});
 map.listeners.pointerup({type:'pointerup',pointerId:1,clientX:600,clientY:350});
 assert.equal(evalJS('JSON.stringify(ignition)'),original,'drag must not select a fire');
 const expected=evalJS('[domain.x0+(650-plot.x)/plot.w*domain.width,domain.y0+(plot.y+plot.h-330)/plot.h*domain.height]');
 map.listeners.pointerdown({pointerId:2,clientX:650,clientY:330});
 map.listeners.pointerup({type:'pointerup',pointerId:2,clientX:650,clientY:330});
 const selected=evalJS('ignition');assert(Math.abs(expected[0]-selected[0])<1e-8);assert(Math.abs(expected[1]-selected[1])<1e-8);
 elements.get('add-fire').listeners.click();assert.equal(messages.at(-1).type,'ignite');
 elements.get('wind-direction').value='180';elements.get('wind-direction').listeners.input();
 assert.equal(messages.at(-1).type,'wind');assert.equal(messages.at(-1).wind[1],-8);
 worker.onmessage({data:{type:'frame',frame:{time:30,fronts:[],area:0}}});
 elements.get('run').listeners.click();assert.equal(messages.at(-1).type,'run');assert.equal(messages.at(-1).ignition,null);
 elements.get('fit').onclick();assert.equal(evalJS('zoom'),1);assert.equal(evalJS('panX+panY'),0);
});
