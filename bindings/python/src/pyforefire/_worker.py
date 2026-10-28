"""Worker side of `ForeFireRunner`: `python -m pyforefire._worker <fd>`.

Owns one `ForeFire` and answers the requests pickled over socket `fd` until
the caller sends None or goes away.
"""

import sys
from multiprocessing.connection import Connection


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
            reply = (True, getattr(ff, name)(*args))
        except Exception as e:
            reply = (False, e)
        try:
            conn.send(reply)
        except Exception as e:  # an unpicklable result or exception
            conn.send((False, RuntimeError(f"{name}: {e}")))


if __name__ == "__main__":
    _serve(Connection(int(sys.argv[1])))
