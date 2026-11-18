#!/usr/bin/env python3
"""Fresh-process FFState continuation against an uninterrupted real FFTile run.

Uses existing downloaded tiles; all scripts/checkpoints stay under --out.
No NetCDF or third-party Python dependencies.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[2]


def chunks(path):
    data = path.read_bytes()
    assert data[:8] == b'FFSTATE1'
    version, count = struct.unpack_from('<II', data, 8)
    assert version == 1 and count <= 32
    at, result = 16, {}
    for _ in range(count):
        length, size, checksum = struct.unpack_from('<IQI', data, at)
        at += 16
        name = data[at:at+length].decode()
        at += length
        payload = data[at:at+size]
        at += size
        assert len(payload) == size and zlib.crc32(payload) == checksum
        assert name not in result
        result[name] = payload
    assert at == len(data)
    return result


def run(exe, directory, name, commands):
    script = directory/(name+'.ff')
    script.write_text(f'setParameters[caseDirectory={directory};ForeFireDataDirectory=]\n'
                      + '\n'.join(commands)+'\n')
    result = subprocess.run([str(exe), '-i', str(script)], cwd=directory,
                            capture_output=True, text=True, timeout=90)
    log = result.stdout+result.stderr
    (directory/(name+'.log')).write_text(log)
    assert result.returncode == 0, log[-3000:]
    return log


def compare(a, b):
    left, right = chunks(a), chunks(b)
    # Runtime output settings and iteration bookkeeping are not the fire state.
    for name in ('domain', 'objects.ff', 'events', 'arrival'):
        assert left[name] == right[name], f'{name} differs: {a} / {b}'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, default=ROOT/'local/ffstate/build/forefire')
    p.add_argument('--tiles', type=Path, default=ROOT/'test_output_tiles')
    p.add_argument('--out', type=Path, default=ROOT/'local/ffstate/smoke')
    args = p.parse_args()
    exe, tiles, out = args.exe.resolve(), args.tiles.resolve(), args.out.resolve()
    assert exe.is_file() and (tiles/'42_8.5.fftile').is_file()
    out.mkdir(parents=True, exist_ok=True)
    report = []
    for subdivisions in (0, 256):
        case = out/f'grid-{subdivisions}'
        case.mkdir(exist_ok=True)
        seed, continuous, resumed = (case/n for n in
                                    ('seed.ffstate', 'continuous.ffstate', 'resumed.ffstate'))
        log = run(exe, case, 'continuous', [
            'setParameters[propagationModel=Rothermel;fuelsTable=STDlandIndexedFuelsTable;'
            'perimeterResolution=24;spatialIncrement=6;relax=0.5;minSpeed=0;'
            'minimalPropagativeFrontDepth=10;outputsUpdate=0;'
            f'arrivalMapSubdivisions={subdivisions}]',
            f'loadData[provider={tiles};center=(8.8,42.15);size=(.08,.08);date=2025-12-31T23:59:30Z]',
            'trigger[wind;vel=(6,2,0)]',
            'startFire[lonlat=(8.8,42.15,0);t=0]',
            'step[dt=120.25]', f'saveState[file={seed}]',
            'step[dt=300]', f'saveState[file={continuous}]'])
        assert 'ffstate:' not in log, log
        log = run(exe, case, 'resumed', [f'loadData[{seed}]',
                  f'saveState[file={case/"roundtrip.ffstate"}]',
                  'step[dt=300]', f'saveState[file={resumed}]'])
        assert 'Loaded FFState:' in log and 'ffstate:' not in log, log
        compare(seed, case/'roundtrip.ffstate')
        compare(continuous, resumed)
        bad = bytearray(seed.read_bytes())
        bad[-1] ^= 1
        corrupt = case/'corrupt.ffstate'
        corrupt.write_bytes(bad)
        log = run(exe, case, 'corrupt', [f'loadData[{seed}]',
                  f'loadState[file={corrupt};mode=attach]',
                  f'saveState[file={case/"after-corrupt.ffstate"}]'])
        assert 'checksum mismatch' in log
        compare(seed, case/'after-corrupt.ffstate')
        # Failure to serialize unsupported future scripts must not replace a
        # previously valid checkpoint with a partial file.
        preserved = case/'preserved.ffstate'
        preserved.write_bytes(seed.read_bytes())
        log = run(exe, case, 'pending-script', [f'loadData[{seed}]',
                  'include[future-scenario.ff]@t=99999',
                  f'saveState[file={preserved}]'])
        assert 'unsupported scheduled object' in log
        assert preserved.read_bytes() == seed.read_bytes()
        report.append({'subdivisions': subdivisions, 'checkpoint_bytes': seed.stat().st_size,
                       'fresh_process_exact_continuation': True,
                       'fractional_time_and_year_boundary': True,
                       'corrupt_load_preserves_session': True,
                       'failed_save_preserves_checkpoint': True})
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
