#!/usr/bin/env python3
"""Serve the WASM tile demo and an existing .fftile directory on loopback.

python3 tools/tiles/serve_wasm_demo.py --tiles test_output_tiles --port 8765
The server only serves static files. It never runs the simulator.
"""
import argparse
import json
import mimetypes
from pathlib import Path
import re
import shutil
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--tiles',type=Path,default=ROOT/'test_output_tiles')
    p.add_argument('--port',type=int,default=8765)
    p.add_argument('--wasm',type=Path,default=ROOT/'bin')
    args=p.parse_args()
    tiles=args.tiles.resolve();wasm=args.wasm.resolve()
    if not tiles.is_dir():p.error('tile directory does not exist')
    if not all((wasm/name).is_file() for name in ('forefire.mjs','forefire.wasm')):
        p.error('build the WASM module first; see bindings/wasm/tiles-demo/README.md')
    assets=ROOT/'bindings/wasm/tiles-demo'
    routes={'/':assets/'index.html','/index.html':assets/'index.html',
            '/demo.mjs':assets/'demo.mjs','/worker.mjs':assets/'worker.mjs',
            '/style.css':assets/'style.css','/tile-service.mjs':ROOT/'bindings/wasm/tile-service.mjs',
            '/fuels.csv':ROOT/'tests/runff/fuels.csv',
            '/wasm/forefire.mjs':wasm/'forefire.mjs','/wasm/forefire.wasm':wasm/'forefire.wasm'}
    class Handler(BaseHTTPRequestHandler):
        def send_asset_headers(self,status,kind,length):
            self.send_response(status)
            self.send_header('Content-Type',kind)
            self.send_header('Content-Length',str(length))
            self.send_header('Cache-Control','no-cache')
            self.send_header('Access-Control-Allow-Origin','*')
            self.send_header('X-Content-Type-Options','nosniff')
            self.end_headers()
        def serve(self,head=False):
            path=unquote(urlsplit(self.path).path)
            if path=='/health':
                data=json.dumps({'tile_count':len(list(tiles.glob('*.fftile'))),'engine':'WebAssembly','simulation_on_server':False}).encode()
                self.send_asset_headers(200,'application/json',len(data))
                if not head:self.wfile.write(data)
                return
            if path.startswith('/public-tiles/'):
                name=path[len('/public-tiles/'):]
                if not re.fullmatch(r'-?\d+(?:\.5)?_-?\d+(?:\.5)?\.fftile',name):
                    self.send_error(404);return
                # Fixed destination: this is not an arbitrary URL proxy.
                try:
                    with urlopen(Request('https://forefire.univ-corse.fr/fftiles/'+name),timeout=30) as response:
                        length=int(response.headers['Content-Length'])
                        if length>64*1024*1024:raise ValueError('Tile exceeds 64 MiB')
                        self.send_asset_headers(200,'application/octet-stream',length)
                        if not head:shutil.copyfileobj(response,self.wfile)
                except HTTPError as error:self.send_error(error.code if error.code in (404,410) else 502)
                except (URLError,ValueError,TimeoutError):self.send_error(502)
                except (BrokenPipeError,ConnectionResetError):pass
                return
            file=routes.get(path)
            if path.startswith('/tiles/'):
                name=path[len('/tiles/'):]
                if re.fullmatch(r'-?\d+(?:\.5)?_-?\d+(?:\.5)?\.fftile',name):file=tiles/name
            if file is None or not file.is_file():
                data=b'No tile or asset at this address.\n';self.send_asset_headers(404,'text/plain',len(data))
                if not head:self.wfile.write(data)
                return
            kind={'.mjs':'text/javascript','.wasm':'application/wasm','.fftile':'application/octet-stream'}.get(file.suffix,mimetypes.guess_type(file.name)[0] or 'application/octet-stream')
            self.send_asset_headers(200,kind,file.stat().st_size)
            if not head:
                try:
                    with file.open('rb') as src:shutil.copyfileobj(src,self.wfile)
                except (BrokenPipeError,ConnectionResetError):pass
        def do_GET(self):self.serve()
        def do_HEAD(self):self.serve(True)
    server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler)
    server.daemon_threads=True
    print(f'ForeFire WASM tile demo: http://localhost:{server.server_port}/',flush=True)
    print(f'Serving {len(list(tiles.glob("*.fftile")))} tiles from {tiles}',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()

if __name__=='__main__':main()
