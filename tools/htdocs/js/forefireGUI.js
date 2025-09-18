// Updated list of commands.
const commands = {
    "FireDomain": "FireDomain[sw=(0.0,0.0,0.0);ne=(100.0,100.0,0.0);t=0.0]",
    "FireNode": "FireNode[loc=(0.0,0.0,0.0);vel=(0.0,0.0,0.0);t=0.]",
    "FireFront": "FireFront[]",
    "startFire": "startFire[lonlat=(9.0,42.0,0.0);t=0.0]",
    "step": "step[dt=1200]",
    "trigger": "trigger[fuelIndice;loc=(x,y,z);fuelType=int or wind;loc=(x,y,z);vel=(vx,vy,vz);t=f]",
    "goTo": "goTo[t=56.2]",
    "print": "print[]",
    "save": "save[]",
    "addLayer": "addLayer[name=heatFlux;type=flux;modelName=heatFluxBasic;value=3]",
    "plot": "plot[parameter=speed;filename=out_file_name.png;range=(0,0.1);cmap=viridis]",
    "computeSpeed": "computeSpeed[]",
    "setParameter": "setParameter[param=value]",
    "setParameters": "setParameters[param1=val1;param2=val2]",
    "getParameter": "getParameter[paramNames]",
    "include": "include[run.ff]",
    "loadData": "loadData[data.nc;2024-12-13T15:41:33Z]",
    "clear": "clear[]",
    "systemExec": "systemExec[ls]",
    "quit": "quit[]"
  };
  
  // Populate predefined command buttons.
  const commandButtonsDiv = document.getElementById('commandButtons');
  for (const cmd in commands) {
    const btn = document.createElement('button');
    btn.textContent = cmd;
    btn.addEventListener('click', () => {
      // Fill the command input (the "ff:" prefix is added automatically).
      document.getElementById('commandInput').value = commands[cmd];
    });
    commandButtonsDiv.appendChild(btn);
  }
  
  // Initialize Leaflet map.
  const map = L.map('map').setView([42.0, 9.0], 8);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '© OpenStreetMap contributors'
  }).addTo(map);
  
  let mapLayer = null;
  let imageOverlay = null; // For image overlays
  let velocityLayer = null; // For wind velocity layer
  
  // --- Bounding Box Code ---
  let bboxRect = null;
  let bboxMarkers = [];
  let draggedMarkerIndex = null;

  // Create a div element with your logo
  L.Control.Logo = L.Control.extend({
    onAdd: function(map) {
      const img = L.DomUtil.create('img');

      img.src = 'img/ff_logo.png';
      img.style.width = '20px';
      img.style.height = 'auto';
      img.style.padding = '2px';
      img.alt = "ForeFire Logo";

      // Prevent map interaction when clicking logo
      L.DomEvent.disableClickPropagation(img);

      return img;
    },

  });

  // Helper function to add the control easily
  L.control.logo = function(opts) {
      return new L.Control.Logo(opts);
  }

  L.control.logo({ position: 'bottomleft' }).addTo(map);
      
  // Create a bounding box from a given bounds object (from JSON response).
  function createBoundingBoxFromResponse(boundsObj) {
    // Expected keys: SWlon, SWlat, NElon, NElat.
    const bounds = [[boundsObj.SWlat, boundsObj.SWlon], [boundsObj.NElat, boundsObj.NElon]];
    // Create rectangle with transparent fill.
    bboxRect = L.rectangle(bounds, { color: 'red', weight: 2, dashArray: '5,5', fillOpacity: 0 }).addTo(map);
    bboxMarkers = [];
    // Define corners in order: bottom-left, bottom-right, top-right, top-left.
    const corners = [
      { lat: boundsObj.SWlat, lng: boundsObj.SWlon },
      { lat: boundsObj.SWlat, lng: boundsObj.NElon },
      { lat: boundsObj.NElat, lng: boundsObj.NElon },
      { lat: boundsObj.NElat, lng: boundsObj.SWlon }
    ];
    corners.forEach((corner, index) => {
      let marker = L.marker([corner.lat, corner.lng], {
        draggable: true,
        icon: L.divIcon({ className: 'bbox-handle', iconSize: [8, 8] })
      }).addTo(map);
      marker.on('dragstart', function() {
        draggedMarkerIndex = index;
      });
      marker.on('drag', function() {
        updateBoundingBoxWithDragged(index);
      });
      bboxMarkers.push(marker);
    });
  }
  
  // Update the bounding box when one marker is dragged.
  function updateBoundingBoxWithDragged(dragIndex) {
    if (!bboxMarkers || bboxMarkers.length < 4) return;
    const draggedMarker = bboxMarkers[dragIndex];
    const oppositeIndex = (dragIndex + 2) % 4;
    const oppositeMarker = bboxMarkers[oppositeIndex];
  
    // Compute new bounds using the current position of the dragged marker and the opposite marker.
    const newSouth = Math.min(draggedMarker.getLatLng().lat, oppositeMarker.getLatLng().lat);
    const newNorth = Math.max(draggedMarker.getLatLng().lat, oppositeMarker.getLatLng().lat);
    const newWest = Math.min(draggedMarker.getLatLng().lng, oppositeMarker.getLatLng().lng);
    const newEast = Math.max(draggedMarker.getLatLng().lng, oppositeMarker.getLatLng().lng);
  
    const newBounds = [[newSouth, newWest], [newNorth, newEast]];
    bboxRect.setBounds(newBounds);
  
    // Reposition all markers to the new rectangle corners.
    bboxMarkers[0].setLatLng([newSouth, newWest]);
    bboxMarkers[1].setLatLng([newSouth, newEast]);
    bboxMarkers[2].setLatLng([newNorth, newEast]);
    bboxMarkers[3].setLatLng([newNorth, newWest]);
  }
  
  function getBBoxParam() {
    if (!bboxRect) return "";
    const bounds = bboxRect.getBounds();
    const south = bounds.getSouth();
    const north = bounds.getNorth();
    const west = bounds.getWest();
    const east = bounds.getEast();
    // Format: BBoxWSEN=west,south,east,north
    return `;BBoxWSEN=${west.toFixed(5)},${south.toFixed(5)},${east.toFixed(5)},${north.toFixed(5)}`;
  }
  // --- End Bounding Box Code ---
  
  // Update map with KML overlay.
  function updateMapWithKML(kmlText) {
    if (mapLayer) {
      map.removeLayer(mapLayer);
    }
    const parser = new DOMParser();
    const kmlDoc = parser.parseFromString(kmlText, 'text/xml');
    const geojson = toGeoJSON.kml(kmlDoc);
    mapLayer = L.geoJSON(geojson).addTo(map);
    if (mapLayer.getBounds && mapLayer.getBounds().isValid()) {
      map.fitBounds(mapLayer.getBounds());
    }
  }
