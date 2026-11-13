#!/usr/bin/env python3
"""Run this nested case with native FFTile providers in an isolated directory.

python3 tests/mnh_real_nested/run_tiles.py --mesonh-root /path/to/meso-nh-6-dev
python3 tests/mnh_real_nested/run_tiles.py --validate /path/to/completed/run
Uses Python's standard library; simulation runs in the MPI Meso-NH executable.
"""
import argparse
from array import array
from bisect import bisect_right
from collections import defaultdict
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[1]
INPUTS = ('RUN12.1.PRUN1.004.nc', 'RUN12.1.PRUN1.004.des',
          'M2.20170617.00.15.nc', 'M2.20170617.00.15.des',
          'PGD_D3200mA.nested.nc', 'PGD_D800mA.nested.nc')
PROVIDER = 'https://forefire.univ-corse.fr/fftiles/'


def ensure(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    with path.open('rb') as stream:
        checksum = hashlib.sha256()
        for block in iter(lambda: stream.read(1024*1024), b''):
            checksum.update(block)
        return checksum.hexdigest()


def front_frame(path):
    polygons = []
    for line in path.read_text().splitlines():
        if 'FireFront[' in line:
            polygons.append([])
        elif 'FireNode[' in line:
            match = re.search(r'loc=\(([^)]+)\)', line)
            ensure(match and polygons, f'Invalid node in {path}')
            point = tuple(float(v) for v in match[1].split(','))
            ensure(len(point) == 3 and all(map(math.isfinite, point)), f'Nonfinite front in {path}')
            polygons[-1].append(point[:2])
    area = 0.
    for polygon in polygons:
        if len(polygon) < 3:
            continue
        x0, y0 = polygon[0]
        area += abs(sum((x-x0)*(polygon[(i+1) % len(polygon)][1]-y0) -
                        (polygon[(i+1) % len(polygon)][0]-x0)*(y-y0)
                        for i, (x, y) in enumerate(polygon)))/2
    return {'time': float(path.name.rsplit('.', 1)[1]), 'area_m2': area,
            'fronts': len(polygons), 'nodes': sum(map(len, polygons))}


def binary_summary(path):
    # FFDumpDoubleArray: native-endian uint64 nx,ny,nz; double time; doubles.
    header = struct.Struct('=QQQd')
    low, high, frames, count = math.inf, -math.inf, 0, 0
    times = []
    with path.open('rb') as stream:
        while raw := stream.read(header.size):
            ensure(len(raw) == header.size, f'Partial field header: {path}')
            nx, ny, nz, stamp = header.unpack(raw)
            size = nx*ny*nz
            ensure(all(0 < v <= 100000 for v in (nx, ny, nz)) and size <= 64000000,
                   f'Invalid field dimensions: {path}')
            values = array('d')
            values.frombytes(stream.read(size*8))
            ensure(len(values) == size and math.isfinite(stamp), f'Partial field frame: {path}')
            ensure(all(map(math.isfinite, values)), f'Nonfinite field: {path}')
            low, high = min(low, min(values)), max(high, max(values))
            frames += 1
            count += size
            times.append(stamp)
    ensure(frames > 1 and times[-1] > times[0], f'Field did not update: {path}')
    return {'frames': frames, 'values_checked': count, 'min': low, 'max': high,
            'first_time': times[0], 'last_time': times[-1]}


def geography_check(case, first_front, meta):
    """Compare native ignition coordinates with Meso-NH's own mass-point map."""
    # Prefer the dumper built with this model's HDF5/NetCDF versions. A
    # Conda/system dumper may not understand its ZSTD filters or HDF version.
    root = Path(meta['mesonh_root']) if meta.get('mesonh_root') else None
    bundled = list(root.glob('src/dir_obj-*/MASTER/IO_LIBS/bin/ncdump')) if root else []
    dump = str(max(bundled, key=lambda p: p.stat().st_mtime)) if bundled else shutil.which('ncdump')
    if not dump:
        return {'checked': False, 'reason': 'ncdump is not on PATH'}
    env = os.environ.copy()
    plugins = Path(dump).parent.parent/'lib/hdf5/plugin'
    if plugins.is_dir():
        env['HDF5_PLUGIN_PATH'] = str(plugins)
    text = subprocess.check_output([dump, '-p', '17,17', '-v', 'ni,nj,LAT,LON',
                                   str(case/'RUN12.2.FIRE.001.nc')], text=True, env=env)
    data = text.split('data:', 1)[1]
    def values(name):
        match = re.search(r'(?m)^\s*'+name+r'\s*=([^;]+);', data)
        ensure(match, f'Missing coordinate field {name}')
        return [float(v.strip()) for v in match[1].split(',')]
    xs, ys, lat, lon = (values(name) for name in ('ni', 'nj', 'LAT', 'LON'))
    ensure(len(lat) == len(lon) == len(xs)*len(ys), 'Coordinate grid dimensions do not match')
    def sample(grid, x, y):
        i, j = bisect_right(xs, x)-1, bisect_right(ys, y)-1
        ensure(0 <= i < len(xs)-1 and 0 <= j < len(ys)-1, 'Ignition is outside the mass grid')
        u, v = (x-xs[i])/(xs[i+1]-xs[i]), (y-ys[j])/(ys[j+1]-ys[j])
        k = j*len(xs)+i
        return (1-v)*((1-u)*grid[k]+u*grid[k+1])+v*((1-u)*grid[k+len(xs)]+u*grid[k+len(xs)+1])
    polygons = []
    for line in first_front.read_text().splitlines():
        if 'FireFront[' in line:
            polygons.append([])
        elif 'FireNode[' in line:
            polygons[-1].append(tuple(float(x) for x in re.search(r'loc=\(([^)]+)', line)[1].split(',')[:2]))
    targets = re.findall(r'startFire\[lonlat=\(([^)]+)', (case/'ForeFire/Init.ff').read_text())
    perim = float(re.search(r'perimeterResolution=([\d.]+)', (case/'ForeFire/Params.ff').read_text())[1])
    ensure(len(targets) == len(polygons), 'Initial ignition count changed')
    result = []
    for points, target in zip(polygons, targets):
        expected_lon, expected_lat = (float(v.strip()) for v in target.split(',')[:2])
        # startFire creates a triangle with its +y vertex at loc+(0,2*perim).
        # Its centroid is offset, and completeFront may add collinear nodes.
        x, y = max(points, key=lambda xy: xy[1])
        y -= 2*perim
        observed_lat, observed_lon = sample(lat, x, y), sample(lon, x, y)
        error = 111195*math.hypot(observed_lat-expected_lat,
                                 (observed_lon-expected_lon)*math.cos(math.radians(observed_lat)))
        ensure(error < 1., f'Ignition alignment error is {error} m')
        result.append({'cartesian_ignition': [x, y], 'expected_lonlat': [expected_lon, expected_lat],
                       'observed_lonlat': [observed_lon, observed_lat], 'error_m': error})
    return {'checked': True, 'ignitions': result, 'max_error_m': max(r['error_m'] for r in result)}


def validate(case):
    meta = json.loads((case/'case.json').read_text())
    ensure(meta.get('return_code') == 0, 'MPI process did not finish successfully')
    outputs = case/'ForeFire/Outputs'
    paths = sorted(outputs.glob('output.0.*'), key=lambda p: float(p.name.rsplit('.', 1)[1]))
    ensure(len(paths) >= 2, 'No fire timeline was written')
    frames = [front_frame(p) for p in paths]
    ensure(frames[-1]['time']-frames[0]['time'] >= meta['duration']-1e-6,
           'Fire did not reach the requested final time')
    ensure(frames[-1]['area_m2'] > frames[0]['area_m2'] and frames[-1]['nodes'] > 0,
           'Fire did not grow')
    ensure(not (case/'ForeFire/data.nc').exists(), 'Legacy landscape file was staged unexpectedly')
    tiles = sorted((case/'ForeFire/tiles-cache').rglob('*.fftile'))
    ensure(tiles, 'No provider tiles were cached')
    ensure(all(p.read_bytes()[:8] == b'FFTILE01' for p in tiles), 'Invalid cached tile')
    fields = {}
    for rank in range(1, meta['mpi_ranks']+1):
        for name in ('U', 'V', 'BRatio'):
            path = case/f'MODEL2/output.{rank}.{name}'
            ensure(path.is_file(), f'Missing MPI field: {path}')
            fields[f'{rank}.{name}'] = binary_summary(path)
    ensure(any(v['max'] > 0 for k, v in fields.items() if k.endswith('.BRatio')),
           'No nonzero fire tracer returned to the nested atmosphere')
    audit = defaultdict(list)
    for path in sorted(outputs.glob('coupling.*.csv')):
        with path.open() as stream:
            for row in csv.DictReader(stream):
                ensure(int(row['nonfinite']) == 0 and int(row['count']) > 0,
                       f'Invalid coupling values: {path}')
                ensure(all(math.isfinite(float(row[k])) for k in ('time', 'min', 'max', 'sum')),
                       f'Nonfinite coupling summary: {path}')
                audit[(int(row['domain']), row['field'], row['direction'])].append(row)
    if meta.get('audit_required', False):
        for rank in range(1, meta['mpi_ranks']+1):
            for name, direction in [('windU', 'MNH_to_FF'), ('windV', 'MNH_to_FF'),
                                    ('heatFlux', 'FF_to_MNH'), ('vaporFlux', 'FF_to_MNH')]:
                rows = audit[(rank, name, direction)]
                ensure(len({row['time'] for row in rows}) >= 2,
                       f'Missing/repeated exchange on rank {rank}: {name}')
                if name.startswith('wind'):
                    ensure(len({(r['min'], r['max'], r['sum']) for r in rows}) > 1,
                           f'Atmospheric wind did not change on rank {rank}: {name}')
        for name in ('heatFlux', 'vaporFlux'):
            ensure(any(float(row['max']) > 0 for key, rows in audit.items()
                       if key[1] == name for row in rows), f'All returned {name} values were zero')
    for model in (1, 2):
        ensure((case/f'RUN12.{model}.FIRE.001.nc').stat().st_size > 15000,
               f'Missing final atmospheric backup for model {model}')
    geography = geography_check(case, paths[0], meta)
    report = {'passed': True, 'duration_seconds': meta['duration'], 'mpi_ranks': meta['mpi_ranks'],
              'wall_seconds': meta.get('wall_seconds'), 'provider': meta['provider'],
              'tiles': [{'name': p.name, 'bytes': p.stat().st_size, 'sha256': digest(p)} for p in tiles],
              'fire_frames': frames, 'atmospheric_fields': fields,
              'coupling_audit': {f'{rank}.{field}.{direction}': {
                  'samples': len(rows), 'first_time': float(rows[0]['time']),
                  'last_time': float(rows[-1]['time']), 'min': min(float(r['min']) for r in rows),
                  'max': max(float(r['max']) for r in rows)}
                  for (rank, field, direction), rows in audit.items()},
              'legacy_landscape_present': False, 'geography': geography}
    (case/'validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'passed': True, 'case': str(case), 'duration_seconds': meta['duration'],
                      'initial_area_m2': frames[0]['area_m2'], 'final_area_m2': frames[-1]['area_m2'],
                      'tiles_loaded': len(tiles), 'audit_streams': len(audit),
                      'geography_max_error_m': geography.get('max_error_m')}, indent=2), flush=True)
    return report


