#!/usr/bin/env python3
"""Dependency-light .fftile writer/inspector (NumPy and Pillow only).

Arrays have row zero at SOUTH. Bounds describe outer pixel edges, not centers.
Continuous values are never clipped to improve reported errors. An encoding
that cannot meet the requested RMSE budget is rejected, even at JPEG quality 100.
"""
from __future__ import annotations
import argparse
import io
import json
import math
from pathlib import Path
import struct
import time
import zlib

import numpy as np
from PIL import Image

HEADER = struct.Struct('<8s2h2H8d2f8H14f8IQI52s')
assert HEADER.size == 256
FIELDS = ('altitude', 'sx', 'sy', 'muu', 'muv', 'mvu', 'mvv')


def geometry(lat0, lon0):
    if not (-90 <= lat0 < 90 and -180 <= lon0 < 180):
        raise ValueError('tile origin must be inside the global geographic grid')
    if lat0 * 2 != round(lat0 * 2) or lon0 * 2 != round(lon0 * 2):
        raise ValueError('tile origin must be on the half-degree grid')
    core = (lon0, lat0, lon0 + .5, lat0 + .5)
    # Fuel needs a global pixel lattice: a 0.5-degree displacement must be
    # an integer number of pixels. 5538 core + 111 pixels per halo = 5760.
    # Longitude uses the same lattice at every latitude; otherwise adjacent
    # latitude bands with different widths disagree categorically in halos.
    step = .5 / 5538
    halo = 111 * step
    image = (lon0 - halo, max(-90., lat0 - halo), lon0 + .5 + halo, min(90., lat0 + .5 + halo))
    fuel_h = 5538 + (111 if lat0 > -90 else 0) + (111 if lat0 < 89.5 else 0)
    ww = max(16, round(576 * math.cos(math.radians(lat0 + .25)) / 16) * 16)
    return core, image, ((5760, fuel_h), (2*ww, 1152), (2*ww, 1152), (ww, 576))


def pixel_centers(bounds, width, height):
    w, s, e, n = bounds
    return w + (np.arange(width)+.5)*(e-w)/width, s + (np.arange(height)+.5)*(n-s)/height


def _image(array, codec, **options):
    out = io.BytesIO()
    Image.fromarray(np.asarray(array, dtype=np.uint8)).save(out, codec, **options)
    return out.getvalue()


def _decode(payload):
    with Image.open(io.BytesIO(payload)) as im:
        if im.mode not in ('L','I;16','I;16B','I'):
            raise ValueError('expected a grayscale image')
        return np.array(im)


