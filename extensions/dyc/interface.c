/*

  FILE   : interface.c
  HEADER : interface.h
  BAS    : dyc.bas
  AUTHOR : Peter Verhas

  DATE:    December 23, 2002

  CONTENT:
  This is the interface.c file for the ScriptBasic module dyc
  
  This module makes it possible to call arbitrary C functions implemented
  in a DLL that were not developed specifically for ScriptBasic. The module
  uses the code
  
      Dynacall.c - 32-bit Dynamic function calls. Ton Plooy 1998

  included into the interface.c file.

  This module is very much Windows specific and thus can not be compiled
  or used under UNIX/Linux or Mac.

NTLIBS:  
*/
#include <windows.h>


#define  DC_MICROSOFT           0x0000      // Default
#define  DC_BORLAND             0x0001      // Borland compat
#define  DC_CALL_CDECL          0x0010      // __cdecl
#define  DC_CALL_STD            0x0020      // __stdcall
#define  DC_RETVAL_MATH4        0x0100      // Return value in ST
#define  DC_RETVAL_MATH8        0x0200      // Return value in ST

#define  DC_CALL_STD_BO         (DC_CALL_STD | DC_BORLAND)
#define  DC_CALL_STD_MS         (DC_CALL_STD | DC_MICROSOFT)
#define  DC_CALL_STD_M8         (DC_CALL_STD | DC_RETVAL_MATH8)

#define  DC_FLAG_ARGPTR         0x00000002

#pragma pack(1)                 // Set struct packing to one byte

typedef union T_RESULT {          // Various result types
    int     Int;                // Generic four-byte type
    long    Long;               // Four-byte long
    void   *Pointer;            // 32-bit pointer
    float   Float;              // Four byte real
    double  Double;             // 8-byte real
    __int64 int64;              // big int (64-bit)
} T_RESULT;

typedef struct _DYNAPARM {
    DWORD       dwFlags;        // Parameter flags
    int         nWidth;         // Byte width
    union {                     //
        DWORD   dwArg;          // 4-byte argument
        void   *pArg;           // Pointer to argument
    };
} DYNAPARM;

static
DWORD WINAPI SearchProcAddress(HMODULE hInst, LPSTR szFunction)
{
    // Add some simple searching to the GetProcAddress function.
    // Various Win32 functions have two versions, a ASCII and
    // a Unicode version.
    DWORD dwAddr;
    char  szName[128];
#pragma warning ( disable : 4311 )
    if ((dwAddr = (DWORD)GetProcAddress(hInst, szFunction)) == 0) {
        // Function name not found, try some variants
        strcpy(szName, szFunction);
        strcat(szName, "A");            // ASCII
        dwAddr = (DWORD)GetProcAddress(hInst, szName);
    }
    return dwAddr;
}

