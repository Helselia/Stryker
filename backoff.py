import random

class Backoff:
  def __init__(self, min_delay=0.5, max_delay=None, jitter=True):
    self._min = min_delay
    if max_delay is None:
      max_delay = min_delay * 10
    
    self._max = max_delay
    self._jitter = jitter

    self._current = self._min
    self._fails = 0
  
  def fails(self) -> int:
    return self._fails
  
  def current(self) -> float:
    return self._current
  
  def succeed(self):
    self._fails = 0
    self._current = self._min
  
  def fail(self) -> float:
    self._fails += 1
    delay = self._current * 2
    if self._jitter:
      delay *= random.random()
    
    self._current += delay

    if self._max:
      self._current = min(self._current, self._max)
    
    self._current = round(self._current, 2)
    return self._current
