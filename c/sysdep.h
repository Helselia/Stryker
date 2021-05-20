#ifndef TOKU_SYSDEP_H__
#define TOKU_SYSDEP_H__

#include <stdlib.h>
#include <stddef.h>
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#elif defined(_MSC_VER)  // && _MSC_VER >= 1600
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
/* numeric_limits<T>::min,max */
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

#else
#include <arpa/inet.h>  /* __BYTE_ORDER */
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN__
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __BIG_ENDIAN__
#elif _WIN32
#define __LITTLE_ENDIAN__
#endif
#endif


#ifdef __LITTLE_ENDIAN__

#ifdef _WIN32
#  if defined(ntohs)
#    define _toku_be16(x) ntohs(x)
#  elif defined(_byteswap_ushort) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#    define _toku_be16(x) ((uint16_t)_byteswap_ushort((unsigned short)x))
#  else
#    define _toku_be16(x) ( \
        ((((uint16_t)x) <<  8) ) | \
        ((((uint16_t)x) >>  8) ) )
#  endif
#else
#  define _toku_be16(x) ntohs(x)
#endif

#ifdef _WIN32
#  if defined(ntohl)
#    define _toku_be32(x) ntohl(x)
#  elif defined(_byteswap_ulong) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#    define _toku_be32(x) ((uint32_t)_byteswap_ulong((unsigned long)x))
#  else
#    define _toku_be32(x) \
        ( ((((uint32_t)x) << 24)               ) | \
          ((((uint32_t)x) <<  8) & 0x00ff0000U ) | \
          ((((uint32_t)x) >>  8) & 0x0000ff00U ) | \
          ((((uint32_t)x) >> 24)               ) )
#  endif
#else
#  define _toku_be32(x) ntohl(x)
#endif

#if defined(_byteswap_uint64) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#  define _toku_be64(x) (_byteswap_uint64(x))
#elif defined(bswap_64)
#  define _toku_be64(x) bswap_64(x)
#elif defined(__DARWIN_OSSwapInt64)
#  define _toku_be64(x) __DARWIN_OSSwapInt64(x)
#elif defined(__linux__)
#  define _toku_be64(x) be64toh(x)
#else
#  define _toku_be64(x) \
    ( ((((uint64_t)x) << 56)                         ) | \
      ((((uint64_t)x) << 40) & 0x00ff000000000000ULL ) | \
      ((((uint64_t)x) << 24) & 0x0000ff0000000000ULL ) | \
      ((((uint64_t)x) <<  8) & 0x000000ff00000000ULL ) | \
      ((((uint64_t)x) >>  8) & 0x00000000ff000000ULL ) | \
      ((((uint64_t)x) >> 24) & 0x0000000000ff0000ULL ) | \
      ((((uint64_t)x) >> 40) & 0x000000000000ff00ULL ) | \
      ((((uint64_t)x) >> 56)                         ) )
#endif


#define _toku_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)(from))[0]) << 8) | \
        (((uint16_t)((uint8_t*)(from))[1])     ) ))

#define _toku_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)(from))[0]) << 24) | \
        (((uint32_t)((uint8_t*)(from))[1]) << 16) | \
        (((uint32_t)((uint8_t*)(from))[2]) <<  8) | \
        (((uint32_t)((uint8_t*)(from))[3])      ) ))

#else
#define _toku_be16(x) (x)
#define _toku_be32(x) (x)
#define _toku_be64(x) (x)

#define _toku_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)from)[0]) << 8) | \
        (((uint16_t)((uint8_t*)from)[1])     ) ))

#define _toku_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)from)[0]) << 24) | \
        (((uint32_t)((uint8_t*)from)[1]) << 16) | \
        (((uint32_t)((uint8_t*)from)[2]) <<  8) | \
        (((uint32_t)((uint8_t*)from)[3])      ) ))

#endif


#define _toku_store16(to, num) \
    do { uint16_t val = _toku_be16(num); memcpy(to, &val, 2); } while(0)
#define _toku_store32(to, num) \
    do { uint32_t val = _toku_be32(num); memcpy(to, &val, 4); } while(0)

#endif /* sysdep.h */