
from libc.stdint cimport uint32_t, uint8_t, uint16_t

cdef extern from "../../c/buffer.h":
    ctypedef struct toku_buffer_t:
        char *buf
        size_t length
        size_t allocated_size

    ctypedef struct toku_decode_buffer_t:
        toku_buffer_t toku_buffer;
        uint8_t opcode;
        size_t data_size_remaining;
        size_t header_size;

cdef extern from "../../c/encoder.h":
    int toku_append_hello(toku_buffer_t *b, uint8_t flags, uint32_t size, const char *data)
    int toku_append_hello_ack(toku_buffer_t *b, uint8_t flags, uint32_t ping_interval, uint32_t size, const char *data)
    int toku_append_ping(toku_buffer_t *b, uint8_t flags, uint32_t seq)
    int toku_append_pong(toku_buffer_t *b, uint8_t flags, uint32_t seq)
    int toku_append_request(toku_buffer_t *b, uint8_t flags, uint32_t seq, uint32_t size, const char *data)
    int toku_append_response(toku_buffer_t *b, uint8_t flags, uint32_t seq, uint32_t size, const char *data)
    int toku_append_push(toku_buffer_t *b, uint8_t flags, uint32_t size, const char *data)
    int toku_append_goaway(toku_buffer_t *b, uint8_t flags, uint16_t code, uint32_t size, const char *data)
    int toku_append_error(toku_buffer_t *b, uint8_t flags, uint16_t code, uint32_t seq, uint32_t size, const char *data)

cdef extern from "../../c/decoder.h":
    toku_decoder_status toku_decoder_read_data(toku_decode_buffer_t *pk, size_t size, const char *data, size_t* consumed)
    toku_decoder_status toku_decoder_reset(toku_decode_buffer_t *pk)
    uint32_t toku_get_seq(toku_decode_buffer_t *pk)
    size_t toku_get_data_payload_size(toku_decode_buffer_t *pk)
    uint8_t toku_get_version(toku_decode_buffer_t *pk)
    uint8_t toku_get_flags(toku_decode_buffer_t *pk)
    uint16_t toku_get_code(toku_decode_buffer_t *pk)
    uint32_t toku_get_ping_interval(toku_decode_buffer_t *pk)

cdef extern from "../../c/constants.h":
    const unsigned char TOKU_VERSION;
    const size_t TOKU_DATA_SIZE_MAX;

    ctypedef enum toku_opcodes:
        TOKU_OP_HELLO
        TOKU_OP_HELLO_ACK
        TOKU_OP_PING
        TOKU_OP_PONG
        TOKU_OP_REQUEST
        TOKU_OP_RESPONSE
        TOKU_OP_PUSH
        TOKU_OP_GOAWAY
        TOKU_OP_ERROR

    ctypedef enum toku_decoder_status:
        TOKU_DECODE_NEEDS_MORE
        TOKU_DECODE_COMPLETE
        TOKU_DECODE_MEMORY_ERROR
        TOKU_DECODE_INVALID_OPCODE
        TOKU_DECODE_INVALID_SIZE

    ctypedef enum toku_flags:
        TOKU_FLAG_COMPRESSED