//------------------------------------------------------------------
static
T_RESULT WINAPI DynaCall(int Flags, DWORD lpFunction,
                                  int nArgs, DYNAPARM Parm[],
                                  LPVOID pRet, int nRetSiz)
{
    // Call the specified function with the given parameters. Build a
    // proper stack and take care of correct return value processing.
    T_RESULT  Res = { 0 };
    int     i, nInd, nSize;
    DWORD   dwEAX, dwEDX, dwVal, *pStack, dwStSize = 0;
    BYTE   *pArg;

    // Reserve 256 bytes of stack space for our arguments
    _asm mov pStack, esp
    _asm sub esp, 0x100

    // Push args onto the stack. Every argument is aligned on a
    // 4-byte boundary. We start at the rightmost argument.
    for (i = 0; i < nArgs; i++) {
        nInd  = (nArgs - 1) - i;
        // Start at the back of the arg ptr, aligned on a DWORD
        nSize = (Parm[nInd].nWidth + 3) / 4 * 4;
        pArg  = (BYTE *)Parm[nInd].pArg + nSize - 4;
        dwStSize += (DWORD)nSize; // Count no of bytes on stack
        while (nSize > 0) {
            // Copy argument to the stack
            if (Parm[nInd].dwFlags & DC_FLAG_ARGPTR) {
                // Arg has a ptr to a variable that has the arg
                dwVal = *(DWORD *)pArg; // Get first four bytes
                pArg -= 4;              // Next part of argument
            }
            else {
                // Arg has the real arg
                dwVal = Parm[nInd].dwArg;
            }
            // Do push dwVal
            pStack--;           // ESP = ESP - 4
            *pStack = dwVal;    // SS:[ESP] = dwVal
            nSize -= 4;
        }
    }
    if ((pRet != NULL) && ((Flags & DC_BORLAND) || (nRetSiz > 8))) {
        // Return value isn't passed through registers, memory copy
        // is performed instead. Pass the pointer as hidden arg.
        dwStSize += 4;          // Add stack size
        pStack--;               // ESP = ESP - 4
        *pStack = (DWORD)pRet;  // SS:[ESP] = pMem
    }

    _asm add esp, 0x100         // Restore to original position
    _asm sub esp, dwStSize      // Adjust for our new parameters

    // Stack is now properly built, we can call the function
    _asm call [lpFunction]

    _asm mov dwEAX, eax         // Save eax/edx registers
    _asm mov dwEDX, edx         //

    // Possibly adjust stack and read return values.
    if (Flags & DC_CALL_CDECL) {
        _asm add esp, dwStSize
    }
    if (Flags & DC_RETVAL_MATH4) {
        _asm fstp dword ptr [Res]
    }
    else if (Flags & DC_RETVAL_MATH8) {
        _asm fstp qword ptr [Res]
    }
    else if (pRet == NULL) {
        _asm mov  eax, [dwEAX]
        _asm mov  DWORD PTR [Res], eax
        _asm mov  edx, [dwEDX]
        _asm mov  DWORD PTR [Res + 4], edx
    }
    else if (((Flags & DC_BORLAND) == 0) && (nRetSiz <= 8)) {
        // Microsoft optimized less than 8-bytes structure passing
        _asm mov ecx, DWORD PTR [pRet]
        _asm mov eax, [dwEAX]
        _asm mov DWORD PTR [ecx], eax
        _asm mov edx, [dwEDX]
        _asm mov DWORD PTR [ecx + 4], edx
    }
    return Res;
}

#include <stdio.h>
#include "../../basext.h"


/*
TO_BAS:
*/

typedef struct _DllLoaded {
  HMODULE hDll;  /* habdle to the loaded DLL or NULL if loading was unsuccessful */
  char *pszName; /* the name of the loaded library as it was specified by the program */
  struct _DllLoaded *next; /*pointer to the next on the list or NULL if the last */
  } DllLoaded, *pDllLoaded;

typedef struct _ModuleObject {
  pDllLoaded pDllList;
  }ModuleObject,*pModuleObject;

besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND

/*
*TODO*
ALTER THE ERROR MESSAGE FUNCTION
*/
besSUB_ERRMSG

  switch( iError ){
    case 0x00080000: return "ERROR HAS HAPPENED";
    }
  return "Unknown dyc module error.";
besEND


besSUB_START
  pModuleObject p;

  besMODULEPOINTER = besALLOC(sizeof(ModuleObject));
  if( besMODULEPOINTER == NULL )return COMMAND_ERROR_MEMORY_LOW;
  p = besMODULEPOINTER;
  p->pDllList = NULL; /* no Dlls are loaded (by the module) when the module starts */
  return COMMAND_ERROR_SUCCESS;
besEND

besSUB_FINISH
  pModuleObject p;
  pDllLoaded pDll;
  
  p = (pModuleObject)besMODULEPOINTER;
  if( p == NULL )return 0;
  
  /* free the libraries that were loaded by teh module */
  pDll = p->pDllList;
  while( pDll ){
    FreeLibrary(pDll->hDll);
    pDll = pDll->next;
    }
  return 0;
besEND

/* This function loads the requested DLL safely. This means that the DLL will
   only be loaded calling LoadLibrary only if it was not loaded yet. In other
   cases the function returns without calling the system function. Thus a DLL
   will only be loaded only once by the module.
   
   The function returns TRUE if the module was loaded successfully and FALSE
   otherwise.
   
   The function can be called using the macro bLibraryLoaded(x) without the
   special arguments.
   
   The function can be called from within any besFUNCTION function. The arguments
   passed to this function allow the function to call the bes macros from
   within this function especially to call the memory allocation functions.
   
   Note that this function does not check if a library is specified with
   different names. For example if the library is named 'simple.dll' and also
   'c:\\winnt\\system32\\simple.dll' the function will load the dll twice.
   
*/
#define LibraryLoaded(pszName) F_LibraryLoaded(pSt,ppModuleInternal,pszName)
static HMODULE F_LibraryLoaded(pSupportTable pSt,
                            void **ppModuleInternal,
                            char *pszName){
  pDllLoaded pDll,pNew;
  pModuleObject p = (pModuleObject)besMODULEPOINTER;

  pDll = p->pDllList;

  while( pDll ){
    if( ! stricmp(pszName,pDll->pszName) )break;
    pDll = pDll->next;
    }
  if( pDll )
    return pDll->hDll;

  pNew = (pDllLoaded)besALLOC(sizeof(DllLoaded));
  /* If there is no memory then the DLL can not be loaded. The function
     design does not allow any more specific error handling. */
  if( NULL == pNew )return NULL;
  
  /* allocate space for the name of the library to be loaded */
  pNew->pszName = besALLOC(strlen(pszName)+1);
  if( NULL == pNew->pszName ){
    besFREE(pNew);
    return NULL;
    }
  /* copy the name of the library into the structure thus it can be checked
     against library names to avoid double loading */
  strcpy(pNew->pszName,pszName);
  
  /* link the new structure to the head of the list */
  pNew->next = p->pDllList;
  p->pDllList = pNew;
  pNew->hDll = LoadLibrary(pNew->pszName);
  return pNew->hDll;
  }

