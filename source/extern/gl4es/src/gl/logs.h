#ifndef _GL4ES_LOGS_H_
#define _GL4ES_LOGS_H_
//----------------------------------------------------------------------------
#include <stdio.h>
#include "init.h"
#include "attributes.h"
//----------------------------------------------------------------------------
void LogPrintf_NoPrefix(const char *fmt,...);
void LogFPrintf(FILE *fp,const char *fmt,...);
EXPORT void LogPrintf(const char *fmt,...);
void write_log(const char* format, ...);
//----------------------------------------------------------------------------
#ifdef GL4ES_SILENCE_MESSAGES
	#define SHUT_LOGD(...)
	#define SHUT_LOGD_NOPREFIX(...)
	#define SHUT_LOGE(...)
#else
	#define SHUT_LOGD(...) {printf(__VA_ARGS__);printf("\n");write_log(__VA_ARGS__);}
	#define SHUT_LOGD_NOPREFIX(...) {if(!globals4es.nobanner) LogPrintf_NoPrefix(__VA_ARGS__);}
	#define SHUT_LOGE(...) {printf(__VA_ARGS__);printf("\n");write_log(__VA_ARGS__);}
#endif
//----------------------------------------------------------------------------
#define LOGD(...) SHUT_LOGD(__VA_ARGS__);
#define LOGE(...) SHUT_LOGD(__VA_ARGS__);
//----------------------------------------------------------------------------
#endif // _GL4ES_LOGS_H_
