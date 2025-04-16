/*
 Generic  Canvas Layer for leaflet 0.7 and 1.0-rc,
 copyright Stanislav Sumbera,  2016 , sumbera.com , license MIT
 originally created and motivated by L.CanvasOverlay  available here: https://gist.github.com/Sumbera/11114288
*/

// ----------------------------------------------------------------------------------
// The L.CanvasLayer, L.Control.Velocity, and L.VelocityLayer definitions remain the same,
// except we've replaced the Windy definition at the bottom with one that supports useVectors.
// ----------------------------------------------------------------------------------

"use strict";

//-- L.DomUtil.setTransform from leaflet 1.0.0 to work on 0.0.7
if (!L.DomUtil.setTransform) {
  L.DomUtil.setTransform = function (el, offset, scale) {
    var pos = offset || new L.Point(0, 0);
    el.style[L.DomUtil.TRANSFORM] =
      (L.Browser.ie3d
        ? "translate(" + pos.x + "px," + pos.y + "px)"
        : "translate3d(" + pos.x + "px," + pos.y + "px,0)") +
      (scale ? " scale(" + scale + ")" : "");
  };
}

L.CanvasLayer = (L.Layer ? L.Layer : L.Class).extend({
  initialize: function (options) {
    this._map = null;
    this._canvas = null;
    this._frame = null;
    this._delegate = null;
    L.setOptions(this, options);
  },

  delegate: function (del) {
    this._delegate = del;
    return this;
  },

  needRedraw: function () {
    if (!this._frame) {
      this._frame = L.Util.requestAnimFrame(this.drawLayer, this);
    }
    return this;
  },

  _onLayerDidResize: function (resizeEvent) {
    this._canvas.width = resizeEvent.newSize.x;
    this._canvas.height = resizeEvent.newSize.y;
  },

  _onLayerDidMove: function () {
    var topLeft = this._map.containerPointToLayerPoint([0, 0]);
    L.DomUtil.setPosition(this._canvas, topLeft);
    this.drawLayer();
  },

  getEvents: function () {
    var events = {
      resize: this._onLayerDidResize,
      moveend: this._onLayerDidMove,
    };
    if (this._map.options.zoomAnimation && L.Browser.any3d) {
      events.zoomanim = this._animateZoom;
    }
    return events;
  },

  onAdd: function (map) {
    this._map = map;
    this._canvas = L.DomUtil.create("canvas", "leaflet-layer");
    this.tiles = {};

    var size = this._map.getSize();
    this._canvas.width = size.x;
    this._canvas.height = size.y;

    var animated = this._map.options.zoomAnimation && L.Browser.any3d;
    L.DomUtil.addClass(
      this._canvas,
      "leaflet-zoom-" + (animated ? "animated" : "hide")
    );
    this.options.pane.appendChild(this._canvas);

    map.on(this.getEvents(), this);

    var del = this._delegate || this;
    del.onLayerDidMount && del.onLayerDidMount(); // callback

    this.needRedraw();

    var self = this;
    setTimeout(function () {
      self._onLayerDidMove();
    }, 0);
  },

  onRemove: function (map) {
    var del = this._delegate || this;
    del.onLayerWillUnmount && del.onLayerWillUnmount(); // callback
    this.options.pane.removeChild(this._canvas);
    map.off(this.getEvents(), this);
    this._canvas = null;
  },

  addTo: function (map) {
    map.addLayer(this);
    return this;
  },

  drawLayer: function () {
    var size = this._map.getSize();
    var bounds = this._map.getBounds();
    var zoom = this._map.getZoom();
    var center = this._map.options.crs.project(this._map.getCenter());
    var corner = this._map.options.crs.project(
      this._map.containerPointToLatLng(this._map.getSize())
    );

    var del = this._delegate || this;
    del.onDrawLayer &&
      del.onDrawLayer({
        layer: this,
        canvas: this._canvas,
        bounds: bounds,
        size: size,
        zoom: zoom,
        center: center,
        corner: corner,
      });
    this._frame = null;
  },

  _setTransform: function (el, offset, scale) {
    var pos = offset || new L.Point(0, 0);
    el.style[L.DomUtil.TRANSFORM] =
      (L.Browser.ie3d
        ? "translate(" + pos.x + "px," + pos.y + "px)"
        : "translate3d(" + pos.x + "px," + pos.y + "px,0)") +
      (scale ? " scale(" + scale + ")" : "");
  },

  _animateZoom: function (e) {
    var scale = this._map.getZoomScale(e.zoom);
    // different calc of offset in leaflet 1.0.0 and 0.0.7
    var offset = L.Layer
      ? this._map._latLngToNewLayerPoint(
          this._map.getBounds().getNorthWest(),
          e.zoom,
          e.center
        )
      : this._map
          ._getCenterOffset(e.center)
          ._multiplyBy(-scale)
          .subtract(this._map._getMapPanePos());

    L.DomUtil.setTransform(this._canvas, offset, scale);
  },
});

