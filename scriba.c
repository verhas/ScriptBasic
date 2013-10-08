/*
FILE:   scriba.c
HEADER: scriba.h

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
#include "lexer.h"
#include "sym.h"
#include "expression.h"
#include "syntax.h"
#include "reader.h"
#include "myalloc.h"
#include "builder.h"
#include "memory.h"
#include "execute.h"
#include "buildnum.h"
#include "conftree.h"
#include "filesys.h"
#include "errcodes.h"
#ifdef _DEBUG
#include "testalloc.h"
#endif
#include "command.h"
#include "epreproc.h"
#include "ipreproc.h"
#include "uniqfnam.h"
#include "modumana.h"
#include "ipreproc.h"

typedef struct _SbProgram {
  void *pMEM;
  void * (*maf)(size_t);
  void   (*mrf)(void *);
  unsigned long fErrorFlags;
  char *pszFileName;
  char *pszCacheFileName;
  char *FirstUNIXline;

  void *fpStdouFunction;
  void *fpStdinFunction;
  void *fpEnvirFunction;
  void *pEmbedder;
  void *fpReportFunction;
  void *pReportPointer;
  pSupportTable pSTI;
  ExecuteObject *pEPo;

  tConfigTree   *pCONF;
  ReadObject    *pREAD;
  LexObject     *pLEX;
  eXobject      *pEX;
  BuildObject   *pBUILD;
  ExecuteObject *pEXE;
  PreprocObject *pPREP;
  } SbProgram, *pSbProgram;

// type to pass and receive arguments and result values from ScriptBasic functions
typedef struct _SbData {
  unsigned char type;
  unsigned long size;
  union {
    double d;
    long   l;
    unsigned char *s;
    } v;
  } SbData, *pSbData;
#define SBT_UNDEF  0
#define SBT_DOUBLE 1
#define SBT_LONG   2
#define SBT_STRING 3
#define SBT_ZCHAR  4

// Access SbData content. Y is present to emulate class argument passing.
#define scriba_GetType(Y,X)   ( (X).type )
#define scriba_GetLength(Y,X) ( (X).size )
#define scriba_GetString(Y,X) ( (X).v.s  )
#define scriba_GetLong(Y,X)   ( (X).v.l  )
#define scriba_GetDouble(Y,X) ( (X).v.d  )


#ifdef WIN32
#define CONFIG_ENVIR "Software\\ScriptBasic\\config"
#define CONFIG_FILE  "SCRIBA.INI"
#else
#define CONFIG_ENVIR "SCRIBACONF"
#define CONFIG_FILE  "/etc/scriba/basic.conf"
#endif


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include "scriba.h"
#include "basext.h"

/*POD
=H scriba_new()

To create a new T<SbProgram> object you have to call this function. The two arguments
should point to T<malloc> and T<free> or similar functions. All later memory allocation
and releasing will be performed using these functions.

Note that this is the only function that does not require a pointer to an
T<SbProgram> object.

/*FUNCTION*/
pSbProgram scriba_new(void * (*maf)(size_t),
                      void   (*mrf)(void *)
  ){
/*noverbatim
CUT*/
  pSbProgram pProgram;
  void *p;

  p = alloc_InitSegment(maf,mrf);
  if( p == NULL )return NULL;

  pProgram = (pSbProgram)alloc_Alloc(sizeof(SbProgram),p);
  if( pProgram == NULL ){
    alloc_FinishSegment(p);
    return NULL;
    }

  pProgram->maf              = maf;
  pProgram->mrf              = mrf;
  pProgram->pMEM             = p;
  pProgram->fErrorFlags      = 0;
  pProgram->pszFileName      = NULL;
  pProgram->pszCacheFileName = NULL;
  pProgram->FirstUNIXline    = NULL;
  pProgram->fpStdouFunction  = NULL;
  pProgram->fpStdinFunction  = NULL;
  pProgram->fpEnvirFunction  = NULL;
  pProgram->pEmbedder        = NULL;
  pProgram->fpReportFunction = report_report;
  pProgram->pReportPointer   = (void *)stderr;
  pProgram->pSTI             = NULL;
  pProgram->pEPo             = NULL;

  pProgram->pCONF  = NULL;
  pProgram->pREAD  = NULL;
  pProgram->pLEX   = NULL;
  pProgram->pEX    = NULL;
  pProgram->pBUILD = NULL;
  pProgram->pEXE   = NULL;
  pProgram->pPREP  = NULL;

  return pProgram;
  }

/*POD
=H scriba_destroy()

After a ScriptBasic program was successfully execued and there is no need to
run it anymore call this function to release all memory associated with the
code.

/*FUNCTION*/
void scriba_destroy(pSbProgram pProgram
  ){
/*noverbatim
CUT*/


  scriba_PurgeReaderMemory(pProgram);
  scriba_PurgeLexerMemory(pProgram);
  scriba_PurgeSyntaxerMemory(pProgram);
  scriba_PurgeBuilderMemory(pProgram);
  scriba_PurgeExecuteMemory(pProgram);
  scriba_PurgePreprocessorMemory(pProgram);

  /* Note that finishing this segment will release the configuration 
     information in case it was loaded for this object and not 
     inherited from another one. */
  alloc_FinishSegment(pProgram->pMEM);
  }

/*POD
=H scriba_NewSbData()

Allocate and return a pointer to the allocated T<SbData> structure.

This structure can be used to store ScriptBasic variable data,
long, double or string. This function is called by other functions
from this module. Usually the programmer, who embeds ScriptBasic will rarely
call this function directly. Rather he/she will use R<scriba_NewSbLong()> (as an example)
that creates a variable capable holding a T<long>, sets the type to 
be T<SBT_LNG> and stores initial value.

See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.

/*FUNCTION*/
pSbData scriba_NewSbData(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  pSbData p;
  p = alloc_Alloc(sizeof(SbData),pProgram->pMEM);
  if( p )scriba_InitSbData(pProgram,p);
  return p;
  }

/*POD
=H scriba_InitSbData()

This function initializes an SbData structure to hold undef value.
This function should be used to initialize an allocated T<SbData>
memory structure. This function internally is called by R<scriba_NewSbData()>.

See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.

/*FUNCTION*/
void scriba_InitSbData(pSbProgram pProgram,
                         pSbData p
  ){
/*noverbatim
CUT*/
  p->v.s = NULL;
  p->type = SBT_UNDEF;
  }

/*POD
=H scriba_UndefSbData()

This function sets an T<SbData> structure to hold the undefined value.

This function should should not be used instead of R<scriba_InitSbData()>.
While that function should be used to inititalize the memory structure this
function should be used to set the value of an alreasdy initialized and probably
used T<SbData> variable to T<undef>.

The difference inside is that if the T<SbData> structure is a string then this
function releases the memory occupied by the string, while R<scriba_InitSbData()> does not.

See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.

/*FUNCTION*/
void scriba_UndefSbData(pSbProgram pProgram,
                        pSbData p
  ){
/*noverbatim
CUT*/
  if( p->type == SBT_STRING && p->v.s )alloc_Free(p->v.s,pProgram->pMEM);
  p->v.s = NULL;
  p->type = SBT_UNDEF;
  }


/*POD
=H scriba_NewSbLong()

This function allocates and returns a pointer pointing to a structure of
type T<SbData> holding a T<long> value. If the allocation failed the return
value is T<NULL>. If the memory allocation was successful the allocated
structure will have the type T<SBT_LONG> and will hold the initial value
specified by the argument T<lInitValue>.

/*FUNCTION*/
pSbData scriba_NewSbLong(pSbProgram pProgram,
                         long lInitValue
  ){
/*noverbatim
See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  pSbData p;

  p = scriba_NewSbData(pProgram);
  if( p == NULL )return NULL;
  p->type = SBT_LONG;
  p->v.l = lInitValue;
  return p;
  }

/*POD
=H scriba_NewSbDouble()

This function allocates and returns a pointer pointing to a structure of
type T<SbData> holding a T<double> value. If the allocation failed the return
value is T<NULL>. If the memory allocation was successful the allocated
structure will have the type T<SBT_DOUBLE> and will hold the initial value
specified by the argument T<dInitValue>.

/*FUNCTION*/
pSbData scriba_NewSbDouble(pSbProgram pProgram,
                           double dInitValue
  ){
/*noverbatim
See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  pSbData p;

  p = scriba_NewSbData(pProgram);
  if( p == NULL )return NULL;
  p->type = SBT_DOUBLE;
  p->v.d = dInitValue;
  return p;
  }

/*POD
=H scriba_NewSbUndef()

This function allocates and returns a pointer pointing to a structure of
type T<SbData> holding an T<undef> value. If the allocation failed the return
value is T<NULL>. If the memory allocation was successful the allocated
structure will have the type T<SBT_UNDEF>.

/*FUNCTION*/
pSbData scriba_NewSbUndef(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  pSbData p;

  p = scriba_NewSbData(pProgram);
  if( p == NULL )return NULL;
  p->type = SBT_UNDEF;
  return p;
  }


/*POD
=H scriba_NewSbString()

This function allocates and returns a pointer pointing to a structure of
type T<SbData> holding a string value. If the allocation failed the return
value is T<NULL>. If the memory allocation was successful the allocated
structure will have the type T<SBT_STRING> and will hold the initial value
specified by the argument T<pszInitValue>.

/*FUNCTION*/
pSbData scriba_NewSbString(pSbProgram pProgram,
                           char *pszInitValue
  ){
/*noverbatim
B<Note on ZCHAR termination:>

The init value T<pszInitValue> should be a zchar terminated string. Note that
ScriptBasic internally stores the strings as series byte and the length of the
string without any terminating zchar. Therefore the length of the string
that is stored should have been T<strlen(pszInitValue)>. This does not contain the
terminating zchar.

In reality however we allocate an extra byte that stores the zchar, but the
size of the string is one character less. Therefore ScriptBasic routines
will recognize the size of the string correct and also the caller can
use the string using the macro T<scriba_GetString> as a zchar terminated
C string. This requires an extra byte of storage for each string passed from the
embedding C application to ScriptBasic, but saves a lot of hedeache and also
memory copy when the string has to be used as a zchar terminated string.

See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  pSbData p;

  if( pszInitValue == NULL )return scriba_NewSbUndef(pProgram);

  p = scriba_NewSbData(pProgram);
  if( p == NULL )return NULL;
  p->type = SBT_STRING;
  p->size = strlen(pszInitValue);
  if( p->size ){
    p->v.s = alloc_Alloc(p->size+1,pProgram->pMEM);
    if( p->v.s == NULL ){
      alloc_Free(p,pProgram->pMEM);
      return NULL;
      }
    memcpy(p->v.s,pszInitValue,p->size+1);
    }else{
    p->v.s = NULL;
    }
  return p;
  }

/*POD
=H scriba_NewSbBytes()

This function allocates and returns a pointer pointing to a structure of
type T<SbData> holding a string value. If the allocation failed the return
value is T<NULL>. If the memory allocation was successful the allocated
structure will have the type T<SBT_STRING> and will hold the initial value
specified by the argument T<pszInitValue> of the length T<len>.

/*FUNCTION*/
pSbData scriba_NewSbBytes(pSbProgram pProgram,
                          unsigned long len,
                          unsigned char *pszInitValue
  ){
/*noverbatim
This function allocates T<len>+1 number of bytes data and
stores the initial value pointed by T<pszInitValue> in it.

The extra plus one byte is an extra terminating zero char
that may help the C programmers to handle the string
in case it is not binary. Please also read the not on the terminating ZChar
in the function R<scriba_NewSbString()>.

See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  pSbData p;

  if( pszInitValue == NULL )return scriba_NewSbUndef(pProgram);

  p = scriba_NewSbData(pProgram);
  if( p == NULL )return NULL;
  p->type = SBT_STRING;
  p->size = len;
  if( p->size ){
    p->v.s = alloc_Alloc(p->size+1,pProgram->pMEM);
    if( p->v.s == NULL ){
      alloc_Free(p,pProgram->pMEM);
      return NULL;
      }
    memcpy(p->v.s,pszInitValue,p->size);
    /* for the sake of lasy C programmers we store a terminating zchar */
    p->v.s[p->size] = (char)0;
    }else{
    p->v.s = NULL;
    }
  return p;
  }

/*POD
=H scriba_DestroySbData()

Call this function to release the memory that was allocated by any
of the T<NewSbXXX> functions. This function releases the memory and
also cares to release the memory occupied by the characters in case the
value had the type T<SBT_STRING>.

/*FUNCTION*/
void scriba_DestroySbData(pSbProgram pProgram,
                          pSbData p
  ){
/*noverbatim
See also R<scriba_NewSbLong()>, R<scriba_NewSbDouble()>, R<scriba_NewSbUndef()>, R<scriba_NewSbString()>,
R<scriba_NewSbBytes()>, R<scriba_DestroySbData()>.
CUT*/
  if( p->type == SBT_STRING )
    alloc_Free(p->v.s,pProgram->pMEM);
  alloc_Free(p,pProgram->pMEM);
  }

/*POD
=H scriba_PurgeReaderMemory()

Call this function to release all memory that was allocated by the
reader module. The memory data is needed so long as long the lexical analyzer
has finished.
/*FUNCTION*/
void scriba_PurgeReaderMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  if( pProgram->pREAD ){
    alloc_FinishSegment(pProgram->pREAD->pMemorySegment);
    alloc_Free(pProgram->pREAD,pProgram->pMEM);
    }
  pProgram->pREAD = NULL;
  }

/*POD
=H scriba_PurgeLexerMemory()

/*FUNCTION*/
void scriba_PurgeLexerMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  if( pProgram->pLEX )
    alloc_FinishSegment(pProgram->pLEX->pMemorySegment);
  alloc_Free(pProgram->pLEX,pProgram->pMEM);
  pProgram->pLEX = NULL;
  }

