import vm from 'node:vm';import {readFile} from 'node:fs/promises';
const nativeFetch=fetch;globalThis.fetch=(url,opts)=>nativeFetch(new URL(url,'https://forefire.univ-corse.fr'),opts);
globalThis.window=globalThis;vm.runInThisContext(await readFile(new URL('../vendor/mp4box.js',import.meta.url),'utf8'));
const {Weather}=await import('../weather.js');const weather=new Weather({center:[8.9,42.01],size:[.08,.08]});
const {model,frames}=await weather.year(2011);console.log(JSON.stringify({frames:frames.length,first:new Date(frames[0].ms).toISOString(),last:new Date(frames.at(-1).ms).toISOString(),tracks:model.videoTracks.map(t=>({codec:t.codec,width:t.video.width,height:t.video.height})),geometry:model.geometry,bytes:model.transport.bytes,requests:model.transport.requests},null,2));weather.close();
