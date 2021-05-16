#ifndef TOKU_COMMON_H__
#define TOKU_COMMON_H__

#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#elif defined(_MSC_VER)
#include <stdint.h>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#if defined(__linux__)
#include <endian.h>
#endif

#ifdef _WIN32

#ifdef __cplusplus
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

#else
#include <arpa/inet.h>
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN__
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __BIG_ENDIAN__
#elif _WIN32
#define __LITTLE_ENDIAN
#endif
#endif

#ifdef __LITTLE_ENDIAN__

#ifdef _WIN32
#if defined(ntohs)
#define _toku_be16(x) ntohs(x)
#elif defined(_byteswap_ushort) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#define _toku_be16(x) ((uint16_t)_byteswap_ushort((unsigned short)x))
#else
#define _toku_be16(x) (      \
    ((((uint16_t)x) << 8)) | \
    ((((uint16_t)x) >> 8)))
#endif
#else
#define _toku_be16(x) ntohs(x)
#endif

#ifdef _WIN32
#if defined(ntohl)
#define _toku_be32(x) ntohl(x)
#elif defined(_byteswap_ulong) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#define _toku_be32(x) ((uint32_t)_byteswap_ulong((unsigned long)x))
#else
#define _toku_be32(x)                     \
  (((((uint32_t)x) << 24)) |              \
   ((((uint32_t)x) << 8) & 0x00ff0000U) | \
   ((((uint32_t)x) >> 8) & 0x0000ff00U) | \
   ((((uint32_t)x) >> 24)))
#endif
#else
#define _toku_be32(x) ntohl(x)
#endif

#if defined(_byteswap_uint64) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#define _toku_be64(x) (_byteswap_uint64(x))
#elif defined(bswap_64)
#define _toku_be64(x) bswap_64(x)
#elif defined(__DARWIN_OSSwapInt64)
#define _toku_be64(x) __DARWIN_OSSwapInt64(x)
#elif defined(__linux__)
#define _toku_be64(x) be64toh(x)
#else
#define _toku_be64(x)                                \
  (((((uint64_t)x) << 56)) |                         \
   ((((uint64_t)x) << 40) & 0x00ff000000000000ULL) | \
   ((((uint64_t)x) << 24) & 0x0000ff0000000000ULL) | \
   ((((uint64_t)x) << 8) & 0x000000ff00000000ULL) |  \
   ((((uint64_t)x) >> 8) & 0x00000000ff000000ULL) |  \
   ((((uint64_t)x) >> 24) & 0x0000000000ff0000ULL) | \
   ((((uint64_t)x) >> 40) & 0x000000000000ff00ULL) | \
   ((((uint64_t)x) >> 56)))
#endif

#define _toku_load16(cast, from) ((cast)((((uint16_t)((uint8_t *)(from))[0]) << 8) | \
                                         (((uint16_t)((uint8_t *)(from))[1]))))

#define _toku_load32(cast, from) ((cast)((((uint32_t)((uint8_t *)(from))[0]) << 24) | \
                                         (((uint32_t)((uint8_t *)(from))[1]) << 16) | \
                                         (((uint32_t)((uint8_t *)(from))[2]) << 8) |  \
                                         (((uint32_t)((uint8_t *)(from))[3]))))

#else
#define _toku_be16(x) (x)
#define _toku_be32(x) (x)
#define _toku_be64(x) (x)

#define _toku_load16(cast, from) ((cast)((((uint16_t)((uint8_t *)(from))[0]) << 8) | \
                                         (((uint16_t)((uint8_t *)(from))[1]))))

#define _toku_load32(cast, from) ((cast)((((uint32_t)((uint8_t *)(from))[0]) << 24) | \
                                         (((uint32_t)((uint8_t *)(from))[1]) << 16) | \
                                         (((uint32_t)((uint8_t *)(from))[2]) << 8) |  \
                                         (((uint32_t)((uint8_t *)(from))[3]))))

#endif

#define _toku_store16(to, num)      \
  do                                \
  {                                 \
    uint16_t val = _toku_be16(num); \
    memcpy(to, &val, 2);            \
  } while (0)
#define _toku_store32(to, num)      \
  do                                \
  {                                 \
    uint32_t val = _toku_be32(num); \
    memcpy(to, &val, 4);            \
  } while (0)

static const unsigned char TOKU_VERSION = 1;
static const size_t TOKU_DATA_SIZE_MAX = 1024 * 1024 * 50;

typedef struct {
  char *buf;
  size_t length;
  size_t allocated_size;
} toku_buffer;

typedef struct {
  toku_buffer toku_buffer;
  uint8_t opcode;
  uint8_t data_size_remaining;
  uint8_t header_size;
  uint8_t decode_complete;
} toku_decode_buffer;

typedef enum {
  TOKU_OP_HELLO = 0,
  TOKU_OP_HELLO_ACK = 1,
  TOKU_OP_PING = 2,
  TOKU_OP_PONG = 3,
  TOKU_OP_REQUEST = 4,
  TOKU_OP_RESPONSE = 5,
  TOKU_OP_PUSH = 6,
  TOKU_OP_GOAWAY = 7,
  TOKU_OP_ERROR = 9
} toku_opcodes;

typedef enum {
  TOKU_DECODE_NEEDS_MORE = 1,
  TOKU_DECODE_COMPLETE = 2,
  TOKU_DECODE_MEMORY_ERROR = -1,
  TOKU_DECODE_INVALID_OPCODE = -2,
  TOKU_DECODE_INVALID_SIZE = -3
} toku_decoder_status;

typedef enum {
  TOKU_FLAG_COMPRESSED = 1 << 0
} toku_flags;


/* buffer.c */
int toku_buffer_write(toku_buffer *pk, const char *bytes, size_t l);
int toku_buffer_ensure_size(toku_buffer *pk, size_t at_least);

/* decoder.c */
size_t _toku_get_header_size(uint8_t opcode);
uint8_t toku_get_flags(toku_decode_buffer *pk);
size_t toku_get_data_payload_size(toku_decode_buffer *pk);
uint32_t toku_get_seq(toku_decode_buffer *pk);
uint8_t toku_get_version(toku_decode_buffer *pk);
uint16_t toku_get_code(toku_decode_buffer *pk);
uint32_t toku_get_ping_interval(toku_decode_buffer *pk);
void toku_decoder_reset(toku_decode_buffer *pk);
toku_decoder_status toku_read_append_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed);
toku_decoder_status toku_read_new_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed);
toku_decoder_status toku_decoder_read_data(toku_decode_buffer *pk, size_t size, const char *data, size_t *consumed);

/* encoder.c */
int toku_append_hello(toku_buffer *b, uint8_t flags, uint32_t size, const char *data);
int toku_append_ping(toku_buffer *b, uint8_t flags, uint32_t seq);
int toku_append_pong(toku_buffer *b, uint8_t flags, uint32_t seq);
int toku_append_request(toku_buffer *b, uint8_t flags, uint32_t seq, uint32_t size, const char *data);
int toku_append_response(toku_buffer *b, uint8_t flags, uint32_t seq, uint32_t size, const char *data);
int toku_append_push(toku_buffer *b, uint8_t flags, uint32_t size, const char *data);
int toku_append_goaway(toku_buffer *b, uint8_t flags, uint16_t code, uint32_t size, const char *data);
int toku_append_error(toku_buffer *b, uint8_t flags, uint16_t code, uint32_t seq, uint32_t size, const char *data);


#endif