/*POD
=H scriba_PurgeSyntaxerMemory()

/*FUNCTION*/
void scriba_PurgeSyntaxerMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  if( pProgram->pEX )
    ex_free( pProgram->pEX );
  alloc_Free(pProgram->pEX,pProgram->pMEM);
  pProgram->pEX = NULL;
  }

/*POD
=H scriba_PurgeBuilderMemory()

/*FUNCTION*/
void scriba_PurgeBuilderMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  if( pProgram->pBUILD && pProgram->pBUILD->pMemorySegment)
    alloc_FinishSegment(pProgram->pBUILD->pMemorySegment);
  alloc_Free(pProgram->pBUILD,pProgram->pMEM);
  pProgram->pBUILD = NULL;
  }


/*POD
=H scriba_PurgePreprocessorMemory()

This function purges the memory that was needed to run the preprocessors.

/*FUNCTION*/
void scriba_PurgePreprocessorMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/

  if( pProgram->pPREP ){
    ipreproc_PurgePreprocessorMemory(pProgram->pPREP);
    alloc_Free(pProgram->pPREP,pProgram->pMEM);
    pProgram->pPREP = NULL;
    }
  }

/*POD
=H scriba_PurgeExecuteMemory()

This function purges the memory that was needed to execute the program,
but before that it executes the finalization part of the execution.

/*FUNCTION*/
void scriba_PurgeExecuteMemory(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  int iErrorCode;

  if( pProgram->pEXE ){
    execute_FinishExecute(pProgram->pEXE,&iErrorCode);
    if( pProgram->pEXE->pMo &&
        pProgram->pEXE->pMo->pMemorySegment )alloc_FinishSegment(pProgram->pEXE->pMo->pMemorySegment);
    alloc_FinishSegment(pProgram->pEXE->pMemorySegment);
    }
  alloc_Free(pProgram->pEXE,pProgram->pMEM);
  pProgram->pEXE = NULL;
  }

