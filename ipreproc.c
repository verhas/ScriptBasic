/*
FILE: ipreproc.c
HEADER: ipreproc.h

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

#include "report.h"
#include "sym.h"
#include "lexer.h"
#include "expression.h"
#include "syntax.h"
#include "reader.h"
#include "myalloc.h"
#include "builder.h"
#include "memory.h"
#include "execute.h"
#include "prepext.h"

typedef struct _Preprocessor {
  void *pDllHandle;
  void *pFunction;
  char *pszPreprocessorName;
  struct _Preprocessor *next,*prev;
  Prepext pEXT; //extension structure that is passed to the preprocessor
  } Preprocessor, *pPreprocessor;

typedef struct _PreprocObject {
  void *pMemorySegment;
  unsigned long n;// the number of loaded preprocessors
  pPreprocessor pFirst,pLast;
  ExecuteObject EXE; // dummy execute object to handle support functions
  struct _SbProgram *pSB;
  }PreprocObject,*pPreprocObject;

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <process.h>
#else
#ifndef __MACOS__
#include <sys/types.h>
#include <sys/wait.h>
#endif
#endif

#include "errcodes.h"
#include "conftree.h"
#include "myalloc.h"
#include "uniqfnam.h"
#include "dynlolib.h"
#include "scriba.h"
#include "basext.h"
#include "prepext.h"
#include "ipreproc.h"

/*POD
=H Internal preprocessor handling
=abstract
This module loads the internal preprocessors
=end

=toc

CUT*/


/*POD
=section InitStructure
=H Initialize the preprocessor structure

This function is called after the T<PreprocObject> was allocated.
It initializes the preprocessor handling structures.
/*FUNCTION*/
void ipreproc_InitStructure(pPreprocObject pPre
  ){
/*noverbatim
CUT*/

  pPre->n = 0;
  pPre->pFirst = NULL;
  pPre->pLast = NULL;

/*
The preprocessor object includes an ExecuteObject. This is needed because
the execute object has a pointer to the support function table that any
extension including preprocessors can use to access system functions
through ScriptBasic. During reading, lexical analysis, syntax analysis
and building there is no real execution context and thus no support table.
(Yes, support table was originally designed for run-time extension modules.)
To have support function during these pre-run steps here we have a dummy
execution context that has nothing but the appropriate pointer to a support
function table. This way preprocessors can use the support functions and the
besXXX macros.
*/
  memset(&(pPre->EXE),0,sizeof(ExecuteObject));
  pPre->EXE.pST = NULL;
  pPre->EXE.pSTI = NULL;
  }

/*POD
=section PurgePreprocessorMemory
=H Release all memories allocated by preprocessors

This fucntion is called from the module T<scriba_*> to release all memory
that was allocated by the preprocessors and were not released.
/*FUNCTION*/
void ipreproc_PurgePreprocessorMemory(pPreprocObject pPre
  ){
/*noverbatim
CUT*/
  pPreprocessor p;

  for( p = pPre->pFirst ; p ; p = p->next ){
    alloc_FinishSegment(p->pEXT.pMemorySegment);
    p->pEXT.pMemorySegment = NULL;
    }
  alloc_FinishSegment(pPre->pMemorySegment);
  }

/*POD
=section InsertPreprocessor
=H Insert a new preprocessor into the preprocessor list

The preprocessors that are active are stored in a linked list.
When there is any action that needs a preprocessor this list is used
to invoke the preprocessors. The preprocessors are invoked in the order
they were entered into the system. For example if there are two lines
in the source code saying:

=verbatim
use pre1
use pre2
=noverbatim

then the preprocessor T<pre1> is loaded first and T<pre2> is loaded afterwards.
When a preprocessor is invoked the preprocesor T<pre1> is called first and T<pre2> is
called on the result. This function allocates a list element and inserts it to the end
of the list.

/*FUNCTION*/
pPreprocessor ipreproc_InsertPreprocessor(pPreprocObject pPre
  ){
/*noverbatim
The argument is the preprocessor object, environment.

The return value is pointer to the list element or T<NULL> if memory allocation occured.
CUT*/
  pPreprocessor pNewPreprocessor;

  pNewPreprocessor = alloc_Alloc(sizeof(Preprocessor),pPre->pMemorySegment);
  if( pNewPreprocessor == NULL )return NULL;

  pNewPreprocessor->next = NULL;

  /* if the list is not empty then */
  if( pPre->pLast ){/* append it to the end */
    pPre->pLast->next = pNewPreprocessor;
    pNewPreprocessor->prev = pPre->pLast;
    }else
    pNewPreprocessor->prev = NULL;
  /* this is the last one since now */
  pPre->pLast = pNewPreprocessor;

  /* if the list is empty then this is the first one as well */
  if( pPre->pFirst == NULL )
    pPre->pFirst = pNewPreprocessor;
  pPre->n++;

  return pNewPreprocessor;
  }

