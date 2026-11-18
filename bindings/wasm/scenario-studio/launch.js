/** Calormat: pass the selected isochrone or its collection and UTC timestamp.
 * Call directly in a click handler; the browser navigates into the WASM app.
 */
export function launchForeFire(geojson,timestamp,{wind={mode:'manual',vector:[6,0]},size,target='_blank'}={}) {
 const form=document.createElement('form');form.method='POST';form.action=new URL('./start.php',import.meta.url).href;form.target=target;
 const input=document.createElement('input');input.type='hidden';input.name='payload';input.value=JSON.stringify({geojson,timestamp,wind,...(size?{size}:{})});form.append(input);document.body.append(form);form.submit();form.remove();
}
