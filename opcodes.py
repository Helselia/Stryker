class Empty:
  pass

class Response:
  def __init__(self, flags, seq, data):
    self.flags = flags
    self.seq = seq
    self.data = data

class Request:
  def __init__(self, flags, seq, data):
    self.flags = flags
    self.seq = seq
    self.data = data

class Push:
  def __init__(self, flags, data):
    self.flags = flags
    self.data = data

class Ping:
  def __init__(self, flags, seq):
    self.flags = flags
    self.seq = seq

class Pong:
  def __init__(self, flags, seq):
    self.flags = flags
    self.seq = seq

class Hello:
  def __init__(self, flags, version, supported_encodings, supported_compressors):
    self.flags = flags
    self.version = version
    self.supported_encodings = supported_encodings
    self.supported_compressions = supported_compressors

class HelloAck:
  def __init__(self, flags, ping_interval, selected_encoding, selected_compression):
    self.flags = flags
    self.ping_interval = ping_interval
    self.selected_encoding = selected_encoding
    self.selected_compresion = selected_compression

class GoAway:
  def __init__(self, flags, code, reason):
    self.flags = flags
    self.code = code
    self.reason = reason

class Error:
  def __init__(self, flags, code, seq, data):
    self.flags = flags
    self.code = code
    self.seq = seq
    self.data = data
