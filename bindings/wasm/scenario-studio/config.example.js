const base=globalThis.location?.href??globalThis.self?.location?.href??import.meta.url;
export const cesiumToken='';
export const tileProvider=new URL('./tiles/',base).href;
export const eraProvider=new URL('/eraland/',base).href;
