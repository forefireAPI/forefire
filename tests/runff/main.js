// Updated list of commands.
const commands = {
  "FireDomain": "FireDomain[sw=(0.0,0.0,0.0);ne=(100.0,100.0,0.0);t=0.0]",
  "FireNode": "FireNode[loc=(0.0,0.0,0.0);vel=(0.0,0.0,0.0);t=0.]",
  "FireFront": "FireFront[]",
  "startFire": "startFire[lonlat=(9.0,42.0,0.0);t=0.0]",
  "step": "step[dt=5]",
  "trigger": "trigger[fuelIndice;loc=(x,y,z);fuelType=int or wind;loc=(x,y,z);vel=(vx,vy,vz);t=f]",
  "goTo": "goTo[t=56.2]",
  "print": "print[]",
  "save": "save[]",
  "load": "load[]",
  "plot": "plot[parameter=speed;filename=out_file_name.png;range=(0,0.1);cmap=viridis]",
  "computeSpeed": "computeSpeed[]",
  "setParameter": "setParameter[param=value]",
  "setParameters": "setParameters[param1=val1;param2=val2]",
  "getParameter": "getParameter[key=]",
  "include": "include[input]",
  "help": "help",
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
function updateMapWithGeoJSON(geojson) {
  if (mapLayer) {
    map.removeLayer(mapLayer);
  }
  mapLayer = L.geoJSON(geojson).addTo(map);
  if (mapLayer.getBounds && mapLayer.getBounds().isValid()) {
    map.fitBounds(mapLayer.getBounds());
  }
}

// Send command function: auto-prefix with "ff:" if needed.
async function sendCommand(command) {
        // Append the sent command (in green) to the Command History.
  const responseDiv = document.getElementById('responseConsole');
  const historyDiv = document.getElementById('commandHistory');
  historyDiv.innerHTML += "<div class='commandText'> " + command + "</div>";
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
    // Append the response to the Response Console.
    
 
    responseDiv.innerHTML += "<div><pre>" + text + "</pre></div>";
    responseDiv.scrollTop = responseDiv.scrollHeight;

    // If the response looks like KML, update the map.
    const trimmed = text.trim();
    if (trimmed.startsWith("<?xml") || trimmed.startsWith("<kml")) {
      updateMapWithKML(text);
    }
    return text;
  } catch (error) {
    document.getElementById('responseConsole').innerHTML += "<div>Error: " + error + "</div>";
    return null;
  } finally {
    // Update the header date.
    updateISODate();
  }
}

// Update ISOdate in the header.
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

// Refresh Map button: set dump mode to geojson then print.
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

// Send command when Send button is clicked.
document.getElementById('sendCommand').addEventListener('click', () => {
  const command = document.getElementById('commandInput').value.trim();
  if (command !== "") {
    sendCommand(command);
    document.getElementById('commandInput').value = "";
  }
});

// Also send command when Enter is pressed.
document.getElementById('commandInput').addEventListener('keypress', (e) => {
  if (e.key === "Enter") {
    const command = document.getElementById('commandInput').value.trim();
    if (command !== "") {
      sendCommand(command);
      document.getElementById('commandInput').value = "";
    }
  }
});

// On double-clicking the map, populate the command input with coordinates.
map.on('dblclick', function(e) {
  const lat = e.latlng.lat.toFixed(6);
  const lon = e.latlng.lng.toFixed(6);
  document.getElementById('commandInput').value = "lonLng(" + lon + ", " + lat + ",0)";
});