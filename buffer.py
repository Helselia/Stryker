import ctypes

class TokuBuffer(ctypes.BigEndianStructure):
  _fields_ = [
    ("length", ctypes.c_size_t),
    ("allocated_size", ctypes.c_size_t),
    ("buf", ctypes.c_char * length) # type: ignore
  ]

class TokuDecodeBuffer(ctypes.BigEndianStructure):
  _fields_ = [
    ("toku_buffer", TokuBuffer),
    ("opcode", ctypes.c_uint8),
    ("data_size_remaining", ctypes.c_size_t),
    ("header_size", ctypes.c_size_t),
    ("decode_complete", ctypes.c_uint8)
  ]

def toku_buffer_write(pk: TokuBuffer, b: ctypes.c_char_p, l: ctypes.c_size_t) -> ctypes.c_int:
  buf = pk.buf
  allocated_size = pk.allocated_size
  length = pk.length

  if length + l > allocated_size:
    allocated_size = (length + l) * 2
    buf = ctypes.c_char_p(ctypes.resize(buf, allocated_size))

    if not buf:
      return -1
    
  buf[length:length+l] = ctypes.c_char_p()

