#include "common.h"

#ifndef TOKU_DECODER_H__
#define TOKU_DECODER_H__

size_t _toku_get_header_size(uint8_t opcode) {
  switch(opcode) {
    case TOKU_OP_HELLO:
      return sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t);
    case TOKU_OP_HELLO_ACK:
      return sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t);
    case TOKU_OP_ERROR:
      return sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t);
    case TOKU_OP_PING:
    case TOKU_OP_PONG:
      return sizeof(uint8_t) + sizeof(uint32_t);
    case TOKU_OP_REQUEST:
    case TOKU_OP_RESPONSE:
      return sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t);
    case TOKU_OP_PUSH:
      return sizeof(uint8_t) + sizeof(uint32_t);
    case TOKU_OP_GOAWAY:
      return sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
    default:
      return 0;
  }
}

uint8_t toku_get_flags(toku_decode_buffer *pk) {
  return (uint8_t)pk->toku_buffer.buf[0];
}

size_t toku_get_data_payload_size(toku_decode_buffer *pk) {
  switch(pk->opcode) {
    case TOKU_OP_REQUEST:
    case TOKU_OP_RESPONSE:
    case TOKU_OP_HELLO_ACK:
      return _toku_load32(size_t, pk->toku_buffer.buf + sizeof(uint8_t) + sizeof(uint32_t));
    case TOKU_OP_PUSH:
      return _toku_load32(size_t, pk->toku_buffer.buf + sizeof(uint8_t));
    case TOKU_OP_GOAWAY:
      return _toku_load32(size_t, pk->toku_buffer.buf + sizeof(uint8_t) + sizeof(uint16_t));
    case TOKU_OP_ERROR:
      return _toku_load32(size_t, pk->toku_buffer.buf + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t));
    case TOKU_OP_HELLO:
      return _toku_load32(size_t, pk->toku_buffer.buf + sizeof(uint8_t) + sizeof(uint8_t));
    default:
      return 0;
  }
}

uint32_t toku_get_seq(toku_decode_buffer *pk) {
  switch(pk->opcode) {
    case TOKU_OP_REQUEST:
    case TOKU_OP_RESPONSE:
    case TOKU_OP_PING:
    case TOKU_OP_PONG:
    case TOKU_OP_ERROR:
      return _toku_load32(uint32_t, pk->toku_buffer.buf + sizeof(uint8_t));
    default:
      return 0;
  }
}

uint8_t toku_get_version(toku_decode_buffer *pk) {
  switch(pk->opcode) {
    case TOKU_OP_HELLO:
      return (uint8_t)pk->toku_buffer.buf[sizeof(uint8_t)];
    default:
      return 0;
  }
}

uint16_t toku_get_code(toku_decode_buffer *pk) {
  switch(pk->opcode) {
    case TOKU_OP_GOAWAY:
      return _toku_load16(uint16_t, pk->toku_buffer.buf + sizeof(uint8_t));
    case TOKU_OP_ERROR:
      return _toku_load16(uint16_t, pk->toku_buffer.buf + sizeof(uint8_t) + sizeof(uint32_t));
    default:
      return 0;
  }
}

uint32_t toku_get_ping_interval(toku_decode_buffer *pk) {
  switch(pk->opcode) {
    case TOKU_OP_HELLO_ACK:
      return _toku_load32(uint32_t, pk->toku_buffer.buf + sizeof(uint8_t));
    default:
      return 0;
  }
}

void toku_decoder_reset(toku_decode_buffer *pk) {
  pk->opcode = 0;
  pk->data_size_remaining = 0;
  pk->decode_complete = 0;
}

toku_decoder_status toku_read_append_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed) {
  if(pk->opcode == 0) {
    return TOKU_DECODE_INVALID_OPCODE;
  }

  int rv;

  if(pk->toku_buffer.length < pk->header_size) {
    size_t header_chunk_to_read = pk->header_size - pk->toku_buffer.length;
    if(header_chunk_to_read > size) {
      header_chunk_to_read = size;
    }

    rv = toku_buffer_write(&pk->toku_buffer, data, header_chunk_to_read);
    if(rv < 0) {
      return TOKU_DECODE_MEMORY_ERROR;
    }

    *consumed += header_chunk_to_read;

    if(pk->toku_buffer.length < pk->header_size) {
      return TOKU_DECODE_NEEDS_MORE;
    }

    pk->data_size_remaining = toku_get_data_payload_size(pk);

    if(pk->data_size_remaining > 0) {
      rv = toku_buffer_ensure_size(&pk->toku_buffer, pk->header_size + pk->data_size_remaining);
      if(rv < 0) {
        return TOKU_DECODE_MEMORY_ERROR;
      }
    }

    data = data + header_chunk_to_read;
    size -= header_chunk_to_read;
  }

  size_t bytes_to_consume = pk->data_size_remaining;

  if(bytes_to_consume > size) {
    bytes_to_consume = size;
  }

  if(bytes_to_consume > 0) {
    rv = toku_buffer_write(&pk->toku_buffer, data, bytes_to_consume);
    if(rv < 0) {
      return TOKU_DECODE_MEMORY_ERROR;
    }

    pk->data_size_remaining -= bytes_to_consume;
    *consumed += bytes_to_consume;
  }

  if(pk->data_size_remaining == 0) {
    pk->decode_complete = 1;
    return TOKU_DECODE_COMPLETE;
  } else {
    return TOKU_DECODE_NEEDS_MORE;
  }
}

toku_decoder_status toku_read_new_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed) {
  uint8_t opcode = (uint8_t)data[0];
  size_t header_size = _toku_get_header_size(opcode);
  if(header_size == 0) {
    return TOKU_DECODE_INVALID_OPCODE;
  }

  *consumed += 1;
  data = data + 1;
  size -= 1;

  pk->toku_buffer.length = 0;
  pk->data_size_remaining = 0;
  pk->opcode = opcode;
  pk->header_size = header_size;

  if(size > 0) {
    return toku_read_append_data(pk, size, data, consumed);
  }

  return TOKU_DECODE_NEEDS_MORE;
}

toku_decoder_status toku_decoder_read_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed) {
  if(pk->decode_complete == 1) {
    return TOKU_DECODE_COMPLETE;
  } else if(pk->opcode == 0) {
    return toku_read_new_data(pk, size, data, consumed);
  } else {
    return toku_read_append_data(pk, size, data, consumed);
  }
}

#endif