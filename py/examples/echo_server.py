from __future__ import absolute_import
from __future__ import print_function
import time

import gevent
from gevent.monkey import patch_all

patch_all()

i = 0
from toku.server import TokuServer

def log_loop():
    last_i = 0
    last = time.time()
    while True:
        gevent.sleep(1)
        now = time.time()
        elapsed = now - last
        req_sec = (i - last_i) / elapsed

        print('%s total requests (%.2f/sec). last log %.2f sec ago.' % (
            i, req_sec, elapsed
        ))
        last_i = i
        last = now


class Server(TokuServer):
    def handle_request(self, request, session):
        global i
        i += 1
        if i and i % 50000 == 0:
            session.close()

        return 'm' * 1024

    def handle_push(self, push, session):
        print('pushed', push.data)
        return


if __name__ == '__main__':
    s = Server(('localhost', 8080))
    gevent.spawn(log_loop)
    s.serve_forever()
