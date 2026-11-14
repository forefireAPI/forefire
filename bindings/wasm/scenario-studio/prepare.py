#!/usr/bin/env python3
"""Stage the source-only studio with a locally built engine and optional Corsica tiles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import urllib.error
import urllib.request

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[2]
CORSICA = [f'{lat:g}_{lon:g}.fftile' for lat in (41, 41.5, 42, 42.5, 43) for lon in (8.5, 9, 9.5)]


def prepare(output, module, corsica=False):
    output, module = output.resolve(), module.resolve()
    if output == SOURCE or SOURCE in output.parents or output in SOURCE.parents:
        raise ValueError('Stage outside the source directory, e.g. local/scenario-studio')
    output.mkdir(parents=True, exist_ok=True)
    for path in SOURCE.rglob('*'):
        rel = path.relative_to(SOURCE)
        if any(p.startswith('.') or p == '__pycache__' for p in rel.parts):
            continue
        if path.is_file() and path.name not in ('config.js', 'prepare.py'):
            target = output / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    shutil.copy2(module, output/'assets/forefire.js')
    shutil.copy2(module.with_suffix('.wasm'), output/'assets/forefire.wasm')
    token = os.environ.get('CESIUM_ION_TOKEN', '')
    era = os.environ.get('FOREFIRE_ERA_PROVIDER', '')
    config = 'const base=globalThis.location?.href??globalThis.self?.location?.href??import.meta.url;\n'
    config += 'export const cesiumToken=' + json.dumps(token) + ';\n'
    config += "export const tileProvider=new URL('./tiles/',base).href;\n"
    config += 'export const eraProvider=' + (json.dumps(era.rstrip('/')+'/') if era else "new URL('/eraland/',base).href") + ';\n'
    (output/'config.js').write_text(config)
    if corsica:
        tiles = output/'tiles'
        tiles.mkdir(exist_ok=True)
        manifest = []
        for name in CORSICA:
            target = tiles/name
            try:
                with urllib.request.urlopen('https://forefire.univ-corse.fr/fftiles/'+name, timeout=90) as response:
                    data = response.read(64*1024*1024+1)
            except urllib.error.HTTPError as error:
                if error.code in (404, 410):
                    raise RuntimeError(f'Corsica coverage missing: {name}') from error
                raise
            if len(data) > 64*1024*1024 or data[:8] != b'FFTILE01':
                raise ValueError(f'Invalid tile: {name}')
            target.write_bytes(data)
            manifest.append(dict(name=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        (tiles/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(f'Studio staged at {output}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'local/scenario-studio')
    parser.add_argument('--module', type=Path, default=ROOT/'bin/forefire.mjs')
    parser.add_argument('--corsica', action='store_true', help='Download 15 demonstration tiles into the staged site')
    args = parser.parse_args()
    prepare(args.output, args.module, args.corsica)