// Update map with GeoJSON overlay.
function updateMapWithGeoJSON(geojson, refit = true) {
  if (mapLayer) {
      map.removeLayer(mapLayer);
  }

  // Define fire-like colors for styling
  const fireStyle = {
      color: "#ff4500",  // Fire orange-red outline
      weight: 2,         // Line thickness
      opacity: 1,        // Full opacity for edges
      fillColor: "#ff6600", // Fire orange fill
      fillOpacity: 0.7   // Semi-transparent fill
  };

  // Apply the fire-style to the GeoJSON layer
  mapLayer = L.geoJSON(geojson, {
      style: function (feature) {
          return fireStyle;
      }
  }).addTo(map);

  // Fit map to the bounds of the new GeoJSON layer if refit is true
  if (refit && mapLayer.getBounds && mapLayer.getBounds().isValid()) {
      map.fitBounds(mapLayer.getBounds());
  }
}

  
  // Add image overlay.
  function addImageOverlay(imageUrl, bounds) {
    if (imageOverlay) {
      map.removeLayer(imageOverlay);
    }
    const leafletBounds = [[bounds.SWlat, bounds.SWlon], [bounds.NElat, bounds.NElon]];
    imageOverlay = L.imageOverlay(imageUrl, leafletBounds, { opacity: 0.7 });
    imageOverlay.addTo(map);
    imageOverlay.setZIndex(0);
  }
  
  // Send command function: auto-prefix with "ff:".
  async function sendCommand(command) {
    const responseDiv = document.getElementById('responseConsole');
    const historyDiv = document.getElementById('commandHistory');
    historyDiv.innerHTML += "<div class='commandText'>" + command + "</div>";
    historyDiv.scrollTop = historyDiv.scrollHeight;
    responseDiv.innerHTML += "<div class='commandText'>forefire >" + command + "</div>";
    responseDiv.scrollTop = responseDiv.scrollHeight;
  
    if (!command.startsWith("ff:")) {
      command = "ff:" + command;
    }
    try {
      const response = await fetch('/', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: command
      });
      const text = await response.text();


        // Process text: limit line length to 150 characters and line count if necessary.
        let lines = text.split('\n').map(line => {
            return (line.length > 150) ? line.substring(0, 150) + "..." : line;
        });
        if (lines.length > 20) {
            const first15 = lines.slice(0, 15);
            const last5 = lines.slice(lines.length - 5);
            lines = first15.concat(["[...]"], last5);
        }
        const displayText = lines.join("\n");
    
        responseDiv.innerHTML += "<div><pre>" + displayText + "</pre></div>";



      responseDiv.scrollTop = responseDiv.scrollHeight;
      const trimmed = text.trim();
      if (trimmed.startsWith("<?xml") || trimmed.startsWith("<kml")) {
        updateMapWithKML(text);
      }
      return text;
    } catch (error) {
      responseDiv.innerHTML += "<div>Error: " + error + "</div>";
      return null;
    } finally {
      updateISODate();
    }
  }
  
  async function updateISODate() {
    try {
      const response = await fetch('/', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: "ff:getParameter[ISOdate]"
      });
      const isoText = await response.text();
      document.getElementById('dateHeader').textContent = isoText;
    } catch (error) {
      document.getElementById('dateHeader').textContent = "Error retrieving date";
    }
  }
  
  document.getElementById('refreshMap').addEventListener('click', async () => {
    await sendCommand("setParameter[dumpMode=geojson]");
    const geojsonText = await sendCommand("print[]");
    if (geojsonText) {
      try {
        const geojson = JSON.parse(geojsonText);
        updateMapWithGeoJSON(geojson);
      } catch (e) {
        console.error("Failed to parse GeoJSON:", e);
      }
    }
  });
  
  document.getElementById('sendCommand').addEventListener('click', () => {
    const command = document.getElementById('commandInput').value.trim();
    if (command !== "") {
      sendCommand(command);
      document.getElementById('commandInput').value = "";
    }
  });
  
  document.getElementById('commandInput').addEventListener('keypress', (e) => {
    if (e.key === "Enter") {
      const command = document.getElementById('commandInput').value.trim();
      if (command !== "") {
        sendCommand(command);
        document.getElementById('commandInput').value = "";
      }
    }
  });
  
  map.on('dblclick', function(e) {
    const lat = e.latlng.lat.toFixed(6);
    const lon = e.latlng.lng.toFixed(6);
    document.getElementById('commandInput').value += "startFire[lonlat=(" + lon + ", " + lat + ",0)]";
  });
  
  // --- Layers Sidebar and Update ---
  async function updateLayerList() {
    const layersResponse = await sendCommand("getParameter[layerNames]");
    if (layersResponse) {
      const layers = layersResponse.trim().split(/\s*,\s*/);
      const layerListEl = document.getElementById('layerList');
      layerListEl.innerHTML = "";
      let hasWindU = false, hasWindV = false;
      layers.forEach(layer => {
        const li = document.createElement('li');
        li.textContent = layer;
        li.addEventListener('click', () => {
          // Build the appropriate ff:plot command.
          let cmd = "";
          if (layer === "fuel") {
            cmd = `plot[parameter=fuel;filename=fuel.png;range=(0,255);cmap=fuel;projectionOut=json${getBBoxParam()}]`;
          } else if (layer === "windU" || layer === "windV") {
            cmd = `plot[parameter=${layer};filename=${layer}.png;size=320,320;projectionOut=json${getBBoxParam()}]`;
            if (layer === "windU") hasWindU = true;
            if (layer === "windV") hasWindV = true;
          } else {
            cmd = `plot[parameter=${layer};filename=${layer}.png;size=320,320;projectionOut=json${getBBoxParam()}]`;
          }
          sendCommand(cmd).then(responseText => {
            try {
              const bounds = JSON.parse(responseText);
              // If no bounding box exists yet, use these bounds.
              if (!bboxRect && bounds.SWlon !== undefined) {
                createBoundingBoxFromResponse(bounds);
              }
              const imageUrl = cmd.match(/filename=([^;]+)/)[1];
              addImageOverlay(imageUrl, bounds);
            } catch (err) {
              console.error("Error parsing overlay bounds:", err);
            }
          });
        });
        layerListEl.appendChild(li);
      });
      // If both windU and windV exist, add an extra "wind" button.
      if (layers.includes("windU") && layers.includes("windV")) {
        const li = document.createElement('li');
        li.textContent = "wind";
        li.style.fontWeight = "bold";
        li.addEventListener('click', () => {
          // Toggle wind layer: if it exists, remove it.
          if (velocityLayer) {
            map.removeLayer(velocityLayer);
            velocityLayer = null;
            return;
          }
          let cmd = `plot[parameter=wind;filename=windCS.json;size=60,60;projectionOut=json${getBBoxParam()}]`;
          sendCommand(cmd).then(responseText => {
            let bboxData;
            try {
              bboxData = JSON.parse(responseText);
            } catch (e) {
              console.error("Error parsing bounding box data:", e);
            }
            // If no bounding box exists yet and the response contains bounds, create one.
            if (!bboxRect && bboxData && bboxData.SWlon !== undefined) {
              createBoundingBoxFromResponse(bboxData);
            }
            // Use minVal and maxVal from bboxData if available.
            let windMin = (bboxData && bboxData.minVal !== undefined) ? bboxData.minVal : 0;
            let windMax = (bboxData && bboxData.maxVal !== undefined) ? bboxData.maxVal : 10;
            console.log("Wind min/max:", windMin, windMax);
            // Now fetch the actual wind data from the file windCS.json.
            fetch("windCS.json")
              .then(resp => resp.json())
              .then(windData => {
                if (velocityLayer) {
                  map.removeLayer(velocityLayer);
                }
                velocityLayer = L.velocityLayer({
                  displayValues: true,
                  displayOptions: {
                    velocityType: "Wind",
                    position: "bottomleft",
                    emptyString: "No velocity data",
                    angleConvention: "bearingCW",
                    showCardinal: false,
                    useVectors: true,    
                    speedUnit: "ms",
                    directionString: "Direction",
                    speedString: "Speed"
                  },
                  data: windData,               
                  lineWidth: 3,
                  minVelocity: windMin,
                  maxVelocity: windMax,
                  velocityScale: 0.005,
                  opacity: 1,
                  paneName: "overlayPane"
                });
                velocityLayer.addTo(map);
              })
              .catch(e => {
                console.error("Error fetching wind data:", e);
              });
          });
        });
        layerListEl.appendChild(li);
      }
    }
  }
  
  // Attach event to the Update Layers button.
  document.getElementById('updateLayers').addEventListener('click', updateLayerList);
  // Note: Layer list is updated only when Update Layers is clicked.
  
  // --- rosacewind Initialization ---
  let ventX = 0, ventY = 0;
  let rosace = new RosaceWind("wind", 50);
  rosace.onMouseUpCallback = function () {
    ventX = rosace.getWindU();
    ventY = rosace.getWindV();
  };
  
  // Optionally, add a "trigger wind" button to use ventX and ventY.
  const triggerWindButton = document.createElement('button');
  triggerWindButton.textContent = "trigger wind";
  triggerWindButton.addEventListener('click', () => {
    const cmd = `trigger[wind;loc=(0.,0.,0.);vel=(${ventX},${ventY},0.)]`;
    sendCommand(cmd);
  });
  commandButtonsDiv.appendChild(triggerWindButton);

  const haltMNH = document.createElement('button');
  haltMNH.textContent = "Halt MNH";
  haltMNH.addEventListener('click', () => {
    const cmd = `setParameter[MNHalt=10]`;
    sendCommand(cmd);
  });
  commandButtonsDiv.appendChild(haltMNH);

  const freeMNH = document.createElement('button');
  freeMNH.textContent = "free MNH";
  freeMNH.addEventListener('click', () => {
    const cmd = `setParameter[MNHalt=0]`;
    sendCommand(cmd);
  });
  commandButtonsDiv.appendChild(freeMNH);

  function setWindMapUseVector(useVectors) {
    if (velocityLayer) {
      velocityLayer.setOptions({
        displayOptions: {
          useVectors: useVectors
        }
      });
    } else {
      console.error("Velocity layer is not initialized");
    }
  }


  let autoRefreshInterval = null;

