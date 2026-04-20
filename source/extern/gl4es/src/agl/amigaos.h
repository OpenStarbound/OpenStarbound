#ifndef _AMIGAOS_H_
#define _AMIGAOS_H_

void* os4GetProcAddress(const char* name);

void os4OpenLib(void** lib);
void os4CloseLib();

#endif //_AMIGAOS_H_