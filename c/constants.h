#ifndef TOKU_CONSTANTS
#define TOKU_CONSTANTS

#include "sysdep.h"

const unsigned char TOKU_VERSION = 1;
const size_t TOKU_DATA_SIZE_MAX = 1024 * 1024 * 50;

typedef enum {
  TOKU_OP_HELLO = 1,
  TOKU_OP_HELLO_ACK = 2,
  TOKU_OP_PING = 3,
  TOKU_OP_PONG = 4,
  TOKU_OP_REQUEST = 5,
  TOKU_OP_RESPONSE = 6,
  TOKU_OP_PUSH = 7,
  TOKU_OP_GOAWAY = 8,
  TOKU_OP_ERROR = 9,
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

#endif
