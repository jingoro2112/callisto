#ifndef CALLISTO_ARCH_H
#define CALLISTO_ARCH_H
/*------------------------------------------------------------------------------*/

#include <stdint.h>

#ifndef CALLISTO_ENDIAN_NEUTRAL

 #define ntohl(a) a
 #define ntohll(a) a
 #define ntohs(a) a
 #define htonl(a) a
 #define htonll(a) a
 #define htons(a) a

#else 

 #ifdef _WIN32
 #include <Winsock2.h>
 #include <windows.h>
 #endif

 #ifdef __linux__
 #include <arpa/inet.h>
 #include <pthread.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/time.h>
 #include <sys/syscall.h>
 #endif

// htobe16
// htole16
// htobe32
// htole32
// htobe64
// htole64

// be16toh
// le16toh
// be32toh
// le32toh
// be64toh
// le64toh

 #if defined(__linux__) || defined(__CYGWIN__)
  #include <endian.h>

 #elif defined(__APPLE__)

  #include <libkern/OSByteOrder.h>
  #define htobe16(x) OSSwapHostToBigInt16(x)
  #define htole16(x) OSSwapHostToLittleInt16(x)
  #define htobe32(x) OSSwapHostToBigInt32(x)
  #define htole32(x) OSSwapHostToLittleInt32(x)
  #define htobe64(x) OSSwapHostToBigInt64(x)
  #define htole64(x) OSSwapHostToLittleInt64(x)
  #define be16toh(x) OSSwapBigToHostInt16(x)
  #define le16toh(x) OSSwapLittleToHostInt16(x)
  #define be32toh(x) OSSwapBigToHostInt32(x)
  #define le32toh(x) OSSwapLittleToHostInt32(x)
  #define be64toh(x) OSSwapBigToHostInt64(x)
  #define le64toh(x) OSSwapLittleToHostInt64(x)

 #elif defined(__OpenBSD__)
  #include <sys/endian.h>

 #elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
  #include <sys/endian.h>
  #define be16toh(x) betoh16(x)
  #define le16toh(x) letoh16(x)
  #define be32toh(x) betoh32(x)
  #define le32toh(x) letoh32(x)
  #define be64toh(x) betoh64(x)
  #define le64toh(x) letoh64(x)

 #elif (defined(_WIN16) || defined(_WIN32) || defined(_WIN64))
  #if 1 // little endian assumed for now (TODO-- something?)
   #if defined(_MSC_VER)
    #include <stdlib.h>
    #define htobe16(x) _byteswap_ushort(x)
    #define htole16(x) (x)
    #define be16toh(x) _byteswap_ushort(x)
    #define le16toh(x) (x)
    #define htobe32(x) _byteswap_ulong(x)
    #define htole32(x) (x)
    #define be32toh(x) _byteswap_ulong(x)
    #define le32toh(x) (x)
    #define htobe64(x) _byteswap_uint64(x)
    #define htole64(x) (x)
    #define be64toh(x) _byteswap_uint64(x)
    #define le64toh(x) (x)
   #elif defined(__GNUC__) || defined(__clang__)
    #define htobe16(x) __builtin_bswap16(x)
    #define htole16(x) (x)
    #define be16toh(x) __builtin_bswap16(x)
    #define le16toh(x) (x)
    #define htobe32(x) __builtin_bswap32(x)
    #define htole32(x) (x)
    #define be32toh(x) __builtin_bswap32(x)
    #define le32toh(x) (x)
    #define htobe64(x) __builtin_bswap64(x)
    #define htole64(x) (x)
    #define be64toh(x) __builtin_bswap64(x)
    #define le64toh(x) (x)
   #else
    #error "platform not supported <1>"
   #endif
  #else // big endian
  #endif
 #else
  #error "platform not supported <2>"
 #endif

 #define htonll htobe64
 #define ntohll be64toh

#endif //CALLISTO_ENDIAN_NEUTRAL

#endif
