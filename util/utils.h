#ifndef _UTILS_H_
#define _UTILS_H_

#include <assert.h>
#include<string.h>
#include<stdio.h>
#include<stdint.h>
#include <time.h>

#ifdef _WIN32
#include<windows.h>
#include<io.h>
#include<winsock2.h>
#include<MSWSock.h>
#else
#include <sys/stat.h>
#include <arpa/inet.h>
#include<unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/time.h>
#endif


#ifndef MAX
#define MAX(a,b) (a>b)?(a):(b)
#endif

#ifndef MIN
#define MIN(a,b) (a<b)?(a):(b)
#endif


#define EPSINON  0.000001

int GetNumProcessors();  // 获得本机中处理器的数量
void GetLocalIP(char* ip);

#endif

#ifdef _WIN32

#if _WIN32_WINNT < 0x0600 
uint64_t GetTickCount64();
#endif

#ifdef UNICODE
#define odprintf odprintfw
#else
#define odprintf odprintfa
#endif // !UNICODE


uint64_t GetSysTickCount64();

struct timezone;
int gettimeofday(struct timeval *tv, struct timezone *tz);
#else
uint64_t GetTickCount64();
#endif

void __cdecl odprintfw(const wchar_t *format, ...);
void __cdecl odprintfa(const char *format, ...);

