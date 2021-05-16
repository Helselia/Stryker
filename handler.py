from ctypes import cast, byref, sizeof, create_string_buffer as csb, c_uint8, c_uint16, c_uint32, c_size_t, c_char_p
import opcodes
from toku_c import TokuDecodeBuffer, TokuBuffer, TokuDecoderStatus, TokuOpcodes, toku as toku_c, FromString, AsString, u8, u16, u32
from exceptions import TokuDecoderError, TokuEncoderError

BBS = 1024 * 1024 * 2 # big_buffer_size
IBS = 1024 * 512 # initial_buffer_size
SEQ_MAX = (2 ** 32) - 2

def _reset_buffer(toku_buffer: TokuBuffer):
  ret = toku_c._reset_buffer(byref(TokuBuffer))

  if ret.value == -1:
    raise TokuDecoderError('Unable to allocate buffer')

def _get_payload_from_decode_buffer(decode_buffer: toku_c.TokuDecodeBuffer):
  buf = toku_c._traverse(byref(decode_buffer.toku_buffer.buf), u32(decode_buffer.header_size))
  size = toku_c.toku_get_data_payload_size(decode_buffer)

  if size == 0:
    return None
  
  return FromString(buf, bytearray(size), size)

class TokuStreamHandler:
  def __init__(self):
    self.seq = 0
    self.write_buffer_position = 0
    self.write_buffer = toku_c.TokuBuffer()
    self.decode_buffer = toku_c.TokuDecodeBuffer()

    _reset_buffer(byref(self.write_buffer))
    _reset_buffer(byref(self.decode_buffer.toku_buffer))
  
  def current_seq(self):
    return self.seq
  
  def next_seq(self):
    self.seq += 1
    
    if self.seq >= SEQ_MAX:
      self.seq = 0
    
    return self.seq
  
  def send_ping(self, flags: int) -> int:
    seq = self.next_seq()
    rv = toku_c.toku_append_ping(byref(self.write_buffer), u8(flags), u32(seq))
    if rv.value < 0:
      raise TokuEncoderError()
    
    return seq
  
  def send_pong(self, flags: int, seq: int) -> int:
    rv = toku_c.toku_append_pong(byref(self.write_buffer), u8(flags), u32(seq))
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def send_request(self, flags: int, data: bytearray) -> int:
    seq = self.next_seq()
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)
    
    rv = toku_c.toku_append_request(byref(self.write_buffer), u8(flags), u32(seq), size, buf)
    if rv.value < 0:
      raise TokuEncoderError()
    
    return seq
  
  def send_push(self, flags: int, data: bytearray) -> int:
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)
    
    rv = toku_c.toku_append_push(byref(self.write_buffer), u8(flags), cast())
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def send_hello(self, flags: int, supported_encodings: list, supported_compressors: list) -> int:
    data = bytes(b'%s|%s' % (
        b','.join([bytes(b) for b in supported_encodings]),
        b','.join([bytes(b) for b in supported_compressors])
    ))
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)
    
    rv = toku_c.toku_append_hello(byref(self.write_buffer), u8(flags), size, buf)
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def send_hello_ack(self, flags: int, ping_interval: int, selected_encoding, selected_compressor) -> int:
    data = bytes(b'%s|%s' % (
      selected_encoding,
      selected_compressor or b''
    ))
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)

    rv = toku_c.toku_append_hello_ack(byref(self.write_buffer), u8(flags), u32(ping_interval), size, buf)
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def send_response(self, flags: int, seq: int, data: bytearray) -> int:
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)

    rv = toku_c.toku_append_response(byref(self.write_buffer), u8(flags), u32(seq), size, buf)
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def send_error(self, flags: int, code: int, seq: int, reason: bytearray = None) -> int:
    rv = 0
    size = c_size_t(0)
    buf = cast(csb(0), c_char_p)

    if reason:
      rv = AsString(reason, buf)
      if rv.value < 0:
        raise TypeError()
    
    rv = toku_c.toku_append_goaway(byref(self.write_buffer), u8(flags), u16(code), size, buf)
    if rv.value < 0:
      raise TokuEncoderError()
    
    return 1
  
  def write_buffer_len(self):
    return self.write_buffer.length.value - self.write_buffer_position
  
  def write_buffer_get_bytes(self, length: int, consume: bool = True):
    buffer_len_remaining = self.write_buffer_len()
    if length > buffer_len_remaining:
      length = buffer_len_remaining
    
    if length == 0:
      return None
    
    write_buffer = FromString(toku_c._traverse(self.write_buffer.buf, u32(self.write_buffer_position)))
    if consume:
      self.write_buffer_position += length
      self._reset_or_compact_write_buf()
    
    return write_buffer
  
  def write_buffer_consume_bytes(self, length: int):
    buffer_len_remaining = self.write_buffer_len()
    if length > buffer_len_remaining:
      length = buffer_len_remaining
    
    self.write_buffer_position += length
    self._reset_or_compact_write_buf()
    return buffer_len_remaining - length
  
  def on_bytes_received(self, data: bytearray):
    size = c_size_t(sizeof(data))
    buf = cast(csb(0), c_char_p)
    AsString(data, buf)
    consumed = c_size_t(0)
    decoder_status = 0

    received_payloads = []
    while size > 0:
      decoder_status = u32(toku_c.toku_decoder_read_data(byref(self.decode_buffer), size, buf, byref(consumed)))
      if decoder_status.value < 0:
        self._reset_decode_buf()
        raise TokuDecoderError('The decoder failed with status %s' % decoder_status.value)
      
      if decoder_status.value == TokuDecoderStatus.TOKU_DECODE_NEEDS_MORE:
        break

      elif decoder_status.value == TokuDecoderStatus.TOKU_DECODE_COMPLETE:
        received_payloads.append(self._consume_decode_buffer())
      
      else:
        raise TokuDecoderError('Unhandled decoder status %s' % decoder_status.value)
      
      size = c_size_t(size.value - consumed.value)
      buf = toku_c._traverse(buf, u32(consumed))
      consumed = c_size_t(0)
    
    return received_payloads
  
  def _consume_decode_buffer(self):
    opcode = self.decode_buffer.opcode.value
    response = opcodes.Empty()

    if opcode == TokuOpcodes.TOKU_OP_RESPONSE:
      response = opcodes.Response(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value,
        _get_payload_from_decode_buffer(self.decode_buffer)
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_REQUEST:
      response = opcodes.Request(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value,
        _get_payload_from_decode_buffer(self.decode_buffer)
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_PUSH:
      response = opcodes.Push(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        _get_payload_from_decode_buffer(self.decode_buffer)
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_PING:
      response = opcodes.Ping(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_PONG:
      response = opcodes.Pong(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_GOAWAY:
      response = opcodes.GoAway(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value,
        _get_payload_from_decode_buffer(self.decode_buffer)
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_HELLO:
      payload = _get_payload_from_decode_buffer(self.decode_buffer)
      supported_encodings, supported_compressions = payload.split(b'|')

      response = opcodes.Hello(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value,
        supported_encodings.split(b','),
        supported_compressions.split(',')
      )

    elif opcode == TokuOpcodes.TOKU_OP_HELLO_ACK:
      payload = _get_payload_from_decode_buffer(self.decode_buffer)
      selected_encoding, selected_compressor = payload.split(b'|')

      response = opcodes.HelloAck(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_ping_interval(byref(self.decode_buffer)).value,
        selected_encoding,
        selected_compressor
      )
    
    elif opcode == TokuOpcodes.TOKU_OP_ERROR:
      response = opcodes.Error(
        toku_c.toku_get_flags(byref(self.decode_buffer)).value,
        toku_c.toku_get_code(byref(self.decode_buffer)).value,
        toku_c.toku_get_seq(byref(self.decode_buffer)).value,
        _get_payload_from_decode_buffer(self.decode_buffer)
      )
    
    self._reset_decode_buf()
    return response
  
  def _reset_decode_buf(self):
    toku_c.toku_decoder_reset(byref(self.decode_buffer))
    toku_c._free_big_buffer(byref(self.decode_buffer.toku_buffer))
    _reset_buffer(self.decode_buffer.toku_buffer)
  
  def _reset_or_compact_write_buf(self):
    if self.write_buffer_position == self.write_buffer.length.value:
      toku_c._free_big_buffer(byref(self.write_buffer))
      _reset_buffer(self.write_buffer)
      self.write_buffer_position = 0
    
    elif self.write_buffer.length.value > self.write_buffer_position > self.write_buffer.allocated_size.value / 2:
      toku_c._compact_write_buffer(byref(self.write_buffer), u32(self.write_buffer_position))

      self.write_buffer.length = c_size_t(self.write_buffer.length.value - self.write_buffer_position)
      self.write_buffer_position = 0
