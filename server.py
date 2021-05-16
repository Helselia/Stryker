import socket
from gevent import StreamServer
from .encoders import ENCODERS

class Server:
  def __init__(self, server_addr, encoders=ENCODERS):
    self._encoders = encoders
    self._server = StreamServer(server_addr, self._handle_connection)
  
  def start(self):
    self._server.serve_forever()

  def stop(self):
    self._server.stop()

  def _handle_connection(self, sock, addr):
    print('handling connection from', sock)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sock.setblocking(False)

    # session = TokuSocketSession(sock, self._encoders, False, self.handle_request, self.handle_push)

    # self.handle_new_session(session)
    # try:
      # session.join()
    # finally:
      # self.handle_session_gone(session)
    
  def handle_request(self, req, session):
    pass

  def handle_push(self, push, session):
    pass

  def handle_new_session(self, session):
    pass

  def handle_session_gone(self, session):
    pass
