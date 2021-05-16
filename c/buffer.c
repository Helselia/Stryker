#include "common.h"

#ifndef TOKU_BUFFER_H__
#define TOKU_BUFFER_H__

int toku_buffer_write(toku_buffer *pk, const char *bytes, size_t l) {
  char *buf = pk->buf;
  size_t allocated_size = pk->allocated_size;
  size_t length = pk->length;

  if(length + l > allocated_size) {
    allocated_size = (length + l) * 2;
    buf = (char*)realloc(buf, allocated_size);

    if(!buf)
      return -1;
  }

  memcpy(buf + length, bytes, l);
  length += l;

  pk->buf = buf;
  pk->allocated_size = allocated_size;
  pk->length = length;

  return 0;
}

int toku_buffer_ensure_size(toku_buffer *pk, size_t at_least) {
  char *buf = pk->buf;
  size_t allocated_size = pk->allocated_size;

  if(at_least > allocated_size) {
    buf = (char*)realloc(buf, at_least);

    if(!buf)
      return -1;
    
    pk->allocated_size = at_least;
    pk->buf = buf;
  }

  return 0;
}

int _reset_buffer(toku_buffer *pk) {
  if(pk->buf != NULL)
    pk->length = 0;
  else {
    pk->buf = (char*)malloc(1024 * 512);
    
    if(pk->buf == NULL)
      return -1;
    
    pk->allocated_size = 1024 * 512;
    pk->length = 0;
  }

  return 0;
}

int _free_big_buffer(toku_buffer *pk) {
  if(pk->allocated_size >= 1024 * 1024 * 2) {
    free(pk->buf);
    pk->buf = NULL;
    pk->length = 0;
    pk->allocated_size = 0;
  } else
    pk->length = 0;
  
  return 0;
}

// SUPER FRICKIN USEFUL :02Dead:
void* _traverse(void *buf, uint32_t traversal_length) {
  return buf + traversal_length;
}

void _compact_write_buffer(toku_buffer *buf, uint32_t pos) {
  memcpy(
    buf->buf,
    buf->buf + pos,
    buf->length - pos
  );
}

#endif
