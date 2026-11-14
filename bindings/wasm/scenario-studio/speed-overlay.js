import {geographic,wrap} from './core.js?v=overlay-1';
export class SpeedOverlay {
 constructor(viewer){this.viewer=viewer;this.layer=null;this.url=null;this.generation=0;this.current=null;}
 clear(){this.generation++;this.current=null;if(this.layer)this.viewer.imageryLayers.remove(this.layer,true);this.layer=null;if(this.url)URL.revokeObjectURL(this.url);this.url=null;}
 async show(raster){if(!raster){this.clear();return;}if(this.current===raster)return;this.current=raster;const id=++this.generation,C=globalThis.Cesium,[x0,y0,x1,y1]=raster.bounds,d=raster.domain;
  const corners=[[x0,y0],[x1,y0],[x0,y1],[x1,y1]].map(p=>geographic(d,p)),center=d.longitude,lons=corners.map(p=>center+wrap(p[0]-center)),west=Math.min(...lons),east=Math.max(...lons),south=corners[0][1],north=corners[2][1];
  const image=await createImageBitmap(new Blob([raster.png],{type:'image/png'})),canvas=document.createElement('canvas');canvas.width=image.width;canvas.height=image.height;const ctx=canvas.getContext('2d');
  // ForeFire's PNG rows are Cartesian; warp them to geographic parallels so
  // the raster stays aligned with the latitude-dependent tile projection.
  for(let row=0;row<image.height;row++){const y=y1-(row+.5)*(y1-y0)/image.height,a=geographic(d,[x0,y]),b=geographic(d,[x1,y]),left=center+wrap(a[0]-center),right=center+wrap(b[0]-center);ctx.drawImage(image,0,row,image.width,1,(left-west)/(east-west)*canvas.width,row,(right-left)/(east-west)*canvas.width,1);}image.close();
  const blob=await new Promise(r=>canvas.toBlob(r,'image/png'));if(!blob)throw Error('Cannot prepare speed map.');const url=URL.createObjectURL(blob);
  try{const provider=await C.SingleTileImageryProvider.fromUrl(url,{rectangle:C.Rectangle.fromDegrees(wrap(west),south,wrap(east),north)});if(id!==this.generation){URL.revokeObjectURL(url);return;}if(this.layer)this.viewer.imageryLayers.remove(this.layer,true);if(this.url)URL.revokeObjectURL(this.url);this.layer=this.viewer.imageryLayers.addImageryProvider(provider);this.layer.alpha=.88;this.url=url;this.viewer.scene.requestRender();}catch(e){URL.revokeObjectURL(url);throw e;}
 }
}