def write_tile(path, lat0, lon0, fuel, altitude, sx, sy, muu, muv, mvu, mvv,
               *, quality=92, timestamp=None, enforce_accuracy=True):
    """Write arrays at their supplied resolutions; return measured codec errors.

    Supports smaller grids for tests/previews. Production generators use geometry().
    The atlas components must share shape. File size has no acceptance limit.
    """
    if not isinstance(quality,int) or not 1 <= quality <= 100:
        raise ValueError("JPEG quality must be an integer in 1..100")
    core, bounds, _ = geometry(lat0, lon0)
    fuel = np.asarray(fuel)
    if fuel.ndim != 2 or not np.isfinite(fuel).all() or np.any(fuel != np.floor(fuel)) or np.any((fuel < 0) | (fuel > 255)):
        raise ValueError('fuel must be a 2D grid of integer IDs in 0..255')
    arrays = [np.asarray(a, dtype=np.float32) for a in (altitude,sx,sy,muu,muv,mvu,mvv)]
    if any(a.ndim != 2 or not np.isfinite(a).all() for a in arrays):
        raise ValueError('continuous grids must be finite and two dimensional')
    if arrays[1].shape != arrays[2].shape or any(a.shape != arrays[3].shape for a in arrays[4:]):
        raise ValueError('atlas components must have the same shape')
    dims = [(fuel.shape[1],fuel.shape[0])] + [(arrays[i].shape[1],arrays[i].shape[0]) for i in (0,1,3)]
    if any(not 0 < d <= 8192 for pair in dims for d in pair):
        raise ValueError('dimensions must be in 1..8192')
    ranges, quantized = [], []
    for a in arrays:
        lo, hi = float(a.min()), float(a.max())
        # A zero-width range is valid: decode as the exact constant.
        ranges.extend((lo,hi))
        quantized.append(np.zeros(a.shape,dtype=np.uint8) if lo == hi else
                         np.rint((a.astype(np.float64)-lo)/(hi-lo)*255).clip(0,255).astype(np.uint8))
    budgets = [max(min((ranges[1]-ranges[0])*.005,.2),1e-6),.01,.01,.015,.015,.015,.015]
    groups = [(0,), (1,2), (3,4,5,6)]
    payloads = [_image(fuel,'PNG',optimize=True)]
    report = {}
    flags = 1
    for group in groups:
        atlas = quantized[group[0]] if len(group)==1 else (
            np.hstack([quantized[i] for i in group]) if len(group)==2 else
            np.vstack([np.hstack([quantized[i] for i in group[:2]]),np.hstack([quantized[i] for i in group[2:]])]))
        for q in sorted(set([quality,95,98,100])):
            if q < quality:
                continue
            encoded = _image(atlas,'JPEG',quality=q,subsampling=0)
            decoded = _decode(encoded)
            errors = []
            for j,i in enumerate(group):
                h,w = arrays[i].shape
                tile = decoded[(j//2)*h:(j//2+1)*h,(j%2)*w:(j%2+1)*w]
                lo,hi = ranges[2*i:2*i+2]
                reconstructed = lo + tile.astype(np.float64)*(hi-lo)/255
                errors.append(float(np.sqrt(np.mean((reconstructed-arrays[i])**2))))
            if all(e <= budgets[i] for e,i in zip(errors,group)):
                break
        codec = 'JPEG8'
        if group == (0,) and any(e > budgets[i] for e,i in zip(errors,group)):
            lo,hi = ranges[:2]
            quantized16 = (np.rint((arrays[0].astype(np.float64)-lo)/(hi-lo)*65535)
                           .clip(0,65535).astype(np.uint16))
            buffer = io.BytesIO()
            Image.fromarray(quantized16).save(buffer,'PNG',optimize=True)
            encoded = buffer.getvalue()
            reconstructed = lo + _decode(encoded).astype(np.float64)*(hi-lo)/65535
            errors = [float(np.sqrt(np.mean((reconstructed-arrays[0])**2)))]
            flags |= 2
            codec = 'PNG16'
            q = None
        if enforce_accuracy and any(e > budgets[i] for e,i in zip(errors,group)):
            raise ValueError(f'accuracy budget exceeded: {dict(zip((FIELDS[i] for i in group),errors))}')
        payloads.append(encoded)
        for i,e in zip(group,errors):
            report[FIELDS[i]] = dict(rmse=e,limit=budgets[i],quality=q,codec=codec)
    offsets, end = [], HEADER.size
    for p in payloads:
        offsets.extend((end,len(p))); end += len(p)
    args = [b'FFTILE01',round((lat0+90)*2),round((lon0+180)*2),flags,11,
            *core,*bounds,111195.,111195.*math.cos(math.radians(lat0+.25)),
            *(d for pair in dims for d in pair),*ranges,*offsets,
            int(time.time() if timestamp is None else timestamp),0,bytes(52)]
    header = bytearray(HEADER.pack(*args))
    struct.pack_into('<I',header,200,zlib.crc32(header[:200]))
    path = Path(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    temp = path.with_name(path.name+'.tmp')
    try:
        temp.write_bytes(header+b''.join(payloads))
        temp.replace(path)
    finally:
        temp.unlink(missing_ok=True)
    return dict(bytes=end,accuracy=report)


def read_tile(path):
    data = Path(path).read_bytes()
    if len(data) < 256:
        raise ValueError('truncated header')
    h = HEADER.unpack_from(data)
    if h[0] != b'FFTILE01' or (h[3] & 1) == 0 or h[3] & ~3:
        raise ValueError('unsupported magic/flags')
    crc = struct.unpack_from('<I',data,200)[0]
    if crc and crc != zlib.crc32(data[:200]):
        raise ValueError('header CRC mismatch')
    dimensions = list(zip(h[15:23:2],h[16:23:2]))
    offsets = list(zip(h[37:45:2],h[38:45:2]))
    arrays = []
    end = 256
    for i,((w,height),(offset,size)) in enumerate(zip(dimensions,offsets)):
        if offset != end or not size or offset+size > len(data):
            raise ValueError('invalid payload bounds')
        a = _decode(data[offset:offset+size])
        expected = (height*(2 if i==3 else 1),w*(2 if i>=2 else 1))
        if (i == 1 and bool(h[3] & 2)) != (a.dtype == np.uint16):
            raise ValueError("image bit depth disagrees with header")
        if a.shape != expected:
            raise ValueError('image dimensions disagree with header')
        arrays.append(a)
        end = offset+size
    if end != len(data):
        raise ValueError('trailing bytes')
    meta = dict(magic=h[0].decode(),lat_idx=h[1],lon_idx=h[2],flags=h[3],
                core=h[5:9],image=h[9:13],dimensions=dimensions,
                halo_wind_px=h[4],m_per_deg_lat=h[13],m_per_deg_lon=h[14],header_crc32=h[46],reserved=h[47].hex(),
                ranges=dict(zip(FIELDS,zip(h[23:37:2],h[24:37:2]))),
                payloads=offsets,created=h[45],bytes=len(data))
    return meta,arrays


def decoded_fields(path):
    meta,images = read_tile(path)
    out = dict(fuel=images[0])
    components = [images[1],*np.hsplit(images[2],2),
                  *[a for row in np.vsplit(images[3],2) for a in np.hsplit(row,2)]]
    for name,a in zip(FIELDS,components):
        lo,hi = meta['ranges'][name]
        out[name] = lo+a.astype(np.float64)*(hi-lo)/(65535 if name == 'altitude' and meta['flags'] & 2 else 255)
    return meta,out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tile',type=Path)
    parser.add_argument('--inspect',action='store_true')
    parser.add_argument('--unpack',type=Path)
    args = parser.parse_args()
    meta,_ = read_tile(args.tile)
    print(json.dumps(meta,indent=2))
    if args.unpack:
        args.unpack.mkdir(parents=True,exist_ok=True)
        data = args.tile.read_bytes()
        for name,(offset,size) in zip(('fuel.png','altitude.png' if meta['flags'] & 2 else 'altitude.jpg','slope.jpg','wind.jpg'),meta['payloads']):
            (args.unpack/name).write_bytes(data[offset:offset+size])

if __name__ == '__main__':
    main()
