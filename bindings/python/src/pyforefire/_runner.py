"""Run a ForeFire simulation in its own worker process.

ForeFire keeps simulation state in process-wide statics, so two `ForeFire()`
objects in one process share it. `ForeFireRunner` gives each simulation a
separate process instead, which isolates its memory and leaves the kernel
unchanged (see issue #175).

The worker is a fresh interpreter (`python -m pyforefire._worker <fd>`) rather
than a fork of the caller, because the caller may have threads. Requests and
replies are pickled over a socket pair. The worker's stdout and stderr are the
caller's, so kernel output still shows up where it did before.

    runner = ForeFireRunner()
    runner.execute("FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]")
    runner["propagationModel"] = "Iso"
    runner.close()
"""

import socket
import subprocess
import sys
import threading
from multiprocessing.connection import Connection

__all__ = ["ForeFireRunner", "WorkerDied"]


class WorkerDied(RuntimeError):
    """The worker process ended while a call was pending, or before it."""


class ForeFireRunner:
    """A `pyforefire.ForeFire` that lives in a separate process.

    Every method of `ForeFire` is forwarded, along with `runner[key]` and
    `runner[key] = value`. Arguments and results are copied between the two
    processes, numpy arrays included. An exception raised in the worker is
    raised again here; a crash of the worker raises `WorkerDied` instead of
    ending the caller.

    Calls on one runner are serialised by a lock. Separate runners do not
    share anything and can be driven from separate threads.
    """

    def __init__(self):
        parent, child = socket.socketpair()
        try:
            self._process = subprocess.Popen(
                [sys.executable, "-m", "pyforefire._worker", str(child.fileno())],
                pass_fds=(child.fileno(),),
            )
        finally:
            child.close()
        self._conn = Connection(parent.detach())
        self._lock = threading.Lock()

    def _call(self, name, *args):
        with self._lock:
            if self._conn is None:
                raise WorkerDied("the runner is closed")
            try:
                self._conn.send((name, args))
                ok, value = self._conn.recv()
            except (EOFError, OSError):
                code = self._process.wait()
                self._conn.close()
                self._conn = None
                raise WorkerDied(f"the ForeFire worker exited with status {code}") from None
        if not ok:
            raise value
        return value

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(name)
        return lambda *args: self._call(name, *args)

    def __getitem__(self, key):
        return self._call("__getitem__", key)

    def __setitem__(self, key, value):
        self._call("__setitem__", key, value)

    def close(self):
        """Stop the worker and wait for it. Calling it twice is harmless."""
        with self._lock:
            if self._conn is not None:
                try:
                    self._conn.send(None)
                except OSError:
                    pass
                self._conn.close()
                self._conn = None
        if self._process.poll() is None:
            try:
                self._process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._process.kill()
                self._process.wait()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        if getattr(self, "_process", None) is not None:
            self.close()

