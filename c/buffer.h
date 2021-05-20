#ifndef TOKU_BUFFER_H__
#define TOKU_BUFFER_H__

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

static inline int toku_buffer_write(toku_buffer_t *pk, const char *bytes, size_t l) {
  char *buf = pk->buf;
  size_t allocated_size = pk->allocated_size;
  size_t length = pk->length;

  if (length + l > allocated_size) {
    // Grow buffer 2x to avoid excessive re-allocations.
    allocated_size = (length + l) * 2;
    buf = (char *)realloc(buf, allocated_size);

    if (!buf)
      return -1;
  }

  memcpy(buf + length, bytes, l);
  length += l;

  pk->buf = buf;
  pk->allocated_size = allocated_size;
  pk->length = length;
  return 0;
}

static inline int toku_buffer_ensure_size(toku_buffer_t *pk, size_t at_least_allocated_size) {
  char *buf = pk->buf;
  size_t allocated_size = pk->allocated_size;

  if (at_least_allocated_size > allocated_size) {
    // Grow buffer to the at-least allocated size.
    buf = (char *)realloc(buf, at_least_allocated_size);

    if (!buf)
      return -1;

    pk->allocated_size = at_least_allocated_size;
    pk->buf = buf;
  }

  return 0;
}

#endif