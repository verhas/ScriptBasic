/*
variations/win32dll/basicdll.h
*/
#ifndef __BASICDLL_H __
#define __BASICDLL_H __ 1
#ifdef  __cplusplus
extern "C" {
#endif

#define BAFLG_BINARY 0x00000001
#ifdef WIN32
#define DLL_EXPORT __declspec(dllexport)
#endif

DLL_EXPORT 
int _stdcall basic(char *szInputFile,
                   void *StdinFunction,
                   void *StdouFunction,
                   void *EnvirFunction,
                   void *EmbedPointer,
                   char *CmdLineOptions,
                   unsigned long ulFlag);

#ifdef __cplusplus
}
#endif
#endif