/*POD
=section DeleteOldPreprocessor
=H Delete a preprocessor from the list of preprocessors

This function deletes a preprocessor from the list of preprocessors.
The preprocessor was inserted into the list using the function
T<InsertPreprocessor>.

This function unhooks the element from the list and also releases the memory
that was occupied by the list element. The function does not unload the 
shared object (or DLL under Windows NT) from the memory.

/*FUNCTION*/
void ipreproc_DeletePreprocessor(pPreprocObject pPre,
                                 pPreprocessor pOld
  ){
/*noverbatim

The first argument is the preprocessor object environment. The second argument
is the pointer to the list element to be deleted.

CUT*/
  /* if this is not the first in the list then hook
     the previous element to the one that follows this one */
  if( pOld->prev )
    pOld->prev->next = pOld->next;
    else
    pPre->pFirst = pOld->next;

  /* if this is not the last one then hook the element that is after it to the previous one */
  if( pOld->next )
    pOld->next->prev = pOld->prev;
  else
    pPre->pLast = pOld->prev;

  pPre->n--;

  alloc_FinishSegment(pOld->pEXT.pMemorySegment);
  alloc_Free(pOld,pPre->pMemorySegment);
  }

/*POD
=section LoadInternalPreprocessor
=H Load an internal preprocessor

This function gets the name of an external preprocessor to load. The function
searches the configuration information for the named preprocessor, loads the DLL/SO
and invokes the initiation function of the preprocessor.

/*FUNCTION*/
int ipreproc_LoadInternalPreprocessor(pPreprocObject pPre,
                                      char *pszPreprocessorName
  ){
/*noverbatim
The first argument is the pointer to the ScriptBasic preprocessor object to access the
configuration information and the list of loaded preprocessors to put the actual one
on the list.

The second argument is the name of the preprocessor as named in the configuration file, for example

=verbatim
preproc (
  internal (
    sample "C:\\ScriptBasic\\bin\\samplepreprocessor.dll"
    )
=noverbatim

The return value is zero or the error code.

CUT*/
#define FNLEN 1024
  char szBuffer[FNLEN];
  char *s;
  void *pDllHandle,*pFunction;
  int (*preproc)(void *,long *,void *);
  pSbProgram pProgram;
  int iError;
  pPreprocessor pThisPre;
  long lCommand;
  int bFirst;
#define PREFLEN 17
  char *pszDllExtension;
  unsigned int cbDllExtension;
  CFT_NODE Node;

  pProgram = pPre->pSB;

  pszDllExtension = cft_GetString(pProgram->pCONF,"dll");
  if( pszDllExtension == NULL ){
#ifdef WIN32
    pszDllExtension = ".dll";
#elif defined(__DARWIN__)
    pszDllExtension = ".dylib";
#elif defined(__MACOS__)
    pszDllExtension = "";
#else
    pszDllExtension = ".so";
#endif
    }
  cbDllExtension = strlen(pszDllExtension);

  /* check that the preprocessor was not loaded yet */
  for( pThisPre = pPre->pFirst ; pThisPre ; pThisPre = pThisPre->next )
    if( !strcmp(pThisPre->pszPreprocessorName,pszPreprocessorName) )return COMMAND_ERROR_SUCCESS;

  strcpy(szBuffer,"preproc.internal.");
  if( strlen(pszPreprocessorName) > FNLEN - PREFLEN )return READER_ERROR_PREPROC_LONG;
  strcpy(szBuffer+PREFLEN,pszPreprocessorName);
  s = szBuffer+PREFLEN;
  while( *s && ! isspace(*s) )s++; /* chop off optional parameters and/or NL from the end of line */
  *s = (char)0;
  s = cft_GetString(pProgram->pCONF,szBuffer);
  /* if the internal preprocessor was not configured then it still can be used if the DLL or SO file
     is copied into the modules library. */
  if( NULL == s ){
      if( ! cft_GetEx(pProgram->pCONF,"module",&Node,&s,NULL,NULL,NULL) ){
        while( 1 ){
          if( cft_GetEx(pProgram->pCONF,NULL,&Node,&s,NULL,NULL,NULL) ){
            /* if there are no more directories in the configuration */
            break;
            }
          if( ! strcmp(cft_GetKey(pProgram->pCONF,Node),"module") ){
            if( strlen(s) + strlen(pszPreprocessorName) > FNLEN )return READER_ERROR_PREPROC_LONG;
            strcpy(szBuffer,s);
            strcat(szBuffer,pszPreprocessorName);
            if( strlen(szBuffer) + cbDllExtension > FNLEN )return READER_ERROR_PREPROC_LONG;
            strcat(szBuffer,pszDllExtension);
            pDllHandle = dynlolib_LoadLibrary( szBuffer );
            if( pDllHandle != NULL )break;
            }
          Node = cft_EnumNext(pProgram->pCONF,Node);
          }
        }
    }else{
    /* if the preprocessor was configured in the config file
       not only copied into one of the module directories */
    pDllHandle = dynlolib_LoadLibrary(s);
    }
  if( pDllHandle == NULL )return READER_ERROR_PREPROC_NOTAVA;

  pFunction = dynlolib_GetFunctionByName(pDllHandle,"preproc");
  if( pFunction == NULL )return READER_ERROR_PREPROC_NOTVAL;

  bFirst = (pPre->pFirst == NULL);
  pThisPre = ipreproc_InsertPreprocessor(pPre);
  if( pThisPre == NULL  )return COMMAND_ERROR_MEMORY_LOW;
  pThisPre->pszPreprocessorName = alloc_Alloc(strlen(pszPreprocessorName)+1,pPre->pMemorySegment);
  if( pThisPre->pszPreprocessorName == NULL )return COMMAND_ERROR_MEMORY_LOW;

  strcpy(pThisPre->pszPreprocessorName,pszPreprocessorName);
  pThisPre->pDllHandle = pDllHandle;
  pThisPre->pFunction = pFunction;
  pThisPre->pEXT.lVersion = IP_INTERFACE_VERSION;
  pThisPre->pEXT.pPointer = NULL;
  pThisPre->pEXT.pMemorySegment = alloc_InitSegment(pPre->pSB->maf,pPre->pSB->mrf);
  if( pThisPre->pEXT.pMemorySegment == NULL )return COMMAND_ERROR_MEMORY_LOW;

  /* if this is the first preprocessor loaded then init
     the support function table*/
  if( bFirst ){
    pPre->EXE.pMemorySegment = pPre->pMemorySegment;
    modu_Init(&(pPre->EXE),0);
    pPre->EXE.pST->pEo = &(pPre->EXE);
    pThisPre->pEXT.pST = pPre->EXE.pST;
    }

  preproc = pFunction;
  lCommand = PreprocessorLoad;
  iError = preproc(&(pThisPre->pEXT),&lCommand,NULL);
  if( lCommand == PreprocessorUnload ){
    /* unload the current preprocessor */
    pDllHandle = pThisPre->pDllHandle;
    ipreproc_DeletePreprocessor(pPre,pThisPre);
    /* this may happen if the preprocessor is statically linked */
    if( pDllHandle )
      dynlolib_FreeLibrary(pDllHandle);
    }
  return iError;
  }

