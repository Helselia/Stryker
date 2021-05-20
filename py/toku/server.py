from __future__ import absolute_import
from __future__ import print_function
import socket
from gevent.server import StreamServer

from toku.encoders import ENCODERS
from toku.socket_session import TokuSocketSession


class TokuServer:
    def __init__(self, server_address, encoders=ENCODERS):
        self._encoders = encoders
        self.server = StreamServer(server_address, self._handle_connection)

    def serve_forever(self):
        self.server.serve_forever()

    def start(self):
        self.server.start()

    def stop(self):
        self.server.stop()

    def _handle_connection(self, sock, addr):
        print('handling connection from', sock)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.setblocking(False)
        session = TokuSocketSession(sock, self._encoders, False, self.handle_request, self.handle_push)

        self.handle_new_session(session)
        try:
            session.join()
        finally:
            self.handle_session_gone(session)

    def handle_request(self, request, session):
        pass

    def handle_push(self, push, session):
        pass

    def handle_new_session(self, session):
        pass

    def handle_session_gone(self, session):
        pass
