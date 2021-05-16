from ctypes import CDLL, Structure, c_char_p, c_size_t, c_uint16, c_uint32, c_uint8, cast, create_string_buffer

toku = CDLL("libtoku_c.so")

class TokuBuffer(Structure):
  _fields_ = [
    ("buf", c_char_p),
    ("length", c_size_t),
    ("allocated_size", c_size_t)
  ]

  def __init__(self):
    self.buf = cast(create_string_buffer(0), c_char_p)
    self.length = cast(0, c_size_t)
    self.allocated_size = cast(0, c_size_t)

class TokuDecodeBuffer(Structure):
  _fields_ = [
    ("toku_buffer", TokuBuffer),
    ("opcode", c_uint8),
    ("data_size_remaining", c_uint8),
    ("header_size", c_uint8),
    ("decode_complete", c_uint8)
  ]

  def __init__(self):
    self.toku_buffer = TokuBuffer()
    self.opcode = cast(0, c_uint8)
    self.data_size_remaining = cast(0, c_uint8)
    self.header_size = cast(0, c_uint8)
    self.decode_complete = cast(0, c_uint8)

class TokuOpcodes:
  TOKU_OP_HELLO = 0
  TOKU_OP_HELLO_ACK = 1
  TOKU_OP_PING = 2
  TOKU_OP_PONG = 3
  TOKU_OP_REQUEST = 4
  TOKU_OP_RESPONSE = 5
  TOKU_OP_PUSH = 6
  TOKU_OP_GOAWAY = 7
  TOKU_OP_ERROR = 9

class TokuDecoderStatus:
  TOKU_DECODE_NEEDS_MORE = 1
  TOKU_DECODE_COMPLETE = 2
  TOKU_DECODE_MEMORY_ERROR = -1
  TOKU_DECODE_INVALID_OPCODE = -2
  TOKU_DECODE_INVALID_SIZE = -3

class TokuFlags:
  TOKU_FLAG_COMPRESSED = 1 << 0

def u8(v: int) -> c_uint8:
  cast(v, c_uint8)

def u16(v: int) -> c_uint16:
  cast(v, c_uint16)

def u32(v: int) -> c_uint32:
  cast(v, c_uint32)

def FromString(data: c_char_p, buf: bytearray, size: int):
  for i in range(size):
    buf[i] = data[i]

def AsString(data: bytearray, buf: c_char_p):
  for i in range(len(data)):
    buf[i] = data[i]