/*POD
=section Process
=H Process preprocessor requests

This function is used by ScriptBasic at certain points of the execution to
start the preprocessors. It calls each loaded preprocessor one after the
other until there is no more preprocessors or one of them alters the command
variable to T<PreprocessorDone>.

This function gets three arguments.

/*FUNCTION*/
int ipreproc_Process(pPreprocObject pPre,
                     long lCommand,
                     void *pPointer
  ){
/*noverbatim
T<pRe> is the preprocessor object.

T<lCommand> is the command for the preprocessor to execute. For the possible
values look at the file T<prepext.h> (created from T<prepext.c>)
T<enum PreprocessorCommands>

T<pPointer> is a pointer to a structure. The structure actually depends on the
actual value of T<lCommand>. For different commands this pointer points to different
structures.

When the preprocessors are called they can alter the T<long> variable T<lCommand>
passed to them by reference.

When a preprocessor in this variable returns the value T<PreprocessorDone> the
preprocessing in the actual stage is stopped and no further proreprocessor
is invoked. However this has no effect on later preprocessor incocation.
Returning this value in this variable solely means that the preprocessor
has done all work that has to be done at the very point and thus there is
no need of further preprocessor handling.

When a preprocessor in this variable returns the value T<PreprocessorUnload>
the function unhooks the preprocessor from the list af active preprocessors,
releases all memory that the preprocessor used up and frees the library.

The return value of the preprocessor functions should be zero or error code.

The return value of the function T<ipreproc_Process> is zero or the error value of
a preprocessor. If a preprocessor returns a non-zero error code no further
preprocessors are invoked.

This function can not be used in situation when the preprocessors may return
other value in T<lCommand> than T<PreprocessorDone> or T<PreprocessorContinue>.
CUT*/
  pPreprocessor p,pn;
  long lCmd;
  int (*preproc)(void *,long *,void *);
  int iError;
  void *pDllHandle;

  p = pPre->pFirst;
  while( p ){
    lCmd = lCommand;
    if( lCommand == 21 ){
printf("");
}
    preproc = p->pFunction;
    iError = preproc(&(p->pEXT),&lCmd,pPointer);
    if( lCmd == PreprocessorDone )break;
    if( lCmd == PreprocessorUnload ){
      /* unload the current preprocessor */
      pDllHandle = p->pDllHandle;
      pn = p->next;
      ipreproc_DeletePreprocessor(pPre,p);
      dynlolib_FreeLibrary(pDllHandle);
      p = pn;
      continue;
      }
    if( iError )return iError;
    p = p->next;
    }
  return COMMAND_ERROR_SUCCESS;
  }


/*POD
=section preproc
=H Preprocessor function

This function has to be implemented in the external preprocessor and exported from the
DLL/SO file. It has to have the following prototype.

=verbatim
int DLL_EXPORT preproc(void *p, long *pFUN, void *q);

=noverbatim

CUT*/

/*
If the macro NO_IPREPROC is defined then the calls to ipreproc_Process will not be compiled into the
code and thus it will not waste time. This may be essential to improve the performance on a production
environment that executes already compiled code only and there is no need for anything like a preprocessor
or debugger.

TO_HEADER:
#ifdef NO_IPREPROC
#define ipreproc_Process(X,Y,Z)
#endif

*/
