#ifndef TOKU_BUFFER
#define TOKU_BUFFER

#include "sysdep.h"

typedef struct {
  char *buf;
  size_t length;
  size_t allocated_size;
} toku_buffer_t;

typedef struct {
  toku_buffer_t toku_buffer;
  uint8_t opcode;
  size_t data_size_remaining;
  size_t header_size;
  uint8_t decode_complete;
} toku_decode_buffer_t;

static inline int toku_buffer_write(toku_buffer_t *pk, const char *bytes, size_t len) {
  char *buf = pk->buf;
  size_t alloc_size = pk->allocated_size;
  size_t length = pk->length;

  if(length + len > alloc_size) {
    alloc_size = (length + len) * 2;
    buf = (char*)realloc(buf, alloc_size);

    if(!buf)
      return -1;
  }

  memcpy(buf + length, bytes, len);
  length += len;

  pk->buf = buf;
  pk->allocated_size = alloc_size;
  pk->length = length;
  return 0;
}

static inline int toku_buffer_ensure_size(toku_buffer_t *pk, size_t at_least) {
  char *buf = pk->buf;
  size_t s = pk->allocated_size;

  if(at_least > s) {
    buf = (char*)realloc(buf, at_least);

    if(!buf)
      return -1;
    
    pk->allocated_size = at_least;
    pk->buf = buf;
  }

  return 0;
}

#endif
