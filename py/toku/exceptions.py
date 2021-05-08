class TokuException(Exception):
  pass

class TokuDecoderError(TokuException):
  pass

class NoEncoderAvailable(TokuException):
  pass

class ConnectionError(TokuException):
  pass

class StreamDefunct(TokuException):
  pass

class ConnectionTerminated(ConnectionError):
  pass

class ConnectionPingTimeout(ConnectionError):
  pass

class InvalidSendException(TokuException):
  pass

class NotClientException(InvalidSendException):
  pass

class NotServerException(InvalidSendException):
  pass

class TokuErrorReceived(TokuException):
  pass
