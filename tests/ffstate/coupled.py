#!/usr/bin/env python3
"""Run/validate the real nested MPI case attached to a standalone FFState."""
import argparse
from bisect import bisect_right
import csv
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

from smoke import chunks

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tests/mnh_real_nested'))
from run_tiles import prepare, front_frame


def domain(data):
    tokens = iter(data.decode().split())
    number = lambda: float(next(tokens))
    result = dict(zip(('clock', 'time', 'update', 'epoch', 'output_time', 'output_update'),
                      [number() for _ in range(6)]))
    result['next_id'] = int(next(tokens))
    for key, n in [('sw', 3), ('ne', 3), ('frame', 4)]:
        result[key] = [number() for _ in range(n)]
    result['nx'], result['ny'] = int(next(tokens)), int(next(tokens))
    result['dx'], result['dy'], result['subdivisions'] = number(), number(), int(next(tokens))
    result['bounds'] = [number() for _ in range(4)]
    result['wind'] = [number(), number()]
    for key in ('xs', 'ys', 'lat', 'lon'):
        result[key] = [number() for _ in range(int(next(tokens)))]
    return result


def nodes(data):
    lines = data.decode().splitlines()
    fronts = int(lines[1])
    return [[float(v) for v in line.split()] for line in lines[3+fronts:]]


def geographic(d, x, y):
    if not d['xs']:
        lat, lon, px, py = d['frame']
        latitude = lat+(y-py)/111195
        return latitude, lon+(x-px)/(111195*math.cos(math.radians(latitude)))
    xs, ys = d['xs'], d['ys']
    i = min(max(bisect_right(xs, x)-1, 0), len(xs)-2)
    j = min(max(bisect_right(ys, y)-1, 0), len(ys)-2)
    u, v = (x-xs[i])/(xs[i+1]-xs[i]), (y-ys[j])/(ys[j+1]-ys[j])
    def sample(a):
        k = j*len(xs)+i
        return (1-v)*((1-u)*a[k]+u*a[k+1])+v*((1-u)*a[k+len(xs)]+u*a[k+len(xs)+1])
    return sample(d['lat']), sample(d['lon'])


def history(data, d):
    count = struct.unpack_from('<Q', data)[0]
    runs = [struct.unpack_from('<QQd', data, 8+24*i) for i in range(count)]
    cells = sum(r[1] for r in runs)
    ages = [d['clock']-r[2] for r in runs]
    return {'cells': cells, 'native_raster_area_m2': cells*d['dx']*d['dy'],
            'age_range_s': [min(ages), max(ages)] if ages else [0, 0]}


