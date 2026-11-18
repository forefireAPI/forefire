importScripts('./vendor/polygon-clipping.umd.min.js');
const helpers=import('./visuals.js?v=overlay-1');
self.onmessage=async({data})=>{try{const {buildBands}=await helpers;const bands=buildBands(data.snapshots,self.polygonClipping,data.center);postMessage({id:data.id,bands,fallback:bands.some(b=>b.fill===false)});}catch(e){postMessage({id:data.id,error:e.message});}};
