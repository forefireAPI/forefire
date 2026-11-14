// AVENC uses MP4Box's browser global; all native-depth decoding stays off the UI.
self.window=self;
importScripts('./vendor/mp4box.js');
let weather,queue=Promise.resolve();
self.onmessage=({data:d})=>{queue=queue.then(async()=>{try{
 if(d.type==='init'){weather?.close();const {Weather}=await import('./weather.js?v=overlay-1');weather=new Weather({...d.config,onStatus:message=>postMessage({type:'status',message})});}
 if(d.type==='preload'){if(!weather)throw Error('Weather is not initialized.');const window=await weather.preload(d.ms,d.hours??6);postMessage({type:'weather',id:d.id,window});}
 }catch(e){postMessage({type:'weather',id:d.id,error:e.message});}});};
