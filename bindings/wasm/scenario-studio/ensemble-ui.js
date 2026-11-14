import {perturbations,runMember,ensembleGrid,burnCounts,probabilityPixels} from './ensemble.js?v=ensemble-2';
export function ensembleControls({viewer,getConfig,pause,reset,notice,visibility}){
 const $=id=>document.getElementById(id),C=globalThis.Cesium;
 const panel=document.createElement('section');panel.className='ensemble-panel';panel.hidden=true;panel.innerHTML=`<div class="ensemble-heading">ENSEMBLE FORECAST<button id="ensembleClose" aria-label="Close ensemble settings">×</button></div><div class="ensemble-fields"><label>Members<input id="ensembleCount" type="number" min="3" max="15" value="15"></label><label>Seed<input id="ensembleSeed" type="number" min="0" max="4294967295" value="19073"></label></div><label>Numerics<select id="ensembleNumerics"><option value="reference">Same as reference</option><option value="super">Ultrafast preview</option></select></label><details><summary>Perturbations · paper presets</summary><label>Wind direction σ · degrees<input id="ensembleDirection" type="number" value="30" min="0" max="90"></label><label>Wind speed · maximum factor<input id="ensembleSpeed" type="number" value="1.5" min="1" max="3" step=".1"></label><label>Dead moisture · relative σ<input id="ensembleMoisture" type="number" value="0.15" min="0" max=".3" step=".01"></label><p>Global biases, constant per member. Truncated normal (±2σ); wind speed truncated log-normal. Allaire, Filippi & Mallet (2020), Table 2. Wind and dead moisture only; no site-specific calibration.</p></details><button id="ensembleRun" class="primary">RUN ENSEMBLE</button><progress id="ensembleProgress" max="15" value="0"></progress><p id="ensembleStatus" role="status">Same initial state and scenario · 15 members maximum.</p><label>Display<select id="ensembleView"><option value="probability">Burn probability</option><option value=".1">Arrival-time P10 zone</option><option value=".5">Arrival-time P50 zone</option><option value=".9">Arrival-time P90 zone</option><option value="off">Reference simulation</option></select></label><div class="ensemble-scale"></div><div class="ensemble-scale-labels"><span>0%</span><span>50%</span><span>100%</span></div><p id="ensembleLegend">Fraction of members burned by the selected hour. Arrival P10/P50/P90: cells reached by at least 10/50/90% of members. Grey: already burned at startup. Red: all members reached this cell. Solver snapshots in the first hour, then hourly; 384 × 384 raster preview. P10 is the outer early-arrival zone; P90 the more consistently reached inner zone.</p><button id="ensembleExport" class="quiet" disabled>DOWNLOAD ENSEMBLE RESULTS</button>`;
 document.body.append(panel);
 const tools=document.createElement('div');tools.className='ensemble-actions';tools.innerHTML='<button id="resetSimulation" title="Restart from the initial state, retaining scenario actions">↺ RESET</button><button id="ensembleOpen">ENSEMBLE</button>';$('play').after(tools);
 let usedNumerics='super',usedSeed=19073;let controller=null,members=[],grid=null,layer=null,url=null,generation=0,cacheKey='',active=false,jobId=0;
 function clearLayer(){generation++;cacheKey='';if(layer)viewer.imageryLayers.remove(layer,true);layer=null;if(url)URL.revokeObjectURL(url);url=null;visibility(false);viewer.scene.requestRender();}
 function stop(){controller?.abort();controller=null;active=false;$('settingsPanel').inert=false;document.querySelector('.tools').inert=false;$('ensembleRun').textContent='RUN ENSEMBLE';}
 function cancel(){jobId++;stop();members=[];grid=null;clearLayer();$('ensembleExport').disabled=true;$('ensembleStatus').textContent='Same initial state and scenario · 15 members maximum.';}
 async function paint(time){
  if(!members.length||!grid||$('ensembleView').value==='off'){clearLayer();return;}
  const key=time+':'+$('ensembleView').value+':'+members.length;
  const represented=Math.min(...members.map(m=>m.frames.filter(f=>f.time<=time+1e-7).at(-1)?.time??0));$('frameLabel').textContent=`ENSEMBLE · ${members.length} MEMBERS · DATA ${(represented/3600).toFixed(2)} h`; visibility(true);if(key===cacheKey)return;cacheKey=key;const id=++generation;
  try{const counts=burnCounts(members,time,grid),q=Number($('ensembleView').value)||0,canvas=document.createElement('canvas');canvas.width=grid.width;canvas.height=grid.height;canvas.getContext('2d').putImageData(new ImageData(probabilityPixels(counts,members.length,q,burnCounts(members,Math.min(...members.map(m=>m.frames[0].time)),grid)),grid.width,grid.height),0,0);
   const blob=await new Promise(r=>canvas.toBlob(r,'image/png'));if(id!==generation||!blob)return;const next=URL.createObjectURL(blob);
   try{const provider=await C.SingleTileImageryProvider.fromUrl(next,{rectangle:C.Rectangle.fromDegrees(grid.west,grid.south,grid.east,grid.north)});if(id!==generation){URL.revokeObjectURL(next);return;}if(layer)viewer.imageryLayers.remove(layer,true);if(url)URL.revokeObjectURL(url);url=next;layer=viewer.imageryLayers.addImageryProvider(provider);viewer.scene.requestRender();}catch(e){URL.revokeObjectURL(next);throw e;}
  }catch(e){notice('Ensemble display: '+e.message,true);}
 }
 $('ensembleOpen').onclick=()=>panel.hidden=!panel.hidden;$('ensembleClose').onclick=()=>panel.hidden=true;
 $('resetSimulation').onclick=()=>{cancel();reset();};
 $('ensembleView').onchange=()=>paint(Number($('timeline').value));
 $('timeline').addEventListener('input',()=>paint(Number($('timeline').value)));
 $('ensembleRun').onclick=async()=>{
  if(active){stop();$('ensembleStatus').textContent=`Cancelled · ${members.length} completed members retained.`;return;}
  let job;try{
   const config=getConfig();if(!config)throw Error('Load an ignition or state first.');
   usedSeed=Number($('ensembleSeed').value);const samples=perturbations(Number($('ensembleCount').value),usedSeed,{directionSigma:Number($('ensembleDirection').value),speedMax:Number($('ensembleSpeed').value),moistureSigma:Number($('ensembleMoisture').value)});
   $('ensembleRun').disabled=true;await pause();$('ensembleRun').disabled=false;cancel();job=jobId;controller=new AbortController();const signal=controller.signal;active=true;$('settingsPanel').inert=true;document.querySelector('.tools').inert=true;$('ensembleRun').textContent='CANCEL ENSEMBLE';$('ensembleProgress').max=samples.length;$('ensembleProgress').value=0;
   usedNumerics=$('ensembleNumerics').value==='super'?'super':config.numerics;const begun=performance.now();let clipped=0;
   for(const p of samples){
    const result=await runMember({...structuredClone(config),numerics:usedNumerics,perturbation:p},{signal,onProgress:t=>{$('ensembleStatus').textContent=`Member ${p.member} / ${samples.length} · ${(t/3600).toFixed(1)} h`;}});
    members.push(result);if(result.frames.some(f=>f.boundaryDistance<256))clipped++;$('ensembleProgress').value=members.length;grid=ensembleGrid(members,config.center[0]);cacheKey='';await paint(Number($('timeline').value));
   }
   $('ensembleStatus').textContent=`${members.length} members · ${((performance.now()-begun)/1000).toFixed(1)} s${clipped?' · '+clipped+' reached the fixed boundary':''}`;
   $('ensembleExport').disabled=false;notice('Ensemble ready. Move the timeline to compare probabilities and arrival quantiles.');
  }catch(e){if(job!==undefined&&job!==jobId)return;if(e.name!=='AbortError'){notice('Ensemble: '+e.message,true);$('ensembleStatus').textContent='Incomplete ensemble · rerun to obtain all requested members.';}}
  finally{$('ensembleRun').disabled=false;if(job===jobId){stop();if(members.length){grid=ensembleGrid(members,getConfig().center[0]);cacheKey='';paint(Number($('timeline').value));}}}
 };
 $('ensembleExport').onclick=()=>{const config=getConfig(),blob=new Blob([JSON.stringify({format:'forefire-ensemble',version:1,seed:usedSeed,epoch:config.epoch,size:config.size,center:config.center,horizon:config.horizon,model:config.model,numerics:usedNumerics,sampling:'Allaire et al. 2020 Table 2: wind and dead moisture subset',members})],{type:'application/json'}),link=document.createElement('a'),u=URL.createObjectURL(blob);link.href=u;link.download='forefire-ensemble.json';link.click();setTimeout(()=>URL.revokeObjectURL(u),1000);};
 return {cancel,paint,get active(){return active;},get showing(){return members.length>0&&$('ensembleView').value!=='off';}};
}