L.canvasLayer = function (pane) {
  return new L.CanvasLayer(pane);
};

// ------------------------------------------------
// L.Control.Velocity
// ------------------------------------------------

L.Control.Velocity = L.Control.extend({
  options: {
    position: "bottomleft",
    emptyString: "Unavailable",
    angleConvention: "bearingCCW",
    showCardinal: false,
    speedUnit: "m/s",
    directionString: "Direction",
    speedString: "Speed",
    onAdd: null,
    onRemove: null,
  },

  onAdd: function (map) {
    this._container = L.DomUtil.create("div", "leaflet-control-velocity");
    L.DomEvent.disableClickPropagation(this._container);
    map.on("mousemove", this._onMouseMove, this);
    this._container.innerHTML = this.options.emptyString;

    if (this.options.leafletVelocity.options.onAdd)
      this.options.leafletVelocity.options.onAdd();
    return this._container;
  },

  onRemove: function (map) {
    map.off("mousemove", this._onMouseMove, this);
    if (this.options.leafletVelocity.options.onRemove)
      this.options.leafletVelocity.options.onRemove();
  },

  vectorToSpeed: function (uMs, vMs, unit) {
    var velocityAbs = Math.sqrt(uMs * uMs + vMs * vMs);
    if (unit === "k/h") {
      return this.meterSec2kilometerHour(velocityAbs);
    } else if (unit === "kt") {
      return this.meterSec2Knots(velocityAbs);
    } else if (unit === "mph") {
      return this.meterSec2milesHour(velocityAbs);
    } else {
      return velocityAbs; // m/s by default
    }
  },

  vectorToDegrees: function (uMs, vMs, angleConvention) {
    // Default angle convention is CW
    if (angleConvention.endsWith("CCW")) {
      // vMs comes out upside-down..
      vMs = vMs > 0 ? (vMs = -vMs) : Math.abs(vMs);
    }
    var velocityAbs = Math.sqrt(uMs * uMs + vMs * vMs);
    var velocityDir = Math.atan2(uMs / velocityAbs, vMs / velocityAbs);
    var velocityDirToDegrees = (velocityDir * 180) / Math.PI + 180;

    if (angleConvention === "bearingCW" || angleConvention === "meteoCCW") {
      velocityDirToDegrees += 180;
      if (velocityDirToDegrees >= 360) velocityDirToDegrees -= 360;
    }
    return velocityDirToDegrees;
  },

  degreesToCardinalDirection: function (deg) {
    var cardinalDirection = "";

    if ((deg >= 0 && deg < 11.25) || deg >= 348.75) {
      cardinalDirection = "N";
    } else if (deg >= 11.25 && deg < 33.75) {
      cardinalDirection = "NNW";
    } else if (deg >= 33.75 && deg < 56.25) {
      cardinalDirection = "NW";
    } else if (deg >= 56.25 && deg < 78.75) {
      cardinalDirection = "WNW";
    } else if (deg >= 78.25 && deg < 101.25) {
      cardinalDirection = "W";
    } else if (deg >= 101.25 && deg < 123.75) {
      cardinalDirection = "WSW";
    } else if (deg >= 123.75 && deg < 146.25) {
      cardinalDirection = "SW";
    } else if (deg >= 146.25 && deg < 168.75) {
      cardinalDirection = "SSW";
    } else if (deg >= 168.75 && deg < 191.25) {
      cardinalDirection = "S";
    } else if (deg >= 191.25 && deg < 213.75) {
      cardinalDirection = "SSE";
    } else if (deg >= 213.75 && deg < 236.25) {
      cardinalDirection = "SE";
    } else if (deg >= 236.25 && deg < 258.75) {
      cardinalDirection = "ESE";
    } else if (deg >= 258.75 && deg < 281.25) {
      cardinalDirection = "E";
    } else if (deg >= 281.25 && deg < 303.75) {
      cardinalDirection = "ENE";
    } else if (deg >= 303.75 && deg < 326.25) {
      cardinalDirection = "NE";
    } else if (deg >= 326.25 && deg < 348.75) {
      cardinalDirection = "NNE";
    }
    return cardinalDirection;
  },

  meterSec2Knots: function (meters) {
    return meters / 0.514;
  },

  meterSec2kilometerHour: function (meters) {
    return meters * 3.6;
  },

  meterSec2milesHour: function (meters) {
    return meters * 2.23694;
  },

  _onMouseMove: function (e) {
    var self = this;
    var pos = this.options.leafletVelocity._map.containerPointToLatLng(
      L.point(e.containerPoint.x, e.containerPoint.y)
    );
    var gridValue = this.options.leafletVelocity._windy.interpolatePoint(
      pos.lng,
      pos.lat
    );
    var htmlOut = "";

    if (
      gridValue &&
      !isNaN(gridValue[0]) &&
      !isNaN(gridValue[1]) &&
      gridValue[2]
    ) {
      var deg = self.vectorToDegrees(
        gridValue[0],
        gridValue[1],
        this.options.angleConvention
      );
      var cardinal = this.options.showCardinal
        ? " (" + self.degreesToCardinalDirection(deg) + ") "
        : "";

      htmlOut =
        "<strong> " +
        this.options.velocityType +
        " " +
        this.options.directionString +
        ": </strong> " +
        deg.toFixed(2) +
        "°" +
        cardinal +
        ", <strong> " +
        this.options.velocityType +
        " " +
        this.options.speedString +
        ": </strong> " +
        self
          .vectorToSpeed(
            gridValue[0],
            gridValue[1],
            this.options.speedUnit
          )
          .toFixed(2) +
        " " +
        this.options.speedUnit;
    } else {
      htmlOut = this.options.emptyString;
    }
    self._container.innerHTML = htmlOut;
  },
});

