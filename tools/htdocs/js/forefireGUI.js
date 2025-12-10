// Updated list of commands.
const commands = {
    "include": "include[real_case.ff]",
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
    "loadData": "loadData[data.nc;2024-12-13T15:41:33Z]",
    "clear": "clear[]",
    "systemExec": "systemExec[ls]",
    "quit": "quit[]"
  };
  
  // Populate predefined command buttons.
  const commandButtonsDiv = document.getElementById('commandButtons');
  const responseConsoleEl = document.getElementById('responseConsole');
  const commandHistoryEl = document.getElementById('commandHistory');

  function uiLog(message, type = 'info') {
    console.log(`[UI ${type}] ${message}`);
    if (responseConsoleEl) {
      const div = document.createElement('div');
      div.className = `ui-log ui-${type}`;
      div.textContent = message;
      responseConsoleEl.appendChild(div);
      responseConsoleEl.scrollTop = responseConsoleEl.scrollHeight;
    }
  }

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
  const map = L.map('map', { doubleClickZoom: false }).setView([42.0, 9.0], 8);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '© OpenStreetMap contributors'
  }).addTo(map);
  
  let mapLayer = null;
  let imageOverlay = null; // For image overlays
  let velocityLayer = null; // For wind velocity layer
  let activeLayer = null; // Track the last clicked layer
  let lastPlotCommand = null;
  // --- Colorbar UI State ---
  const colorbarElements = {
    canvas: document.getElementById('colorbarCanvas'),
    minLabel: document.getElementById('colorbarMinLabel'),
    maxLabel: document.getElementById('colorbarMaxLabel'),
    minInput: document.getElementById('colorbarMinInput'),
    maxInput: document.getElementById('colorbarMaxInput'),
    cmapSelect: document.getElementById('colorbarCmapSelect'),
    autoFitCheckbox: document.getElementById('colorbarAutoFit'),
    applyBtn: document.getElementById('colorbarApply'),
    useDataBtn: document.getElementById('colorbarUseData'),
    status: document.getElementById('colorbarStatus')
  };
  colorbarElements.ctx = colorbarElements.canvas ? colorbarElements.canvas.getContext('2d') : null;

  const COLORBAR_GRADIENTS = {
    viridis: ['#440154', '#46337f', '#365c8d', '#277f8e', '#1fa187', '#95d840', '#fde725'],
    plasma: ['#0d0887', '#7e03a8', '#cc4778', '#f89540', '#f0f921'],
    plasma_r: ['#f0f921', '#f89540', '#cc4778', '#7e03a8', '#0d0887'],
    hot: ['#000000', '#ff0000', '#ffff00', '#ffffff'],
    hot_r: ['#ffffff', '#ffff00', '#ff0000', '#000000'],
    coolwarm: ['#3b4cc0', '#ffffff', '#b40426'],
    grey: ['#000000', '#7f7f7f', '#ffffff'],
    spring: ['#ff00ff', '#ffff00'],
    jet: ['#00007f', '#007fff', '#00ff00', '#ff7f00', '#7f0000'],
    turbo: ['#30123b', '#4145ab', '#2c87f0', '#29d7b1', '#8cf473', '#f9ec4f', '#f9a13c', '#d9381e'],
    fuel: ['#120400', '#5f2c09', '#c76a17', '#f5c453', '#fff6cf']
  };
  const AVAILABLE_COLORMAPS = ['viridis', 'plasma', 'plasma_r', 'hot', 'hot_r', 'coolwarm', 'grey', 'spring', 'jet', 'turbo', 'fuel'];
  const DEFAULT_COLORMAP = 'viridis';
  const INVALID_DATA_SENTINELS = new Set([-999, -9999]);

  const colorbarState = {
    min: null,
    max: null,
    cmap: DEFAULT_COLORMAP,
    autoRange: true,
    lastDataMin: null,
    lastDataMax: null,
    userCmapOverride: false
  };
  let suppressCmapEvent = false;

  function formatLabelValue(value) {
    if (!Number.isFinite(value)) {
      return 'N/A';
    }
    const absVal = Math.abs(value);
    if ((absVal > 0 && absVal < 0.001) || absVal >= 1000) {
      return value.toExponential(2);
    }
    return value.toFixed(3);
  }

  function drawColorbar(cmapName) {
    if (!colorbarElements.ctx || !colorbarElements.canvas) return;
    const palette = COLORBAR_GRADIENTS[cmapName] || COLORBAR_GRADIENTS[DEFAULT_COLORMAP];
    const ctx = colorbarElements.ctx;
    const width = colorbarElements.canvas.width;
    const height = colorbarElements.canvas.height;
    const gradient = ctx.createLinearGradient(0, 0, width, 0);
    const stops = palette.length - 1;
    palette.forEach((color, index) => {
      const stop = stops === 0 ? 0 : index / stops;
      gradient.addColorStop(stop, color);
    });
    ctx.clearRect(0, 0, width, height);
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, width, height);
  }

  function updateColorbarStatus(message = '') {
    if (colorbarElements.status) {
      colorbarElements.status.textContent = message;
    }
  }

  function sanitizeDataValue(value) {
    if (typeof value !== 'number' || !Number.isFinite(value)) {
      return null;
    }
    if (INVALID_DATA_SENTINELS.has(value)) {
      return null;
    }
    return value;
  }

  function updateColorbarData(minVal, maxVal) {
    const dataMin = sanitizeDataValue(minVal);
    const dataMax = sanitizeDataValue(maxVal);

    colorbarState.lastDataMin = dataMin;
    colorbarState.lastDataMax = dataMax;

    if (colorbarElements.minLabel) {
      colorbarElements.minLabel.textContent = formatLabelValue(dataMin ?? NaN);
    }
    if (colorbarElements.maxLabel) {
      colorbarElements.maxLabel.textContent = formatLabelValue(dataMax ?? NaN);
    }

    if (colorbarState.autoRange) {
      if (dataMin !== null && dataMax !== null && dataMin < dataMax) {
        colorbarState.min = dataMin;
        colorbarState.max = dataMax;
        if (colorbarElements.minInput) {
          colorbarElements.minInput.value = dataMin;
        }
        if (colorbarElements.maxInput) {
          colorbarElements.maxInput.value = dataMax;
        }
        updateColorbarStatus('Using data range');
      } else {
        colorbarState.min = null;
        colorbarState.max = null;
        if (colorbarElements.minInput) {
          colorbarElements.minInput.value = '';
        }
        if (colorbarElements.maxInput) {
          colorbarElements.maxInput.value = '';
        }
        updateColorbarStatus('Data range unavailable');
      }
    } else {
      updateColorbarStatus('Custom range locked');
    }
  }

  function setColorbarCmap(newCmap, userInitiated = false) {
    const cmap = AVAILABLE_COLORMAPS.includes(newCmap) ? newCmap : DEFAULT_COLORMAP;
    colorbarState.cmap = cmap;
    colorbarState.userCmapOverride = userInitiated;
    if (colorbarElements.cmapSelect) {
      suppressCmapEvent = true;
      colorbarElements.cmapSelect.value = cmap;
      suppressCmapEvent = false;
    }
    drawColorbar(cmap);
  }

  function handleColorbarApply() {
    if (!colorbarElements.minInput || !colorbarElements.maxInput) return;
    const minValue = colorbarElements.minInput.value.trim();
    const maxValue = colorbarElements.maxInput.value.trim();
    if (minValue === '' || maxValue === '') {
      updateColorbarStatus('Provide both min and max values.');
      return;
    }
    const parsedMin = Number(minValue);
    const parsedMax = Number(maxValue);
    if (!Number.isFinite(parsedMin) || !Number.isFinite(parsedMax)) {
      updateColorbarStatus('Values must be numeric.');
      return;
    }
    if (parsedMin >= parsedMax) {
      updateColorbarStatus('Min must be lower than max.');
      return;
    }
    colorbarState.min = parsedMin;
    colorbarState.max = parsedMax;
    colorbarState.autoRange = false;
    syncAutoFitCheckbox();
    updateColorbarStatus('Custom range applied.');
    replotActiveLayer("colorbar apply");
  }

  function handleColorbarUseData() {
    colorbarState.autoRange = true;
    syncAutoFitCheckbox();
    if (colorbarState.lastDataMin !== null && colorbarState.lastDataMax !== null && colorbarState.lastDataMin < colorbarState.lastDataMax) {
      colorbarState.min = colorbarState.lastDataMin;
      colorbarState.max = colorbarState.lastDataMax;
      if (colorbarElements.minInput) {
        colorbarElements.minInput.value = colorbarState.lastDataMin;
      }
      if (colorbarElements.maxInput) {
        colorbarElements.maxInput.value = colorbarState.lastDataMax;
      }
      updateColorbarStatus('Using data range');
    } else {
      colorbarState.min = null;
      colorbarState.max = null;
      if (colorbarElements.minInput) {
        colorbarElements.minInput.value = '';
      }
      if (colorbarElements.maxInput) {
        colorbarElements.maxInput.value = '';
      }
      updateColorbarStatus('Waiting for data range');
    }
    replotActiveLayer("data range");
  }

  function replotActiveLayer(reason = "colorbar update") {
    if (!activeLayer) {
      uiLog("Select a layer before applying colorbar changes", "warn");
      return;
    }
    runLayerPlot(activeLayer, false, reason);
  }

  function syncAutoFitCheckbox() {
    if (colorbarElements.autoFitCheckbox) {
      colorbarElements.autoFitCheckbox.checked = !!colorbarState.autoRange;
    }
  }

  function ensureColorbarDefaultsForLayer(layerName) {
    if (layerName === 'fuel' && !colorbarState.userCmapOverride && colorbarState.cmap !== 'fuel') {
      setColorbarCmap('fuel', false);
    }
  }

  function getRangeParam(layerName) {
    if (Number.isFinite(colorbarState.min) && Number.isFinite(colorbarState.max) && colorbarState.min < colorbarState.max) {
      return `;range=(${colorbarState.min},${colorbarState.max})`;
    }
    if (layerName === 'fuel') {
      return ';range=(0,255)';
    }
    return '';
  }

  function getCmapParam(layerName) {
    let cmap = colorbarState.cmap;
    if (!cmap) {
      cmap = layerName === 'fuel' ? 'fuel' : DEFAULT_COLORMAP;
    }
    return cmap ? `;cmap=${cmap}` : '';
  }

  function handlePlotMetadata(layerName, metadata) {
    if (!metadata || typeof metadata !== 'object') {
      return;
    }
    if (layerName === 'wind' || layerName === 'windU' || layerName === 'windV') {
      return;
    }
    if (Object.prototype.hasOwnProperty.call(metadata, 'minVal') || Object.prototype.hasOwnProperty.call(metadata, 'maxVal')) {
      updateColorbarData(metadata.minVal, metadata.maxVal);
    }
  }

  function buildPlotCommand(layerName) {
    ensureColorbarDefaultsForLayer(layerName);
    const bboxSegment = getBBoxParam();
    const rangeSegment = getRangeParam(layerName);
    const cmapSegment = getCmapParam(layerName);
    const filename = `${layerName}.png`;
    return `plot[parameter=${layerName};filename=${filename};size=320,320${rangeSegment}${cmapSegment};projectionOut=json${bboxSegment}]`;
  }

  function extractLayerFromPlotCommand(commandString) {
    if (typeof commandString !== 'string') {
      return null;
    }
    const match = commandString.match(/plot\[(.*?)\]/i);
    if (!match) {
      return null;
    }
    const inner = match[1];
    const paramMatch = inner.match(/parameter=([^;]+)/i);
    if (!paramMatch) {
      return null;
    }
    return paramMatch[1];
  }

  function initializeColorbarUI() {
    if (colorbarElements.cmapSelect) {
      AVAILABLE_COLORMAPS.forEach((name) => {
        const option = document.createElement('option');
        option.value = name;
        option.textContent = name;
      colorbarElements.cmapSelect.appendChild(option);
      });
      colorbarElements.cmapSelect.addEventListener('change', (event) => {
        if (suppressCmapEvent) {
          return;
        }
        setColorbarCmap(event.target.value, true);
        replotActiveLayer("colormap change");
      });
    }
    if (colorbarElements.autoFitCheckbox) {
      colorbarElements.autoFitCheckbox.checked = colorbarState.autoRange;
      colorbarElements.autoFitCheckbox.addEventListener('change', (event) => {
        colorbarState.autoRange = event.target.checked;
        if (colorbarState.autoRange) {
          handleColorbarUseData();
        } else {
          updateColorbarStatus('Custom range locked');
          replotActiveLayer("autofit off");
        }
      });
    }
    if (colorbarElements.applyBtn) {
      colorbarElements.applyBtn.addEventListener('click', handleColorbarApply);
    }
    if (colorbarElements.useDataBtn) {
      colorbarElements.useDataBtn.addEventListener('click', handleColorbarUseData);
    }
    setColorbarCmap(DEFAULT_COLORMAP, false);
    updateColorbarStatus('Waiting for data range');
  }

  initializeColorbarUI();
  
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
  function updateMapWithGeoJSON(geojson, refit = false) {
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
    // Bust cache so freshly generated image is fetched.
    const cacheBustedUrl = imageUrl ? `${imageUrl}${imageUrl.includes('?') ? '&' : '?'}_cb=${Date.now()}` : imageUrl;
    const leafletBounds = [[bounds.SWlat, bounds.SWlon], [bounds.NElat, bounds.NElon]];
    imageOverlay = L.imageOverlay(cacheBustedUrl, leafletBounds, { opacity: 0.7 });
    imageOverlay.addTo(map);
    imageOverlay.setZIndex(0);
  }
  
  // Send command function: auto-prefix with "ff:".
  async function sendCommand(command) {
    const responseDiv = responseConsoleEl || document.getElementById('responseConsole');
    const historyDiv = commandHistoryEl || document.getElementById('commandHistory');
    historyDiv.innerHTML += "<div class='commandText'>" + command + "</div>";
    historyDiv.scrollTop = historyDiv.scrollHeight;
    responseDiv.innerHTML += "<div class='commandText'>forefire >" + command + "</div>";
    responseDiv.scrollTop = responseDiv.scrollHeight;
    const originalCommand = command;

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
      if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
        try {
          const metadata = JSON.parse(trimmed);
          const derivedLayer = extractLayerFromPlotCommand(originalCommand);
          handlePlotMetadata(derivedLayer, metadata);
        } catch (e) {
          // Ignore JSON parse issues for non-plot responses.
        }
      }
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
    uiLog("Requesting layer list...");
    const layersResponse = await sendCommand("getParameter[layerNames]");
    if (!layersResponse) {
      uiLog("Layer list request returned no data", "warn");
      return;
    }
    const layers = layersResponse.trim().split(/\s*,\s*/).filter(Boolean);
    const layerListEl = document.getElementById('layerList');
    layerListEl.innerHTML = "";
    activeLayer = null;
    layers.forEach(layer => {
      const li = document.createElement('li');
      li.textContent = layer;
      li.addEventListener('click', () => {
        activeLayer = layer;
        runLayerPlot(layer, true, "layer click");
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
          uiLog("Wind layer removed");
          return;
        }
        activeLayer = "wind";
        let cmd = `plot[parameter=wind;filename=windCS.json;size=60,60;projectionOut=json${getBBoxParam()}]`;
        sendCommand(cmd).then(responseText => {
          let bboxData;
          try {
            bboxData = JSON.parse(responseText);
          } catch (e) {
            console.error("Error parsing bounding box data:", e);
            uiLog("Wind plot response was not JSON", "warn");
          }
          // If no bounding box exists yet and the response contains bounds, create one.
          if (!bboxRect && bboxData && bboxData.SWlon !== undefined) {
            createBoundingBoxFromResponse(bboxData);
          }
          // Use minVal and maxVal from bboxData if available.
          let windMin = (bboxData && bboxData.minVal !== undefined) ? bboxData.minVal : 0;
          let windMax = (bboxData && bboxData.maxVal !== undefined) ? bboxData.maxVal : 10;
          uiLog(`Wind min/max: ${windMin}/${windMax}`);
          // Now fetch the actual wind data from the file windCS.json.
          fetch(`windCS.json?_cb=${Date.now()}`)
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
              uiLog("Wind layer updated");
            })
            .catch(e => {
              console.error("Error fetching wind data:", e);
              uiLog("Failed to fetch wind data", "error");
            });
        });
      });
      layerListEl.appendChild(li);
    }
    uiLog(`Layer list refreshed (${layers.length} items)`);
  }

  async function runLayerPlot(layer, refit = true, reason = "rerender") {
    activeLayer = layer;
    ensureColorbarDefaultsForLayer(layer);
    const cmd = buildPlotCommand(layer);
    lastPlotCommand = cmd;
    uiLog(`Plotting ${layer} (${reason})...`);
    try {
      const responseText = await sendCommand(cmd);
      if (!responseText) {
        uiLog(`No data returned for ${layer}`, "warn");
        return;
      }
      try {
        const bounds = JSON.parse(responseText);
        // If no bounding box exists yet, use these bounds.
        if (!bboxRect && bounds.SWlon !== undefined) {
          createBoundingBoxFromResponse(bounds);
        }
        const match = cmd.match(/filename=([^;]+)/);
        if (match) {
          addImageOverlay(match[1], bounds);
        }
        handlePlotMetadata(layer, bounds);
        uiLog(`Overlay updated for ${layer}`);
      } catch (err) {
        console.error("Error parsing overlay bounds:", err);
        uiLog(`Plot response for ${layer} was not JSON`, "warn");
      }
    } catch (err) {
      uiLog(`Failed to plot ${layer}: ${err.message}`, "error");
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
async function refreshMap(refit = false) {
  const shouldRefit = (typeof refit === "boolean") ? refit : false;
  uiLog(`Refreshing map${shouldRefit ? " with refit" : ""}...`);
  // The same logic you had in the 'refreshMap' button’s click handler
  await sendCommand("setParameter[dumpMode=geojson]");
  const geojsonText = await sendCommand("print[]");
  if (geojsonText) {
    try {
      const geojson = JSON.parse(geojsonText);
        updateMapWithGeoJSON(geojson, shouldRefit);
      } catch (e) {
        console.error("Failed to parse GeoJSON:", e);
        uiLog("GeoJSON parse failed during refresh", "error");
      }
    } else {
      uiLog("No GeoJSON returned during refresh", "warn");
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
      uiLog("Failed to refresh wind layer", "error");
      });
  }
  // Refresh current image layer (if any) when live-refreshing.
  if (activeLayer && activeLayer !== "wind") {
    runLayerPlot(activeLayer, false, "auto-refresh");
  }
  uiLog("Refresh complete");
}


// 2) Link existing "Refresh Map" button to our new function without recentering:
document.getElementById('refreshMap').addEventListener('click', () => refreshMap(false));

// 3) Handle the auto-refresh checkbox changes:
document.getElementById('autoRefreshCheckbox').addEventListener('change', (e) => {
  if (e.target.checked) {
    // Start auto-refresh (adjust as needed)
      autoRefreshInterval = setInterval(() => {
      refreshMap(false); 
    }, 500);
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

toggleLogs.addEventListener('change', function() {
  const displayStyle = this.checked ? 'block' : 'none';
  if (responseConsoleEl) {
    responseConsoleEl.style.display = displayStyle;
  }
  if (commandHistoryEl) {
    commandHistoryEl.style.display = displayStyle;
  }
  
  // Optional: Adjust map height if needed when logs are hidden
  // For example, increase the map height when logs are hidden
  if (!this.checked) {
    document.getElementById('map').style.height = '700px';
  } else {
    document.getElementById('map').style.height = '500px';
  }
});
