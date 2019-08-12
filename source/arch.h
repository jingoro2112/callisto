#ifndef ARCH_H
#define ARCH_H
/*------------------------------------------------------------------------------*/

#include <stdint.h>

#ifdef __APPLE__
#endif

#ifdef _WIN32
#include <Winsock2.h>
#endif

#ifdef __MINGW32__
#endif

#ifdef __linux__
#include <arpa/inet.h>
#define htonll htobe64
#define ntohll be64toh
#endif

#endif