def validate(case, state):
    meta = json.loads((case/'case.json').read_text())
    log = (case/'run.log').read_text()
    assert 'Loaded FFState:' in log and 'Saved FFState:' in log
    assert 'ffstate:' not in log
    source, attached = chunks(state), chunks(case/'coupled-initial.ffstate')
    a, b = domain(source['domain']), domain(attached['domain'])
    assert abs(a['epoch']+a['clock']-b['epoch']-b['clock']) < 1e-6
    errors = []
    source_nodes, host_nodes = nodes(source['objects.ff']), nodes(attached['objects.ff'])
    assert len(source_nodes) == len(host_nodes) > 0
    for s, t in zip(source_nodes, host_nodes):
        lat, lon = geographic(a, s[6], s[7])
        hlat, hlon = geographic(b, t[6], t[7])
        errors.append(111195*math.hypot(lat-hlat, (lon-hlon)*math.cos(math.radians(lat))))
        assert (s[3] == 4) == (t[3] == 4), 'Stopped node was reactivated'
    assert max(errors) < .01, max(errors)
    source_history, host_history = history(source['arrival'], a), history(attached['arrival'], b)
    for x, y in zip(source_history['age_range_s'], host_history['age_range_s']):
        assert abs(x-y) < 1e-5, 'Burn ages changed during time-reference transfer'
    same_grid_objects = a['xs'] and a['nx'] == b['nx'] and a['ny'] == b['ny']
    if same_grid_objects:
        assert source['objects.ff'] == attached['objects.ff']
        assert source['arrival'] == attached['arrival']
    rows = []
    for path in sorted((case/'ForeFire/Outputs').glob('coupling.*.csv')):
        with path.open() as stream:
            current = list(csv.DictReader(stream))
        assert current
        for row in current:
            assert row['nonfinite'] == '0'
            assert all(math.isfinite(float(row[key])) for key in ('time', 'min', 'max', 'sum'))
        rows += current
    assert len({r['domain'] for r in rows}) == 4
    flux = {name: max(float(r['max']) for r in rows if r['field'] == name)
            for name in ('heatFlux', 'vaporFlux')}
    assert all(v > 0 for v in flux.values())
    first = min(float(r['time']) for r in rows if r['field'] == 'heatFlux')
    first_heat = max(float(r['max']) for r in rows
                     if r['field'] == 'heatFlux' and float(r['time']) == first)
    assert first_heat > 1000, 'Initial heat flux diluted by a pre-attachment integration interval'
    perimeters = sorted((case/'ForeFire/Outputs').glob('output.0.*'),
                        key=lambda p: float(p.name.rsplit('.', 1)[1]))
    frames = [front_frame(p) for p in perimeters]
    assert frames[-1]['time'] >= b['clock']+meta['duration']-1e-6
    assert frames[-1]['area_m2'] > frames[0]['area_m2'] > 0
    assert (case/'RUN12.1.FIRE.001.nc').is_file() and (case/'RUN12.2.FIRE.001.nc').is_file()
    report = {'checkpoint_bytes': state.stat().st_size,
              'coupled_checkpoint_bytes': (case/'coupled-initial.ffstate').stat().st_size,
              'mpi_ranks': 4, 'duration_s': meta['duration'], 'nodes_attached': len(host_nodes),
              'max_geographic_error_m': max(errors), 'checkpoint_utc_preserved': True,
              'initial_area_m2': frames[0]['area_m2'], 'final_area_m2': frames[-1]['area_m2'],
              'peak_fluxes': flux, 'first_heat_flux_max': first_heat,
              'exchange_rows_checked': len(rows), 'all_exchange_values_finite': True,
              'atmospheric_checkpoints_written': 2,
              'source_arrival_history': source_history, 'attached_arrival_history': host_history,
              'same_grid_live_objects_exact': bool(same_grid_objects),
              'scenario_executions': log.count('FFSTATE_SCENARIO_APPLIED')}
    if meta.get('scenario'):
        assert report['scenario_executions'] == 4, 'Scenario did not execute once on every rank'
    (case/'ffstate-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--state', type=Path, required=True)
    p.add_argument('--mesonh-root', type=Path)
    p.add_argument('--out', type=Path)
    p.add_argument('--validate', type=Path)
    p.add_argument('--provider', default='https://forefire.univ-corse.fr/fftiles/')
    p.add_argument('--duration', type=int, default=1200)
    p.add_argument('--scenario', type=Path)
    args = p.parse_args()
    state = args.state.resolve()
    if args.validate:
        validate(args.validate.resolve(), state)
        return
    assert args.mesonh_root and args.out and args.duration >= 120 and args.duration % 60 == 0
    case, mesonh = args.out.resolve(), args.mesonh_root.resolve()
    meta = prepare(case, args.provider, args.duration, 4, mesonh)
    params = case/'ForeFire/Params.ff'
    params.write_text(params.read_text()+f'\nsetParameter[ffstateFile={state}]\n')
    (case/'ForeFire/Init.ff').write_text('loadState[mode=attach]\n'
        + f'print[{case/"initial-attached.ff"}]\nsaveState[file={case/"coupled-initial.ffstate"}]\n')
    if args.scenario:
        # GlobalInitFile is read on every rank. Existing scheduled commands run
        # at the same model instant; the snapshot itself contains no script history.
        params.write_text(params.read_text()+'setParameter[GlobalInitFile=ScenarioInit.ff]\n')
        (case/'ForeFire/ScenarioInit.ff').write_text(
            f'include[{args.scenario.resolve()}]@t={54000+args.duration//2}\n')
    command = '. "$1/conf/profile_mesonh"\nexport OMP_NUM_THREADS=1\nulimit -s 65520\nexec "$2" -np 4 "$SRC_MESONH/exe/MESONH$XYZ"\n'
    mpi = shutil.which('mpiexec')
    assert mpi
    start = time.monotonic()
    with (case/'run.log').open('w') as stream:
        result = subprocess.run([shutil.which('bash'), '-c', command, 'ffstate', str(mesonh), mpi],
                                cwd=case, stdout=stream, stderr=subprocess.STDOUT,
                                env={**os.environ, 'OMP_NUM_THREADS': '1'})
    meta.update(return_code=result.returncode, wall_seconds=time.monotonic()-start,
                input_ffstate=str(state), scenario=str(args.scenario) if args.scenario else None)
    (case/'case.json').write_text(json.dumps(meta, indent=2)+'\n')
    assert result.returncode == 0, f'Inspect {case/"run.log"}'
    validate(case, state)


if __name__ == '__main__':
    main()