/**
=section dyc
=H DYC("format", arguments)

=verbatim
import dyc.bas
print dyc::dyc("ms,i,USER32.DLL,MessageBox,PZZL",0,"test message","title",3)
=noverbatim

This function calls an arbitrary function from an arbitrary dll. The first argument to the function has to be a format string
and the rest of the arguments are the arguments for the function to be called. The format string has to specify the calling
convention of the function, the return value, the name of the DLL and the function to call and the argument types. These have
to be specified one after the other separated by commas. The format string should not contain space.

The format string has the following format:

T<"Xc,Xr,DllName,FunctionName,Xargs">

where

=itemize
=item T<Xc> specifies the calling convention
=item T<Xr> specifies the return value
=item T<DllName> is the name of the DLL
=item T<FunctionName> is the name of the function
=item T<Xargs> specifies the arguments
=noitemize

When the function is called the arguments are converted from their BASIC value and the function is called according to the
format specification. However note that a misformed format string can cause access violation in the program and thus stopping
the process. Therefore it is recommended that you fully debug your code and the way you use this function. It may be a wise
idea not to install this module on a server where different programmers can develop their programs and run in shared process
n multiple threads. For example a hosted web server running the Eszter SB Application Engine can be stopped by a BASIC program
using this external module.

In the following I describe the format string specifiers.

=itemize
=item T<Xc> CALLING CONVENTION
=noitemize

The calling convention can be one, two or at most three characters. The character T<m> or T<M> means that the code was compiled
using Microsoft compiler. This is the default behavour, thus there is no need to specify this. The opposite is T<b> or T<B>
meaning that the code was compiled using Borland compiler. The difference between these two compilers is how the return value
is passed back to the caller. You should not use both T<b> and T<m> at a time. Actually T<m> will be ignored.

The calling convention can also be T<s> or T<S> meaning standard callign convention or T<c> or T<C> meaning language C
calling convention. Only one of them is to be used in a function call. If you are callign some function from a Windows system
DLL then it is certainly T<s>. If you do not know which to use write a small test program and experiment.

The difference between standard and C calling convention is the order of the arguments placed on the stack and also who the
responsible is to clean the arguments from the stack (the called function or the calling code).

Finally you can specify T<4> or T<8> to specify that the function is returning a four or eight-byte floating point number.
Although this is a kind of return value specification, it is stated here, because this affects the calling convention. These
values are returned not in a memory place from the function but rather in the co-processor register and function T<dyc> has
to know to fetch them from there rather than expection the function to return a four or eight-byte memory chunk.

=itemize
=item T<Xr> RETURN VALUE
=noitemize

The return value should be specified using a single character. This can be:

T<i> or T<I> T<int>, T<l> or T<L> T<long>, T<p> or T<P> pointer, T<f> or T<F> T<float>,
T<d> or T<D> for T<double> or T<v> or T<V> for T<__int64>.

The int and long types are converted to a BASIC integer number, which is stored as a long in ScriptBasic. float and double values
are returned as real number, which is stored as double in ScriptBasic. A pointer value is converted to long and is returned in an
integer value. An T<__int64> value is returned as an 8 byte string copiing the bytes of the original T<__int64> value to the bytes
of the BASIC string.

=itemize
=item T<DllName>
=noitemize

This parameter has to specify the name of the DLL. This name will be used to load the DLL calling the system function T<LoadLibrary>.
This means that the name can but also may not include the full path to the file. In the latter case the system function will search
the path for the DLL as specified int he Microsoft provided documentation for the function T<LoadLibrary>.

When a function from a certain DLL is called first the module loads the DLL and when the BASIC program finishes and the module is
unloaded it unloads all DLLs it loaded. Any DLL by the module will only be loaded once. Whent he module is used in a multi-thread
environment the interpreter threads load and unload the DLLs independently. If you do not understand what it means then just ignore
this explanation: nothing to worry about.

=itemize
=item T<FunctionName>
=noitemize

The name of the function to be called from the certain DLL. If the function is not present in the DLL then the program tries to
use the function with the original name with an 'A' appended to it. Many system functions happen to have this format in the
Windows librares.

=itemize
=item T<Xargs> argument types
=noitemize

This parameter should specify the arguments. It has to have as many character as many arguments there are. Each character should specify
exactly one argument and will control how the actual BASIC arguments are converted to their native format. For each argument one of the
following characters can be used:

=itemize
=item T<1>
=item T<2>
=item T<4>
=item T<8> specifies that the argument is an arbitrary 1-, 2-, 4- or 8-byte argument. The BASIC argument should be string value and
should have at least as many characters as needed (1, 2, 4 or 8 as specified).

It is possible to use undefined, integer or real value for 1-, 2- or 4-byte values. In this case the value will be converted to integer and
the bytes of the value will be used as argument. In case of 8-byte argument the BASIC argument is converted to string.

=item T<c> specifies that the argument is a single character. If the BASIC argument is a string then the first character of the string is
used. If the argument is a real number or an integer number then the value will be used as ASCII code. If the argument is T<undef> or if the
string has no characters in it then the value will be zero.

=item T<s> specifies that the argument is a short(2-byte) value. The BASIC argument is converted to an integervalue if needed and
truncated to two bytes if needed.

=item T<f> specifies that the argument is a float value. The BASIC argument is converted to a real value and its precision is decreased
from double to float.
=item T<h>
=item T<p>
=item T<l> specifies that the argument is a handle, pointer or long. In these cases the BASIC argument is converted to an integer value if
needed.

=item T<z> specifies that the argument is a pointer that should point to a zero terminated string. The BASIC argument is converted to string and
a pointer to the start of the string is passed as actual argument. Note that BASIC strings are not zero terminated and the function T<dyc>
does not append the terminating zero character to the string. Thus you have to append a zero character to the BASIC string before you
pass it as zero terminated string. For example:

=verbatim
import dyc.bas

a$ = "message"
a$ &= " text"

REM Make the string zero character terminated
a$ &= chr$(0)

print dyc::dyc("ms,i,USER32.DLL,MessageBox,PZZL",0,a$,"title",3)
=noverbatim

On the other hand you can safely use string constants as argument like in the example above T<"title"> because string constants in ScriptBasic
contain an extra zero character following their normal characters.

=item T<d> specifies that the argument is a double value. The BASIC argument is converted to a real value if needed, which is stored in BASIC
internally as double and is passed to the function.
=noitemize

Note that this is a wise idea to write a wrapper function in BASIC that gets the arguments performs some checks if needed and calls the
module T<dyc> instead of putting the dyc call into the main BASIC code.

*/
besFUNCTION(dyc)
  pModuleObject p;
  VARIABLE Argument;
  int Flags;
  DWORD lpFunction;
  int nArgs;
  DYNAPARM *Parm;
  int nRetSiz;
  char *pszFormat;
  DWORD cbFormat;
  int iParseState;
  char cRet;
  char szDll[MAX_PATH];
  char szFunction[MAX_PATH];
  char *pszS;
  int i;
  HMODULE hDll;
  T_RESULT RetVal;
  float fTmp;

  p = (pModuleObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);

  /* if argument is undef then raise the error */
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  /* make sure that the format argument is a string */
  Argument = besCONVERT2STRING(Argument);
  pszFormat = STRINGVALUE(Argument);
  cbFormat = STRLEN(Argument);
  if( cbFormat < 1 )return COMMAND_ERROR_ARGUMENT_TYPE;
  iParseState = 0;
  Flags = 0;
  while( cbFormat ){
    switch( iParseState ){
      case 0: //define the call type
        switch( *pszFormat ){
          case 'm': case 'M': Flags |= DC_MICROSOFT;    break;
          case 'b': case 'B': Flags |= DC_BORLAND;      break;
          case 'c': case 'C': Flags |= DC_CALL_CDECL;   break;
          case 's': case 'S': Flags |= DC_CALL_STD;     break;
          case '4':           Flags |= DC_RETVAL_MATH4; break;
          case '8':           Flags |= DC_RETVAL_MATH8; break;
          case ',' : iParseState++; break;
          }
        cbFormat --;
        pszFormat++;
        break;
      case 1: // define the return value using a single character
        if( cbFormat < 1 )return COMMAND_ERROR_ARGUMENT_TYPE;
        cRet = tolower(*pszFormat);
        switch( cRet ){
          case 'I': case 'i': nRetSiz = sizeof(int);    break;
          case 'L': case 'l': nRetSiz = sizeof(long);   break;
          case 'P': case 'p': nRetSiz = sizeof(void *); break;
          case 'F': case 'f': nRetSiz = sizeof(float);  break;
          case 'D': case 'd': nRetSiz = sizeof(double); break;
          case 'V': case 'v': nRetSiz = sizeof(__int64);break;
          default: return COMMAND_ERROR_ARGUMENT_RANGE;
          }
        cbFormat --;
        pszFormat++;
        if( cbFormat < 1 )return COMMAND_ERROR_ARGUMENT_TYPE;
        if( *pszFormat != ',' )return COMMAND_ERROR_ARGUMENT_RANGE;
        cbFormat --;
        pszFormat++;
        iParseState++;
        break;
      case 2: // define the name of the dll
      case 3: //define the name of the function
        if( iParseState == 2 )pszS = szDll; else pszS = szFunction;
        i = 0;
        while( cbFormat && *pszFormat != ',' ){
          pszS[i] = *pszFormat;
          pszFormat ++;
          i++;
          cbFormat --;
          if( i >= MAX_PATH )return COMMAND_ERROR_ARGUMENT_RANGE;
          }
        if( cbFormat < 1 )return COMMAND_ERROR_ARGUMENT_RANGE;
        cbFormat--;
        pszFormat++;
        pszS[i] = (char)0;
        iParseState++;
        break;
      case 4: // define the arguments
        //first of all get sure that the library loads
        if( NULL == (hDll = LibraryLoaded(szDll)) )return COMMAND_ERROR_MODULE_NOT_LOADED;
        // the rest of the format string should define the arguments, each character
        // should specify exactly one argument, thus the number of characters left
        // should give exactly the number of arguments
        nArgs = cbFormat;
        Parm = besALLOC(nArgs*sizeof(DYNAPARM));
        if( NULL == Parm )return COMMAND_ERROR_MEMORY_LOW;
        i = 0; // count the arguments
        while( cbFormat ){
          if( i <= besARGNR-1 ){
            Argument = besARGUMENT(i+2);
            besDEREFERENCE(Argument);
            }else{
            Argument = NULL;
            }
          switch( *pszFormat ){
            case '1': // just any 1-byte argument
              Parm[i].nWidth  = 1;
              Parm[i].dwFlags = 0;
              if( NULL == Argument ){
                Parm[i].dwArg = 0;
                }else
                switch( TYPE(Argument) ){
                  case VTYPE_STRING:
                    if( STRLEN(Argument) < 1 )
                      Parm[i].dwArg = 0;
                    else
                      memcpy(&(Parm[i].dwArg),STRINGVALUE(Argument),1);
                    break;
                  case VTYPE_UNDEF:
                  case VTYPE_LONG:
                  case VTYPE_DOUBLE: Parm[i].dwArg = 0xFF & (unsigned long)besGETLONGVALUE(Argument); break;
                  default: return COMMAND_ERROR_ARGUMENT_RANGE;
                  }
            case '2': // just any 2-byte argument
              Parm[i].nWidth  = 2;
              Parm[i].dwFlags = 0;
              if( NULL == Argument ){
                Parm[i].dwArg = 0;
                }else
                switch( TYPE(Argument) ){
                  case VTYPE_STRING:
                    if( STRLEN(Argument) < 2 )
                      Parm[i].dwArg = 0;
                    else
                      memcpy(&(Parm[i].dwArg),STRINGVALUE(Argument),2);
                    break;
                  case VTYPE_UNDEF:
                  case VTYPE_LONG:
                  case VTYPE_DOUBLE: Parm[i].dwArg = 0xFFFF & (unsigned long)besGETLONGVALUE(Argument); break;
                  default: return COMMAND_ERROR_ARGUMENT_RANGE;
                  }
            case '4': // just any 4-byte argument
              Parm[i].nWidth  = 4;
              Parm[i].dwFlags = 0;
              if( NULL == Argument ){
                Parm[i].dwArg = 0;
                }else
                switch( TYPE(Argument) ){
                  case VTYPE_STRING:
                    if( STRLEN(Argument) < 4 )
                      Parm[i].dwArg = 0;
                    else
                      memcpy(&(Parm[i].dwArg),STRINGVALUE(Argument),4);
                      break;
                  case VTYPE_UNDEF:
                  case VTYPE_LONG:
                  case VTYPE_DOUBLE: Parm[i].dwArg = (long)besGETLONGVALUE(Argument); break;
                  default: return COMMAND_ERROR_ARGUMENT_RANGE;
                  }
            case '8':
              Argument = besCONVERT2STRING(Argument);
              if( STRLEN(Argument) < 8 )return COMMAND_ERROR_ARGUMENT_RANGE;
              Parm[i].nWidth  = 8;
              Parm[i].dwFlags = DC_FLAG_ARGPTR;
              Parm[i].pArg = STRINGVALUE(Argument);
              break;

            case 'c': case 'C': //char
              Parm[i].nWidth  = 1;
              Parm[i].dwFlags = 0;
              if( NULL == Argument ){
                Parm[i].dwArg = 0;
                }else
                switch( TYPE(Argument) ){
                  case VTYPE_STRING:
                    if( STRLEN(Argument) < 1 )
                      Parm[i].dwArg = 0;
                    else
                      Parm[i].dwArg = *STRINGVALUE(Argument);
                    break;
                  case VTYPE_UNDEF:
                  case VTYPE_LONG:
                  case VTYPE_DOUBLE: Parm[i].dwArg = (char)besGETLONGVALUE(Argument); break;
                  default: return COMMAND_ERROR_ARGUMENT_RANGE;
                  }
            case 's': case 'S': //short
              Parm[i].nWidth  = 2;
              Parm[i].dwFlags = 0;
              Parm[i].dwArg = (short)besGETLONGVALUE(Argument); break;
              break;

            case 'f': case 'F': //float
              Parm[i].nWidth  = 4;
              Parm[i].dwFlags = 0;
              fTmp = (float)besGETDOUBLEVALUE(Argument);
              memcpy(&(Parm[i].dwArg) , &fTmp, sizeof(float)) ; break;
              break;
              
            case 'h': case 'H': //handle
            case 'p': case 'P': //pointer
            case 'l': case 'L': //long
              Parm[i].nWidth  = 4;
              Parm[i].dwFlags = 0;
              Parm[i].dwArg = (DWORD)besGETLONGVALUE(Argument); break;
              break;

            case 'z': case 'Z': //ZCHAR
              Parm[i].nWidth  = 4;
              Parm[i].dwFlags = 0;
              Argument = besCONVERT2STRING(Argument);
              Parm[i].pArg = STRINGVALUE(Argument); break;
              break;

            case 'd': case 'D': //double
              Argument = besCONVERT2DOUBLE(Argument);
              Parm[i].nWidth  = 8;
              Parm[i].dwFlags = DC_FLAG_ARGPTR;
              Parm[i].pArg = & (DOUBLEVALUE(Argument));
              break;
            default: return COMMAND_ERROR_ARGUMENT_RANGE;
            }
          i++;
          cbFormat--;
          pszFormat++;
          }
        break;
      }
    }
  lpFunction = (DWORD)SearchProcAddress(hDll,szFunction);
  if( lpFunction == 0 )return COMMAND_ERROR_MODULE_FUNCTION;
  RetVal = DynaCall(Flags,lpFunction,nArgs,Parm,NULL,0);
  switch( cRet ){
    case 'i':
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = (long)RetVal.Int;
      break;
    case 'l':
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = RetVal.Long;
      break;
    case 'p':
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = (long)RetVal.Pointer;
      break;
    case 'f':
      besALLOC_RETURN_DOUBLE;
      DOUBLEVALUE(besRETURNVALUE) = (double)RetVal.Float;
      break;
    case 'd':
      besALLOC_RETURN_DOUBLE;
      DOUBLEVALUE(besRETURNVALUE) = (double)RetVal.Double;
      break;
    case 'v':
      besALLOC_RETURN_STRING(8);
      memcpy(STRINGVALUE(besRETURNVALUE),&(RetVal.int64),8);
      break;
    }

  return COMMAND_ERROR_SUCCESS;
besEND


SLFST DYC_SLFST[] ={

{ "versmodu" , versmodu },
{ "bootmodu" , bootmodu },
{ "finimodu" , finimodu },
{ "emsgmodu" , emsgmodu },
{ "dyc" , dyc },
{ NULL , NULL }
  };
