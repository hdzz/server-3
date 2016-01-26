#pragma once

#pragma comment(lib,"ws2_32.lib")


//log4cplus
#ifdef _DEBUG
//#pragma comment(lib,"../Classes/utils/lib/logcplus/win32/log4cplusUD.lib")
#else
//#pragma comment(lib,"../Classes/utils/lib/logcplus/win32/log4cplusU.lib")
#endif 


//libcurl
#ifdef _DEBUG
//#pragma comment(lib,"../Classes/utils/lib/libcurl/libcurld.lib")
#else
//#pragma comment(lib,"../utils/lib/libcurl/libcurl.lib")
#endif 


//pthread
#ifdef _DEBUG
#pragma comment(lib,"../server/util/lib/pthread/dll/x86/pthreadVC2d.lib")
#else
#pragma comment(lib,"../server/util/lib/pthread/dll/x86/pthreadVC2.lib")
#endif 

#define LOG4CPLUS_BUILD_DLL