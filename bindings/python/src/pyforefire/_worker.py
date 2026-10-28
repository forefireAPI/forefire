"""Worker side of `ForeFireRunner`: `python -m pyforefire._worker <fd>`.

Owns one `ForeFire` and answers the requests pickled over socket `fd` until
the caller sends None or goes away.
"""

import os
import sys
from multiprocessing.connection import Connection
from multiprocessing.reduction import recv_handle


def _fork(conn):
    """Answer `ForeFireRunner.fork()`: copy this process onto a new socket.

    The caller sends the socket after the request. The copy is forked twice,
    so it is reparented as soon as the middle process exits and nobody has to
    reap it. Returns the new connection in the copy, None in the original.
    """
    fd = recv_handle(conn)
    pid = os.fork()
    if pid == 0:
        if os.fork() != 0:
            os._exit(0)
        try:
            conn.close()
            copy = Connection(fd)
            copy.send(os.getpid())
            return copy
        except BaseException:
            os._exit(1)
    os.close(fd)
    os.waitpid(pid, 0)
    return None


def _serve(conn):
    from ._pyforefire import ForeFire

    ff = ForeFire()
    while True:
        try:
            request = conn.recv()
        except EOFError:
            return
        if request is None:
            return
        name, args = request
        try:
            if name == "__fork__":
                copy = _fork(conn)
                if copy is not None:
                    conn = copy
                    continue
                reply = (True, None)
            else:
                reply = (True, getattr(ff, name)(*args))
        except Exception as e:
            reply = (False, e)
        try:
            conn.send(reply)
        except Exception as e:  # an unpicklable result or exception
            conn.send((False, RuntimeError(f"{name}: {e}")))


if __name__ == "__main__":
    _serve(Connection(int(sys.argv[1])))
