#include"utils.h"



/////////////////////////////////////////////////////////////////////////
uint64_t GetSysTickCount64()
{
	uint64_t Ret;
#ifdef _WIN32
	static LARGE_INTEGER TicksPerSecond = { 0 };
	LARGE_INTEGER Tick;

	if (!TicksPerSecond.QuadPart)
	{
		QueryPerformanceFrequency(&TicksPerSecond);
	}

	QueryPerformanceCounter(&Tick);

	LONGLONG Seconds = Tick.QuadPart / TicksPerSecond.QuadPart;
	LONGLONG LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart*Seconds);
	LONGLONG MillSeconds = LeftPart * 1000 / TicksPerSecond.QuadPart;
    Ret = Seconds * 1000 + MillSeconds;
	
#else
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    Ret =  (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);  
#endif

	return Ret;
}

/////////////////////////////////////////////////////////////////////////
int GetNumProcessors()
{
#ifdef _WIN32
         SYSTEM_INFO si;
	     GetSystemInfo(&si);
         return si.dwNumberOfProcessors;
#else
        return sysconf(_SC_NPROCESSORS_CONF);
#endif
}



void GetLocalIP(char* ip)
{
#ifdef _WIN32
	// 获得本机主机名
	char hostname[MAX_PATH] = { 0 };

	gethostname(hostname, MAX_PATH);

	struct hostent*  lpHostEnt = gethostbyname(hostname);

	if (lpHostEnt == NULL)
	{
		return;
	}

	// 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)
	char* lpAddr = lpHostEnt->h_addr_list[0];

	// 将IP地址转化成字符串形式
	struct in_addr inAddr;
	memmove(&inAddr, lpAddr, 4);
	char* ips = inet_ntoa(inAddr);

	for (int i = 0; ips[i] != '\0'; i++)
		ip[i] = ips[i];


#else
	int  MAXINTERFACES = 16;
	char *ips = NULL;
	int fd, intrface, retn = 0;
	struct ifreq buf[MAXINTERFACES]; ///if.h
	struct ifconf ifc; ///if.h

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) //socket.h
	{
		ifc.ifc_len = sizeof buf;
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) //ioctl.h
		{
			intrface = ifc.ifc_len / sizeof (struct ifreq);

			while ((intrface--)>0)
			{
				if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
				{
					ips = (inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));//types
					break;
				}
			}
		}
		close(fd);
	}

	for (int i = 0; ips[i] != '\0'; i++)
		ip[i] = ips[i];

#endif
}


void __cdecl odprintfw(const wchar_t *format, ...)
{
	wchar_t buf[4096], *p = buf;
	va_list args;
	va_start(args, format);
	p += _vsnwprintf(p, sizeof(buf)/sizeof(wchar_t), format, args);
	va_end(args);
	OutputDebugStringW(buf);
}

void __cdecl odprintfa(const char *format, ...)
{
	char buf[4096], *p = buf;
	va_list args;
	va_start(args, format);
	p += _vsnprintf(p, sizeof(buf) - 1, format, args);
	va_end(args);

	OutputDebugStringA(buf);
}
