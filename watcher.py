import gevent
from gevent.hub import Waiter
from greenlet import getcurrent

class SocketWatcher:
  def __init__(self, sock_fileno: int):
    self.sock_fileno = sock_fileno
    self.sock_write_blocked = False
    self.waiter = Waiter()
    self.waiter_get = self.waiter.get
    self.waiter_clear = self.waiter.clear
    self.waiter_switch = lambda: self.waiter.switch(None)
    self.hub = gevent.get_hub()
    self.is_switching = False
    self.reset()
  
  def mark_ready(self, fileno: int, read: bool = True):
    if fileno != self.sock_fileno:
      return
    
    if read:
      self.sock_read_ready = True
    
    else:
      
      self.sock_write_ready = True
    
    self.waiter_switch()
  
  def reset(self):
    self.sock_read_ready = False
    self.sock_write_ready = False
    self.is_switching = False
    self.waiter_clear()
  
  def wait(self):
    return self.waiter_get()
  
  def request_switch(self):
    if self.is_switching:
      return
    
    if self.hub is getcurrent():
      self._request_switch()
    else:
      self.hub.loop.run_callback(self._request_switch)
  
  def _request_switch(self):
    if self.is_switching:
      return
    
    self.is_switching = True
    self.waiter_switch()
  
  def switch_if_write_unblocked(self):
    if not self.sock_write_blocked:
      self.request_switch()
