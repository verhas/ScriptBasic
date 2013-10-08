/* 
FILE:   dynlolib.c
HEADER: dynlolib.h

--GNU LGPL
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

TO_HEADER:

*/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#elif defined(__DARWIN__)
#include <mach-o/dyld.h>
#elif defined(__MACOS__)
#include <string.h>
#include <Files.h>
#include <CodeFragments.h>
#else
#include <dlfcn.h>
#endif

/*POD
@c Handling Dynamic Load Libraries

The Dynamic Load Libraries are handled different on all operating systems. This file
implements a common functional base handling the DLLs for ScriptBasic. All other modules
of ScriptBasic that want to use DLLs should call only the functions implemented in this
file.

=toc

CUT*/

/* This is a global variable that is "violation" of thread safeness. This is
   used in the standalone version to ask the low level LoadLibrary function
   to display on the standard error debug messages when a library is not
   loadable. So don't worry about threads.
*/
int GlobalDebugDisplayFlag = 0;

/*POD
=H dynlolib_LoadLibrary
@c Load a library

This function loads a library and returns a pointer that can be used in other functions
referencing the loaded library.
/*FUNCTION*/
void *dynlolib_LoadLibrary(
  char *pszLibraryFile
  ){
/*noverbatim
The argument T<pszLibraryFile> is the ZCHAR file name.

The file name is either absolute or relative. When a relative file name is specified the
directories searched may be different on different operating systems.
CUT*/
  char *s,*r;
  void *pLib;

/* This code was modified in v1.0b16 to make a copy of the library string before altering \ and /
   This was needed, because the module loader depends on the actual file name to recognize
   modules already loaded. The user provided the exactly same file name for each function it declared
   but using forward slashes under WNT and the module loader loaded the module many times because 
   the stored file name was stored with the converted slash.
*/

  r = s = malloc(strlen(pszLibraryFile)+1);
  if( s == NULL )return NULL;
  strcpy(s,pszLibraryFile);

#ifdef _WIN32
  for(  ;  *s ; s++ )
    if( *s == '/') *s = '\\'; /* unix file separators -> DOS */
  pLib = (void *)LoadLibrary(r);
#elif defined(__DARWIN__)
  for( ;  *s ; s++ )
    if( *s == '\\' || *s == ':') *s = '/'; /* DOS/mac file separators -> unix */

  if( !_dyld_present() ){
    pLib = NULL;
    if( GlobalDebugDisplayFlag ) fprintf(stderr,"dynamic linking not available.\n");
    }
  else {
    pLib = (void *)NSAddImage(r, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
    if( pLib == NULL && GlobalDebugDisplayFlag ){
      NSLinkEditErrors c;
      int errorNumber;
      const char *fileName;
      const char *errorString;
      NSLinkEditError(&c,&errorNumber,&fileName,&errorString);
      fprintf( stderr, "%s", errorString );
      }
    }
#elif defined(__MACOS__)
    {
    FSSpec fileSpec;
    Str255 errMsg;
    int err, slen;
    Ptr mainAddr;
    char *hasColon = strchr(s, ':');
    pLib = NULL;
    
    /* Convert to Apple string (length + nonZ chars) */
    slen = strlen(r);
    memmove( r+1, r, slen);
    *r=(unsigned char)slen;
    
    if( hasColon ){
      err = FSMakeFSSpec (0, 0, r, &fileSpec);
      if( err == noErr)
        err = GetDiskFragment (&fileSpec, 0, 0, NULL, kLoadCFrag, (CFragConnectionID *)&pLib, mainAddr, errMsg );
      }
    else
      err = GetSharedLibrary( r, kPowerPCCFragArch, kLoadCFrag, (CFragConnectionID *)&pLib, mainAddr, errMsg );
    if( err < 0 && GlobalDebugDisplayFlag )
      fprintf(stderr,"GetSharedLibrary error %d, message=%*s\n", err, *errMsg, errMsg+1);
    }
#else
  for( ;  *s ; s++ )
    if( *s == '\\' || *s == ':') *s = '/'; /* DOS/mac file separators -> unix */
  pLib = (void *)dlopen(r,RTLD_LAZY);
  if( pLib == NULL && GlobalDebugDisplayFlag ){
    fprintf(stderr,"dlopen failed.\n");
    fprintf(stderr,"dlerror message=%s\n",dlerror());
    }
#endif
  free(r);
  return pLib;
  }

/*POD
=H dynlolib_FreeLibrary
@c Release a library

This function releases the library that was loaded before using R<dynlolib_LoadLibrary>
/*FUNCTION*/
void dynlolib_FreeLibrary(
  void *pLibrary
  ){
/*noverbatim
The argument T<pLibrary> is the pointer, which was returned by the function R<dynlolib_LoadLibrary>
CUT*/

#ifdef _WIN32
  FreeLibrary((HMODULE)pLibrary);
#elif defined(__DARWIN__)
  /* Can't unload a dylib on darwin */
#elif defined(__MACOS__)
  /* CloseConnection ( (CFragConnectionID *)&pLibrary ); */
  /* This seems to cause the app to exit completely - probably something to do with SIOUX */
#else
  dlclose(pLibrary);
#endif
  return;
  }

/*POD
=H dynlolib_GetFunctionByName
@c Get the entry point of a function by its name

This function can be used to get the entry point of a function of a loaded module
specifying the name of the function.
/*FUNCTION*/
void *dynlolib_GetFunctionByName(
  void *pLibrary,
  char *pszFunctionName
  ){
/*noverbatim
The argument T<pLibrary> is the pointer, which was returned by the function R<dynlolib_LoadLibrary>

The argument T<pszFunctionName> is the ZCAR function name.
CUT*/

#ifdef _WIN32
  return GetProcAddress((HMODULE)pLibrary,pszFunctionName);
#elif defined(__DARWIN__)
  char *fnName;
  NSSymbol sym;
  
  fnName = malloc(strlen(pszFunctionName)+2); /* C functions have underscore pre-appended */
  if( fnName==NULL ) return NULL;
  strcpy(fnName+1,pszFunctionName);
  *fnName = '_';
  
  sym = NSLookupSymbolInImage((struct mach_header *)pLibrary, fnName, 
          NSLOOKUPSYMBOLINIMAGE_OPTION_BIND | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
  free(fnName);
  if( !sym ) {
    if( GlobalDebugDisplayFlag ){
      NSLinkEditErrors c;
      int errorNumber;
      const char *fileName;
      const char *errorString;
      NSLinkEditError(&c,&errorNumber,&fileName,&errorString);
      fprintf( stderr, "%s", errorString );
      }
    return NULL;
    }
  return NSAddressOfSymbol(sym);
#elif defined(__MACOS__)
  {
  Str255 str255;
  void *symAdr = NULL;
  CFragSymbolClass symClass;
  int err;
  
  /* Convert to Apple string (length + nonZ chars) */
  *str255 = (unsigned char)strlen(pszFunctionName);
  memcpy( str255+1, pszFunctionName, *str255 );
  
  err = FindSymbol( (CFragConnectionID)pLibrary, str255, &symAdr, &symClass );
  if( err < 0 && GlobalDebugDisplayFlag )
    fprintf(stderr,"FindSymbol error %d: %s\n", err, pszFunctionName);
  return symAdr;
  }
#else
  return dlsym(pLibrary,pszFunctionName);
#endif
  }
