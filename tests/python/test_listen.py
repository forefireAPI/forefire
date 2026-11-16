"""Listen mode (`listenHTTP`) over an FFTile landscape, driven from Python.

A `ForeFireRunner` worker starts the HTTP command server; the test then sends
the commands a client would, by GET and by POST, while the worker is idle.
Checks that a run driven over HTTP saves a checkpoint that a fresh worker
restores and continues exactly like the served session.

    python tests/python/test_listen.py

Exit status is 0 only if every check passes.
"""

import socket
import sys
import tempfile
import time
import traceback
import urllib.parse
import urllib.request
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests" / "python"))
from test_fftile_ffstate import ignite, same_state, session, write_landscape  # noqa: E402


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class Client:
    def __init__(self, port):
        self.base = f"http://127.0.0.1:{port}"

    def get(self, command):
        quoted = urllib.parse.quote(command, safe="[]=;,()/:.")
        with urllib.request.urlopen(f"{self.base}/ff:{quoted}", timeout=60) as r:
            return r.read().decode()

    def post(self, command):
        request = urllib.request.Request(f"{self.base}/", data=("ff:" + command).encode())
        with urllib.request.urlopen(request, timeout=60) as r:
            return r.read().decode()

    def wait(self):
        for _ in range(100):
            try:
                return self.get("getParameter[ISOdate]")
            except OSError:
                time.sleep(0.05)
        raise RuntimeError("the HTTP server never answered")


def split_post(port, command):
    """POST whose body arrives after the headers, in its own segment."""
    body = ("ff:" + command).encode()
    with socket.create_connection(("127.0.0.1", port)) as c:
        c.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        c.sendall(f"POST / HTTP/1.1\r\nHost: 127.0.0.1:{port}\r\n"
                  f"Content-Length: {len(body)}\r\n\r\n".encode())
        time.sleep(0.05)
        c.sendall(body)
        return c.makefile("rb").read().decode()


def check_served_run(case, tiles):
    port = free_port()
    served = session(case, 0)
    restored = session(case, 0)
    try:
        served.execute(f"listenHTTP[127.0.0.1:{port}]")
        http = Client(port)
        http.wait()

        # The landscape and the ignition, as a client would send them.
        out = http.get(f"loadData[provider={tiles};center=(8.8,42.15);size=(.08,.08);"
                       "date=2025-06-01T12:00:00Z]")
        assert "1 available" in out, out
        http.post("trigger[wind;vel=(6,2,0)]")
        http.post("startFire[lonlat=(8.8,42.15,0);t=0]")
        assert "FireNode" in http.get("print[]"), "POST startFire[] did not start a fire"

        http.post("step[dt=120.25]")
        seed = case / "served.ffstate"
        assert "Saved FFState" in http.post(f"saveState[file={seed}]")
        assert http.get("getParameter[ISOdate]").startswith("2025-06-01T12:02:00")

        # What the web page reads back on a tile domain.
        assert "fuel" in http.get("getParameter[layerNames]")
        wind = http.post(f"plot[parameter=wind;filename={case / 'wind.json'};size=8,8;"
                         "projectionOut=json]")
        assert '"SWlon":8.76' in wind, wind
        http.post("setParameter[dumpMode=geojson]")
        geojson = http.post("print[]")
        http.post("setParameter[dumpMode=ff]")
        assert '"FeatureCollection"' in geojson and "8.79" in geojson, geojson[:300]

        # The served session is the worker's own one.
        assert served.execute("print[]") == http.get("print[]")

        http.post("step[dt=300]")
        continuous = case / "continuous.ffstate"
        http.post(f"saveState[file={continuous}]")

        out = restored.execute(f"loadState[file={seed}]")
        assert "Loaded FFState" in out, out
        restored.execute("step[dt=300]")
        resumed = case / "resumed.ffstate"
        restored.execute(f"saveState[file={resumed}]")
        differ = same_state(continuous, resumed)
        assert not differ, f"restored run differs from the served one in {differ}"
        assert np.array_equal(served.getDoubleArray("BMap"), restored.getDoubleArray("BMap"),
                              equal_nan=True)
    finally:
        served.close()
        restored.close()


def check_split_post(case):
    """A body sent after the headers is still read."""
    port = free_port()
    ff = session(case, 0)
    try:
        ff.execute(f"listenHTTP[127.0.0.1:{port}]")
        Client(port).wait()
        reply = split_post(port, "getParameter[propagationModel]")
        assert reply.startswith("HTTP/1.0 200") and "Rothermel" in reply, reply
        for _ in range(20):
            assert "Rothermel" in Client(port).post("getParameter[propagationModel]")
    finally:
        ff.close()


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        tiles = tmp / "tiles"
        tiles.mkdir()
        write_landscape(tiles)
        checks = [
            ("run served over HTTP, restored in a fresh worker", lambda: check_served_run(tmp, tiles)),
            ("POST body in its own segment", lambda: check_split_post(tmp)),
        ]
        failed = 0
        for name, check in checks:
            try:
                check()
                print(f"PASS  {name}")
            except Exception:
                failed += 1
                print(f"FAIL  {name}")
                traceback.print_exc()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