/*POD
=H scriba_SetFileName()

Call this function to set the file name where the source informaton is.
This file name is used by the functions R<scriba_LoadBinaryProgram()> and
R<scriba_LoadSourceProgram> as well as error reporting functions to display
the location of the error.

/*FUNCTION*/
int scriba_SetFileName(pSbProgram pProgram,
                       char *pszFileName
  ){
/*noverbatim
The argument T<pszFileName> should be zchar terminated
string holding the file name.
CUT*/
  if( pProgram->pszFileName )alloc_Free(pProgram->pszFileName,pProgram->pMEM);
  pProgram->pszFileName = NULL;
  if( pszFileName ){
    pProgram->pszFileName = alloc_Alloc(strlen(pszFileName)+1,pProgram->pMEM);
    if( pProgram->pszFileName == NULL )SCRIBA_ERROR_MEMORY_LOW;
    strcpy(pProgram->pszFileName,pszFileName);
    }
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_GettingConfiguration()

R<scriba_LoadConfiguration()> and R<scriba_InheritConfiguration()> can be used to
specify configuration information for a ScriptBasic program. Here
we describe the differences and how to use the two functions for
single-process single-basic and for single-process multiple-basic
applications.

To execute a ScriptBasic program you usually need configuration information.
The configuration information for the interpreter is stored in a file.
The function R<scriba_LoadConfiguration()> reads the file and loads it into memory
into the T<SbProgram> object. When the object is destroyed the configuration
information is automatically purged from memory.

Some implementations like the Eszter SB Engine variation of ScriptBasic starts
several interpreter thread within the same process. In this case the configuration
information is read only once and all the running interpreters share the same
configuration information.

To do this the embedding program has to create a pseudo T<SbProgram> object that
does not run any ScriptBasic program, but is used only to load the configuration
information calling the function R<scriba_LoadConfiguration()>. Other T<SbProgram> objects
that do intepret ScriptBasic program should inherit this configuration calling the
function R<scriba_InheritConfiguration()>. When a T<SbProgram> object is destroyed the
configuration is not destroyed if that was inherited belonging to a different object.
It remains in memory and can later be used by other intrepreter instances.

Inheriting the configuration is fast because it does not require loading the
configuration information from file. This is essentially sets a pointer in the
internal interpreter structure to point to the configuration information held
by the other object and all the parallel running interpreters structures
point to the same piece of memory holding the common configuration information.

See the configuration handling functions R<scriba_LoadConfiguration()> and R<scriba_InheritConfiguration()>.
CUT*/

/*POD
=H scriba_LoadConfiguration()

This function should be used to load the configuration information
from a file.

The return value is zero on success and the error code when error happens.
/*FUNCTION*/
int scriba_LoadConfiguration(pSbProgram pProgram,
                             char *pszForcedConfigurationFileName
  ){
/*noverbatim
CUT*/
  int iError;
  pProgram->pCONF = alloc_Alloc(sizeof(tConfigTree),pProgram->pMEM);
  if( pProgram->pCONF == NULL )return SCRIBA_ERROR_MEMORY_LOW;

  iError = cft_start(pProgram->pCONF,alloc_Alloc,alloc_Free,pProgram->pMEM,CONFIG_ENVIR,CONFIG_FILE,pszForcedConfigurationFileName);
  return iError;
  }

/*POD
=H scriba_GetConfigFileName()

This function tells whet the configuration file is. There is no need to call this function to read the configuration file.
This is needed only when the main program want to manipulate the configuration file in some way. For example the command
line version of ScriptBasic uses this function when the option T<-k> is used to compile a configuration file.

The first argument has to be a valid ScriptBasic program object. The second argument should point to a valid T<char *> pointer that
will get the pointer value to the configuration file name after the function returns.
/*FUNCTION*/
int scriba_GetConfigFileName(pSbProgram pProgram,
                             char **ppszFileName
  ){
/*noverbatim
The function returns zero if no error happens, or the error code.
CUT*/
  int iError;

  iError = cft_GetConfigFileName(pProgram->pCONF,ppszFileName,CONFIG_ENVIR,CONFIG_FILE);
  return iError;
  }

/*POD
=H scriba_InheritConfiguration()

Use this function to get the configuration from another program object.

The return value is zero on success and error code if error has happened.
/*FUNCTION*/
int scriba_InheritConfiguration(pSbProgram pProgram,
                                pSbProgram pFrom
  ){
/*noverbatim
CUT*/
  if( pFrom == NULL )return 1;
  pProgram->pCONF = pFrom->pCONF;
  if( pProgram->pCONF == NULL )return 1;
  return 0;
  }

/*POD
=H scriba_InitModuleInterface()

Initialize the Support Function Table of a process level ScriptBasic program object to be inherited
by other program objects. If you read it first time, read on until you understand what this
function really does and rather how to use it!

This is going to be a bit long, but you better read it along with the documentation of the
function R<scriba_InheritModuleInterface()>.

This function is needed only for programs that are
=itemize
=item multi thread running several interpreters simultaneous in a single process
=item support modules like the sample module T<mt> that support multithread behaviour and
      need to implement worker thread needing call-back functions.
=noitemize

You most probably know that modules can access system and ScriptBasic fucntions via a
call-back table. That is a huge T<struct> containing pointers to the functions that
ScriptBasic implements. This is the T<ST> (aka support table).

This helps module writers to write system independent
code as well as to access ScriptBasic functions easily. On the other hand modules are
also free to alter this table and because many functions, tough not all are called via this
table by ScriptBasic itself a module may alter the core behavior of ScriptBasic.

For this reason each interpreter has its own copy of T<ST>.
This means that if an interpreter alters the table it has no effect on another interpreter
running in the same process in anther thread.

This is fine so far. How about modules that run asynchronous threads? For example the very first
interpter thread that uses the module T<mt> starts in the initialization a thread that later 
deletes all sessions that time out. This thread lives a long life.

The thread that starts the worker thread is an interpreter thread and has its own copy of the T<ST>.
The thread started asynchronous however should not use this T<ST> because the table is purged 
from memory when the interpreter instance it blelonged to finishes.

To have T<ST> for worker threads there is a need for a program object that is not purged
from memory so long as long the process is alive. Fortunately there is such an object: the
configuration program object. Configuration is usually read only once by multi-thread implementations
and the same configuration information is shared by the serveral threads. The same way the
several program objects may share a T<ST>.

The difference is that configuration is NOT altered by the interpreter or by any module in any way
but T<ST> may. Thus each  execution object has two pointers: T<pST> and T<pSTI>. While T<pST> points to
the support table that belongs to the interpreter instance the secondpointer T<pSTI> points to
a T<ST> that is global for the whole process and is permanent. This T<ST> is to be used by worker threads
and should not be altered by the module without really good reason.

Thus: Don't call this function for normal program objects! For usualy program objects module
interface is automatically initialized when the first module function is called. Call this function
to initialize a T<ST> for a pseudo program object that is never executed but rather used to inherit this
T<ST> for worker threads.

/*FUNCTION*/
int scriba_InitModuleInterface(pSbProgram pProgram
  ){
/*noverbatim
CUT*/

  if( pProgram->pEXE == NULL ){
    pProgram->pEXE = alloc_Alloc( sizeof(ExecuteObject) , pProgram->pMEM );
    if( pProgram->pEXE == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    /* The support table should be set to NULL because this is checked by modu_Init. */
    pProgram->pEXE->pST = NULL;
    /* The inherited execution object is itself so that besPROCXXX works correct. */
    pProgram->pEXE->pEPo = pProgram->pEXE;
    thread_InitMutex( &(pProgram->pEXE->mxModules) );
    pProgram->pEXE->memory_allocating_function = pProgram->maf;
    pProgram->pEXE->memory_releasing_function = pProgram->mrf;
    pProgram->pEXE->pMemorySegment = alloc_InitSegment(pProgram->pEXE->memory_allocating_function,
                                                       pProgram->pEXE->memory_releasing_function);
    if( pProgram->pEXE->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    /* we set this field also and thus in case the config program object and the module interface
       program object is the same the external module worker thread can access config information */
    pProgram->pEXE->pConfig = pProgram->pCONF;
    }
  modu_Init(pProgram->pEXE,1);
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_InheritModuleInterface()

Inherit the support function table (T<ST>) from another program object.

Note that the program object is going to initialize its own T<ST> the normal
way. The inherited T<ST> will only be used by worker threads that live a long
life and may exist when the initiating interpreter thread already exists.

For further information please read the description of the function R<scriba_InitModuleInterface()>.

/*FUNCTION*/
int scriba_InheritModuleInterface(pSbProgram pProgram,
                                  pSbProgram pFrom
  ){
/*noverbatim
CUT*/

  pProgram->pSTI = pFrom->pEXE->pST;
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_InheritExecuteObject()

/*FUNCTION*/
int scriba_InheritExecuteObject(pSbProgram pProgram,
                                  pSbProgram pFrom
  ){
/*noverbatim
CUT*/
  pProgram->pEPo = pFrom->pEXE;
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_SetProcessSbObject()

Use this program in multi-thread environment to tell the actual interpreter
which object is the process level pseudo object that 

=itemize
=item holds the shared (among interpreter thread objects) configuration 
information (see R<scriba_InheritConfiguration()>)
=item holds the process level module interface (see R<scriba_InheritModuleInterface()>)
=item holds the list of loaded modules that are not unloaded by the thread loaded the module
=noitemize

If the embeddingprogram calls this function there is no need to call R<scriba_InheritConfiguration()>
and R<scriba_InheritModuleInterface()>. This function call does all those tasks and also other things.

/*FUNCTION*/
int scriba_SetProcessSbObject(pSbProgram pProgram,
                              pSbProgram pProcessObject
  ){
/*noverbatim
CUT*/
  int iError;

  if( iError = scriba_InheritConfiguration(pProgram,pProcessObject) )return iError;
  if( iError = scriba_InheritModuleInterface(pProgram,pProcessObject) )return iError;
  if( iError = scriba_InheritExecuteObject(pProgram,pProcessObject) )return iError;
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_ShutdownMtModules()

A multi threaded application should call this function for the process SB object
when the process finishes. Calling this function will call each of the shutdown
functions of those external modules that decided to keep in memory and export
the shutdown function named T<shutmodu>. This allows these modules to gracefully
shut down their operation. As an example cached data can be written to disk, or
database connections can be closed.

/*FUNCTION*/
int scriba_ShutdownMtModules(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  pModule pThisModule;

  thread_LockMutex( &(pProgram->pEXE->mxModules) );
  pThisModule = pProgram->pEXE->modules;
  while( pThisModule ){
    modu_ShutdownModule(pProgram->pEXE,pThisModule);
    pThisModule = pThisModule->next;
    }
  thread_UnlockMutex( &(pProgram->pEXE->mxModules) );
  return SCRIBA_ERROR_SUCCESS;
  }


/*POD
=H scriba_SetCgiFlag()

You can call this function to tell the reporting subsystem that
this code runs in a CGI environment and therefore it should format
error messages according to the CGI standard sending to the 
standard output including HTTP headers and HTML code pieces.

/*FUNCTION*/
void scriba_SetCgiFlag(pSbProgram pProgram
  ){
/*noverbatim
CUT*/
  pProgram->fErrorFlags |= REPORT_F_CGI;
  }

/*POD
=H scriba_SetReportFunction()

This function should be used to set the report function for a program. The report function
is used to send info, warning, error, fatal and internal error messages to the user.

In case you want to implement a specific report function see the sample implementation in the
file T<report.c>. The documentation of the function T<report_report> describes not only the details
of the sample implementation but also the implementation requests for other reporting functions.

/*FUNCTION*/
void scriba_SetReportFunction(pSbProgram pProgram,
                              void *fpReportFunction
  ){
/*noverbatim
CUT*/
  pProgram->fpReportFunction = fpReportFunction;
  if( pProgram->pEXE )pProgram->pEXE->report = fpReportFunction;
  }

/*POD
=H scriba_SetReportPointer()

This pointer will be passed to the reporting function. The default
reporting uses this pointer as a T<FILE *> pointer. The default value
for this pointer is T<stderr>.

Other implementations of the reporting function may use this pointer
according their needs. For example the WIN32 IIS ISAPI implementation
uses this pointer to point to the extension controll block structure.
/*FUNCTION*/
void scriba_SetReportPointer(pSbProgram pProgram,
                             void *pReportPointer
  ){
/*noverbatim
CUT*/
  pProgram->pReportPointer = pReportPointer;
  if( pProgram->pEXE )pProgram->pEXE->reportptr = pReportPointer;
  }

/*POD
=H scriba_SetStdin()

You can call this function to define a special standard input function. This
pointer should point to a function that accepts a T<void *> pointer
as argument. Whenever the ScriptBasic program tries to read from the
standard input it calls this function pasing the embedder pointer as
argument.

If the T<stdin> function is not defined or the parameter is T<NULL>
the interpreter will read the normal T<stdin> stream.

/*FUNCTION*/
void scriba_SetStdin(pSbProgram pProgram,
                     void *fpStdinFunction
  ){
/*noverbatim
CUT*/
  pProgram->fpStdinFunction = fpStdinFunction;
  if( pProgram->pEXE )pProgram->pEXE->fpStdinFunction = fpStdinFunction;
  }

/*POD
=H scriba_SetStdout()

You can call this function to define a special standard output function. This
pointer should point to a function that accepts a T<(char, void *)> arguments.
Whenever the ScriptBasic program tries to send a character to the standard output
it calls this function. The first parameter is the character to write, the second
is the embedder pointer.

If the standard output function is not defined or the parameter is T<NULL>
the interpreter will write the normal T<stdout> stream.

/*FUNCTION*/
void scriba_SetStdout(pSbProgram pProgram,
                      void *fpStdoutFunction
  ){
/*noverbatim
CUT*/
  pProgram->fpStdouFunction = fpStdoutFunction;
  if( pProgram->pEXE )pProgram->pEXE->fpStdouFunction = fpStdoutFunction;
  }

/*POD
=H scriba_SetEmbedPointer()

This function should be used to set the embed pointer.

The embed pointer is a pointer that is not used by ScriptBasic itself. This
pointer is remembered by ScriptBasic and is passed to call-back functions.
Like the standard input, output and environment functions that the embedding
application may provide this pointer is also available to external modules implemented
in C or other compiled language in DLL or SO files.

The embedder pointer should usually point to the T<struct> of the thread local data.
For example the Windows NT IIS variation of ScriptBasic sets this variable to point to
the extension control block.

If this pointer is not set ScriptBasic will pass T<NULL> pointer to the extensions and
to the call-back function.
/*FUNCTION*/
void scriba_SetEmbedPointer(pSbProgram pProgram,
                            void *pEmbedder
  ){
/*noverbatim
CUT*/
  pProgram->pEmbedder = pEmbedder;
  if( pProgram->pEXE )pProgram->pEXE->pEmbedder = pEmbedder;
  }

/*POD
=H scriba_SetEnvironment()

You can call this function to define a special environment query function. This
pointer should point to a function that accepts a T<(void *, char *, long )> arguments.

Whenever the ScriptBasic program tries to get the value of an enviroment variable
it calls this function. The first argument is the embedder pointer.

The second argument is the name of the environment variable to retrieve or T<NULL>.

The third argument is either zero or is the serial number of the environment variable.

ScriptBasic never calls this function with both specifying the environment variable name
and the serial number.

The return value of the function should either be T<NULL> or should point to a string that
holds the zero character terminated value of the environment variable. This string is not
changed by ScriptBasic.

If the special environment function is not defined or is T<NULL> ScriptBasic uses the
usual environment of the process calling the system functionT<getenv>.

/*FUNCTION*/
void scriba_SetEnvironment(pSbProgram pProgram,
                           void *fpEnvirFunction
  ){
/*noverbatim
For a good example of a self-written environment function see the source of the Eszter SB Engine
that alters the environment function so that the ScriptBasic programs feel as if they were executed in a
real CGI environment.
CUT*/
  pProgram->fpEnvirFunction = fpEnvirFunction;
  if( pProgram->pEXE )pProgram->pEXE->fpEnvirFunction = fpEnvirFunction;
  }


/*POD
=H scriba_LoadBinaryProgramWithOffset()

Use this function to load ScriptBasic program from a file that is already compiled into
internal form, and the content of the program is starting on T<lOffset>

The return value is the number of errors (hopefully zero) during program load.

/*FUNCTION*/
int scriba_LoadBinaryProgramWithOffset(pSbProgram pProgram,
                                       long lOffset,
                                       long lEOFfset
  ){
/*noverbatim
Before calling this function the function R<scriba_SetFileName()> should have been called specifying the
file name.
CUT*/
  pProgram->pBUILD = alloc_Alloc( sizeof(BuildObject) , pProgram->pMEM);
  if( pProgram->pBUILD == NULL )return 1;

  pProgram->pBUILD->memory_allocating_function = pProgram->maf;
  pProgram->pBUILD->memory_releasing_function  = pProgram->mrf;
  pProgram->pBUILD->iErrorCounter = 0;
  pProgram->pBUILD->reportptr = pProgram->pReportPointer;
  pProgram->pBUILD->report   = pProgram->fpReportFunction;
  pProgram->pBUILD->fErrorFlags = pProgram->fErrorFlags;
  build_LoadCodeWithOffset(pProgram->pBUILD,pProgram->pszFileName,lOffset,lEOFfset);
  return pProgram->pBUILD->iErrorCounter;
  }

/*POD
=H scriba_LoadBinaryProgram()

Use this function to load ScriptBasic program from a file that is already compiled into
internal form.

The return value is the number of errors (hopefully zero) during program load.

/*FUNCTION*/
int scriba_LoadBinaryProgram(pSbProgram pProgram
  ){
/*noverbatim
Before calling this function the function R<scriba_SetFileName()> should have been called specifying the
file name.
CUT*/
  return scriba_LoadBinaryProgramWithOffset(pProgram,0L,0L);
  }

/*POD
=H scriba_InheritBinaryProgram()

Use this function in application that keeps the program code in memory.

/*FUNCTION*/
int scriba_InheritBinaryProgram(pSbProgram pProgram,
                                pSbProgram pFrom
  ){
/*noverbatim

The function inherits the binary code from the program object T<pFrom>.
In server type applications the compiled binary code of a BASIC program may
be kept in memory. To do this a pseudo program object should be created that
loads the binary code and is not destroyed.

The program object used to execute the code should inherit the binary code from
this pseudo object calling this function. This is similar to the configuration
inheritance.

CUT*/
  pProgram->pBUILD = alloc_Alloc( sizeof(BuildObject) , pProgram->pMEM);
  if( pProgram->pBUILD == NULL )return SCRIBA_ERROR_MEMORY_LOW;

  memcpy(pProgram->pBUILD,pFrom->pBUILD,sizeof(BuildObject));
  pProgram->pBUILD->memory_allocating_function = pProgram->maf;
  pProgram->pBUILD->memory_releasing_function  = pProgram->mrf;
  pProgram->pBUILD->iErrorCounter = 0;
  pProgram->pBUILD->reportptr = pProgram->pReportPointer;
  pProgram->pBUILD->report   = pProgram->fpReportFunction;
  pProgram->pBUILD->fErrorFlags = pProgram->fErrorFlags;
  return SCRIBA_ERROR_SUCCESS;
  }


/*POD
=H scriba_LoadInternalPreprocessor()

This function can be used by embedding applications to load an
internal preprocessor into the interpereter. Note that preprocessors
are usually loaded by the reader module when a T<preprocess> statement
is found. However some preprocessors in some variation of the interpreter
may be loaded due to configuration or command line option and not
because the source requests it.

The preprocessors that are requested to be loaded because the source
contains a T<preprocess> line usually implement special language
fetures. The preprocessors that are loaded independent of the source
because command line option or some other information tells the variation
to call this function are usually debuggers, profilers.

(To be honest, by the time I write it there is no any internal preprocessors
developed except the test one, but the statement above will become true.)

/*FUNCTION*/
int scriba_LoadInternalPreprocessor(pSbProgram pProgram,
                            char *ppszPreprocessorName[]
  ){
/*noverbatim
The first argument is the program object. If the program object does not
have a preprocessor object the time it is called the preprocessor object is
created and initiated.

The second argument is the array of names of the preprocessor as it is present
in the configuration file. This is not the name of the DLL/SO file, but
rather the symbolic name, which is associated with the file. The final element
of the array has to be T<NULL>.

The return value is zero or the error code.
CUT*/
  int iError,i;

  /* if the program object does not have a PreprocObject then create it */
  if( pProgram->pPREP == NULL ){
    pProgram->pPREP = alloc_Alloc( sizeof(PreprocObject) , pProgram->pMEM );
    if( pProgram->pPREP == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    ipreproc_InitStructure(pProgram->pPREP);
    pProgram->pPREP->pMemorySegment = alloc_InitSegment(pProgram->maf,
                                                        pProgram->mrf);
    if( pProgram->pPREP->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    pProgram->pPREP->pSB = pProgram;
    }

  for( i = 0 ; ppszPreprocessorName[i] ; i++ )
    if( iError = ipreproc_LoadInternalPreprocessor(
                   pProgram->pPREP,
                   ppszPreprocessorName[i]) )return iError;
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H scriba_ReadSource()

Loads the source code of a ScriptBasic program from a text file.

The return code is the number of errors happened during read.

/*FUNCTION*/
int scriba_ReadSource(pSbProgram pProgram
  ){
/*noverbatim
B<Do not get confused!> This function only reads the source. Does not compile it.
You will usually need R<scriba_LoadSourceProgram()> that does reading, analyzing, building
and all memory releases leaving finally a ready-to-run code in memory.

Before calling this function the function R<scriba_SetFileName()> should have been called specifying the
file name.

See also R<scriba_ReadSource()>, R<scriba_DoLexicalAnalysis()>,
R<scriba_DoSyntaxAnalysis()>, R<scriba_BuildCode()>.
CUT*/

  pProgram->pREAD = alloc_Alloc( sizeof(ReadObject) , pProgram->pMEM );
  if( pProgram->pREAD == NULL )return SCRIBA_ERROR_MEMORY_LOW;

  reader_InitStructure(pProgram->pREAD);
  pProgram->pREAD->memory_allocating_function = alloc_Alloc;
  pProgram->pREAD->memory_releasing_function = alloc_Free;
  pProgram->pREAD->pMemorySegment = alloc_InitSegment(
                                     pProgram->maf,
                                     pProgram->mrf);
  if( pProgram->pREAD->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
  pProgram->pREAD->report = pProgram->fpReportFunction;
  pProgram->pREAD->reportptr = pProgram->pReportPointer;
  pProgram->pREAD->iErrorCounter = 0;
  pProgram->pREAD->fErrorFlags = pProgram->fErrorFlags;
  pProgram->pREAD->pConfig = pProgram->pCONF;

  /* here we have to initialize the preprocessor object 
     if it was not initialized before because the reader uses that */
  if( pProgram->pPREP == NULL ){
    pProgram->pPREP = alloc_Alloc( sizeof(PreprocObject) , pProgram->pMEM );
    if( pProgram->pPREP == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    ipreproc_InitStructure(pProgram->pPREP);
    pProgram->pPREP->pMemorySegment = alloc_InitSegment(
                                       pProgram->maf,
                                       pProgram->mrf);
    if( pProgram->pPREP->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    pProgram->pPREP->pSB = pProgram;
    }
  pProgram->pREAD->pPREP = pProgram->pPREP; /* this is needed by the reader to handle the internal preprocessors */

  if( ! reader_ReadLines(pProgram->pREAD,pProgram->pszFileName) ){
    if( pProgram->pREAD->FirstUNIXline ){
      pProgram->FirstUNIXline = alloc_Alloc(strlen(pProgram->pREAD->FirstUNIXline)+1,pProgram->pMEM);
      if( pProgram->FirstUNIXline == NULL )return SCRIBA_ERROR_MEMORY_LOW;
      strcpy(pProgram->FirstUNIXline,pProgram->pREAD->FirstUNIXline);
      }
    }
  return pProgram->pREAD->iErrorCounter;
  }

/*POD
=H scriba_DoLexicalAnalysis()

This function performs lexical analysis after the source file has beed read.

This function is rarely needeed by applicationdevelopers. See R<scriba_LoadSourceProgram()>
instead.
/*FUNCTION*/
int scriba_DoLexicalAnalysis(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_ReadSource()>, R<scriba_DoLexicalAnalysis()>,
R<scriba_DoSyntaxAnalysis()>, R<scriba_BuildCode()>.
CUT*/
  pProgram->pLEX = alloc_Alloc( sizeof(LexObject) , pProgram->pMEM );
  if( pProgram->pLEX == NULL )return 1;

  reader_StartIteration(pProgram->pREAD);

  pProgram->pLEX->memory_allocating_function = alloc_Alloc;
  pProgram->pLEX->memory_releasing_function = alloc_Free;
  pProgram->pLEX->pMemorySegment = alloc_InitSegment(
                                       pProgram->maf,
                                       pProgram->mrf);
  if( pProgram->pLEX->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
  /* Inherit the preprocessor object to the lexical analyzer */
  pProgram->pLEX->pPREP = pProgram->pPREP;
  lex_InitStructure(  pProgram->pLEX);

  pProgram->pLEX->pfGetCharacter = reader_NextCharacter;
  pProgram->pLEX->pfFileName = reader_FileName;
  pProgram->pLEX->pfLineNumber = reader_LineNumber;

  if( pProgram->pLEX->pNASymbols == NULL )pProgram->pLEX->pNASymbols = NASYMBOLS;
  if( pProgram->pLEX->pASymbols == NULL )pProgram->pLEX->pASymbols  = ASYMBOLS;
  if( pProgram->pLEX->pCSymbols == NULL )pProgram->pLEX->pCSymbols  = CSYMBOLS;
  pProgram->pLEX->report = pProgram->fpReportFunction;
  pProgram->pLEX->reportptr = pProgram->pReportPointer;
  pProgram->pLEX->fErrorFlags = pProgram->fErrorFlags;
  pProgram->pLEX->iErrorCounter = 0;
  pProgram->pLEX->pLexResult = (void *)stderr;


  pProgram->pLEX->pvInput = (void *)  pProgram->pREAD;
  lex_ReadInput(pProgram->pLEX);

  if( pProgram->pLEX->iErrorCounter )return pProgram->pLEX->iErrorCounter;
  lex_RemoveComments(pProgram->pLEX);
  lex_RemoveSkipSymbols(pProgram->pLEX);
  lex_HandleContinuationLines(pProgram->pLEX);
  return pProgram->pLEX->iErrorCounter;
  }

/*POD
=H scriba_DoSyntaxAnalysis()

This function performs syntax analysis after the lexical analysis has been finished.

This function is rarely needeed by applicationdevelopers. See R<scriba_LoadSourceProgram()>
instead.
/*FUNCTION*/
int scriba_DoSyntaxAnalysis(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_ReadSource()>, R<scriba_DoLexicalAnalysis()>,
R<scriba_DoSyntaxAnalysis()>, R<scriba_BuildCode()>.
CUT*/
  peNODE_l CommandList;
  int iError;

  pProgram->pEX = alloc_Alloc( sizeof(eXobject) , pProgram->pMEM);
  if( pProgram->pEX == NULL )return 1;

  pProgram->pEX->pPREP = pProgram->pPREP;
  pProgram->pEX->memory_allocating_function = pProgram->maf;
  pProgram->pEX->memory_releasing_function = pProgram->mrf;
  pProgram->pEX->cbBuffer = 1024; /* init will allocate the space of this number of characters */
  pProgram->pEX->cbCurrentNameSpace = 1024; /* init will allocate the space of this number of characters */
  pProgram->pEX->pLex = pProgram->pLEX;

  pProgram->pEX->Unaries  = UNARIES;
  pProgram->pEX->Binaries = BINARIES;
  pProgram->pEX->BuiltInFunctions = INTERNALFUNCTIONS;
  pProgram->pEX->MAXPREC  = MAX_BINARY_OPERATOR_PRECEDENCE;
  pProgram->pEX->PredeclaredLongConstants = PREDLCONSTS;
  pProgram->pEX->reportptr = pProgram->pReportPointer;
  pProgram->pEX->report   = pProgram->fpReportFunction;
  pProgram->pEX->fErrorFlags = pProgram->fErrorFlags;
  pProgram->pEX->iErrorCounter = 0;
  iError = 0;

  pProgram->pEX->Command = COMMANDS;

  ex_init(pProgram->pEX);

  ex_Command_l(pProgram->pEX,&CommandList);

  pProgram->pEX->pCommandList = CommandList;
  if( pProgram->pPREP && pProgram->pPREP->n )
    iError = ipreproc_Process(pProgram->pPREP,PreprocessorExFinish,pProgram->pEX);
  if( iError )pProgram->pEX->iErrorCounter++;

  return pProgram->pEX->iErrorCounter;
  }

/*POD
=H scriba_BuildCode()

This function builds the finall ready-to-run code after the syntax
analisys has been finished.

This function is rarely needeed by applicationdevelopers. See R<scriba_LoadSourceProgram()>
instead.
/*FUNCTION*/
int scriba_BuildCode(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_ReadSource()>, R<scriba_DoLexicalAnalysis()>,
R<scriba_DoSyntaxAnalysis()>, R<scriba_BuildCode()>.
CUT*/

  pProgram->pBUILD = alloc_Alloc( sizeof(BuildObject) , pProgram->pMEM );
  if( pProgram->pBUILD == NULL )return 1;

  pProgram->pBUILD->pPREP = pProgram->pPREP;
  pProgram->pBUILD->memory_allocating_function = pProgram->maf;
  pProgram->pBUILD->memory_releasing_function  = pProgram->mrf;
  pProgram->pBUILD->pEx =   pProgram->pEX;
  pProgram->pBUILD->iErrorCounter = 0;
  pProgram->pBUILD->fErrorFlags = pProgram->pEX->fErrorFlags;
  pProgram->pBUILD->FirstUNIXline = pProgram->FirstUNIXline;

  build_Build(pProgram->pBUILD);

  if( pProgram->pBUILD->iErrorCounter )return pProgram->pBUILD->iErrorCounter;
  return 0;
  }

/*POD
=H scriba_IsFileBinaryFormat()

This function decides if a file is a correct binary format ScriptBasic
code file and returns true if it is binary. If the file is a ScriptBasic
source file or an older version binary of ScriptBasic or any other file
it returns zero.

This function just calls the function T<build_IsFileBinaryFormat>

/*FUNCTION*/
int scriba_IsFileBinaryFormat(pSbProgram pProgram
  ){
/*noverbatim
CUT*/

  return  build_IsFileBinaryFormat(pProgram->pszFileName);
  }

/*POD
=H scriba_GetCacheFileName()

Calculate the name of the cache file for the given source file name and
store the calculated file name in the program object.

/*FUNCTION*/
int scriba_GetCacheFileName(pSbProgram pProgram
  ){
/*noverbatim
The program returns zero or the error code. It returns T<SCRIBA_ERROR_FAIL> if there
is no cache directory configured.

The code uses a local buffer of length 256 bytes. The full cached file name should
fit into this otherwise the program will return T<SCRIBA_ERROR_BUFFER_SHORT>.

The code does not check if there exists an appropriate cache directory or file. It
just calculates the file name.
CUT*/
#define FULL_PATH_BUFFER_LENGTH 256
  char *pszCache;
  char *s,*q;
  char CachedFileName[FULL_PATH_BUFFER_LENGTH];

  if( pProgram->pszFileName == NULL )return SCRIBA_ERROR_FAIL;
  pszCache = cft_GetString(pProgram->pCONF,"cache");
  if( pszCache == NULL)return SCRIBA_ERROR_FAIL;
  if( strlen(pszCache) >= FULL_PATH_BUFFER_LENGTH )return SCRIBA_ERROR_BUFFER_SHORT;
  strcpy(CachedFileName,pszCache);
  s = CachedFileName + strlen(CachedFileName); /* point to the end of the cache directory */

#ifdef WIN32
/* under Win32 we convert the argv[0] to the full path file name */
  if( GetFullPathName(pProgram->pszFileName,
                      FULL_PATH_BUFFER_LENGTH-strlen(CachedFileName),s,&q)==0 )
    return SCRIBA_ERROR_FAIL;
#else
/* under UNIX we can not convert, but it usually contains the full path */
  if( strlen(pProgram->pszFileName) > FULL_PATH_BUFFER_LENGTH - strlen(CachedFileName) )
    return SCRIBA_ERROR_BUFFER_SHORT;
  strcpy(s,pProgram->pszFileName);
#endif
  /* convert the full path to MD5 digest unique file name */
  uniqfnam(s,s);
  if( pProgram->pszCacheFileName )alloc_Free(pProgram->pszCacheFileName,pProgram->pMEM);
  pProgram->pszCacheFileName = alloc_Alloc(strlen(CachedFileName)+1,pProgram->pMEM);
  if( pProgram->pszCacheFileName == NULL )return SCRIBA_ERROR_MEMORY_LOW;
  strcpy(pProgram->pszCacheFileName,CachedFileName);
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_UseCacheFile()

Call this function to test that the cache file is usable. This function
calls the function R<scriba_GetCacheFileName()> to calculate the cache file name.

If 
=itemize
=item the cache file exists
=item is newer than the source file set by R<scriba_SetFileName()>
=item is a correct ScriptBasic binary file
=noitemize
then this function alters the source file name property (T<pszFileName>)
of the program object so that the call to R<scriba_LoadBinaryProgram()> will try to
load the cache file.

/*FUNCTION*/
int scriba_UseCacheFile(pSbProgram pProgram
  ){
/*noverbatim
The function returns zero or the error code. The function returns T<SCRIBA_ERROR_FAIL>
in case the cache file is old, or not valid. Therefore returning a positive value
does not neccessarily mean a hard error.
CUT*/
  unsigned long FileTime,CacheTime;
  int iError;

  if( iError = scriba_GetCacheFileName(pProgram) )return iError;

  FileTime  = file_time_modified(pProgram->pszFileName);
  CacheTime = file_time_modified(pProgram->pszCacheFileName);
  if( FileTime && CacheTime && CacheTime > FileTime &&
      build_IsFileBinaryFormat(pProgram->pszCacheFileName) ){
    alloc_Free(pProgram->pszFileName,pProgram->pMEM);
    pProgram->pszFileName = alloc_Alloc(strlen(pProgram->pszCacheFileName)+1,
                                                               pProgram->pMEM);
    if( pProgram->pszFileName == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    strcpy(pProgram->pszFileName,pProgram->pszCacheFileName);
    return SCRIBA_ERROR_SUCCESS;
    }
  return SCRIBA_ERROR_FAIL;
  }

/*POD
=H scriba_SaveCacheFile()

Call this function to generate a cache file after
a successful program compilation.

/*FUNCTION*/
int scriba_SaveCacheFile(pSbProgram pProgram
  ){
/*noverbatim
The function returns zero (T<SCRIBA_ERROR_SUCCESS>) if there was no error.
This does not mean that the cache file was saved. If there is no cache
directory configured doing nothing is success.

Returning any positive error code means that ScriptBasic tried to write a
cache file but it could not.
CUT*/

  if( ! pProgram->pszCacheFileName )
    scriba_GetCacheFileName(pProgram);
  if( pProgram->pszCacheFileName )
    return scriba_SaveCode(pProgram,pProgram->pszCacheFileName);
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_RunExternalPreprocessor()

This function should be called to execute external preprocessors.

This function does almost nothing else but calls the function
T<epreproc()>.

/*FUNCTION*/
int scriba_RunExternalPreprocessor(pSbProgram pProgram,
                                   char **ppszArgPreprocessor
  ){
/*noverbatim
The argument T<ppszArgPreprocessor> should point to a string array. This string array
should contain the configured names of the preprocessors that are applied one after the
other in the order they are listed in the array.

Note that this array should contain the symbolic names of the preprocessors. The actual
preprocessor executable programs, or command lines are defined in the configuration
file.

After calling this function the source file name property of the program object (T<pszFileName>)
is also modified so that it points to the result of the preprocessor. This means that after the
successful return of this function the application may immediately call R<scriba_LoadSourceProgram()>.

If there is any error during the preprocessor execution the function returns some error code
(returned by T<epreproc>) otherwise the return value is zero.
CUT*/
  int iError;
  char *pszPreprocessedFileName=NULL;

  iError = epreproc(pProgram->pCONF,
                    pProgram->pszFileName,
                    &pszPreprocessedFileName,
                    ppszArgPreprocessor,
                    pProgram->maf,
                    pProgram->mrf);

  /* If there was error then return it. */
  if( iError )return iError;

  /* If there was no error, but there is no need to preprocess. */
  if( pszPreprocessedFileName == NULL )return SCRIBA_ERROR_SUCCESS;

  if( pProgram->pszFileName ){
    alloc_Free(pProgram->pszFileName,pProgram->pMEM);
    pProgram->pszFileName = NULL;
    }

  /* Allocated space for the preprocessed file name and store it in the
     memory segment pProgram->pMEM. */
  pProgram->pszFileName = alloc_Alloc(strlen(pszPreprocessedFileName)+1,pProgram->pMEM);
  if( pProgram->pszFileName == NULL )return SCRIBA_ERROR_MEMORY_LOW;
  strcpy(pProgram->pszFileName,pszPreprocessedFileName);
  pProgram->mrf(pszPreprocessedFileName);
  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_SaveCode()

Call this function to save the compiled byte code of the program
into a specific file. This function is called by the function R<scriba_SaveCacheFile()>.
/*FUNCTION*/
int scriba_SaveCode(pSbProgram pProgram,
                     char *pszCodeFileName
  ){
/*noverbatim
The function does nothing else, but calls T<build_SaveCode>.

The return code is zero or the error code returned by T<build_SaveCode>.
CUT*/
  return build_SaveCode(pProgram->pBUILD,pszCodeFileName);
  }

/*POD
=H scriba_SaveCCode()

/*FUNCTION*/
void scriba_SaveCCode(pSbProgram pProgram,
                      char *pszCodeFileName
  ){
/*noverbatim
CUT*/
  build_SaveCCode(pProgram->pBUILD,pszCodeFileName);
  }

/*POD
=H scriba_SaveECode()

/*FUNCTION*/
void scriba_SaveECode(pSbProgram pProgram,
                      char *pszInterpreter,
                      char *pszCodeFileName
  ){
/*noverbatim
CUT*/
  build_SaveECode(pProgram->pBUILD,pszInterpreter,pszCodeFileName);
  }

/*POD
=H scriba_LoadSourceProgram()

Call this function to load a BASIC program from its source format after
optionally checking that there is no available cache file and after
executing all required preprocessors. This function calls
R<scriba_ReadSource()>, R<scriba_DoLexicalAnalysis()>, R<scriba_DoSyntaxAnalysis()>, R<scriba_BuildCode()>,
and also releases the memory that was needed only for code building
calling R<scriba_PurgeReaderMemory()>, R<scriba_PurgeLexerMemory()>, R<scriba_PurgeSyntaxerMemory()>.

After the successful completion of this program the BASIC program is in
the memory in the ready-to-run state.
/*FUNCTION*/
int scriba_LoadSourceProgram(pSbProgram pProgram
  ){
/*noverbatim
Before calling this function the function R<scriba_SetFileName()> should have been called specifying the
file name.

The return value is zero (T<SCRIBA_ERROR_SUCCESS>) or the error code returned by the
underlying layer that has detected the error.
CUT*/
  int iError;

  if( iError = scriba_ReadSource(pProgram) )return iError;
  if( iError = scriba_DoLexicalAnalysis(pProgram) )return iError;
  if( iError = scriba_DoSyntaxAnalysis(pProgram) )return iError;
  if( iError = scriba_BuildCode(pProgram) )return iError;

  /* we can not purge these memory areas sooner because some
     error messages even during build refer to names read
     by the reader and still stored intheir memory heap. */
  scriba_PurgeReaderMemory(pProgram);
  scriba_PurgeLexerMemory(pProgram);
  scriba_PurgeSyntaxerMemory(pProgram);
  return SCRIBA_ERROR_SUCCESS;
  }

/*
This structure is used to imitate character fetching from a file when
the source code is in a string that the caller has already loaded.
*/
typedef struct _StringInputState {
  char *pszFileName;
  char *pszBuffer;
  unsigned long cbBuffer;
  unsigned long lBufferPosition;
  /* pointers to store the original functions */
  void * (*fpOpenFile)(char *, void *);
  int (*fpGetCharacter)(void *, void *);
  void (*fpCloseFile)(void *, void *);
  int iActive;
  } StringInputState, *pStringInputState;

static void *StringOpen(char *psz, pStringInputState p){
  p->iActive = 0;
  if( psz != p->pszFileName )return p->fpOpenFile(psz,NULL);
  p->iActive = 1;
  p->lBufferPosition = 0;
  return (void *)p;
  }

static void StringClose(void *p, pStringInputState q){
  if( ! q->iActive ){
    q->fpCloseFile(p,NULL);
    return;
    }
  return;
  }

static int StringGetCharacter(void *pv, pStringInputState p){
  if( ! p->iActive )return p->fpGetCharacter(pv,NULL);

  if( p->lBufferPosition < p->cbBuffer )return p->pszBuffer[p->lBufferPosition++];
  return -1;
  }

/*POD
=H scriba_LoadProgramString()

Use this function to convert a string containing a BASIC program
that is already in memory to ready-to-run binary format. This function
is same as R<scriba_LoadSourceProgram()> except that this function reads the source
code from a string instead of a file.
/*FUNCTION*/
int scriba_LoadProgramString(pSbProgram pProgram,
                             char *pszSourceCode,
                             unsigned long cbSourceCode
  ){
/*noverbatim
The argument T<pProgram> is the program object. The argument T<pszSourceCode>
is the BASIC program itself in text format. Because the source code may
contain ZCHAR just for any chance the caller has to provide the number of
characters in the buffer via the argument T<cbSourceCode>. In case the
source program is zero terminated the caller can simply say
T<strlen(pszSourceCode)> to give this argument.

Before calling this function the function R<scriba_SetFileName()> may be called. Altough
the source code is read from memory and thus there is no source file the
BASIC program may use the command T<include> or T<import> that includes
another source file after reading the code. If the program does so the reader
functions need to know the actual file name of the source code to find
the file to be included. To help this process the caller using this function
may set the file name calling R<scriba_SetFileName()>. However that file is never used
and need not even exist. It is used only to calculate the path of included files
that are specified using relative path.

The return value is zero (T<SCRIBA_ERROR_SUCCESS>) or the error code returned by the
underlying layer that has detected the error.
CUT*/
  int iError;
  StringInputState SIS;

  if( pProgram->pszFileName == NULL )scriba_SetFileName(pProgram,"");

  pProgram->pREAD = alloc_Alloc( sizeof(ReadObject) , pProgram->pMEM );
  if( pProgram->pREAD == NULL )return SCRIBA_ERROR_MEMORY_LOW;

  reader_InitStructure(pProgram->pREAD);
  /* here we have to alter the default reader functions that would
     originally read a file from disk to perform the reading from
     the string that was specified on the command line. */
  SIS.fpOpenFile = pProgram->pREAD->fpOpenFile;
  pProgram->pREAD->fpOpenFile     = (void *)StringOpen;
  SIS.fpGetCharacter = pProgram->pREAD->fpGetCharacter;
  pProgram->pREAD->fpGetCharacter = (void *)StringGetCharacter;
  SIS.fpCloseFile = pProgram->pREAD->fpCloseFile;
  pProgram->pREAD->fpCloseFile    = (void *)StringClose;

  pProgram->pREAD->memory_allocating_function = alloc_Alloc;
  pProgram->pREAD->memory_releasing_function = alloc_Free;
  pProgram->pREAD->pMemorySegment = alloc_InitSegment(
                                     pProgram->maf,
                                     pProgram->mrf);
  if( pProgram->pREAD->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
  pProgram->pREAD->report = pProgram->fpReportFunction;
  pProgram->pREAD->reportptr = pProgram->pReportPointer;
  pProgram->pREAD->iErrorCounter = 0;
  pProgram->pREAD->fErrorFlags = pProgram->fErrorFlags;
  pProgram->pREAD->pConfig = pProgram->pCONF;
  pProgram->pREAD->pFileHandleClass = &SIS;
  SIS.pszBuffer = pszSourceCode;
  SIS.cbBuffer = cbSourceCode;
  SIS.pszFileName = pProgram->pszFileName;

  /* here we have to initialize the preprocessor object 
     if it was not initialized before because the reader uses that */
  if( pProgram->pPREP == NULL ){
    pProgram->pPREP = alloc_Alloc( sizeof(PreprocObject) , pProgram->pMEM );
    if( pProgram->pPREP == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    ipreproc_InitStructure(pProgram->pPREP);
    pProgram->pPREP->pMemorySegment = alloc_InitSegment(
                                       pProgram->maf,
                                       pProgram->mrf);
    if( pProgram->pPREP->pMemorySegment == NULL )return SCRIBA_ERROR_MEMORY_LOW;
    pProgram->pPREP->pSB = pProgram;
    }
  pProgram->pREAD->pPREP = pProgram->pPREP;

  if( ! (iError = reader_ReadLines(pProgram->pREAD,pProgram->pszFileName)) ){
    if( pProgram->pREAD->FirstUNIXline ){
      pProgram->FirstUNIXline = alloc_Alloc(strlen(pProgram->pREAD->FirstUNIXline)+1,pProgram->pMEM);
      if( pProgram->FirstUNIXline == NULL )return SCRIBA_ERROR_MEMORY_LOW;
      strcpy(pProgram->FirstUNIXline,pProgram->pREAD->FirstUNIXline);
      }
    }else{
    return iError;
    }
  if( pProgram->pREAD->iErrorCounter )return pProgram->pREAD->iErrorCounter;
  if( iError = scriba_DoLexicalAnalysis(pProgram) )return iError;
  if( iError = scriba_DoSyntaxAnalysis(pProgram) )return iError;
  if( iError = scriba_BuildCode(pProgram) )return iError;

  /* we can not purge these memory areas sooner because some
     error messages even during build refer to names read
     by the reader and still stored intheir memory heap. */
  scriba_PurgeReaderMemory(pProgram);
  scriba_PurgeLexerMemory(pProgram);
  scriba_PurgeSyntaxerMemory(pProgram);
  return SCRIBA_ERROR_SUCCESS;
  }

static int scriba_PreRun(pSbProgram pProgram){
  int iError;

  if( pProgram->pEXE == NULL ){
    pProgram->pEXE = alloc_Alloc( sizeof(ExecuteObject) , pProgram->pMEM );
    if( pProgram->pEXE == NULL )return SCRIBA_ERROR_MEMORY_LOW;

    pProgram->pEXE->memory_allocating_function = pProgram->maf;
    pProgram->pEXE->memory_releasing_function = pProgram->mrf;
    pProgram->pEXE->reportptr = pProgram->pReportPointer;
    pProgram->pEXE->report   = pProgram->fpReportFunction;
    pProgram->pEXE->fErrorFlags = pProgram->fErrorFlags;

    pProgram->pEXE->pConfig = pProgram->pCONF;
    build_MagicCode(&(pProgram->pEXE->Ver));
    if( iError=execute_InitStructure(  pProgram->pEXE,pProgram->pBUILD) )
      return iError;
    pProgram->pEXE->fpStdouFunction = pProgram->fpStdouFunction;
    pProgram->pEXE->fpStdinFunction = pProgram->fpStdinFunction;
    pProgram->pEXE->fpEnvirFunction = pProgram->fpEnvirFunction;
    pProgram->pEXE->pEmbedder       = pProgram->pEmbedder;
    pProgram->pEXE->pSTI            = pProgram->pSTI;
    pProgram->pEXE->pEPo            = pProgram->pEPo;
    }else{
    if( iError=execute_ReInitStructure(  pProgram->pEXE,pProgram->pBUILD) )
      return iError;
    }

  pProgram->pEXE->CmdLineArgument = NULL;

  return SCRIBA_ERROR_SUCCESS;
  }

/*POD
=H scriba_Run()

Call this function to execute a program. Note that you can call this function
many times. Repetitive execution of the same program will execute the
ScriptBasic code again and again with the global variables keeping their values.

If you want to reset the global variables you have to call R<scriba_ResetVariables()>.

There is no way to keep the value of the local variables.

The argument T<pszCommandLineArgument> is the command part that is passed to the
BASIC program.
/*FUNCTION*/
int scriba_Run(pSbProgram pProgram,
               char *pszCommandLineArgument
  ){
/*noverbatim
The return value is zero in case of success or the error code returned by the underlying
execution layers.

Note that you can not call BASIC subroutines or functions without initializations that
T<scriba_Run> performs. You also can not access global variables. Therefore you either have
to call T<scriba_Run> or its brother R<scriba_NoRun()> that performs the initializations without execution.

You also have to call R<scriba_NoRun()> if you want to execute a program with some global variables
having preset values that you want to set from the embedding C program. In that case you
have to call R<scriba_NoRun()> then one or more times R<scriba_SetVariable()> and finally T<Run>.

CUT*/
  int iError;

  if( iError = scriba_PreRun(pProgram) ){
    return iError;
    }
  
  pProgram->pEXE->CmdLineArgument = pszCommandLineArgument;  
  execute_InitExecute(pProgram->pEXE,&iError);

  iError = 0;
  if( pProgram->pPREP && pProgram->pPREP->n )
    iError = ipreproc_Process(pProgram->pPREP,PreprocessorExeStart,pProgram->pEXE);
  if( iError )return iError;

  execute_Execute_r(pProgram->pEXE,&iError);
  if( iError )return iError;
  if( pProgram->pPREP && pProgram->pPREP->n )
    iError = ipreproc_Process(pProgram->pPREP,PreprocessorExeFinish,pProgram->pEXE);
  return iError;
  }

/*POD
=H scriba_NoRun()

In case the embedding program want to set global variables and
execute subroutines without or before starting the main program it has to call this
function first. It does all the initializations that are done by
R<scriba_Run()> except that it does not actually execute the program.

After calling this function the main program may access global variables
and call BASIC functions.

/*FUNCTION*/
int scriba_NoRun(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_Run()>.
CUT*/
  int iError;

  scriba_PreRun(pProgram);
  execute_InitExecute(pProgram->pEXE,&iError);
  if( iError )return iError;
  if( pProgram->pPREP && pProgram->pPREP->n )
    iError = ipreproc_Process(pProgram->pPREP,PreprocessorExeNoRun,pProgram->pEXE);
  return iError;
  }

/*POD
=H scriba_ResetVariables()

Call this function if you want to execute a program object that was already executed
but you do not want the global variables to keep their value they had when
the last execution of the BASIC code finished.

Global variables in ScriptBasic are guaranteed to be T<undef> before they get any other
value and some programs depend on this.

/*FUNCTION*/
void scriba_ResetVariables(pSbProgram pProgram
  ){
/*noverbatim
See also R<scriba_SetVariable()>, R<scriba_Run()>, R<scriba_NoRun()>.
CUT*/

  memory_ReleaseVariable(pProgram->pEXE->pMo,pProgram->pEXE->GlobalVariables);
  pProgram->pEXE->GlobalVariables = NULL;
  }

/*POD
=H scriba_Call()

This function can be used to call a function or subroutine. This function does not
get any arguments and does not provide any return value.

/*FUNCTION*/
int scriba_Call(pSbProgram pProgram,
                unsigned long lEntryNode
  ){
/*noverbatim
The return value is zero or the error code returned by the interpreter.

B<Note on how to get the Entry Node value:>

The argument T<lEntryNode> should be the node index of the subroutine or function that
we want to execute. This can be retrieved using the function R<scriba_LookupFunctionByName()> if the
name of the function or subroutine is know. Another method is that the BASIC program
stored this value in some global variables. BASIC programs can access this information
calling the BASIC function T<Address( f() )>.
CUT*/
  int iError;

  execute_ExecuteFunction(pProgram->pEXE,lEntryNode,0,NULL,NULL,&iError);

  return iError;
  }

/*POD
=H scriba_CallArg()

This function can be used to call a function or subroutine with arguments passed by value.
Neither the return value of the SUB nor the modified argument variables are not accessible 
via this function. T<CallArg> is a simple interface to call a ScriptBasic subroutine or
function with argument.

/*FUNCTION*/
int scriba_CallArg(pSbProgram pProgram,
                   unsigned long lEntryNode,
                   char *pszFormat, ...
  ){
/*noverbatim
Arguments

=itemize
=item T<pProgram> is the class variable.
=item T<lEntryNode> is the start node of the SUB. (See R<scriba_Call()> note on how to get the entry node value.)
=item T<pszFormat> is a format string that defines the rest of the areguments
=noitemize

The format string is case insensitive. The characters T<u>, T<i>, T<r>, T<b> and T<s> have meaning.
All other characters are ignored. The format characters define the type of the arguments
from left to right.

=itemize
=item T<u> means to pass an T<undef> to the SUB. This format character is exceptional that it does not
consume any function argument.
=item T<i> means that the next argument has to be T<long> and it is passed to the BASIC SUB as an integer.
=item T<r> means that the next argument has to be T<double> and it is passed to the BASIC SUB as a real.
=item T<s> means that the next argument has to be T<char *> and it is passed to the BASIC SUB as a string.
=item T<b> means that the next two arguments has to be T<long cbBuffer> and T<unsigned char *Buffer>.
The T<cbBuffer> defines the length of the T<Buffer>.
=noitemize

Note that this SUB calling function is a simple interface and has no access to the modified values of the argument
after the call or the return value.

If you need any of the functionalities that are not implemented in this function call a more sophisticated
function.

Example:
=verbatim

  iErrorCode = scriba_CallArg(&MyProgram,lEntry,"i i s d",13,22,"My string.",54.12);

=noverbatim

CUT*/
  int iError;
  VARIABLE vArgs;
  va_list marker;
  unsigned long cArgs,i,slen;
  char *s;
  char *arg;

  cArgs = 0;
  if( pszFormat ){
    s = pszFormat;
    while( *s ){
      switch( *s++ ){
        case 'U': /* undef argument */
        case 'u': /* It eats no actual C level caller argument */

        case 'B': /* byte argument   */
        case 'b': /* it eats two arguments: a length and the pointer to the byte stream */

        case 'S': /* string argument */
        case 's':
        case 'I': /* Integer argument */
        case 'i':
        case 'R': /* Real number argument */
        case 'r':
          cArgs ++;
          break;
        default:; /* ignore all non-format characters */
        }
      }
    }

  if( cArgs )
    vArgs = memory_NewArray(pProgram->pEXE->pMo,0,cArgs-1);
  else
    vArgs = NULL;

  if( vArgs ){
    i = 0;
    va_start(marker,pszFormat);
    s = pszFormat;
    while( *s ){
      switch( *s++ ){
        case 'U':
        case 'u':
          vArgs->Value.aValue[i] = NULL;
          i++;
          break;
        case 'B': /* byte stream argument */
        case 'b':
          slen = va_arg(marker, long);
          arg = va_arg(marker, char *);
          if( arg == NULL )arg = "";
          vArgs->Value.aValue[i] = memory_NewString(pProgram->pEXE->pMo,slen);
          memcpy(STRINGVALUE(vArgs->Value.aValue[i]),arg,slen);
          i++;
          break;
        case 'S': /* string argument */
        case 's':
          arg = va_arg(marker, char *);
          if( arg == NULL )arg = "";
          slen = strlen(arg);
          vArgs->Value.aValue[i] = memory_NewString(pProgram->pEXE->pMo,slen);
          memcpy(STRINGVALUE(vArgs->Value.aValue[i]),arg,slen);
          i++;
          break;
        case 'I': /* Integer argument */
        case 'i':
          vArgs->Value.aValue[i] = memory_NewLong(pProgram->pEXE->pMo);
          LONGVALUE(vArgs->Value.aValue[i]) = va_arg(marker, long);
          i++;
          break;
        case 'R': /* Real number argument */
        case 'r':
          vArgs->Value.aValue[i] = memory_NewDouble(pProgram->pEXE->pMo);
          DOUBLEVALUE(vArgs->Value.aValue[i]) = va_arg(marker, double);
          i++;
          break;
        }
      }
    }

  execute_ExecuteFunction(pProgram->pEXE,lEntryNode,cArgs,vArgs ? vArgs->Value.aValue : NULL ,NULL,&iError);
  memory_ReleaseVariable(pProgram->pEXE->pMo,vArgs);
  return iError;
  }

/*POD
=H scriba_DestroySbArgs()

This function can be used to release the memory used by arguments created by the
function R<scriba_NewSbArgs()>.

/*FUNCTION*/
void scriba_DestroySbArgs(pSbProgram pProgram,
                          pSbData Args,
                          unsigned long cArgs
  ){
/*noverbatim

Arguments:
=itemize
=item T<pProgram> class variable
=item T<Args> pointer returned by R<scriba_NewSbArgs()>
=item T<cArgs> the number of arguments pointed by T<Args>
=noitemize

CUT*/
  unsigned long i;

  for( i=0 ; i<cArgs ; i++ )
    if( Args[i].type == SBT_STRING )
      alloc_Free(Args[i].v.s,pProgram->pMEM);
  alloc_Free(Args,pProgram->pMEM);
  }

/*POD
=H scriba_NewSbArgs()

Whenever you want to handle the variable values that are returned by the scriba subroutine
you have to call R<scriba_CallArgEx()>. This function needs the arguments passed in an array of T<SbDtata> type.

This function is a usefuly tool to convert C variables to an array of T<SbData>

/*FUNCTION*/
pSbData scriba_NewSbArgs(pSbProgram pProgram,
                         char *pszFormat, ...
  ){
/*noverbatim
The arguments passed are 

=itemize
=item T<pProgram> is the class variable
=item T<pszFormat> is the format string
=noitemize

The format string is case insensitive. The characters T<u>, T<i>, T<r>, T<b> and T<s> have meaning.
All other characters are ignored. The format characters define the type of the arguments
from left to right.

=itemize
=item T<u> means to pass an T<undef> to the SUB. This format character is exceptional that it does not
consume any function argument.
=item T<i> means that the next argument has to be T<long> and it is passed to the BASIC SUB as an integer.
=item T<r> means that the next argument has to be T<double> and it is passed to the BASIC SUB as a real.
=item T<s> means that the next argument has to be T<char *> and it is passed to the BASIC SUB as a string.
=item T<b> means that the next two arguments has to be T<long cbBuffer> and T<unsigned char *Buffer>.
The T<cbBuffer> defines the leng of the T<Buffer>.
=noitemize

Example:

=verbatim

pSbData MyArgs;


  MyArgs = scriba_NewSbArgs(pProgram,"i i r s b",13,14,3.14,"string",2,"two character string");
  if( MyArgs == NULL )error("memory alloc");

  scriba_CallArgEx(pProgram,lEntry,NULL,5,MyArgs);

=noverbatim

This example passes five arguments to the ScriptBasic subroutine. Note that the last one is only
two character string, the rest of the characters are ignored.

CUT*/
  va_list marker;
  unsigned long cArgs,i;
  char *s;
  char *arg;
  pSbData p;

  if( pszFormat == NULL )return NULL;

  cArgs = 0;
  s = pszFormat;
  while( *s ){
    switch( *s++ ){
      case 'U': /* undef argument */
      case 'u': /* It eats no actual C level caller argument */

      case 'B': /* byte argument   */
      case 'b': /* it eats two arguments: a length and the pointer to the byte stream */

      case 'S': /* string argument */
      case 's':
      case 'I': /* Integer argument */
      case 'i':
      case 'R': /* Real number argument */
      case 'r':
        cArgs ++;
        break;
      default:; /* ignore all non-format characters */
      }
    }
  p = alloc_Alloc(sizeof(SbData)*cArgs,pProgram->pMEM);
  if( p == NULL )return NULL;  

  i = 0;
  va_start(marker,pszFormat);
  s = pszFormat;
  while( *s ){
    switch( *s++ ){
      case 'U':
      case 'u':
        p[i].type = SBT_UNDEF;
        i++;
        break;
      case 'B': /* byte stream argument */
      case 'b':
        p[i].type = SBT_STRING;
        p[i].size = va_arg(marker, long);
        arg = va_arg(marker, char *);
        if( arg == NULL && p[i].size != 0 ){
          p[i++].type = SBT_UNDEF;
          break;
          }
        p[i].size =  strlen(arg);
        if( p[i].size ){
          p[i].v.s = alloc_Alloc(p[i].size,pProgram->pMEM);
          if( p[i].v.s == NULL ){
            while( i ){
              if( p[i].type == SBT_STRING && p[i].v.s )alloc_Free(p[i].v.s,pProgram->pMEM);
              i--;
              }
            alloc_Free(p,pProgram->pMEM);
            return NULL;
            }
          memcpy(p[i].v.s,arg,p[i].size);
          }else{
          p[i].v.s = NULL;
          }
        i++;
        break;
      case 'S': /* string argument */
      case 's':
        p[i].type = SBT_STRING;
        arg = va_arg(marker, char *);
        if( arg == NULL )arg = "";
        p[i].size = strlen(arg);
        if( p[i].size ){
          p[i].v.s = alloc_Alloc(p[i].size,pProgram->pMEM);
          if( p[i].v.s == NULL ){
            while( i ){
              if( p[i].type == SBT_STRING && p[i].v.s )alloc_Free(p[i].v.s,pProgram->pMEM);
              i--;
              }
            alloc_Free(p,pProgram->pMEM);
            return NULL;
            }
          memcpy(p[i].v.s,arg,p[i].size);
          }else{
          p[i].v.s = NULL;
          }
        i++;
        break;
      case 'I': /* Integer argument */
      case 'i':
        p[i].type = SBT_LONG;
        p[i].v.l = va_arg(marker, long);
        i++;
        break;
      case 'R': /* Real number argument */
      case 'r':
        p[i].type = SBT_DOUBLE;
        p[i].v.d = va_arg(marker, double);
        i++;
        break;
      }
    }

  return p;
  }

/*POD
=H scriba_CallArgEx()

This is the most sophisticated function of the ones that call a ScriptBasic subroutine.
This function is capable handling parameters to scriba subroutines, and returning the
modified argument variables and the return value.

/*FUNCTION*/
int scriba_CallArgEx(pSbProgram pProgram,
                     unsigned long lEntryNode,
                     pSbData ReturnValue,
                     unsigned long cArgs,
                     pSbData Args
  ){
/*noverbatim
The arguments:
=itemize
=item T<pProgram> is the program object pointer.
=item T<lEntryNode> is the entry node index where the BASIC subroutine or function starts
      (See R<scriba_Call()> note on how to get the entry node value.)
=item T<ReturnValue> is the return value of the function or subroutine
=item T<cArgs> is the number of argments passed to the function
=item T<Args> argument data array
=noitemize
CUT*/
  int iError;
  VARIABLE vArgs;
  VARIABLE vReturn;
  unsigned long i;

  if( cArgs )
    vArgs = memory_NewArray(pProgram->pEXE->pMo,0,cArgs-1);
  else
    vArgs = NULL;

  if( vArgs ){
    for( i = 0 ; i < cArgs ; i ++ ){
      switch( Args[i].type ){
        case SBT_UNDEF:
          vArgs->Value.aValue[i] = NULL;
          break;
        case SBT_STRING:
          vArgs->Value.aValue[i] = memory_NewString(pProgram->pEXE->pMo,Args[i].size);
          memcpy(STRINGVALUE(vArgs->Value.aValue[i]),Args[i].v.s,Args[i].size);
          alloc_Free(Args[i].v.s,pProgram->pMEM);
          break;
        case SBT_LONG: /* Integer argument */
          vArgs->Value.aValue[i] = memory_NewLong(pProgram->pEXE->pMo);
          LONGVALUE(vArgs->Value.aValue[i]) = Args[i].v.l;
          break;
        case SBT_DOUBLE: /* Real number argument */
          vArgs->Value.aValue[i] = memory_NewDouble(pProgram->pEXE->pMo);
          DOUBLEVALUE(vArgs->Value.aValue[i]) = Args[i].v.d;
          break;
        }
      }
    }

  execute_ExecuteFunction(pProgram->pEXE,lEntryNode,cArgs,vArgs ? vArgs->Value.aValue : NULL ,&vReturn,&iError);
  scriba_UndefSbData(pProgram,ReturnValue);

  if( ! iError && vReturn ){
    switch( vReturn->vType ){
      case VTYPE_LONG:
        ReturnValue->type = SBT_LONG;
        ReturnValue->v.l = LONGVALUE(vReturn);
        break;
      case VTYPE_DOUBLE:
        ReturnValue->type = SBT_DOUBLE;
        ReturnValue->v.d = DOUBLEVALUE(vReturn);
        break;
      case VTYPE_STRING:
        ReturnValue->type = SBT_STRING;
        /* we allocate a one byte longer buffer and append a terminating zero */
        ReturnValue->size=STRLEN(vReturn);/* size is w/o the terminating zero */
        ReturnValue->v.s = alloc_Alloc(ReturnValue->size+1,pProgram->pMEM);
        if( ReturnValue->v.s ){
          memcpy(ReturnValue->v.s,STRINGVALUE(vReturn),ReturnValue->size);
          ReturnValue->v.s[ReturnValue->size] = (char)0;
          }
        break;
      default:
        ReturnValue->type = SBT_UNDEF;
        break;
      }
    }

  if( vArgs && ! iError ){
    for( i = 0 ; i < cArgs ; i ++ ){
      if( vArgs->Value.aValue[i] == NULL ){
        Args[i].type = SBT_UNDEF;
        continue;
        }
      switch( vArgs->Value.aValue[i]->vType ){
        case VTYPE_LONG:
          Args[i].type = SBT_LONG;
          Args[i].v.l = LONGVALUE(vArgs->Value.aValue[i]);
          break;
        case VTYPE_DOUBLE:
          Args[i].type = SBT_DOUBLE;
          Args[i].v.d = DOUBLEVALUE(vArgs->Value.aValue[i]);
          break;
        case VTYPE_STRING:
          /* we allocate a one byte longer buffer and append a terminating zero */
          Args[i].type = SBT_STRING;
          Args[i].size=STRLEN(vArgs->Value.aValue[i]);/* size is w/o the terminating zero */
          Args[i].v.s = alloc_Alloc(Args[i].size+1,pProgram->pMEM);
          if( Args[i].v.s ){
            memcpy(Args[i].v.s,STRINGVALUE(vArgs->Value.aValue[i]),Args[i].size);
            Args[i].v.s[Args[i].size] = (char)0;
            }
          break;
        default:
          Args[i].type = SBT_UNDEF;
          break;
        }
      }
    }

  memory_ReleaseVariable(pProgram->pEXE->pMo,vArgs);
  memory_ReleaseVariable(pProgram->pEXE->pMo,vReturn); /*Tomasz Lacki realised its missing*/
  return iError;
  }

/*POD
=H scriba_LookupFunctionByName()

This function should be used to get the entry point of a function
knowing the name of the function. The entry point should not be treated as a
numerical value rather as a handle and to pass it to functions like
R<scriba_CallArgEx()>.

/*FUNCTION*/
long scriba_LookupFunctionByName(pSbProgram pProgram,
                                 char *pszFunctionName
  ){
/*noverbatim
The return value of the function is the entry node index of the function named or
zero if the function is not present in the program.
CUT*/
  return build_LookupFunctionByName(pProgram->pBUILD,pszFunctionName);
  }

/*POD
=H scriba_LookupVariableByName()

This function can be used to get the serial number of a global variable
knowing the name of the variable.

Note that all variables belong to a name space. Therefore if you want to
retrieve the global variable T<foo> you have to name it T<main::foo>.

/*FUNCTION*/
long scriba_LookupVariableByName(pSbProgram pProgram,
                                 char *pszVariableName
  ){
/*noverbatim
The return value is the serial number of the global avriable or zero if
there is no variable with that name.

Note that the second argument, the name of the global variable, is not
going under the usual name space manipulation. You have to specify the
variable name together with the name space. For example the variable T<a> will
not be found, but the variable T<main::a> will be.
CUT*/
  if( pProgram->pBUILD == NULL )return 0;
  return build_LookupVariableByName(pProgram->pBUILD,pszVariableName);
  }

/*POD
=H scriba_GetVariableType()

Get the type of the value that a variable is currently holding. This
value can be

=itemize
=item T<SBT_UNDEF>
=item T<SBT_DOUBLE>
=item T<SBT_LONG>
=item T<SBT_STRING>
=noitemize

/*FUNCTION*/
long scriba_GetVariableType(pSbProgram pProgram,
                            long lSerial
  ){
/*noverbatim
The argument T<lSerial> should be the serial number of 
the variable as returned by R<scriba_LookupVariableByName()>.

If there is no variable for the specified serian mumber (T<lSerial> is not positive
or larger than the number of variables) the function returns T<SBT_UNDEF>.
CUT*/
  if( lSerial <= 0 || lSerial > pProgram->pEXE->cGlobalVariables )return SBT_UNDEF;

  if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] == NULL )return SBT_UNDEF;

  switch( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->vType ){
    case VTYPE_LONG: return SBT_LONG;
    case VTYPE_DOUBLE: return SBT_DOUBLE;
    case VTYPE_STRING: return SBT_STRING;
    default: return SBT_UNDEF;
    }  
  }

/*POD
=H scriba_GetVariable()

This function retrieves the value of a variable.
A new T<SbData> object is created and the pointer to it
is returned in T<pVariable>. This memory space is automatically
reclaimed when the program object is destroyed or the function
T<DestroySbData> can be called.

/*FUNCTION*/
int scriba_GetVariable(pSbProgram pProgram,
                       long lSerial,
                       pSbData *pVariable
  ){
/*noverbatim
The argument T<lSerial> should be the serial number of the global variable
as returned by R<scriba_LookupVariableByName()>.

The funtion returns T<SCRIBA_ERROR_SUCCESS> on success,

T<SCRIBA_ERROR_MEMORY_LOW> if the data cannot be created or

T<SCRIBA_ERROR_FAIL> if the parameter T<lSerial> is invalid.
CUT*/

  if( lSerial <= 0 || lSerial > pProgram->pEXE->cGlobalVariables )return SCRIBA_ERROR_FAIL;

  if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] == NULL ){
    *pVariable = scriba_NewSbUndef(pProgram);
    if( *pVariable )return SCRIBA_ERROR_SUCCESS;
    return SCRIBA_ERROR_FAIL;
    }

  switch( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->vType ){
    case VTYPE_LONG:
      *pVariable = scriba_NewSbLong(pProgram,
                                    pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.lValue);
      if( *pVariable )return SCRIBA_ERROR_SUCCESS;
      return SCRIBA_ERROR_MEMORY_LOW;
    case VTYPE_DOUBLE:
      *pVariable = scriba_NewSbDouble(pProgram,
                                      pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.dValue);
      if( *pVariable )return SCRIBA_ERROR_SUCCESS;
      return SCRIBA_ERROR_MEMORY_LOW;
    case VTYPE_STRING:
      *pVariable = scriba_NewSbBytes(pProgram,
                                             pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Size,
                                             pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.pValue);
      if( *pVariable )return SCRIBA_ERROR_SUCCESS;
      return SCRIBA_ERROR_MEMORY_LOW;
    default:
      *pVariable = scriba_NewSbUndef(pProgram);
      if( *pVariable )return SCRIBA_ERROR_SUCCESS;
      return SCRIBA_ERROR_FAIL;
    }  
  }


/*POD
=H scriba_SetVariable()

This function sets the value of a global BASIC variable. You can call this function after
executing the program before it is reexecuted or after successfull call to R<scriba_NoRun()>.

/*FUNCTION*/
int scriba_SetVariable(pSbProgram pProgram,
                       long lSerial,
                       int type,
                       long lSetValue,
                       double dSetValue,
                       char *pszSetValue,
                       unsigned long size
  ){
/*noverbatim
The argument T<lSerial> should be the serial number of the global variable
as returned by R<scriba_LookupVariableByName()>.

The argument T<type> should be one of the followings:

=itemize
=item T<SBT_UNDEF>
=item T<SBT_DOUBLE>
=item T<SBT_LONG>
=item T<SBT_STRING>
=item T<SBT_ZCHAR>
=noitemize

The function uses one of the arguments T<lSetValue>, T<dSetValue> or T<pszSetValue> and
the other two are ignored based on the value of the argument T<type>.

If the value of the argument T<type> is T<SBT_UNDEF> all initialization arguments are ignored and the
global variable will get the value T<undef>.

If the value of the argument T<type> is T<SBT_DOUBLE> the argument T<dSetValue> will be used and the global
variable will be double holding the value.

If the value of the argument T<type> is T<SBT_LONG> the argument T<lSetValue> will be used and the global
variable will be long holding the value.

If the value of the argument T<type> is T<SBT_STRING> the argument T<pszSetValue> 
will be used and the global variable will be long holding the value. The length of the string
should in this case be specified by the variable T<size>.

If the value of the argument T<type> is T<SBT_ZCHAR> the argument T<pszSetValue> 
will be used and the global variable will be long holding the value. The length of the string
is automatically calculated and the value passed in the variable T<size> is ignored. In this case the
string T<pszSetValue> should be zero character terminated.

The funtion returns T<SCRIBA_ERROR_SUCCESS> on success,

T<SCRIBA_ERROR_MEMORY_LOW> if the data cannot be created or

T<SCRIBA_ERROR_FAIL> if the parameter T<lSerial> is invalid. 
CUT*/

  if( lSerial <= 0 || lSerial > pProgram->pEXE->cGlobalVariables )return SCRIBA_ERROR_FAIL;

  if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] ){
    memory_ReleaseVariable(pProgram->pEXE->pMo,pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]);
    pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] = NULL;
    }
  if( type == SBT_UNDEF )return SCRIBA_ERROR_SUCCESS;
  switch( type ){
    case SBT_DOUBLE:
         pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] =
           memory_NewDouble(pProgram->pEXE->pMo);
         if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] == NULL )return SCRIBA_ERROR_MEMORY_LOW;
         pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.dValue = dSetValue;
         return SCRIBA_ERROR_SUCCESS;
    case SBT_LONG:
         pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] =
           memory_NewLong(pProgram->pEXE->pMo);
         if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] == NULL )return SCRIBA_ERROR_MEMORY_LOW;
         pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.lValue = lSetValue;
         return SCRIBA_ERROR_SUCCESS;
    case SBT_ZCHAR:
         size = strlen(pszSetValue);
         type = SBT_STRING;
         /* nobreak flow over */
    case SBT_STRING:
         pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] =
           memory_NewString(pProgram->pEXE->pMo,size);
         if( pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1] == NULL )return SCRIBA_ERROR_MEMORY_LOW;
         memcpy(pProgram->pEXE->GlobalVariables->Value.aValue[lSerial-1]->Value.pValue,pszSetValue,size);
         return SCRIBA_ERROR_SUCCESS;

    default: return SCRIBA_ERROR_FAIL;
    }
  }

extern MODLIST StaticallyLinkedModules[];

/*POD
=H scriba_InitStaticModules()

This function calls the initialization functions of the modules that are statically linked
into the interpreter. This is essential to call this fucntion from the embedding T<main()> program
in a variation that has one or more external modules staticallyl linked. If this function is not
called the module initialization will not be called, because the module is never actually loaded and
thus the operating system does not call the T<DllMain> or T<_init> function.

The function has to be called before the first interpreter thread starts. In case of a single
thread variation this means that the function has to be called before the BASIC program starts.

The function does not take any argument and does not return any value.

/*FUNCTION*/
void scriba_InitStaticModules(void
  ){
/*noverbatim
CUT*/
  MODLIST *SLM = StaticallyLinkedModules;
  PSLFST slf;
  void (*fn)(void);

  while( SLM->name ){
    for( slf = (PSLFST)SLM->table ; slf->name ; slf++ ){
      if( ! strcmp(slf->name,"_init") && (fn = (void *)slf->function) )fn();
      }
    SLM++;
    }

  }

/*POD
=H scriba_FinishStaticModules()

This function calls the finalization functions of the modules that are statically linked
to the interpreter. Such a function for a dynamically loaded module is started by the operating
system. Because the sttaically linked modules are not loaded the T<_fini> function is not called
by the UNIX loader and similarly the function T<DllMain> is not called by Windows NT. Because some
modules depend on the execution of this function this function has to be called after the last
interpreter thread has finished.

/*FUNCTION*/
void scriba_FinishStaticModules(void
  ){
/*noverbatim
CUT*/
  MODLIST *SLM = StaticallyLinkedModules;
  PSLFST slf;
  void (*fn)(void);

  while( SLM->name ){
    for( slf = (PSLFST)SLM->table ; slf->name ; slf++ ){
      if( ! strcmp(slf->name,"_fini") && (fn = (void *)slf->function) )fn();
      }
    SLM++;
    }

  }
