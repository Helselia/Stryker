from cpython cimport *
from libc.stdlib cimport malloc, free
from libc.string cimport memcpy
from libc.stdint cimport uint32_t, uint8_t, uint16_t

from exceptions import TokuDecoderError
cimport toku_c
cimport opcodes

cdef size_t BBS = 1024 * 1024 * 2 # BIG_BUF_SIZE
cdef size_t IBS = 1024 * 512 # INITIAL_BUFFER_SIZE
# TODO: Move to header
cdef uint32_t SEQ_MAX = (2 ** 32) - 2

cdef inline void _reset_buffer(toku_c.toku_buffer_t *toku_buffer):
  """
  Ensures the buffer is in a reset state.
  """
  if toku_buffer.buf != NULL:
    toku_buffer.length = 0
  
  else:
    toku_buffer.buf = <char*> malloc(IBS)
    if toku_buffer.buf == NULL:
      raise MemoryError('Unable to allocate buffer')
    
    toku_buffer.allocated_size = IBS
    toku_buffer.length = 0
  

cdef inline void _free_big_buffer(toku_c.toku_buffer_t *toku_buffer):
  """
  Frees the buffer if it's grown over `BIG_BUF_SIZE`
  """
  if toku_buffer.allocated_size >= BBS:
    free(toku_buffer.buf)
    toku_buffer.buf = NULL
    toku_buffer.length = 0
    toku_buffer.allocated_size = 0
  
  else:
    toku_buffer.length = 0


cdef inline bytes _get_payload_from_decode_buffer(toku_c.toku_decode_buffer_t *decode_buffer):
  """
  Get the payload part of the decode buffer. Copies the data after the headers in the buffer into a PyBytes object.
  """
  cdef char *buf = decode_buffer.toku_buffer.buf + decode_buffer.header_size
  cdef size_t size = toku_c.toku_get_data_payload_size(decode_buffer)
  if size == 0:
    return None
  
  return PyBytes_FromStringAndSize(buf, size)