L.Map.mergeOptions({
  positionControl: false,
});

L.Map.addInitHook(function () {
  if (this.options.positionControl) {
    this.positionControl = new L.Control.MousePosition();
    this.addControl(this.positionControl);
  }
});

L.control.velocity = function (options) {
  return new L.Control.Velocity(options);
};

// ------------------------------------------------
// L.VelocityLayer
// ------------------------------------------------

L.VelocityLayer = (L.Layer ? L.Layer : L.Class).extend({
  options: {
    displayValues: true,
    displayOptions: {
      velocityType: "Velocity",
      position: "bottomleft",
      emptyString: "No velocity data",
      // NEW: useVectors can be specified here
      useVectors: false,
    },
    maxVelocity: 10,
    colorScale: null,
    data: null,
  },

  _map: null,
  _canvasLayer: null,
  _windy: null,
  _context: null,
  _timer: 0,
  _mouseControl: null,

  initialize: function (options) {
    L.setOptions(this, options);
  },

  onAdd: function (map) {
    this._paneName = this.options.paneName || "overlayPane";
    var pane = map._panes.overlayPane;
    if (map.getPane) {
      pane = map.getPane(this._paneName) || map.createPane(this._paneName);
    }
    this._canvasLayer = L.canvasLayer({ pane: pane }).delegate(this);
    this._canvasLayer.addTo(map);
    this._map = map;
  },

  onRemove: function (map) {
    this._destroyWind();
  },

  setData: function (data) {
    this.options.data = data;
    if (this._windy) {
      this._windy.setData(data);
     // this._clearAndRestart();
    }
    this.fire("load");
  },

  setOpacity: function (opacity) {
    this._canvasLayer.setOpacity(opacity);
  },

  setOptions: function (options) {
    this.options = Object.assign(this.options, options);
    if (options.hasOwnProperty("displayOptions")) {
      this.options.displayOptions = Object.assign(
        this.options.displayOptions,
        options.displayOptions
      );
      this._initMouseHandler(true);
    }
    if (options.hasOwnProperty("data")) this.options.data = options.data;
    if (this._windy) {
      this._windy.setOptions(options);
      if (options.hasOwnProperty("data")) this._windy.setData(options.data);
      this._clearAndRestart();
    }
    this.fire("load");
  },

  onDrawLayer: function (overlay, params) {
    if (!this._windy) {
      this._initWindy(this);
      return;
    }
    if (!this.options.data) {
      return;
    }
    if (this._timer) clearTimeout(this._timer);

    var self = this;
    // small delay so the user doesn't get a partial paint
    this._timer = setTimeout(function () {
      self._startWindy();
    }, 750);
  },

  _startWindy: function () {
    var bounds = this._map.getBounds();
    var size = this._map.getSize();
    this._windy.start(
      [[0, 0], [size.x, size.y]],
      size.x,
      size.y,
      [
        [bounds._southWest.lng, bounds._southWest.lat],
        [bounds._northEast.lng, bounds._northEast.lat],
      ]
    );
  },

  _initWindy: function (self) {
    var options = Object.assign(
      {
        canvas: self._canvasLayer._canvas,
        map: this._map,
      },
      self.options
    );
    this._windy = new Windy(options);
    this._context = this._canvasLayer._canvas.getContext("2d");
    this._canvasLayer._canvas.classList.add("velocity-overlay");
    this.onDrawLayer();

    this._map.on("dragstart", self._windy.stop);
    this._map.on("dragend", self._clearAndRestart);
    this._map.on("zoomstart", self._windy.stop);
    this._map.on("zoomend", self._clearAndRestart);
    this._map.on("resize", self._clearWind);

    this._initMouseHandler(false);
  },

  _initMouseHandler: function (voidPrevious) {
    if (voidPrevious) {
      this._map.removeControl(this._mouseControl);
      this._mouseControl = false;
    }
    if (!this._mouseControl && this.options.displayValues) {
      var options = this.options.displayOptions || {};
      options.leafletVelocity = this;
      this._mouseControl = L.control.velocity(options).addTo(this._map);
    }
  },

  _clearAndRestart: function () {
    if (this._context) this._context.clearRect(0, 0, 3000, 3000);
    if (this._windy) this._startWindy();
  },

  _clearWind: function () {
    if (this._windy) this._windy.stop();
    if (this._context) this._context.clearRect(0, 0, 3000, 3000);
  },

  _destroyWind: function () {
    if (this._timer) clearTimeout(this._timer);
    if (this._windy) this._windy.stop();
    if (this._context) this._context.clearRect(0, 0, 3000, 3000);
    if (this._mouseControl) this._map.removeControl(this._mouseControl);
    this._mouseControl = null;
    this._windy = null;
    this._map.removeLayer(this._canvasLayer);
  },
});

