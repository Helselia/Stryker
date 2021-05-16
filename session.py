import errno
import logging
import socket

import gevent
from gevent.event import AsyncRequest, Event

from ctypes import c_uint32, c_uint8

from exceptions import NoEncoderAvailable, ConnectionTerminated, TokuDecoderError, StreamDefunct, NotClientException, NotServerException, TokuErrorReceived
from opcodes import Request, Response, Ping, Pong, Push, Hello, GoAway, HelloAck, Error

from watcher import SocketWatcher
from handler import TokuStreamHandler

OUTBUF_MAX = 65535

class CloseReasons:
  NORMAL = 0
  PING_TIMEOUT = 1
  UNKNOWN_ENCODER = 2
  NO_MUTUAL_ENCODERS = 3
  DIDNT_STOP_IN_TIME = 4
  DECODER_ERROR = 5

class TokuSocketSession:
  def __init__(self, sock, encoders, is_client = True, on_request = None, on_push = None):
    self.is_client = is_client
    self._stream_handler = TokuStreamHandler()
    self._sock = sock
    self._watcher = SocketWatcher(self._sock.fileno())
    self._inflight_requests = {}
    self._available_encoders = encoders
    self._available_compressors = {}
    self._shutdown_event = Event()
    self._close_event = Event()
    self._ready_event = Event()
    self._ping_interval = 5
    self._is_ready = False
    self._shutting_down = False

    if not self._is_client:
      self._on_request = on_request
    
    self._on_push = on_push

    gevent.spawn(self._ping_loop)
    gevent.spawn(self._run_loop)

    if is_client:
      self._send_hello()
    
  def set_push_handler(self, push_handler):
    self._on_push = None
  
  def _resume_sending(self):
    if self._sock is None:
      return
    
    if self._stream_handler.write_buffer_len() == 0:
      return
    
    self._watcher.switch_if_write_unblocked()
  
  def shutdown(self):
    if self._shutting_down:
      return
    
    self._shutting_down = True
  
  def _cleanup_socket(self):
    sock = self._sock
    self._sock = None
    if sock:
      sock.close()
    
    if sock:
      self._watcher.request_switch()
    
    self._cleanup_inflight_requests(ConnectionTerminated())

    if not self._ready_event.is_set():
      self._ready_event.set()
    
    if not self._shutdown_event.is_set():
      self._shutdown_event.set()
    
    if not self._close_event.is_set():
      self._close_event.set()
    
  def close(self, code=CloseReasons.NORMAL, reason=None, block=False, block_timeout=None, close_timeout=None, via_remote_goaway=False):
    if not self._shutting_down:
      self._shutting_down = True
    
      if not self._ready_event.is_set():
        self._ready_event.set()
      
      if not self._shutdown_event.is_set():
        self._shutdown_event.set()
      
      if not via_remote_goaway and self._sock:
        self._stream_handler.send_goaway(0, code, reason)
      
      gevent.spawn(self._close_timeout)
    
    if block:
      self.join(block_timeout)
  
  def _close_timeout(self):
    if self._close_event.wait(self._ping_interval):
      return
    
    self._cleanup_socket()
  
  def join(self, timeout=None):
    self._close_event.wait(timeout=timeout)
  
  def terminate(self):
    self._cleanup_socket()
  
  def _cleanup_inflight_requests(self, close_exception):
    requests = self._inflight_requests.values()
    self._inflight_requests.clear()

    for request in requests:
      if isinstance(request, AsyncRequest):
        request.set_exception(close_exception)
  
  def await_ready(self):
    if not self._is_ready:
      self._ready_event.wait()
  
  def is_ready(self):
    return self._is_ready
  
  def _decode_data(self, flags, data):
    if not self._is_ready:
      self._ready_event.wait()
    
    if not self._encoder_loads:
      raise NoEncoderAvailable()
    
    return self._encoder_loads(data)
  
  def _encode_data(self, data):
    if not self._is_ready:
      self._ready_event.wait()
    
    if not self._encoder_dumps:
      raise NoEncoderAvailable()
    
    return 0, self._encoder_dumps(data)