// 1) Factor out the refresh logic into a separate function:
async function refreshMap() {
  // The same logic you had in the 'refreshMap' button’s click handler
  await sendCommand("setParameter[dumpMode=geojson]");
  const geojsonText = await sendCommand("print[]");
  if (geojsonText) {
    try {
      const geojson = JSON.parse(geojsonText);
      updateMapWithGeoJSON(geojson, false);
    } catch (e) {
      console.error("Failed to parse GeoJSON:", e);
    }
  }

  // If velocityLayer is active, refresh the wind data too:
  if (velocityLayer) {
    // Similar logic to the "wind" layer button
    let cmd = `plot[parameter=wind;filename=windCS.json;size=60,60;projectionOut=json${getBBoxParam()}]`;
    let responseText = await sendCommand(cmd);

    let bboxData;
    try {
      bboxData = JSON.parse(responseText);
    } catch (e) {
      console.error("Error parsing bounding box data:", e);
    }

    // Possibly create bounding box if none is defined yet
    if (!bboxRect && bboxData && bboxData.SWlon !== undefined) {
      createBoundingBoxFromResponse(bboxData);
    }

    let windMin = (bboxData && bboxData.minVal !== undefined) ? bboxData.minVal : 0;
    let windMax = (bboxData && bboxData.maxVal !== undefined) ? bboxData.maxVal : 10;

    // Re-fetch the actual JSON wind data
    fetch("windCS.json")
      .then(resp => resp.json())
      .then(windData => {
        // Remove old layer, if present
        if (velocityLayer) {
          velocityLayer.setData(windData);
        }

      })
      .catch(e => {
        console.error("Error fetching wind data:", e);
      });
  }
}