L.velocityLayer = function (options) {
  return new L.VelocityLayer(options);
};

// ------------------------------------------------
// Windy class, with new useVectors logic
// ------------------------------------------------

var Windy = function Windy(params) {
  // Default values
  var MIN_VELOCITY_INTENSITY = params.minVelocity || 0;
  

  var MAX_VELOCITY_INTENSITY = params.maxVelocity || 10;



  console.log("MIN_VELOCITY_INTENSITY", MIN_VELOCITY_INTENSITY, "MAX_VELOCITY_INTENSITY", MAX_VELOCITY_INTENSITY);

  var VELOCITY_SCALE =
    (params.velocityScale || 0.005) *
    (Math.pow(window.devicePixelRatio, 1 / 3) || 1);
  var MAX_PARTICLE_AGE = params.particleAge || 90;
  var PARTICLE_LINE_WIDTH = params.lineWidth || 1;
  var PARTICLE_MULTIPLIER = params.particleMultiplier || 1 / 300;
  var PARTICLE_REDUCTION = Math.pow(window.devicePixelRatio, 1 / 3) || 1.6;
  var FRAME_RATE = params.frameRate || 15;
  var FRAME_TIME = 1000 / FRAME_RATE;
  var OPACITY = params.opacity != null ? params.opacity : 0.97;
  var colorScale =
  params.colorScale ||
  [
    "rgb(48,18,59)",
    "rgb(70,35,115)",
    "rgb(84,62,167)",
    "rgb(92,93,208)",
    "rgb(94,126,236)",
    "rgb(91,159,244)",
    "rgb(81,190,234)",
    "rgb(68,215,206)",
    "rgb(54,233,167)",
    "rgb(44,246,127)",
    "rgb(44,255,95)",
    "rgb(58,255,71)",
    "rgb(91,251,60)",
    "rgb(132,241,61)",
    "rgb(173,225,68)",
    "rgb(210,204,74)",
    "rgb(236,181,75)",
    "rgb(251,153,66)",
    "rgb(255,120,53)",
    "rgb(248,84,40)",
    "rgb(235,48,33)",
    "rgb(215,19,30)",
    "rgb(187,2,32)",
    "rgb(151,0,34)"
  ];

  var NULL_WIND_VECTOR = [NaN, NaN, null];

  // NEW: user option to just draw vectors (arrows) instead of animating
  var USE_VECTORS = false;
  if (
    params.displayOptions &&
    params.displayOptions.useVectors === true
  ) {
    USE_VECTORS = true;
  }

  // We'll store data, plus the underlying grid
  var builder;
  var grid;
  var gridData = params.data;
  var date;
  var λ0, φ0, Δλ, Δφ, ni, nj;

  function setData(data) {
    gridData = data;
  }

  function setOptions(options) {
    if (options.hasOwnProperty("minVelocity"))
      MIN_VELOCITY_INTENSITY = options.minVelocity;

    if (options.hasOwnProperty("maxVelocity"))
      MAX_VELOCITY_INTENSITY = options.maxVelocity;

    if (options.hasOwnProperty("velocityScale")) {
      VELOCITY_SCALE =
        (options.velocityScale || 0.005) *
        (Math.pow(window.devicePixelRatio, 1 / 3) || 1);
    }
    if (options.hasOwnProperty("particleAge"))
      MAX_PARTICLE_AGE = options.particleAge;
    if (options.hasOwnProperty("lineWidth"))
      PARTICLE_LINE_WIDTH = options.lineWidth;
    if (options.hasOwnProperty("particleMultiplier"))
      PARTICLE_MULTIPLIER = options.particleMultiplier;
    if (options.hasOwnProperty("opacity")) OPACITY = +options.opacity;
    if (options.hasOwnProperty("frameRate")) {
      FRAME_RATE = options.frameRate;
      FRAME_TIME = 1000 / FRAME_RATE;
    }

    // Possibly re-check useVectors
    if (
      options.displayOptions &&
      options.displayOptions.useVectors === true
    ) {
      USE_VECTORS = true;
    } else if (
      options.displayOptions &&
      options.displayOptions.useVectors === false
    ) {
      USE_VECTORS = false;
    }
  }

  // Interpolation for vectors (u,v)
  function bilinearInterpolateVector(x, y, g00, g10, g01, g11) {
    var rx = 1 - x;
    var ry = 1 - y;
    var a = rx * ry,
      b = x * ry,
      c = rx * y,
      d = x * y;
    var u = g00[0] * a + g10[0] * b + g01[0] * c + g11[0] * d;
    var v = g00[1] * a + g10[1] * b + g01[1] * c + g11[1] * d;
    return [u, v, Math.sqrt(u * u + v * v)];
  }

  function createWindBuilder(uComp, vComp) {
    var uData = uComp.data,
      vData = vComp.data;
    return {
      header: uComp.header,
      data: function (i) {
        return [uData[i], vData[i]];
      },
      interpolate: bilinearInterpolateVector,
    };
  }

  function createBuilder(data) {
    var uComp = null,
      vComp = null;
    data.forEach(function (record) {
      // look for the usual parameters
      switch (record.header.parameterCategory + "," + record.header.parameterNumber) {
        case "1,2":
        case "2,2":
          uComp = record;
          break;
        case "1,3":
        case "2,3":
          vComp = record;
          break;
      }
    });
    return createWindBuilder(uComp, vComp);
  }

  function buildGrid(data, callback) {
    builder = createBuilder(data);
    var header = builder.header;
    λ0 = header.lo1;
    φ0 = header.la1;
    Δλ = header.dx;
    Δφ = header.dy;
    ni = header.nx;
    nj = header.ny;

    var isContinuous = Math.floor(ni * Δλ) >= 360;
    date = new Date(header.refTime);
    date.setHours(date.getHours() + header.forecastTime);

    grid = [];
    var p = 0;
    for (var j = 0; j < nj; j++) {
      var row = [];
      for (var i = 0; i < ni; i++, p++) {
        row[i] = builder.data(p);
      }
      if (isContinuous) {
        row.push(row[0]);
      }
      grid[j] = row;
    }
    callback({ date: date, interpolate: interpolate });
  }

  function interpolate(λ, φ) {
    if (!grid) return null;
    // Note: we assume "wrap" in longitude
    var i = floorMod(λ - λ0, 360) / Δλ;
    var j = (φ0 - φ) / Δφ;
    var fi = Math.floor(i),
      ci = fi + 1;
    var fj = Math.floor(j),
      cj = fj + 1;
    var row = grid[fj];
    if (row) {
      var g00 = row[fi];
      var g10 = row[ci];
      if (isValue(g00) && isValue(g10)) {
        row = grid[cj];
        if (row) {
          var g01 = row[fi];
          var g11 = row[ci];
          if (isValue(g01) && isValue(g11)) {
            return builder.interpolate(
              i - fi,
              j - fj,
              g00,
              g10,
              g01,
              g11
            );
          }
        }
      }
    }
    return null;
  }

  function isValue(x) {
    return x !== null && x !== undefined;
  }
  function floorMod(a, n) {
    return a - n * Math.floor(a / n);
  }
  function clamp(x, range) {
    return Math.max(range[0], Math.min(x, range[1]));
  }
  function isMobile() {
    return /android|blackberry|iemobile|ipad|iphone|ipod|opera mini|webos/i.test(
      navigator.userAgent
    );
  }

  // Distortion is used in the original code to shape the wind vectors to the map projection.
  // For arrow drawing, you can keep or skip it. Here we'll keep it for continuity in the animation case.
  function distort(projection, λ, φ, x, y, scale, wind) {
    var u = wind[0] * scale;
    var v = wind[1] * scale;
    var d = distortion(projection, λ, φ, x, y);
    wind[0] = d[0] * u + d[2] * v;
    wind[1] = d[1] * u + d[3] * v;
    return wind;
  }

  function distortion(projection, λ, φ, x, y) {
    var H = 5; // a small value
    var hλ = λ < 0 ? H : -H;
    var hφ = φ < 0 ? H : -H;
    var pλ = project(φ, λ + hλ);
    var pφ = project(φ + hφ, λ);
    var k = Math.cos((φ / 180) * Math.PI);
    return [
      (pλ[0] - x) / hλ / k,
      (pλ[1] - y) / hλ / k,
      (pφ[0] - x) / hφ,
      (pφ[1] - y) / hφ,
    ];
  }

  function project(lat, lon) {
    var xy = params.map.latLngToContainerPoint(L.latLng(lat, lon));
    return [xy.x, xy.y];
  }
  function invert(x, y) {
    var latlon = params.map.containerPointToLatLng(L.point(x, y));
    return [latlon.lng, latlon.lat];
  }

  // The original "createField" + "interpolateField" + "animate" produce the moving particles
  // We will keep them intact, but skip them if `useVectors` is true.

  function createField(columns, bounds, callback) {
    function field(x, y) {
      var column = columns[Math.round(x)];
      return column && column[Math.round(y)] || NULL_WIND_VECTOR;
    }
    field.release = function () {
      columns = [];
    };
    field.randomize = function (o) {
      var x, y;
      var safetyNet = 0;
      do {
        x = Math.round(Math.random() * bounds.width + bounds.x);
        y = Math.round(Math.random() * bounds.height + bounds.y);
      } while (field(x, y)[2] === null && safetyNet++ < 30);
      o.x = x;
      o.y = y;
      return o;
    };
    callback(bounds, field);
  }

  function buildBounds(bounds, width, height) {
    var upperLeft = bounds[0];
    var lowerRight = bounds[1];
    var x = Math.round(upperLeft[0]);
    var y = Math.max(Math.floor(upperLeft[1], 0), 0);
    var xMax = Math.min(Math.ceil(lowerRight[0], width), width - 1);
    var yMax = Math.min(Math.ceil(lowerRight[1], height), height - 1);
    return { x: x, y: y, xMax: width, yMax: yMax, width: width, height: height };
  }

  function interpolateField(grid, bounds, extent, callback) {
    var projection = {};
    var mapArea = (extent.south - extent.north) * (extent.west - extent.east);
    var velocityScale = VELOCITY_SCALE * Math.pow(mapArea, 0.4);
    var columns = [];
    var x = bounds.x;

    function interpolateColumn(x) {
      var column = [];
      for (var y = bounds.y; y <= bounds.yMax; y += 2) {
        var coord = invert(x, y);
        if (coord) {
          var λ = coord[0],
            φ = coord[1];
          if (isFinite(λ)) {
            var wind = grid.interpolate(λ, φ);
            if (wind) {
              wind = distort(projection, λ, φ, x, y, velocityScale, wind);
              column[y + 1] = column[y] = wind;
            }
          }
        }
      }
      columns[x + 1] = columns[x] = column;
    }

    (function batchInterpolate() {
      var start = Date.now();
      while (x < bounds.width) {
        interpolateColumn(x);
        x += 2;
        if (Date.now() - start > 1000) {
          setTimeout(batchInterpolate, 25);
          return;
        }
      }
      createField(columns, bounds, callback);
    })();
  }

  var animationLoop;

  function animate(bounds, field) {
    function windIntensityColorScale(min, max) {
      colorScale.indexFor = function (m) {
        return Math.max(
          0,
          Math.min(
            colorScale.length - 1,
            Math.round(((m - min) / (max - min)) * (colorScale.length - 1))
          )
        );
      };
      return colorScale;
    }

    var colorStyles = windIntensityColorScale(
      MIN_VELOCITY_INTENSITY,
      MAX_VELOCITY_INTENSITY
    );
    var buckets = colorStyles.map(function () {
      return [];
    });
    var particleCount = Math.round(
      bounds.width * bounds.height * PARTICLE_MULTIPLIER
    );
    if (isMobile()) {
      particleCount *= PARTICLE_REDUCTION;
    }
    var fadeFillStyle = "rgba(0, 0, 0, " + OPACITY + ")";
    var particles = [];
    for (var i = 0; i < particleCount; i++) {
      particles.push(field.randomize({ age: ~~(Math.random() * MAX_PARTICLE_AGE) }));
    }

    var g = params.canvas.getContext("2d");
    g.lineWidth = PARTICLE_LINE_WIDTH;
    g.fillStyle = fadeFillStyle;
    g.globalAlpha = 0.6;

    function evolve() {
      buckets.forEach(function (bucket) {
        bucket.length = 0;
      });
      particles.forEach(function (particle) {
        if (particle.age > MAX_PARTICLE_AGE) {
          field.randomize(particle).age = 0;
        }
        var x = particle.x;
        var y = particle.y;
        var v = field(x, y);
        var m = v[2];
        if (m === null) {
          particle.age = MAX_PARTICLE_AGE;
        } else {
          var xt = x + v[0];
          var yt = y + v[1];
          if (field(xt, yt)[2] !== null) {
            particle.xt = xt;
            particle.yt = yt;
            buckets[colorStyles.indexFor(m)].push(particle);
          } else {
            particle.x = xt;
            particle.y = yt;
          }
        }
        particle.age += 1;
      });
    }

    function draw() {
      // Fade existing trails
      var prev = "lighter";
      g.globalCompositeOperation = "destination-in";
      g.fillRect(bounds.x, bounds.y, bounds.width, bounds.height);
      g.globalCompositeOperation = prev;
      g.globalAlpha = OPACITY === 0 ? 0 : OPACITY * 0.9;

      buckets.forEach(function (bucket, i) {
        if (bucket.length > 0) {
          g.beginPath();
          g.strokeStyle = colorStyles[i];
          bucket.forEach(function (particle) {
            g.moveTo(particle.x, particle.y);
            g.lineTo(particle.xt, particle.yt);
            particle.x = particle.xt;
            particle.y = particle.yt;
          });
          g.stroke();
        }
      });
    }

    var then = Date.now();
    (function frame() {
      animationLoop = requestAnimationFrame(frame);
      var now = Date.now();
      var delta = now - then;
      if (delta > FRAME_TIME) {
        then = now - (delta % FRAME_TIME);
        evolve();
        draw();
      }
    })();
  }
  
  function drawVectors(gridData, bounds, width, height) {
    var g = params.canvas.getContext("2d");
    // Clear the entire canvas
    g.clearRect(0, 0, width, height);
  
    // Similar color scale usage
    function windIntensityColorScale(min, max) {
      colorScale.indexFor = function (m) {
        return Math.max(
          0,
          Math.min(
            colorScale.length - 1,
            Math.round(((m - min) / (max - min)) * (colorScale.length - 1))
          )
        );
      };
      return colorScale;
    }
    var colorStyles = windIntensityColorScale(
      MIN_VELOCITY_INTENSITY,
      MAX_VELOCITY_INTENSITY
    );
  
   // console.log("MIN_VELOCITY_INTENSITY", MIN_VELOCITY_INTENSITY, "MAX_VELOCITY_INTENSITY", MAX_VELOCITY_INTENSITY);

    // Set arrow spacing and arrow length limits
    var ARROW_SPACING = 20;
    var minLen = ARROW_SPACING/2;
    var maxLen = ARROW_SPACING;
  
    // Compute the projected coordinates for the extreme grid points.
    // Top-left point
    var ptMin = project(φ0, λ0);
    // Bottom-right point using last indices: j = nj-1, i = ni-1.
    var ptMax = project(φ0 - (nj - 1) * Δφ, λ0 + (ni - 1) * Δλ);
    // Compute the absolute width and height in projected (canvas) space.
    var boundsWidth = Math.abs(ptMax[0] - ptMin[0]);
    var boundsHeight = Math.abs(ptMax[1] - ptMin[1]);
  
    // Determine the step for grid indices approximating ARROW_SPACING in pixels.
    var stepi = Math.round((ARROW_SPACING * ni) / boundsWidth);
    var stepj = Math.round((ARROW_SPACING * nj) / boundsHeight);
    if (stepi < 1) stepi = 1;
    if (stepj < 1) stepj = 1;

    //console.log("stepi, stepj", stepi, stepj);
  
    // Loop over grid indices using the computed steps.
    for (var j = 0; j < nj; j += stepj) {
      var lat = φ0 - j * Δφ;
      for (var i = 0; i < ni; i += stepi) {
        var lon = λ0 + i * Δλ;
        var wind = grid[j][i]; // [u, v, mag]
  
        if (!wind) {
          continue;
        }
        var mag = Math.sqrt(wind[0] * wind[0] + wind[1] * wind[1]);
        // Clamp mag to the range
        var speed =  clamp(mag, [MIN_VELOCITY_INTENSITY, MAX_VELOCITY_INTENSITY]);
        // Determine color based on speed
        var colorIndex = colorStyles.indexFor(speed);
        var arrowColor = colorScale[colorIndex];
  
        // Scale arrow length based on wind speed
        var t = (speed - MIN_VELOCITY_INTENSITY) / (MAX_VELOCITY_INTENSITY - MIN_VELOCITY_INTENSITY);
        t = Math.max(0, Math.min(1, t));
        var arrowLen = minLen + t * (maxLen - minLen);

       // console.log("arrowLen", t, arrowLen );
      
        // Get projected x,y on canvas
        var pt = project(lat, lon); // [x,y]
        var x = pt[0], y = pt[1];
        // Skip if outside the canvas bounds (with a small buffer)
        if (x < bounds[0][0] - 10 || x > bounds[1][0] + 10) continue;
        if (y < bounds[0][1] - 10 || y > bounds[1][1] + 10) continue;
  
        // Calculate arrow angle from the wind vector
        var angle = Math.atan2(-wind[1], wind[0]);
  
        // Draw arrow
        g.save();
        g.translate(x, y);
        g.rotate(angle);
        g.strokeStyle = arrowColor;
        g.lineWidth = PARTICLE_LINE_WIDTH;
        g.beginPath();
        g.moveTo(0, 0);
        g.lineTo(arrowLen, 0);
        g.stroke();
  
        // Draw a small arrowhead
        g.beginPath();
        g.moveTo(arrowLen - 6, -3);
        g.lineTo(arrowLen, 0);
        g.lineTo(arrowLen - 6, 3);
        g.stroke();
        g.restore();
      }
    }
  }

  function start(bounds, width, height, extent) {
    stop();

    // Build the grid first
    buildGrid(gridData, function (gridResult) {
      // If user wants static vectors, draw them once:
      if (USE_VECTORS) {
        // compute the bounding box in pixel coords
        var pxBounds = [[0, 0], [width, height]];
        
        drawVectors(gridData, pxBounds, width, height);
        return;
      }

      // Otherwise, do the original interpolation + animation
      interpolateField(
        gridResult,
        buildBounds(bounds, width, height),
        {
          south: deg2rad(extent[0][1]),
          north: deg2rad(extent[1][1]),
          east: deg2rad(extent[1][0]),
          west: deg2rad(extent[0][0]),
        },
        function (bounds, field) {
          windy.field = field;
          animate(bounds, field);
        }
      );
    });
  }

  function stop() {
    if (windy.field) windy.field.release();
    if (animationLoop) cancelAnimationFrame(animationLoop);
  }

  function deg2rad(deg) {
    return (deg / 180) * Math.PI;
  }

  var windy = {
    params: params,
    start: start,
    stop: stop,
    createField: createField,
    interpolatePoint: interpolate,
    setData: setData,
    setOptions: setOptions,
  };
  return windy;
};

// Fallback for older browsers
if (!window.cancelAnimationFrame) {
  window.cancelAnimationFrame = function (id) {
    clearTimeout(id);
  };
}






