#ifndef ARCH_H
#define ARCH_H
/*------------------------------------------------------------------------------*/

#include <stdint.h>

#ifdef __APPLE__
#endif

#ifdef _WIN32
#include <Winsock2.h>
#include <windows.h>
#endif

#ifdef __MINGW32__
#endif

#ifdef __linux__
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>

#define InterlockedIncrement(V) __sync_add_and_fetch((V), 1)
#define InterlockedDecrement(V) __sync_sub_and_fetch((V), 1)

#define htonll htobe64
#define ntohll be64toh
#endif

#endif
