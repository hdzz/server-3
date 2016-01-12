#ifndef _UTILS_H_
#define _UTILS_H_

#include <assert.h>
#include<string.h>
#include<stdio.h>
#include<stdint.h>

#ifdef _WIN32
#include<windows.h>
#include<io.h>
#include<winsock2.h>
#include<MSWSock.h>
#else
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include<unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif



///////////////////////////////////////////////////////////////////////////
#ifndef MAX
#define MAX(a,b) (a>b)?(a):(b)
#endif

#ifndef MIN
#define MIN(a,b) (a<b)?(a):(b)
#endif


#define EPSINON  0.000001

uint64_t GetSysTickCount64();
int GetNumProcessors();  // 获得本机中处理器的数量
void GetLocalIP(char* ip);

#endif


void __cdecl odprintfw(const wchar_t *format, ...);
void __cdecl odprintfa(const char *format, ...);

#ifdef UNICODE
#define odprintf odprintfw
#else
#define odprintf odprintfa
#endif // !UNICODE