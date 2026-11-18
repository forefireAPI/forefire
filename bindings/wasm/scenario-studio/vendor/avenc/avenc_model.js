import { decompressSync, strFromU8 } from "../fflate.js";

// Supply a dataset URL in the viewer/extractor query string.
export const DEFAULT_DATASETS = [];

const AVENC_PREFIXES = ["AVENCv2|", "AVENCv1|"];

function b64ToBytes(base64) {
  const normalized = base64.replace(/[\n\r\t ]+/g, "");
  const binary = atob(normalized);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function decodeUuH64ToBytes(payload) {
  let text = payload.trim();
  text = text.replace(/^UUH64\|/i, "").replace(/^AVENC_UUH64\|/i, "");
  text = text.replace(/-/g, "+").replace(/_/g, "/").replace(/\./g, "=");
  while (text.length % 4 !== 0) text += "=";
  return b64ToBytes(text);
}

function inflateMaybe(bytes) {
  try {
    return decompressSync(bytes);
  } catch {
    return bytes;
  }
}

function bytesToText(bytes) {
  try {
    return strFromU8(bytes);
  } catch {
    return new TextDecoder("utf-8", { fatal: false }).decode(bytes);
  }
}

function isBase64Byte(code) {
  return (
    (code >= 65 && code <= 90) ||
    (code >= 97 && code <= 122) ||
    (code >= 48 && code <= 57) ||
    code === 43 || code === 47 || code === 61 || code === 45 || code === 95 || code === 46
  );
}

function findMarkerRange(bytes, markerText) {
  const marker = new TextEncoder().encode(markerText);
  const n = bytes.length;
  const m = marker.length;
  if (m === 0 || n < m) return null;
  for (let i = 0; i <= n - m; i += 1) {
    let ok = true;
    for (let j = 0; j < m; j += 1) {
      if (bytes[i + j] !== marker[j]) {
        ok = false;
        break;
      }
    }
    if (!ok) continue;
    let end = i + m;
    while (end < n && isBase64Byte(bytes[end])) end += 1;
    return { start: i, dataStart: i + m, end };
  }
  return null;
}

function safeNum(v, fallback = 0) {
  const x = Number(v);
  return Number.isFinite(x) ? x : fallback;
}

function readU32LE(bytes, offset) {
  if (offset < 0 || offset + 4 > bytes.length) {
    throw new Error(`readU32LE out of range at offset ${offset}`);
  }
  return (
    (bytes[offset]) |
    (bytes[offset + 1] << 8) |
    (bytes[offset + 2] << 16) |
    (bytes[offset + 3] << 24)
  ) >>> 0;
}

function readU64LE(bytes, offset) {
  const lo = readU32LE(bytes, offset);
  const hi = readU32LE(bytes, offset + 4);
  return (BigInt(hi) << 32n) | BigInt(lo);
}

function readI32LE(bytes, offset) {
  const u = readU32LE(bytes, offset);
  return (u & 0x80000000) ? (u - 0x100000000) : u;
}

function readF64LE(bytes, offset) {
  if (offset < 0 || offset + 8 > bytes.length) {
    throw new Error(`readF64LE out of range at offset ${offset}`);
  }
  const dv = new DataView(bytes.buffer, bytes.byteOffset + offset, 8);
  return dv.getFloat64(0, true);
}

function decodeAvencCommentPayload(prefix, payloadB64) {
  if (prefix === "AVENCv1|") {
    const jsonBytes = inflateMaybe(b64ToBytes(payloadB64));
    return JSON.parse(bytesToText(jsonBytes));
  }

  if (prefix === "AVENCv2|") {
    const packed = inflateMaybe(b64ToBytes(payloadB64));
    if (packed.length < 36) throw new Error("AVENCv2 payload too short");
    const expect = [65, 86, 50, 77, 69, 84, 65, 0]; // "AV2META\0"
    for (let i = 0; i < expect.length; i += 1) {
      if (packed[i] !== expect[i]) throw new Error("Invalid AVENCv2 metadata magic");
    }
    const fmtVersion = readU32LE(packed, 8);
    if ((fmtVersion !== 2 && fmtVersion !== 3 && fmtVersion !== 4)) {
      throw new Error(`Unsupported AVENCv2 format version: ${fmtVersion}`);
    }

    let offset = 12;
    const _flags = readU32LE(packed, offset); offset += 4;
    const nx = readU32LE(packed, offset); offset += 4;
    const ny = readU32LE(packed, offset); offset += 4;
    const streamCount = readU32LE(packed, offset); offset += 4;
    const recordCount = readU32LE(packed, offset); offset += 4;
    const _reserved = readU32LE(packed, offset); offset += 4;

    const readU32 = (label) => {
      if (offset + 4 > packed.length) throw new Error(`Truncated AVENCv2 field: ${label}`);
      const v = readU32LE(packed, offset);
      offset += 4;
      return v;
    };
    const readI32 = (label) => {
      if (offset + 4 > packed.length) throw new Error(`Truncated AVENCv2 field: ${label}`);
      const v = readI32LE(packed, offset);
      offset += 4;
      return v;
    };
    const readU64 = (label) => {
      if (offset + 8 > packed.length) throw new Error(`Truncated AVENCv2 field: ${label}`);
      const v = readU64LE(packed, offset);
      offset += 8;
      return v;
    };
    const readF64 = (label) => {
      if (offset + 8 > packed.length) throw new Error(`Truncated AVENCv2 field: ${label}`);
      const v = readF64LE(packed, offset);
      offset += 8;
      return v;
    };
    const readStr = (label) => {
      const n = readU32(`${label}.len`);
      if (offset + n > packed.length) throw new Error(`Truncated AVENCv2 string: ${label}`);
      const s = bytesToText(packed.subarray(offset, offset + n));
      offset += n;
      return s;
    };

    const streams = [];
    for (let si = 0; si < streamCount; si += 1) {
      const streamIndex = readI32("stream_index");
      const frameCount = readU32("frame_count");
      const bits = readU32("bits");
      const storageShift = readI32("storage_shift");
      const minValue = readF64("min_value");
      const maxValue = readF64("max_value");
      const transformCode = readU32("transform");
      if (transformCode > (fmtVersion === 4 ? 4 : fmtVersion === 3 ? 2 : 1)) throw new Error("Unsupported stream transform");
      const applyLog2 = transformCode === 1;
      const applyAsinh = transformCode === 2 || transformCode === 3;
      const logOffset = readF64("log_offset");
      const transformScale=fmtVersion>=4?readF64("transform_scale"):1;
      const transformOrigin=fmtVersion>=4?readF64("transform_origin"):0;
      const transformSpan=fmtVersion>=4?readF64("transform_span"):1;
      if(!Number.isFinite(transformScale)||transformScale<=0||!Number.isFinite(transformOrigin)||!Number.isFinite(transformSpan)||transformSpan<=0)throw new Error("Invalid transform parameters");
      const encodedWidth = readU32("encoded_width");
      const encodedHeight = readU32("encoded_height");
      const cropWidth = readU32("crop_width");
      const cropHeight = readU32("crop_height");
      const bitmapPayload = readStr("bitmap_payload");
      const templateB64 = readStr("template_b64");
      streams.push({
        stream_index: streamIndex,
        frame_count: frameCount,
        bits,
        storage_shift: storageShift,
        min_value: minValue,
        max_value: maxValue,
        apply_log2: applyLog2,
        apply_asinh: applyAsinh,
        log_offset: logOffset,
        apply_shifted_log: transformCode===4,
        transform_scale: transformScale,
        transform_origin: transformOrigin,
        transform_span: transformSpan,
        encoded_width: encodedWidth,
        encoded_height: encodedHeight,
        crop_width: cropWidth,
        crop_height: cropHeight,
        bitmap_payload: bitmapPayload,
        template_b64: templateB64
      });
    }

    const records = [];
    for (let ri = 0; ri < recordCount; ri += 1) {
      const order = Number(readU64("order"));
      const streamIndex = readI32("record_stream_index");
      const frameIndex = readU32("frame_index");
      const fileIndex = readI32("file_index");
      const messageIndex = Number(readU64("message_index"));
      const frameMin = readF64("frame_min");
      const frameMax = readF64("frame_max");
      const bitmapPresent = readU32("bitmap_present") !== 0;
      const bitsPerValue = readI32("bits_per_value");
      const binaryScaleShift = readI32("binary_scale_shift");
      const shortName = readStr("short_name");
      const typeOfLevel = readStr("type_of_level");
      const level = readStr("level");
      const dataDate = readStr("data_date");
      const dataTime = readStr("data_time");
      const stepRange = readStr("step_range");
      const timestamp = readStr("timestamp");
      const ensembleNumber = readStr("ensemble_number");
      const perturbationNumber = readStr("perturbation_number");
      const templateB64 = readStr("template_b64");
      records.push({
        order,
        stream_index: streamIndex,
        frame_index: frameIndex,
        file_index: fileIndex,
        message_index: messageIndex,
        frame_min: frameMin,
        frame_max: frameMax,
        bitmap_present: bitmapPresent,
        bits_per_value: bitsPerValue,
        binary_scale_shift: binaryScaleShift,
        short_name: shortName,
        type_of_level: typeOfLevel,
        level,
        data_date: dataDate,
        data_time: dataTime,
        step_range: stepRange,
        timestamp,
        ensemble_number: ensembleNumber,
        perturbation_number: perturbationNumber,
        template_b64: templateB64
      });
    }

    const groups = [];
    let totalFrames = 0;
    let maxBits = 0;
    for (let gi = 0; gi < streams.length; gi += 1) {
      const s = streams[gi];
      totalFrames += Number(s.frame_count || 0);
      maxBits = Math.max(maxBits, Number(s.bits || 0));
      const recs = records.filter((r) => Number(r.stream_index) === Number(s.stream_index))
        .sort((a, b) => Number(a.frame_index) - Number(b.frame_index));
      const first = recs[0] || {};
      const shortName = String(first.short_name || `stream_${s.stream_index}`);
      const typeOfLevel = String(first.type_of_level || "");
      const level = String(first.level || "");
      groups.push({
        id: gi,
        name: `${shortName}_${typeOfLevel || "level"}`,
        short_name: shortName,
        type_of_level: typeOfLevel,
        level,
        stream_index: Number(s.stream_index),
        frame_start: 0,
        frame_count: Number(s.frame_count || 0),
        value_min: Number(s.min_value),
        value_max: Number(s.max_value),
        options: {
          template_b64: s.template_b64,
          bitmap_payload: s.bitmap_payload,
          bits: Number(s.bits || 8),
          min_value: Number(s.min_value),
          max_value: Number(s.max_value),
          storage_shift: Number(s.storage_shift || 0),
          encoded_width: Number(s.encoded_width || nx),
          encoded_height: Number(s.encoded_height || ny),
          crop_width: Number(s.crop_width || nx),
          crop_height: Number(s.crop_height || ny),
          quantization: "frameMinMax",
          apply_log2: Boolean(s.apply_log2),
          apply_asinh: Boolean(s.apply_asinh),
          log_offset: Number(s.log_offset || 1),
          apply_shifted_log: Boolean(s.apply_shifted_log),
          transform_scale: Number(s.transform_scale ?? 1),
          transform_origin: Number(s.transform_origin ?? 0),
          transform_span: Number(s.transform_span ?? 1)
        },
        records: recs.map((r) => ({
          file_index: Number(r.file_index || -1),
          message_index: Number(r.message_index || 0),
          frame_index: Number(r.frame_index || 0),
          shortName: String(r.short_name || ""),
          typeOfLevel: String(r.type_of_level || ""),
          level: String(r.level || ""),
          number: String(r.ensemble_number || ""),
          perturbationNumber: String(r.perturbation_number || ""),
          timestamp: String(r.timestamp || ""),
          dataDate: String(r.data_date || ""),
          dataTime: String(r.data_time || ""),
          stepRange: String(r.step_range || ""),
          quantization_min: Number(r.frame_min),
          quantization_max: Number(r.frame_max),
          bitmapPresent: Boolean(r.bitmap_present),
          binary_scale_shift: Number(r.binary_scale_shift || 0)
        }))
      });
    }

    // Coordinate extension (flags bit 0): retain the original grid axes.
    const coords = {};
    if (_flags & 1) {
      for (const [name, expected, units] of [["lat", ny, "degrees_north"], ["lon", nx, "degrees_east"]]) {
        const count = readU32(`${name}_count`);
        if (count !== expected) throw new Error(`Unexpected ${name} coordinate count`);
        const values = Array.from({length: count}, () => readF64(name));
        coords[name] = {data: {values, shape: [count]}, attrs: {units}};
      }
    }
    return {
      schema: "avenc.pack.v2.binary",
      width: Number(nx),
      height: Number(ny),
      frames: Number(totalFrames),
      shape: [Number(totalFrames), Number(ny), Number(nx)],
      dtype: "float32",
      options: { bits: Number(maxBits || 8) },
      coords,
      groups
    };
  }

  throw new Error(`Unsupported AVENC metadata prefix: ${prefix}`);
}

function decodeTypedArrayFromBytes(bytes, dtype) {
  const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out = [];
  const t = String(dtype || "").toLowerCase();
  if (t.includes("float64")) {
    for (let i = 0; i + 7 < bytes.byteLength; i += 8) out.push(dv.getFloat64(i, true));
    return out;
  }
  if (t.includes("float32")) {
    for (let i = 0; i + 3 < bytes.byteLength; i += 4) out.push(dv.getFloat32(i, true));
    return out;
  }
  if (t.includes("int16")) {
    for (let i = 0; i + 1 < bytes.byteLength; i += 2) out.push(dv.getInt16(i, true));
    return out;
  }
  if (t.includes("uint16")) {
    for (let i = 0; i + 1 < bytes.byteLength; i += 2) out.push(dv.getUint16(i, true));
    return out;
  }
  if (t.includes("int32")) {
    for (let i = 0; i + 3 < bytes.byteLength; i += 4) out.push(dv.getInt32(i, true));
    return out;
  }
  if (t.includes("uint32")) {
    for (let i = 0; i + 3 < bytes.byteLength; i += 4) out.push(dv.getUint32(i, true));
    return out;
  }
  for (let i = 0; i < bytes.byteLength; i += 1) out.push(bytes[i]);
  return out;
}

function flattenNumberList(values) {
  if (!Array.isArray(values)) return [];
  if (!values.length) return [];
  if (!Array.isArray(values[0])) return values.map((v) => Number(v));
  const out = [];
  for (const row of values) {
    for (const v of row) out.push(Number(v));
  }
  return out;
}

function decodeCoordValues(coordObj) {
  const dataObj = coordObj?.data || {};
  if (Array.isArray(dataObj.values)) {
    return {
      values: flattenNumberList(dataObj.values),
      shape: Array.isArray(dataObj.shape) ? dataObj.shape.map((v) => Number(v)) : [dataObj.values.length]
    };
  }
  if (dataObj.encoding === "python-list" && Array.isArray(dataObj.values)) {
    return {
      values: flattenNumberList(dataObj.values),
      shape: Array.isArray(dataObj.shape) ? dataObj.shape.map((v) => Number(v)) : [dataObj.values.length]
    };
  }
  if (dataObj.encoding === "zlib+base64" && typeof dataObj.data === "string") {
    const raw = inflateMaybe(b64ToBytes(dataObj.data));
    return {
      values: decodeTypedArrayFromBytes(raw, dataObj.dtype),
      shape: Array.isArray(dataObj.shape) ? dataObj.shape.map((v) => Number(v)) : []
    };
  }
  return { values: [], shape: [] };
}

function parseReconTsv(reconText) {
  const lines = reconText.split(/\r?\n/).map((s) => s.trim()).filter(Boolean);
  if (!lines.length || lines[0] !== "AVPACK_RECON_V1") return { streams: new Map(), records: [] };

  const streams = new Map();
  const records = [];
  for (let li = 1; li < lines.length; li += 1) {
    const cols = lines[li].split("\t");
    if (!cols.length) continue;
    if (cols[0] === "STREAM" && cols.length >= 6) {
      const streamIndex = Number(cols[1]);
      streams.set(streamIndex, {
        stream_index: streamIndex,
        frame_count: Number(cols[2]),
        bits: Number(cols[3]),
        min_value: safeNum(cols[4]),
        max_value: safeNum(cols[5], 1),
        storage_shift: Number(cols[6] || 0),
        apply_log2: String(cols[7] || "").toLowerCase().includes("log2"),
        apply_asinh: String(cols[7] || "").toLowerCase() === "asinh",
        log_offset: safeNum(cols[8], 1),
        encoded_width: Number(cols[11] || 0),
        encoded_height: Number(cols[12] || 0),
        crop_width: Number(cols[13] || 0),
        crop_height: Number(cols[14] || 0)
      });
    } else if (cols[0] === "RECORD" && cols.length >= 19) {
      records.push({
        stream_index: Number(cols[2]),
        frame_index: Number(cols[3]),
        frame_min: safeNum(cols[15], NaN),
        frame_max: safeNum(cols[16], NaN),
        bitmapPresent: cols[17] === "1"
      });
    }
  }
  return { streams, records };
}

function collectMessage(msg) {
  if (!msg || typeof msg !== "object") return "";
  return msg.message || msg.error || String(msg);
}

export class AvencModel extends EventTarget {
  constructor(datasets = DEFAULT_DATASETS) {
    super();
    this.datasets = datasets;
    this.arrayBuffer = null;
    this.metadata = null;
    this.recon = null;
    this.groups = [];
    this.currentDataset = null;
    this.currentGroup = null;
    this.videoTracks = [];
    this.decodedFrames = new Map();
    this.decodedStreamIndex = -1;
    this._mp4File = null;
    this._samples = [];
    this._displaySamples = [];
    this._decodeRequest = null;
    this._loadController = null;
    this.fileSizeBytes = 0;
    this.latCoord = { values: [], shape: [], units: "degrees_north" };
    this.lonCoord = { values: [], shape: [], units: "degrees_east" };
    this._rgbaFallbackCanvas = null;
    this._rgbaFallbackCtx = null;
  }

  _emitStatus(message) {
    this.dispatchEvent(new CustomEvent("status", { detail: { message } }));
  }

  _emitProgress(step, value, total, label = "") {
    this.dispatchEvent(new CustomEvent("progress", { detail: { step, value, total, label } }));
  }

  _emitLoaded() {
    this.dispatchEvent(new CustomEvent("loaded"));
  }

  async loadDatasetByUrl(url) {
    this.close();
    const loadController = new AbortController();
    this._loadController = loadController;
    this.currentDataset = url;
    this._emitStatus(`Fetching MP4: ${url}`);

    let response;
    try {
      response = await fetch(url, { mode: "cors", cache: "no-cache", signal: loadController.signal });
    } catch (err) {
      throw new Error(`Fetch failed (CORS/network). ${collectMessage(err)}`);
    }
    if (!response.ok) {
      throw new Error(`HTTP ${response.status} while fetching ${url}`);
    }

    const buffer = await response.arrayBuffer();
    loadController.signal.throwIfAborted();
    this.arrayBuffer = buffer;
    this.fileSizeBytes = this.arrayBuffer.byteLength;
    this._emitStatus("Parsing MP4 structure...");
    const info = await this._parseMp4Info(this.arrayBuffer);
    loadController.signal.throwIfAborted();
    this.videoTracks = (info.videoTracks || []).slice().sort((a, b) => Number(a.id) - Number(b.id));
    if (!this.videoTracks.length) throw new Error("No video tracks found in MP4");

    this._emitStatus("Extracting embedded metadata...");
    this.metadata = this._extractAvencMetadata(new Uint8Array(this.arrayBuffer));
    this.recon = this._extractReconPayload(this.metadata);
    this.groups = Array.isArray(this.metadata.groups) ? this.metadata.groups.slice() : [];
    if (!this.groups.length) {
      throw new Error("Metadata has no groups; AVENC packing metadata missing or incompatible.");
    }

    this._attachRecordsFallback();
    this._decodeCoords();
    await this.selectGroup(this.groups[0].id ?? 0);
    this._emitLoaded();
  }

  _clearDecodedFrames() {
    this._decodeRequest?.controller.abort();
    this._decodeRequest = null;
    for (const d of this.decodedFrames.values()) d.frame.close();
    this.decodedFrames.clear();
  }

  close() {
    this._loadController?.abort();
    this._clearDecodedFrames();
    this.decodedStreamIndex = -1;
    this.currentGroup = null;
    this._mp4File = null;
    this._samples = [];
    this._displaySamples = [];
    this.arrayBuffer = null;
    this.metadata = null;
    this.recon = null;
    this.groups = [];
    this.videoTracks = [];
    this.fileSizeBytes = 0;
    this.latCoord = { values: [], shape: [], units: "degrees_north" };
    this.lonCoord = { values: [], shape: [], units: "degrees_east" };
    this._rgbaFallbackCanvas = null;
    this._rgbaFallbackCtx = null;
  }

  async selectGroup(groupId) {
    const selected = this.groups.find((g) => Number(g.id) === Number(groupId)) || this.groups[0];
    if (!selected) throw new Error("No group found in metadata");
    this.currentGroup = selected;
    const streamIndex = Number(selected.stream_index ?? 0);
    if (this.decodedStreamIndex !== streamIndex) {
      this._clearDecodedFrames();
      const track = this.videoTracks[streamIndex];
      if (!track) throw new Error(`Missing video stream ${streamIndex}`);
      this._samples = this._mp4File.getTrackById(track.id).samples;
      // Metadata frame indices follow presentation order, including B-frames.
      this._displaySamples = this._samples.slice().sort((a, b) => a.cts - b.cts || a.number - b.number);
      this.decodedStreamIndex = streamIndex;
      this._emitStatus(`Ready: stream ${streamIndex}, frames decoded on demand (2-frame cache).`);
    }
  }

  _attachRecordsFallback() {
    for (const g of this.groups) {
      if (Array.isArray(g.records) && g.records.length > 0) continue;
      const streamIndex = Number(g.stream_index ?? 0);
      const fallback = this.recon.records
        .filter((r) => Number(r.stream_index) === streamIndex)
        .map((r) => ({
          frame_index: Number(r.frame_index),
          quantization_min: safeNum(r.frame_min, safeNum(g.value_min, 0)),
          quantization_max: safeNum(r.frame_max, safeNum(g.value_max, 1)),
          bitmapPresent: Boolean(r.bitmapPresent)
        }));
      g.records = fallback;
    }
  }

  _decodeCoords() {
    const latObj = this.metadata?.coords?.lat || {};
    const lonObj = this.metadata?.coords?.lon || {};
    const latDecoded = decodeCoordValues(latObj);
    const lonDecoded = decodeCoordValues(lonObj);
    this.latCoord = {
      values: latDecoded.values,
      shape: latDecoded.shape,
      units: String(latObj?.attrs?.units || "degrees_north")
    };
    this.lonCoord = {
      values: lonDecoded.values,
      shape: lonDecoded.shape,
      units: String(lonObj?.attrs?.units || "degrees_east")
    };
  }

  getGroupList() {
    return this.groups.map((g) => ({
      id: Number(g.id),
      label: `${g.short_name || "var"} / ${g.type_of_level || "level"} / stream ${g.stream_index} / frames ${g.frame_count}`,
      stream_index: Number(g.stream_index),
      frame_count: Number(g.frame_count || 0)
    }));
  }

  getCurrentGroupInfo() {
    if (!this.currentGroup) return null;
    const g = this.currentGroup;
    const opts = g.options || {};
    const cropW = Number(opts.crop_width || this.metadata.width || this.metadata.shape?.[2] || 0);
    const cropH = Number(opts.crop_height || this.metadata.height || this.metadata.shape?.[1] || 0);
    return {
      id: Number(g.id),
      streamIndex: Number(g.stream_index),
      frameCount: Number(g.frame_count || 0),
      shortName: g.short_name || "",
      typeOfLevel: g.type_of_level || "",
      level: g.level ?? "",
      bits: Number(opts.bits || 8),
      quantMode: String(opts.quantization || "frameMinMax"),
      cropWidth: cropW,
      cropHeight: cropH,
      encodedWidth: Number(opts.encoded_width || cropW),
      encodedHeight: Number(opts.encoded_height || cropH),
      valueMin: safeNum(g.value_min, safeNum(opts.min_value, 0)),
      valueMax: safeNum(g.value_max, safeNum(opts.max_value, 1)),
      unit: String(
        g.unit ||
        g.units ||
        g.attrs?.units ||
        opts.unit ||
        opts.units ||
        ""
      )
    };
  }

  getCompressionStats() {
    const gi = this.getCurrentGroupInfo();
    if (!gi) return null;
    const totalFrames = Number(this.metadata?.frames || this.metadata?.shape?.[0] || 0);
    const fullW = Number(this.metadata?.width || this.metadata?.shape?.[2] || gi.cropWidth || 0);
    const fullH = Number(this.metadata?.height || this.metadata?.shape?.[1] || gi.cropHeight || 0);
    const rawFull = Math.max(0, totalFrames) * Math.max(0, fullW) * Math.max(0, fullH) * 4;
    const rawGroup = Math.max(0, gi.frameCount) * Math.max(0, gi.cropWidth) * Math.max(0, gi.cropHeight) * 4;
    const mp4 = Math.max(0, this.fileSizeBytes || 0);
    const ratio = mp4 > 0 ? rawFull / mp4 : 0;
    return {
      fileSizeBytes: mp4,
      rawFullFloat32Bytes: rawFull,
      rawGroupFloat32Bytes: rawGroup,
      compressionRatioFull: ratio
    };
  }

  getLatLonAt(i, j) {
    const latVals = this.latCoord.values || [];
    const lonVals = this.lonCoord.values || [];
    if (!latVals.length || !lonVals.length) return null;
    const gi = this.getCurrentGroupInfo();
    if (!gi) return null;
    const nx = Math.max(1, gi.cropWidth);
    const ny = Math.max(1, gi.cropHeight);
    const ii = Math.max(0, Math.min(nx - 1, Number(i)));
    const jj = Math.max(0, Math.min(ny - 1, Number(j)));

    let lat = NaN;
    let lon = NaN;

    if (this.latCoord.shape.length === 1 && this.lonCoord.shape.length === 1) {
      lat = latVals[Math.min(latVals.length - 1, jj)];
      lon = lonVals[Math.min(lonVals.length - 1, ii)];
    } else if (this.latCoord.shape.length >= 2 && this.lonCoord.shape.length >= 2) {
      const idx = jj * nx + ii;
      lat = latVals[Math.min(latVals.length - 1, idx)];
      lon = lonVals[Math.min(lonVals.length - 1, idx)];
    } else if (this.latCoord.shape.length >= 2) {
      const idx = jj * nx + ii;
      lat = latVals[Math.min(latVals.length - 1, idx)];
      lon = lonVals[Math.min(lonVals.length - 1, ii)];
    } else if (this.lonCoord.shape.length >= 2) {
      const idx = jj * nx + ii;
      lat = latVals[Math.min(latVals.length - 1, jj)];
      lon = lonVals[Math.min(lonVals.length - 1, idx)];
    }

    return {
      lat: Number(lat),
      lon: Number(lon),
      latUnits: this.latCoord.units,
      lonUnits: this.lonCoord.units
    };
  }

  getCurrentFrameCount() {
    if (!this.currentGroup) return 0;
    const gCount = Number(this.currentGroup.frame_count || 0);
    const count = this._displaySamples.length;
    return gCount > 0 ? Math.min(gCount, count) : count;
  }

  _getFrameRecord(frameIndex) {
    if (!this.currentGroup) return null;
    const records = Array.isArray(this.currentGroup.records) ? this.currentGroup.records : [];
    return records.find((r) => Number(r.frame_index) === Number(frameIndex)) || null;
  }

  _getFrameQuantBounds(frameIndex) {
    const g = this.currentGroup;
    const opts = g?.options || {};
    const rec = this._getFrameRecord(frameIndex);
    const fallbackMin = safeNum(g?.value_min, safeNum(opts.min_value, 0));
    const fallbackMax = safeNum(g?.value_max, safeNum(opts.max_value, 1));
    const min = safeNum(rec?.quantization_min, fallbackMin);
    const max = safeNum(rec?.quantization_max, fallbackMax);
    if (max >= min) return { min, max };
    return { min: fallbackMin, max: fallbackMax >= fallbackMin ? fallbackMax : fallbackMin + 1e-12 };
  }

  getFrameTimestampLabel(frameIndex) {
    const rec = this._getFrameRecord(frameIndex);
    if (rec?.timestamp) return String(rec.timestamp);
    const sample = this._displaySamples[frameIndex];
    if (!sample) return "--";
    const ms = sample.cts * 1000 / sample.timescale;
    return `${ms.toFixed(0)} ms`;
  }

  getFrameIntervalMs() {
    if (this._displaySamples.length > 1) {
      const [a, b] = this._displaySamples;
      const dt = (b.cts / b.timescale - a.cts / a.timescale) * 1e6;
      if (dt > 0) return Math.max(40, dt / 1000);
    }
    return 1000;
  }

  getFrameStepHours() {
    if (!this.currentGroup) return 1;
    const records = Array.isArray(this.currentGroup.records) ? this.currentGroup.records : [];
    const ts = [];
    for (const r of records) {
      if (!r?.timestamp) continue;
      const t = Date.parse(String(r.timestamp));
      if (Number.isFinite(t)) ts.push(t);
      if (ts.length >= 2) break;
    }
    if (ts.length < 2) {
      const coords = this.currentGroup.coords || {};
      const tKey = Object.keys(coords).find((k) => k.startsWith("time_"));
      const values = coords?.[tKey]?.data?.values;
      if (Array.isArray(values) && values.length >= 2) {
        const t0 = Date.parse(String(values[0]));
        const t1 = Date.parse(String(values[1]));
        if (Number.isFinite(t0) && Number.isFinite(t1)) {
          const h = Math.abs(t1 - t0) / 3600000;
          if (h > 0) return h;
        }
      }
      return 1;
    }
    const h = Math.abs(ts[1] - ts[0]) / 3600000;
    return h > 0 ? h : 1;
  }

  getDisplayRange(scaleMode, frameIndex) {
    const g = this.currentGroup;
    const opts = g?.options || {};
    const groupMin = safeNum(g?.value_min, safeNum(opts.min_value, 0));
    const groupMax = safeNum(g?.value_max, safeNum(opts.max_value, 1));
    if (scaleMode === "group") return { min: groupMin, max: groupMax };
    return this._getFrameQuantBounds(frameIndex);
  }

  async getFrameVisual(frameIndex, scaleMode = "frame") {
    if (!this.currentGroup) throw new Error("No group selected");
    if (frameIndex < 0 || frameIndex >= this.getCurrentFrameCount()) {
      throw new Error(`Frame ${frameIndex} out of range`);
    }
    const group = this.currentGroup;
    const frameObj = await this._ensureDecodedFrame(frameIndex);
    if (this.currentGroup !== group) throw new DOMException("Group changed", "AbortError");
    const opts = group.options || {};
    const quant = this._getFrameQuantBounds(frameIndex);
    const disp = this.getDisplayRange(scaleMode, frameIndex);
    const cropW = Number(opts.crop_width || frameObj.frame.codedWidth);
    const cropH = Number(opts.crop_height || frameObj.frame.codedHeight);
    const encW = Number(opts.encoded_width || frameObj.frame.codedWidth);
    const encH = Number(opts.encoded_height || frameObj.frame.codedHeight);
    return {
      frame: frameObj.frame,
      timestampUs: frameObj.timestampUs,
      frameIndex,
      quantMin: quant.min,
      quantMax: quant.max,
      displayMin: disp.min,
      displayMax: disp.max,
      cropWidth: cropW,
      cropHeight: cropH,
      encodedWidth: encW,
      encodedHeight: encH
    };
  }

  async getHoverValue(frameIndex, i, j) {
    if (!this.currentGroup) return null;
    const group = this.currentGroup;
    const opts = group.options || {};
    const bitsTarget = Number(opts.bits || 8);
    let sampleBits = 8;
    let qYRaw = 0;
    let qURaw = 128;
    let qVRaw = 128;
    let decodeSource = "plane";
    try {
      const plane = await this._ensurePlaneData(frameIndex);
      sampleBits = Number(plane.bits || 8);
      qYRaw = this._readPlaneSample(plane.y, i, j, plane.yStride, sampleBits);
      qURaw = this._readPlaneSample(
        plane.u,
        Math.floor(i / 2),
        Math.floor(j / 2),
        plane.uStride,
        sampleBits
      );
      qVRaw = this._readPlaneSample(
        plane.v,
        Math.floor(i / 2),
        Math.floor(j / 2),
        plane.vStride,
        sampleBits
      );
    } catch (err) {
      if (err?.name === "AbortError") throw err;
      const rgba = await this._ensureRgbaData(frameIndex);
      const x = Math.max(0, Math.min(rgba.width - 1, Number(i)));
      const y = Math.max(0, Math.min(rgba.height - 1, Number(j)));
      const off = y * rgba.stride + x * 4;
      const r = rgba.data[off];
      const g = rgba.data[off + 1];
      const b = rgba.data[off + 2];
      qYRaw = Math.round((r + g + b) / 3);
      qURaw = g;
      qVRaw = b;
      sampleBits = 8;
      decodeSource = "rgba";
    }

    let qY = qYRaw;
    if (sampleBits < bitsTarget) qY <<= (bitsTarget - sampleBits);
    else if (sampleBits > bitsTarget) qY >>= (sampleBits - bitsTarget);

    const rec = this._getFrameRecord(frameIndex);
    const bounds = this._getFrameQuantBounds(frameIndex);
    const log2Mode = Boolean(opts.apply_log2);
    const logOffset = safeNum(opts.log_offset, 1);
    const storageShift = Number(opts.storage_shift || 0);
    const hasBitmap = Boolean(rec?.bitmapPresent);
    let q = qY;
    if (storageShift > 0) q = q >> storageShift;
    const qmax = (1 << Math.min(30, bitsTarget)) - 1;
    const qvalid = hasBitmap ? Math.max(1, qmax - 1) : Math.max(1, qmax);
    if (hasBitmap) q = Math.max(0, q - 1);
    const sampleMax = Math.max(1, (1 << Math.min(30, sampleBits)) - 1);
    const luma01 = Math.max(0, Math.min(1, qYRaw / sampleMax));
    const norm = luma01;
    let value = bounds.min + norm * (bounds.max - bounds.min);
    if(opts.apply_shifted_log){
      const alpha=Number(opts.transform_scale??1);
      value=Number(opts.transform_origin??0)+Number(opts.transform_span??1)*Math.expm1(value*Math.log1p(alpha))/alpha;
    } else if (opts.apply_asinh) value = Number(opts.transform_scale??1)*Math.sinh(value);
    else if (log2Mode) value = Math.pow(2, value) - logOffset;
    const ll = this.getLatLonAt(i, j);
    const gi = this.getCurrentGroupInfo();
    const unit = gi?.unit || "";

    return {
      i,
      j,
      y: qYRaw,
      u: qURaw,
      v: qVRaw,
      luma01,
      quantizedY: qY,
      value,
      unit,
      source: decodeSource,
      lat: ll?.lat,
      lon: ll?.lon,
      latUnits: ll?.latUnits || "degrees_north",
      lonUnits: ll?.lonUnits || "degrees_east",
      frameIndex,
      timestamp: this.getFrameTimestampLabel(frameIndex)
    };
  }

  _readPlaneSample(bytes, x, y, stride, bits) {
    const offset = y * stride + x * (bits > 8 ? 2 : 1);
    if (bits > 8) return bytes[offset] | (bytes[offset + 1] << 8);
    return bytes[offset];
  }

  async _ensurePlaneData(frameIndex) {
    const fr = await this._ensureDecodedFrame(frameIndex);
    if (fr.planeData) return fr.planeData;
    const targetBits = Number(this.currentGroup?.options?.bits || 8);
    const preferred = targetBits > 8 ? "I420P10" : "I420";
    if (!fr.planePromise) {
      // Keep this copy alive independently of cache eviction during a seek.
      const frame = fr.frame.clone();
      fr.planePromise = this._copyPlanes(frame, preferred)
        .then(data => (fr.planeData = data))
        .finally(() => { frame.close(); fr.planePromise = null; });
    }
    return fr.planePromise;
  }

  async _ensureRgbaData(frameIndex) {
    const fr = await this._ensureDecodedFrame(frameIndex);
    if (fr.rgbaData) return fr.rgbaData;
    if (!fr.rgbaPromise) {
      const frame = fr.frame.clone();
      fr.rgbaPromise = this._copyRgbaFrame(frame)
        .then(data => (fr.rgbaData = data))
        .finally(() => { frame.close(); fr.rgbaPromise = null; });
    }
    return fr.rgbaPromise;
  }

  async _copyRgbaFrame(frame) {
    const width = Number(frame.displayWidth || frame.codedWidth);
    const height = Number(frame.displayHeight || frame.codedHeight);

    try {
      const size = frame.allocationSize({ format: "RGBA" });
      const data = new Uint8Array(size);
      const layout = await frame.copyTo(data, { format: "RGBA" });
      const rgbaData = {
        data,
        stride: Number(layout?.[0]?.stride || (width * 4)),
        width,
        height
      };
      return rgbaData;
    } catch (copyErr) {
      try {
        let ctx = null;
        if (typeof OffscreenCanvas !== "undefined") {
          const osc = new OffscreenCanvas(width, height);
          ctx = osc.getContext("2d", { willReadFrequently: true, alpha: false });
        } else if (typeof document !== "undefined") {
          if (!this._rgbaFallbackCanvas) {
            this._rgbaFallbackCanvas = document.createElement("canvas");
          }
          this._rgbaFallbackCanvas.width = width;
          this._rgbaFallbackCanvas.height = height;
          this._rgbaFallbackCtx = this._rgbaFallbackCanvas.getContext("2d", {
            willReadFrequently: true,
            alpha: false
          });
          ctx = this._rgbaFallbackCtx;
        }
        if (!ctx) {
          throw new Error("No 2D canvas context available for fallback");
        }
        ctx.clearRect(0, 0, width, height);
        ctx.drawImage(frame, 0, 0, width, height);
        const img = ctx.getImageData(0, 0, width, height);
        const rgbaData = {
          data: img.data,
          stride: width * 4,
          width,
          height
        };
        return rgbaData;
      } catch (canvasErr) {
        throw new Error(
          `Unable to access decoded pixel values for hover reconstruction. `
          + `copyTo(RGBA) failed: ${collectMessage(copyErr)}; `
          + `canvas fallback failed: ${collectMessage(canvasErr)}`
        );
      }
    }
  }

  _withDecoderHint(prefix, codec, rawErr) {
    const msg = `${prefix} for codec=${codec}: ${collectMessage(rawErr)}`;
    return new Error(
      `${msg}\n`
      + `This browser/GPU likely does not support this codec/profile/bit-depth combination `
      + `or dynamic stream constraints. Try latest Google Chrome, or another MP4 profile `
      + `(AV1/VP9/H.264, preferably 8/10-bit).`
    );
  }

  async _copyPlanes(frame, preferredFormat) {
    const attempt = async (format) => {
      const size = frame.allocationSize({ format });
      const data = new Uint8Array(size);
      const layout = await frame.copyTo(data, { format });
      const bits = format === "I420P10" ? 16 : 8;
      const yOffset = Number(layout[0].offset);
      const yStride = Number(layout[0].stride);
      const uOffset = Number(layout[1].offset);
      const uStride = Number(layout[1].stride);
      const vOffset = Number(layout[2].offset);
      const vStride = Number(layout[2].stride);
      return {
        bits,
        yStride,
        uStride,
        vStride,
        y: data.subarray(yOffset, uOffset),
        u: data.subarray(uOffset, vOffset),
        v: data.subarray(vOffset)
      };
    };

    try {
      return await attempt(preferredFormat);
    } catch {
      return attempt("I420");
    }
  }

  _extractAvencMetadata(bytes) {
    for (const prefix of AVENC_PREFIXES) {
      const marker = findMarkerRange(bytes, prefix);
      if (!marker) continue;
      try {
        const payload = new TextDecoder().decode(bytes.subarray(marker.dataStart, marker.end));
        return decodeAvencCommentPayload(prefix, payload);
      } catch {
        // Continue with fallback probes.
      }
    }

    const markerUuh = findMarkerRange(bytes, "UUH64|") || findMarkerRange(bytes, "AVENC_UUH64|");
    if (markerUuh) {
      const raw = new TextDecoder().decode(bytes.subarray(markerUuh.start, markerUuh.end));
      const decoded = inflateMaybe(decodeUuH64ToBytes(raw));
      return JSON.parse(bytesToText(decoded));
    }

    const text = new TextDecoder("utf-8", { fatal: false }).decode(bytes);
    for (const prefix of AVENC_PREFIXES) {
      const esc = prefix.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
      const re = new RegExp(`${esc}([A-Za-z0-9+/_=\\-.]+)`);
      const mAvenc = text.match(re);
      if (!mAvenc) continue;
      try {
        return decodeAvencCommentPayload(prefix, mAvenc[1]);
      } catch {
        // Continue with fallback probes.
      }
    }

    const mUuh = text.match(/(?:UUH64|AVENC_UUH64)\|([A-Za-z0-9+/_=\-.]+)/i);
    if (mUuh) {
      const decoded = inflateMaybe(decodeUuH64ToBytes(mUuh[1]));
      return JSON.parse(bytesToText(decoded));
    }
    throw new Error("Unable to locate AVENC metadata payload in MP4.");
  }

  _extractReconPayload(metadata) {
    const reconB64 = metadata?.reconstruct_payload;
    if (!reconB64) return { streams: new Map(), records: [] };
    const format = String(metadata?.reconstruct_format || "");
    if (!format.includes("zlib.base64")) return { streams: new Map(), records: [] };
    const reconZip = b64ToBytes(reconB64);
    const reconText = bytesToText(inflateMaybe(reconZip));
    return parseReconTsv(reconText);
  }

  async _parseMp4Info(buffer) {
    if (!window.MP4Box || !window.MP4Box.createFile) {
      throw new Error("mp4box.js is not loaded.");
    }
    return new Promise((resolve, reject) => {
      const file = window.MP4Box.createFile();
      file.onError = (e) => reject(new Error(`mp4box parse error: ${collectMessage(e)}`));
      file.onReady = (info) => resolve(info);
      this._mp4File = file;
      buffer.fileStart = 0;
      file.appendBuffer(buffer);
      file.flush();
    });
  }

  _getDecoderDescription(mp4boxFile, trackId) {
    try {
      const DS = window.DataStream || window.MP4Box?.DataStream;
      if (!mp4boxFile.getTrackById || !DS) return undefined;
      const track = mp4boxFile.getTrackById(trackId);
      const entry = track?.mdia?.minf?.stbl?.stsd?.entries?.[0];
      if (!entry) return undefined;
      const cfgBox = entry.avcC || entry.hvcC || entry.vpcC || entry.av1C;
      if (!cfgBox) return undefined;
      const stream = new DS(undefined, 0, DS.BIG_ENDIAN);
      cfgBox.write(stream);
      return new Uint8Array(stream.buffer, 8);
    } catch {
      return undefined;
    }
  }

  _codecFamily(codec) {
    const c = String(codec || "").toLowerCase();
    if (c.startsWith("vp09") || c.startsWith("vp9")) return "vp9";
    if (c.startsWith("av01") || c.startsWith("av1")) return "av1";
    if (c.startsWith("hev1") || c.startsWith("hvc1") || c.startsWith("hevc") || c.startsWith("h265")) return "hevc";
    if (c.startsWith("avc1") || c.startsWith("avc3") || c.startsWith("h264") || c.startsWith("avc")) return "avc";
    return "other";
  }

  _bitReadMsb(bytes, bitOffset, bitCount) {
    let out = 0;
    for (let i = 0; i < bitCount; i += 1) {
      const b = bitOffset + i;
      const byteIndex = b >> 3;
      if (byteIndex >= bytes.length) return null;
      const bitInByte = 7 - (b & 7);
      const bit = (bytes[byteIndex] >> bitInByte) & 1;
      out = (out << 1) | bit;
    }
    return out;
  }

  _vp9SampleLooksKey(sampleData) {
    if (!(sampleData instanceof Uint8Array) || sampleData.length < 2) return null;
    let p = 0;

    const frameMarker = this._bitReadMsb(sampleData, p, 2);
    if (frameMarker === null || frameMarker !== 2) return null;
    p += 2;

    const profileLow = this._bitReadMsb(sampleData, p, 1);
    if (profileLow === null) return null;
    p += 1;
    const profileHigh = this._bitReadMsb(sampleData, p, 1);
    if (profileHigh === null) return null;
    p += 1;
    let profile = profileLow | (profileHigh << 1);
    if (profile === 3) {
      const reserved = this._bitReadMsb(sampleData, p, 1);
      if (reserved === null) return null;
      p += 1;
      if (reserved === 1) profile = 3;
    }

    const showExisting = this._bitReadMsb(sampleData, p, 1);
    if (showExisting === null) return null;
    p += 1;
    if (showExisting === 1) return false;

    const frameType = this._bitReadMsb(sampleData, p, 1);
    if (frameType === null) return null;
    return frameType === 0;
  }

  _sampleLooksKey(codec, sample) {
    const family = this._codecFamily(codec);
    const syncFlag = Boolean(
      sample?.is_sync === true ||
      sample?.is_rap === true ||
      sample?.is_rap_sample === true
    );

    if (family === "vp9") {
      const parsed = this._vp9SampleLooksKey(sample?.data);
      if (parsed === true) return true;
      if (parsed === false) return false;
      return syncFlag;
    }

    return syncFlag;
  }

  _isRecoverableChunkTypeError(errMsg, attemptedType) {
    const m = String(errMsg || "").toLowerCase();
    if (attemptedType === "key" && m.includes("wasn't a key frame")) return true;
    if (attemptedType === "key" && m.includes("was not a key frame")) return true;
    if (attemptedType === "delta" && m.includes("a key frame is required")) return true;
    return false;
  }

  _makeChunkFromSample(sample, forcedType = null) {
    return new EncodedVideoChunk({
      type: forcedType || (sample?.is_sync ? "key" : "delta"),
      timestamp: Math.round((Number(sample?.cts ?? sample?.dts ?? 0) * 1e6) / Number(sample?.timescale || 1)),
      duration: Math.max(1, Math.round((Number(sample?.duration || 1) * 1e6) / Number(sample?.timescale || 1))),
      data: sample.data
    });
  }

  _decodeSampleWithType(decoder, sample, chunkType) {
    try {
      const chunk = this._makeChunkFromSample(sample, chunkType);
      decoder.decode(chunk);
      return true;
    } catch (err) {
      const msg = collectMessage(err);
      if (this._isRecoverableChunkTypeError(msg, chunkType)) return false;
      throw err;
    }
  }

  async _ensureDecodedFrame(frameIndex) {
    if (!Number.isInteger(frameIndex) || frameIndex < 0 || frameIndex >= this.getCurrentFrameCount()) {
      throw new Error(`Frame ${frameIndex} out of range`);
    }
    if (this._decodeRequest && this._decodeRequest.frameIndex !== frameIndex) {
      this._decodeRequest.controller.abort();
      this._decodeRequest = null;
    }
    const cached = this.decodedFrames.get(frameIndex);
    if (cached) {
      this.decodedFrames.delete(frameIndex);
      this.decodedFrames.set(frameIndex, cached);
      return cached;
    }
    if (this._decodeRequest?.frameIndex === frameIndex) return this._decodeRequest.promise;
    this._decodeRequest?.controller.abort();
    const controller = new AbortController();
    const request = { frameIndex, controller, promise: null };
    this._decodeRequest = request;
    request.promise = this._decodeFrame(frameIndex, controller.signal).then(frame => {
      if (controller.signal.aborted) {
        frame.frame.close();
        controller.signal.throwIfAborted();
      }
      // The cache owns all retained VideoFrames. Eviction releases GPU surfaces
      // and drops any CPU plane/RGBA copies along with the entry.
      while (this.decodedFrames.size >= 2) {
        const oldest = this.decodedFrames.keys().next().value;
        this.decodedFrames.get(oldest).frame.close();
        this.decodedFrames.delete(oldest);
      }
      this.decodedFrames.set(frameIndex, frame);
      return frame;
    }).finally(() => {
      if (this._decodeRequest === request) this._decodeRequest = null;
    });
    return request.promise;
  }

  async _decodeFrame(frameIndex, signal) {
    if (!window.VideoDecoder) throw new Error("WebCodecs VideoDecoder is not available in this browser.");
    const track = this.videoTracks[this.decodedStreamIndex];
    const samples = this._samples;
    const target = this._displaySamples[frameIndex];
    const codec = String(track.codec || "");
    const sampleData = sample => ({
      ...sample,
      data: new Uint8Array(this.arrayBuffer, sample.offset, sample.size)
    });
    let first = target.number;
    while (first > 0 && !this._sampleLooksKey(codec, sampleData(samples[first]))) first -= 1;
    // Feed through this GOP if necessary to drain presentation reordering.
    let end = target.number + 1;
    while (end < samples.length && !this._sampleLooksKey(codec, sampleData(samples[end]))) end += 1;
    const timestamp = Math.round(target.cts * 1e6 / target.timescale);
    const config = {
      codec,
      codedWidth: Number(track.video?.width || this.currentGroup.options.encoded_width),
      codedHeight: Number(track.video?.height || this.currentGroup.options.encoded_height),
      hardwareAcceleration: this.decodeHardwareAcceleration || "prefer-hardware"
    };
    const description = this._getDecoderDescription(this._mp4File, track.id);
    if (description) config.description = description;
    let retained = null;
    let decoderError = null;
    let wake = null;
    let submitted = 0;
    let outputCount = 0;
    const decoder = new VideoDecoder({
      output: frame => {
        outputCount += 1;
        if (!signal.aborted && frame.timestamp === timestamp && !retained) retained = frame;
        else frame.close();
        wake?.();
      },
      error: err => { decoderError = err; wake?.(); }
    });
    const abort = () => {
      if (decoder.state !== "closed") decoder.close();
      wake?.();
    };
    const check = () => {
      signal.throwIfAborted();
      if (decoderError) throw this._withDecoderHint("VideoDecoder error", codec, decoderError);
    };
    const onDequeue = () => wake?.();
    signal.addEventListener("abort", abort, { once: true });
    decoder.addEventListener("dequeue", onDequeue);
    try {
      check();
      decoder.configure(config);
      this._emitStatus(`Decoding frame ${frameIndex + 1}/${this.getCurrentFrameCount()}...`);
      for (let i = first; i < end && !retained; i += 1) {
        check();
        // Bound queued compressed input independently of total stream length.
        while (decoder.decodeQueueSize >= 4 || submitted - outputCount >= 16) {
          await new Promise((resolve, reject) => {
            const timer = setTimeout(() => reject(new Error("Decoder stalled waiting for output")), 30000);
            wake = () => { clearTimeout(timer); resolve(); };
          });
          wake = null;
          check();
        }
        submitted += 1;
        decoder.decode(this._makeChunkFromSample(sampleData(samples[i]), i === first ? "key" : "delta"));
      }
      await decoder.flush();
      check();
      if (!retained) throw new Error(`Decoder did not output frame ${frameIndex}`);
      const result = {
        frame: retained, timestampUs: retained.timestamp,
        durationUs: retained.duration || 0, planeData: null, rgbaData: null
      };
      retained = null;
      this._emitStatus(`Decoded frame ${frameIndex + 1}/${this.getCurrentFrameCount()} (on demand).`);
      return result;
    } finally {
      signal.removeEventListener("abort", abort);
      decoder.removeEventListener("dequeue", onDequeue);
      if (decoder.state !== "closed") decoder.close();
      retained?.close();
    }
  }
}