def prepare(case, provider, duration, ranks, mesonh):
    case.mkdir(parents=True, exist_ok=False)
    for name in INPUTS:
        source = SOURCE/name
        ensure(source.is_file(), f'Missing input: {source}')
        with source.open('rb') as stream:
            ensure(not stream.read(100).startswith(b'version https://git-lfs'),
                   f'LFS-pointer input: {source}')
        (case/name).symlink_to(source)
    for name in ('EXSEG1.nam', 'EXSEG2.nam'):
        text = (SOURCE/name).read_text()
        if duration != 1200:
            text = re.sub(r'(XSEGLEN\s*=\s*)[\d.]+', rf'\g<1>{duration}.', text, flags=re.I)
            text = re.sub(r'(XBAK_TIME_FREQ(?:_FIRST)?\s*=\s*)[\d.]+',
                          rf'\g<1>{duration}.', text, flags=re.I)
        (case/name).write_text(text)
    for name in ('ForeFire/Outputs', 'MODEL1', 'MODEL2'):
        (case/name).mkdir(parents=True)
    parameters = (SOURCE/'ForeFire/Params.ff').read_text()
    parameters += (f'\nsetParameter[fftilesPath={provider}]\nsetParameter[fftileCacheSize=8]\n'
                   'setParameter[couplingDiagnosticInterval=60]\n')
    (case/'ForeFire/Params.ff').write_text(parameters)
    # Keep the same physical ignition commands; omit GUI/plot output commands.
    init = '\n'.join(line for line in (SOURCE/'ForeFire/Init.ff').read_text().splitlines()
                     if not line.startswith(('include[', 'listenHTTP[')))+'\n'
    (case/'ForeFire/Init.ff').write_text(init)
    meta = {'provider': provider, 'mpi_ranks': ranks, 'duration': duration,
            'audit_required': True, 'mesonh_root': str(mesonh),
            'forefire_library_sha256': digest(ROOT/'lib/libforefireL.dylib')
            if (ROOT/'lib/libforefireL.dylib').is_file() else None,
            'created_utc': datetime.now(timezone.utc).isoformat()}
    (case/'case.json').write_text(json.dumps(meta, indent=2)+'\n')
    return meta


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mesonh-root', type=Path, default=os.environ.get('SRC_MESONH'))
    parser.add_argument('--workdir', type=Path)
    parser.add_argument('--provider', default=PROVIDER)
    parser.add_argument('--mpiexec', default=shutil.which('mpiexec'))
    parser.add_argument('--ranks', type=int, default=4)
    parser.add_argument('--duration', type=int, default=1200)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--validate', type=Path)
    args = parser.parse_args()
    if args.validate:
        validate(args.validate.resolve())
        return
    ensure(args.mesonh_root is not None, 'Specify --mesonh-root or source the Meso-NH profile')
    mesonh = args.mesonh_root.resolve()
    ensure((mesonh/'conf/profile_mesonh').is_file(), 'Meso-NH build profile is missing')
    ensure((mesonh/'exe/libForeFire.so').is_file(), 'Link the MPI ForeFire library as exe/libForeFire.so')
    ensure(args.ranks == 4, 'This validation case is tested with four MPI ranks')
    ensure(60 <= args.duration <= 86400 and args.duration % 60 == 0,
           'Use a positive duration in whole minutes (at least 60 seconds)')
    ensure(args.provider and not any(c in args.provider for c in ';\n\r[]'), 'Invalid provider address')
    case = (args.workdir or ROOT/'local/fftiles/mnh-real-nested'/
            datetime.now(timezone.utc).strftime('run-%Y%m%dT%H%M%S')).resolve()
    meta = prepare(case, args.provider, args.duration, args.ranks, mesonh)
    print(f'Case: {case}\nLog: {case / "run.log"}', flush=True)
    if args.prepare_only:
        return
    # Positional shell arguments preserve paths verbatim. Do not evaluate CLI text.
    command = '''. "$1"
if [ "$VER_MPI" = MPIVIDE ]; then echo "An MPI build is required"; exit 1; fi
export OMP_NUM_THREADS=1
ulimit -s 65520 2>/dev/null || true
exec "$2" -np "$3" "$SRC_MESONH/exe/MESONH$XYZ"
'''
    mpi = args.mpiexec
    ensure(mpi, 'mpiexec is not on PATH')
    start = time.monotonic()
    with (case/'run.log').open('w') as log:
        result = subprocess.run([shutil.which('bash') or '/bin/bash', '-c', command, 'mnh-tiles',
                                 str(mesonh/'conf/profile_mesonh'), mpi, str(args.ranks)],
                                cwd=case, stdout=log, stderr=subprocess.STDOUT)
    meta.update(return_code=result.returncode, wall_seconds=time.monotonic()-start)
    (case/'case.json').write_text(json.dumps(meta, indent=2)+'\n')
    ensure(result.returncode == 0, f'MPI run failed: inspect {case / "run.log"}')
    validate(case)


if __name__ == '__main__':
    main()
