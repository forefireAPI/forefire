"""Run a ForeFire simulation in its own worker process.

ForeFire keeps simulation state in process-wide statics, so two `ForeFire()`
objects in one process share it. `ForeFireRunner` gives each simulation a
separate process instead, which isolates its memory and leaves the kernel
unchanged (see issue #175).

The worker is a fresh interpreter (`python -m pyforefire._worker <fd>`) rather
than a fork of the caller, because the caller may have threads. Requests and
replies are pickled over a socket pair. The worker's stdout and stderr are the
caller's, so kernel output still shows up where it did before.

`fork()` does fork, but the worker rather than the caller: the worker has no
threads, and its copy starts with the whole simulation state, which is what an
ensemble branching from a common spin-up needs (see issue #172).

    runner = ForeFireRunner()
    runner.execute("FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]")
    runner["propagationModel"] = "Iso"
    copy = runner.fork()
    runner.close()
"""

import os
import signal
import socket
import subprocess
import sys
import threading
import time
from multiprocessing.connection import Connection
from multiprocessing.reduction import send_handle

__all__ = ["ForeFireRunner", "WorkerDied"]


class WorkerDied(RuntimeError):
    """The worker process ended while a call was pending, or before it."""


class _ForkedProcess:
    """The part of `subprocess.Popen` the runner uses, for a forked worker.

    A forked worker is not a child of the caller, so its exit status is
    unknown: `poll()` and `wait()` always return None, and `wait()` watches
    the process with signal 0 instead of waiting for it.
    """

    def __init__(self, pid):
        self.pid = pid

    def _alive(self):
        try:
            os.kill(self.pid, 0)
        except (ProcessLookupError, PermissionError):
            return False
        return True

    def poll(self):
        return None

    def wait(self, timeout=None):
        deadline = None if timeout is None else time.monotonic() + timeout
        while self._alive():
            if deadline is not None and time.monotonic() > deadline:
                raise subprocess.TimeoutExpired(f"pid {self.pid}", timeout)
            time.sleep(0.01)

    def kill(self):
        try:
            os.kill(self.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass


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
            process = subprocess.Popen(
                [sys.executable, "-m", "pyforefire._worker", str(child.fileno())],
                pass_fds=(child.fileno(),),
            )
        finally:
            child.close()
        self._attach(Connection(parent.detach()), process)

    def _attach(self, conn, process):
        self._conn = conn
        self._process = process
        self._lock = threading.Lock()

    def _call(self, name, *args, fd=None):
        with self._lock:
            if self._conn is None:
                raise WorkerDied("the runner is closed")
            try:
                self._conn.send((name, args))
                if fd is not None:
                    send_handle(self._conn, fd, self._process.pid)
                ok, value = self._conn.recv()
            except (EOFError, OSError):
                code = self._process.wait()
                self._conn.close()
                self._conn = None
                status = "" if code is None else f" with status {code}"
                raise WorkerDied(f"the ForeFire worker exited{status}") from None
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

    def fork(self):
        """Return a new runner holding a copy of this simulation.

        The worker forks itself, so the copy starts with the same domain,
        layers, fronts and parameters, then shares nothing with this runner:
        each can be stepped or changed without affecting the other, and
        closing one leaves the other running. Memory pages are copied only
        when one side writes them, so forking after a long spin-up is cheap.

        Files the simulation has open are shared with the copy, and a running
        HTTP command server is not copied.
        """
        ours, theirs = socket.socketpair()
        try:
            try:
                self._call("__fork__", fd=theirs.fileno())
            finally:
                theirs.close()
            conn = Connection(ours.detach())
        except BaseException:
            ours.close()
            raise
        try:
            pid = conn.recv()
        except (EOFError, OSError):
            conn.close()
            raise WorkerDied("the forked ForeFire worker exited before it started") from None
        copy = ForeFireRunner.__new__(ForeFireRunner)
        copy._attach(conn, _ForkedProcess(pid))
        return copy

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