// 2) Link existing "Refresh Map" button to our new function:
document.getElementById('refreshMap').addEventListener('click', refreshMap);

// 3) Handle the auto-refresh checkbox changes:
document.getElementById('autoRefreshCheckbox').addEventListener('change', (e) => {
  if (e.target.checked) {
    // Start auto-refresh every 10 seconds (adjust as needed)
    autoRefreshInterval = setInterval(refreshMap, 500);
  } else {
    // Stop auto-refresh
    clearInterval(autoRefreshInterval);
    autoRefreshInterval = null;
  }
});

document.getElementById('WindOrParts').addEventListener('change', (e) => {
  setWindMapUseVector(e.target.checked)
});

const toggleLogs = document.getElementById('toggleLogs');
const responseConsole = document.getElementById('responseConsole');
const commandHistory = document.getElementById('commandHistory');

toggleLogs.addEventListener('change', function() {
  const displayStyle = this.checked ? 'block' : 'none';
  responseConsole.style.display = displayStyle;
  commandHistory.style.display = displayStyle;
  
  // Optional: Adjust map height if needed when logs are hidden
  // For example, increase the map height when logs are hidden
  if (!this.checked) {
    document.getElementById('map').style.height = '700px';
  } else {
    document.getElementById('map').style.height = '500px';
  }
});