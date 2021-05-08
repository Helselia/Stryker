#include <stddef.h>
#include <stdlib.h>
#include "sysdep.h"
#include "constants.h"
#include "buffer.h"
#include <limits.h>
#include <string.h>

#ifndef TOKU_ENCODER_H
#define TOKU_ENCODER_H

#define toku_append(pk, buf, len) toku_buffer_write(pk, (const char *)buf, len)

static inline int toku_append_hello(toku_buffer_t *b, uint8_t flags, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_HELLO;
  buf[1] = flags;
  buf[2] = TOKU_VERSION;
  _toku_store32(buf + 3, size);
  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_hello_ack(toku_buffer_t *b, uint8_t flags, uint32_t ping_interval, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_HELLO_ACK;
  buf[1] = flags;
  _toku_store32(buf + 2, ping_interval);
  _toku_store32(buf + 6, size);
  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_ping(toku_buffer_t *b, uint8_t flags, uint32_t seq) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_PING;
  buf[1] = flags;
  _toku_store32(buf + 2, seq);
  return toku_append(b, buf, SIZE);
  #undef SIZE
}

static inline int toku_append_pong(toku_buffer_t *b, uint8_t flags, uint32_t seq) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_PONG;
  buf[1] = flags;
  _toku_store32(buf + 2, seq);
  return toku_append(b, buf, SIZE);
  #undef SIZE
}

static inline int toku_append_request(toku_buffer_t *b, uint8_t flags, uint32_t seq, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_REQUEST;
  buf[1] = flags;
  _toku_store32(buf + 2, seq);
  _toku_store32(buf + 6, size);

  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_response(toku_buffer_t *b, uint8_t flags, uint32_t seq, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_RESPONSE;
  buf[1] = flags;
  _toku_store32(buf + 2, seq);
  _toku_store32(buf + 6, size);

  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_push(toku_buffer_t *b, uint8_t flags, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_PUSH;
  buf[1] = flags;
  _toku_store32(buf + 2, size);

  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_goaway(toku_buffer_t *b, uint8_t flags, uint16_t code, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_GOAWAY;
  buf[1] = flags;
  _toku_store16(buf + 2, code);
  _toku_store32(buf + 4, size);

  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  return toku_append(b, data, size);
}

static inline int toku_append_error(toku_buffer_t *b, uint8_t flags, uint16_t code, uint32_t seq, uint32_t size, const char* data) {
  #define SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t)
  unsigned char buf[SIZE];
  buf[0] = TOKU_OP_ERROR;
  buf[1] = flags;
  _toku_store32(buf + 2, seq);
  _toku_store16(buf + 6, code);
  _toku_store32(buf + 8, size);

  int ret = toku_buffer_write(b, (const char*) buf, SIZE);
  #undef SIZE

  if (ret < 0)
    return ret;

  if (size == 0)
    return 0;

  return toku_append(b, data, size);
}

#undef toku_append

#endif