cdef class TokuStreamHandler:
  def __cinit__(self):
    self.seq = 0
    self.write_buffer_position = 0

    _reset_buffer(&self.write_buffer)
    _reset_buffer(&self.decode_buffer.toku_buffer)

  def __dealloc__(self):
    if self.write_buffer.buf != NULL:
      free(self.write_buffer.buf)
    
    if self.decode_buffer.toku_buffer.buf != NULL:
      free(self.decode_buffer.toku_buffer.buf)

  cpdef uint32_t current_seq(self):
    return self.seq

  cdef inline uint32_t next_seq(self):
    cdef uint32_t next_seq
    self.seq += 1
    next_seq = self.seq
    if next_seq >= SEQ_MAX:
      self.seq = 0
      next_seq = 0
    
    return next_seq

  cpdef uint32_t send_ping(self, uint8_t flags) except 0:
    cdef int rv
    cdef uint32_t seq = self.next_seq()
    rv = toku_c.toku_append_ping(&self.write_buffer, flags, seq)
    if rv < 0:
      raise MemoryError()
    
    return seq

  cpdef uint32_t send_pong(self, uint8_t flags, uint32_t seq) except 0:
    cdef int rv
    rv = toku_c.toku_append_pong(&self.write_buffer, flags, seq)
    if rv < 0:
      raise MemoryError()
    
    return 1

  cpdef uint32_t send_request(self, uint8_t flags, bytes data) except 0:
    cdef int rv
    cdef uint32_t seq = self.next_seq()
    cdef char *buffer
    cdef size_t size

    rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
    if rv < 0:
      raise TypeError()
    
    rv = toku_c.toku_append_request(&self.write_buffer, flags, seq, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
    
    return seq

  cpdef uint32_t send_push(self, uint8_t flags, bytes data) except 0:
    cdef int rv
    cdef char *buffer
    cdef size_t size

    rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
    if rv < 0:
      raise TypeError()
    
    rv = toku_c.toku_append_push(&self.write_buffer, flags, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()

    return 1

  cpdef uint32_t send_hello(self, uint8_t flags, list supported_encodings, list supported_compressors) except 0:
    cdef int rv
    cdef char *buffer
    cdef size_t size
    cdef bytes data = bytes(b'%s|%s' % (
      b','.join([bytes(b) for b in supported_encodings]),
      b','.join([bytes(b) for b in supported_compressors])
    ))

    rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
    if rv < 0:
      raise TypeError()
    
    rv = toku_c.toku_append_hello(&self.write_buffer, flags, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
    
    return 1

  cpdef uint32_t send_hello_ack(self, uint8_t flags, uint32_t ping_interval, bytes selected_encoding, bytes selected_compressor) except 0:
    cdef int rv
    cdef char *buffer
    cdef size_t size
    cdef bytes data = bytes(b'%s|%s' % (
      selected_encoding,
      selected_compressor or ''
    ))

    rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
    if rv < 0:
      raise MemoryError()
    
    rv = toku_c.toku_append_hello_ack(&self.write_buffer, flags, ping_interval, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
    
    return 1

  cpdef uint32_t send_response(self, uint8_t flags, uint32_t seq, bytes data) except 0:
    cdef int rv
    cdef char *buffer
    cdef size_t size

    rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
    if rv < 0:
      raise TypeError()
    
    rv = toku_c.toku_append_response(&self.write_buffer, flags, seq, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
  
    return 1

  cpdef uint32_t send_error(self, uint8_t flags, uint16_t code, uint32_t seq, bytes data) except 0:
    cdef int rv
    cdef char *buffer = NULL
    cdef size_t size = 0

    if data:
      rv = PyBytes_AsStringAndSize(data, &buffer, <Py_ssize_t*> &size)
      if rv < 0:
        raise TypeError()
      
    rv = toku_c.toku_append_error(&self.write_buffer, flags, code, seq, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
    
    return 1

  cpdef uint32_t send_goaway(self, uint8_t flags, uint16_t code, bytes reason) except 0:
    cdef int rv
    cdef char *buffer = NULL
    cdef size_t size = 0

    if reason:
      rv = PyBytes_AsStringAndSize(reason, &buffer, <Py_ssize_t*> &size)
      if rv < 0:
        raise TypeError()
      
    rv = toku_c.toku_append_goaway(&self.write_buffer, flags, code, size, <const char*> buffer)
    if rv < 0:
      raise MemoryError()
    
    return 1

  cpdef size_t write_buffer_len(self):
    return self.write_buffer.length - self.write_buffer_position

  cpdef bytes write_buffer_get_bytes(self, size_t length, bint consume=True):
    cdef size_t buffer_len_remaining = self.write_buffer_len()
    if length > buffer_len_remaining:
      length = buffer_len_remaining
    
    if length == 0:
      return None
    
    cdef bytes write_buffer = PyBytes_FromStringAndSize(self.write_buffer.buf + self.write_buffer_position, length)
    if consume:
      self.write_buffer_position += length
      self._reset_or_compact_write_buf()
    
    return write_buffer

  cpdef size_t write_buffer_consume_bytes(self, size_t length):
    cdef size_t buffer_len_remaining = self.write_buffer_len()
    if length > buffer_len_remaining:
      length = buffer_len_remaining
    
    self.write_buffer_position += length
    self._reset_or_compact_write_buf()
    return buffer_len_remaining - length

  cpdef list on_bytes_received(self, bytes data):
    cdef size_t size = PyBytes_Size(data)
    cdef char *buf = PyBytes_AS_STRING(data)
    cdef size_t consumed = 0
    cdef toku_c.toku_decoder_status decoder_status

    cdef list received_payloads = []
    while size > 0:
      decoder_status = toku_c.toku_decoder_read_data(&self.decode_buffer, size, buf, &consumed)
      if decoder_status < 0:
        self._reset_decode_buf()
        raise TokuDecoderError('The decoder failed with status %s' &decoder_status)
      
      if decoder_status == toku_c.TOKU_DECODE_NEEDS_MORE:
        break
      
      elif decoder_status == toku_c.TOKU_DECODE_COMPLETE:
        received_payloads.append(self._consume_decode_buffer())
      
      else:
        raise TokuDecoderError('Unhandled decoder status %s' % decoder_status)
      
      size -= consumed
      buf += consumed
      consumed = 0
    
    return received_payloads
  
  cdef object _consume_decode_buffer(self):
    cdef uint8_t opcode = self.decode_buffer.opcode
    cdef object response

    if opcode == toku_c.TOKU_OP_RESPONSE:
      response = opcodes.Response(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer),
        _get_payload_from_decode_buffer(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_REQUEST:
      response = opcodes.Request(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer),
        _get_payload_from_decode_buffer(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_PUSH:
      response = opcodes.Push(
        toku_c.toku_get_flags(&self.decode_buffer),
        _get_payload_from_decode_buffer(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_PING:
      response = opcodes.Ping(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_PONG:
      response = opcodes.Pong(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_GOAWAY:
      response = opcodes.GoAway(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer),
        _get_payload_from_decode_buffer(&self.decode_buffer)
      )
    
    elif opcode == toku_c.TOKU_OP_HELLO:
      payload = _get_payload_from_decode_buffer(&self.decode_buffer)
      supported_encodings, supported_compressions = payload.split(b'|')

      response = opcodes.Hello(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_flags(&self.decode_buffer),
        supported_encodings.split(b'|'),
        supported_compressions.split(b'|')
      )
    
    elif opcode == toku_c.TOKU_OP_HELLO_ACK:
      payload = _get_payload_from_decode_buffer(&self.decode_buffer)
      selected_encoding, selected_compressor = payload.split(b'|')

      response = opcodes.HelloAck(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_ping_interval(&self.decode_buffer),
        selected_encoding,
        selected_compressor
      )

    elif opcode == toku_c.TOKU_OP_ERROR:
      response = opcodes.Error(
        toku_c.toku_get_flags(&self.decode_buffer),
        toku_c.toku_get_code(&self.decode_buffer),
        toku_c.toku_get_seq(&self.decode_buffer),
        _get_payload_from_decode_buffer(&self.decode_buffer)
      )
    
    self._reset_decode_buf()
    return response

  cdef inline _reset_decode_buf(self):
    toku_c.toku_decoder_reset(&self.decode_buffer)
    _free_big_buffer(&self.decode_buffer.toku_buffer)
    _reset_buffer(&self.decode_buffer.toku_buffer)

  cdef _reset_or_compact_write_buf(self):
    if self.write_buffer_position == self.write_buffer.length:
      _free_big_buffer(&self.write_buffer)
      _reset_buffer(&self.write_buffer)
      self.write_buffer_position = 0
    
    elif self.write_buffer.length > self.write_buffer_position > self.write_buffer.allocated_size / 2:
      memcpy(
        self.write_buffer.buf,
        self.write_buffer.buf + self.write_buffer_position,
        self.write_buffer.length - self.write_buffer_position
      )

      self.write_buffer.length -= self.write_buffer_position
      self.write_buffer_position = 0

