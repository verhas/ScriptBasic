/*
FILE:   basext.c
HEADER: basext.h

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

This is a C file that contains mainly header information. Use headerer.pl to
create basext.h

We do not created basext.h using an editor because all .h files may be deleted
in this project during cleanup and we may loose this file accidentally.

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
#include "command.h"
#include "conftree.h"
#include "filesys.h"
#include "errcodes.h"
#include "tools/global.h"
#include "tools/md5.h"
#include "match.h"
#include "thread.h"
#include "scriba.h"
#include "logger.h"
#include "hndlptr.h"
#include "ipreproc.h"


// C++ support was suggested by
// Gavin Jenkins [gavinjenkins@inform-comms.co.uk]
// use -DSTATIC_LINK=1 in case you want to link the module to ScriptBasic
// statically. Though this is experimental.
#if (defined(_WIN32) || defined(__MACOS__))
# if STATIC_LINK
#  ifdef __cplusplus
#   define DLL_EXPORT extern "C" static
#  else
#   define DLL_EXPORT static
#  endif
# else
#  ifdef __cplusplus
#   define DLL_EXPORT extern "C" __declspec(dllexport) 
#  else
#   define DLL_EXPORT __declspec(dllexport) 
#  endif
# endif
#else
# if STATIC_LINK
#  ifdef __cplusplus
#   define DLL_EXPORT extern "C" static
#  else
#   define DLL_EXPORT static
#  endif
# else
#  ifdef __cplusplus
#   define DLL_EXPORT extern "C" 
#  else
#   define DLL_EXPORT
#  endif
# endif
#endif

typedef struct _SLFST {
  char *name;
  void *function;
  } SLFST, *PSLFST;

typedef struct _MODLIST {
  char *name;
  PSLFST *table;
  } MODLIST, *PMODLIST;

=POD
=H Basic Extension Support functions, structures and macros
=CUT

typedef struct _SupportTable {

  pExecuteObject pEo; // The execution context

=POD
=section besALLOC
=H besALLOC(X)

Use this macro to allocate memory in an extension like you would use
T<malloc> in a normal, old fashioned C program. The argument is the size of the memory in
byte count to be allocated. The result is the pointer to the allocated memory or T<NULL> if there
is not enough memory available.

The allocated memory is assigned to the memory segment of the execution thread and thus this memory
is released automatically when the module is unloaded from the interpreter. In other words if
a module uses T<besALLOC> to allocate memory there is no need to call R<besFREE(X)>.

Modules written for multi-thread variations of ScriptBasic should also be aware of the fact that
the memory allocated by this macro is released whent he calling interpreter thread finishes.

This macro calls the function R<alloc_Alloc()>
=CUT
  void * (*Alloc)(size_t n, void *p);
#define besALLOC(X) (pSt->Alloc((X),pSt->pEo->pMemorySegment))

=POD
=section besPROCALLOC
=H besPROCALLOC(X)

Use this macro in multi-thread supporting modules to allocate memory that is
not freed when the actual interpreter finishes but stays in memory so long
as long the process is alive.

=CUT
#define besPROCALLOC(X) (pSt->pEo->pSTI->Alloc((X),besPROCMEMORYSEGMENT))

=POD
=section besFREE
=H besFREE(X)

Use this macro to release memory that was allocated by R<besALLOC(X)>.

Altough all memory allocated by R<besALLOC()> is automatically released when the
interpreter thread calling R<besALLOC()> finishes it is advised to release all
memory chunks, especially large onces when they are not needed anymore.

This macro also T<NULL>ifies the argument.

This macro calls the function R<alloc_Free()>
=CUT
  void   (*Free)(void *pMem, void *p);
#define besFREE(X) (pSt->Free((X),pSt->pEo->pMemorySegment),(X)=NULL)

=POD
=section besPROCFREE
=H besPROCFREE(X)

This is the counterpart of R<besPROCALLOC> releasing memory allocated
for process life time.
=CUT
#define besPROCFREE(X) (pSt->pEo->pSTI->Free((X),besPROCMEMORYSEGMENT),(X)=NULL)

=POD
=section besPROCMEMORYSEGMENT
=H besPROCMEMORYSEGMENT

Use this macro in case you need to pass the process level memory
segment to a function.
=CUT
#define besPROCMEMORYSEGMENT (pSt->pEo->pSTI->pEo->pMemorySegment)

  // create new mortal variables. The memory object should be pSt->pEo->pMo, the mortal list pSt->pEo->pGlobalMortalList
  // use the macros
  pFixSizeMemoryObject (*NewMortalString)(pMemoryObject pMo, unsigned long StringSize, pMortalList pMortal);
  pFixSizeMemoryObject (*NewMortalLong)(pMemoryObject pMo, pMortalList pMortal);
  pFixSizeMemoryObject (*NewMortalRef)(pMemoryObject pMo, pMortalList pMortal);
  pFixSizeMemoryObject (*NewMortalDouble)(pMemoryObject pMo, pMortalList pMortal);
  pFixSizeMemoryObject (*NewMortalArray)(pMemoryObject pMo, pMortalList pMortal, long IndexLow, long IndexHigh);

=POD
=section besNEWMORTALSTRING
=H besNEWMORTALSTRING(X)

Create a new mortal string and return a pointer to it. The argument should be the
number of characters in the string.
This macro calls the function R<memory_NewMortalString()>.

See also 
=itemize
=item R<besNEWMORTALSTRING>
=item R<besNEWMORTALLONG>
=item R<besNEWMORTALREF>
=item R<besNEWMORTALDOUBLE>
=item R<besNEWMORTALARRAY>
=noitemize
=CUT
#define besNEWMORTALSTRING(X)  (pSt->NewMortalString(pSt->pEo->pMo,(X),pSt->pEo->pGlobalMortalList))

=POD
=section besNEWMORTALLONG
=H besNEWMORTALLONG

Create a new mortal T<long> and return the pointer to it.
This macro calls the function R<memory_NewMortalLong()>.

See also 
=itemize
=item R<besNEWMORTALSTRING>
=item R<besNEWMORTALLONG>
=item R<besNEWMORTALREF>
=item R<besNEWMORTALDOUBLE>
=item R<besNEWMORTALARRAY>
=noitemize
=CUT
#define besNEWMORTALLONG       (pSt->NewMortalLong(pSt->pEo->pMo,pSt->pEo->pGlobalMortalList))

=POD
=section besNEWMORTALREF
=H besNEWMORTALREF

Create a new mortal reference and return the pointer to it.
This macro calls the function R<memory_NewMortalRef()>.

See also 
=itemize
=item R<besNEWMORTALSTRING>
=item R<besNEWMORTALLONG>
=item R<besNEWMORTALREF>
=item R<besNEWMORTALDOUBLE>
=item R<besNEWMORTALARRAY>
=noitemize
=CUT
#define besNEWMORTALREF        (pSt->NewMortalRef(pSt->pEo->pMo,pSt->pEo->pGlobalMortalList))

=POD
=section besNEWMORTALDOUBLE
=H besNEWMORTALDOUBLE

Create a new mortal T<double> and return the pointer to it.
This macro calls the function R<memory_NewMortalDouble()>.

See also 
=itemize
=item R<besNEWMORTALSTRING>
=item R<besNEWMORTALLONG>
=item R<besNEWMORTALREF>
=item R<besNEWMORTALDOUBLE>
=item R<besNEWMORTALARRAY>
=noitemize
=CUT
#define besNEWMORTALDOUBLE     (pSt->NewMortalDouble(pSt->pEo->pMo,pSt->pEo->pGlobalMortalList))

=POD
=section besNEWMORTALARRAY
=H besNEWMORTALARRAY(X,Y)

Create a new mortal array and return the pointer to it. The arguments define the array
low index and the array high index. This macro calls the function R<memory_NewMortalArray()>.

See also 
=itemize
=item R<besNEWMORTALSTRING>
=item R<besNEWMORTALLONG>
=item R<besNEWMORTALREF>
=item R<besNEWMORTALDOUBLE>
=item R<besNEWMORTALARRAY>
=noitemize
=CUT
#define besNEWMORTALARRAY(X,Y) (pSt->NewMortalArray(pSt->pEo->pMo,pSt->pEo->pGlobalMortalList,(X),(Y)))

  pFixSizeMemoryObject (*NewString)(pMemoryObject pMo, unsigned long StringSize);
  pFixSizeMemoryObject (*NewLong)(pMemoryObject pMo);
  pFixSizeMemoryObject (*NewRef)(pMemoryObject pMo);
  pFixSizeMemoryObject (*NewDouble)(pMemoryObject pMo);
  pFixSizeMemoryObject (*NewArray)(pMemoryObject pMo, long LowIndex, long HighIndex);

=POD
=section besNEWSTRING
=H besNEWSTRING(X)

Create a new string and return the pointer to it. The argument defines the number
of bytes in the string. This macro calls the function R<memory_NewString()>.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
#define besNEWSTRING(X)  (pSt->NewString(pSt->pEo->pMo,(X)))

=POD
=section besNEWLONG
=H besNEWLONG

Create a new T<long> and return the pointer to it. This macro calls the function
R<memory_NewLong()>.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
#define besNEWLONG       (pSt->NewLong(pSt->pEo->pMo))

=POD
=section besNEWREF
=H besNEWREF

Create a new reference variable and return the pointer to it. This macro calls the function
R<memory_NewRef()>.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
#define besNEWREF        (pSt->NewRef(pSt->pEo->pMo))

=POD
=section besNEWDOUBLE
=H besNEWDOUBLE

Create a new T<double> and return the pointer to it. This macro calls the function
R<memory_NewDouble()>.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
#define besNEWDOUBLE     (pSt->NewDouble(pSt->pEo->pMo))

=POD
=section besNEWARRAY
=H besNEWARRAY(X,Y)

Create a new array and return the pointer to it. The arguments define the array
low index and the array high index. This macro calls the function R<memory_NewArray()>.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
#define besNEWARRAY(X,Y) (pSt->NewArray(pSt->pEo->pMo,(X),(Y)))

=POD
=section besRELEASE
=H besRELEASE(X)

Use this macro to release a non-mortal variable. This macro calls the function
R<memory_ReleaseVariable()> with the appropriate memory segment arguments.

See also 
=itemize
=item R<besNEWSTRING>
=item R<besNEWLONG>
=item R<besNEWREF>
=item R<besNEWDOUBLE>
=item R<besNEWARRAY>
=noitemize
=CUT
  int (*ReleaseVariable)(pMemoryObject, pFixSizeMemoryObject);
#define besRELEASE(X)    (pSt->ReleaseVariable(pSt->pEo->pMo,(X)))

=POD
=section besSETREF
=H besSETREF(VAR,VAL)

Use this macro in external modules to set the value of a variable to a reference to another variable.
The argument T<VAR> is the variable and the argument T<VAL> is the variable to be referenced.
=CUT
  int (*SetRef)(pMemoryObject,pFixSizeMemoryObject *,pFixSizeMemoryObject *);
#define besSETREF(X,Y) (pSt->SetRef(St->pEo->pMo,(X),(Y))

=POD
=section besCONFIG
=H besCONFIG(X)

Get a configuration value that is supposed to be a string. This macro uses the
configuration of the callign interpreter thread and it calls the function
R<cft_GetString>.
=CUT
  char * (*ConfigData)(ptConfigTree p, char *s);
#define besCONFIG(X) (pSt->ConfigData(pSt->pEo->pConfig,(X)))

=POD
=section besCONFIGFINDNODE
=H besCONFIGFINDNODE(X,Y,Z)
This macro calls the function R<cft_FindNode>. Use this macro for sophisticated
configuration handling that needs configuration key enumeration.
=CUT
  CFT_NODE (*FindNode)(ptConfigTree, CFT_NODE, char *);
#define besCONFIGFINDNODE(X,Y,Z) (pSt->FindNode((X),(Y),(Z)))

=POD
=section besCONFIGEX
=H besCONFIGEX(CT,CS,NS,CSS,LS,DS,IS)

This macro calls the function R<cft_GetEx>. Use this macro to retrieve
configuration information that is not a string or the type is not known.
=CUT
  int (*GetEx)(ptConfigTree, char *, CFT_NODE *, char **, long *, double *, int *);
#define besCONFIGEX(CT,CS,NS,CSS,LS,DS,IS) (pSt->GetEx((CT),(CS),(NS),(CSS),(LS),(DS),(IS)))

=POD
=section besCONFIGENUMFIRST
=H besCONFIGENUMFIRST(X,Y)

This macro calls the function R<cft_EnumFirst>.
=CUT
  CFT_NODE (*EnumFirst)(ptConfigTree, CFT_NODE);
#define besCONFIGENUMFIRST(X,Y) (pSt->EnumFirst((X),(Y)))

=POD
=section besCONFIGENUMNEXT
=H besCONFIGENUMNEXT(X,Y)

This macro calls the function R<cft_EnumNext>.
=CUT
  CFT_NODE (*EnumNext)(ptConfigTree, CFT_NODE);
#define besCONFIGENUMNEXT(X,Y) (pSt->EnumNext((X),(Y)))

=POD
=section besCONFIGGETKEY
=H besCONFIGGETKEY(X,Y)

This macro calls the function R<cft_GetKey>.
=CUT
  char * (*GetKey)(ptConfigTree, CFT_NODE );
#define besCONFIGGETKEY(X,Y) (pSt->GetKey((X),(Y)))

  SymbolTable (*NewSymbolTable)(void* (*memory_allocating_function)(size_t,void *),
                                void *pMemorySegment);
  void (*FreeSymbolTable)(SymbolTable table,
                          void (*memory_releasing_function)(void *,void *),
                          void *pMemorySegment);
  void (*TraverseSymbolTable)(SymbolTable table,
                              void (*call_back_function)(char *SymbolName, void *SymbolValue, void *f),
                              void *f);
  void **(*LookupSymbol)(char *s,
                         SymbolTable hashtable,
                         int insert,
                         void* (*memory_allocating_function)(size_t, void *),
                         void (*memory_releasing_function)(void *, void *),
                         void *pMemorySegment);

  int (*DeleteSymbol)(char *s,
                      SymbolTable hashtable,
                      void (*memory_releasing_function)(void *, void *),
                      void *pMemorySegment);

=POD
=section besNEWSYMBOLTABLE
=H besNEWSYMBOLTABLE

This macro allocates a new symbol table and returns the handle pointer to it. This
macro calls the function R<sym_NewSymbolTable>. The allocation function and the memory
segment used to allocate the symbol table is the default one that is used by the interpreter.
=CUT
#define besNEWSYMBOLTABLE() \
  (pSt->NewSymbolTable(pSt->Alloc,pSt->pEo->pMemorySegment))

=POD
=section besFREESYMBOLTABLE
=H besFREESYMBOLTABLE(X)

This macro releases a symbol table calling the function R<sym_FreeSymbolTable>.
The allocation function and the memory segment used to allocate the symbol table
is the default one that is used by the interpreter.
=CUT
#define besFREESYMBOLTABLE(X) \
  (pSt->FreeSymbolTable((X),pSt->Free,pSt->pEo->pMemorySegment))

=POD
=section besTRAVERSESYMBOLTABLE
=H besTRAVERSESYMBOLTABLE(X,Y,Z)

This macro calls the function R<sym_TraverseSymbolTable>.
=CUT
#define besTRAVERSESYMBOLTABLE(X,Y,Z) \
  (pSt->TraverseSymbolTable((X),(Y),(Z)))

=POD
=section besLOOKUPSYMBOL
=H besLOOKUPSYMBOL(X,Y,Z)

This macro calls the function R<sym_LookupSymbol>.
=CUT
#define besLOOKUPSYMBOL(X,Y,Z) \
  (pSt->LookupSymbol((X),(Y),(Z),pSt->Alloc,\
                                 pSt->Free,\
                                 pSt->pEo->pMemorySegment))

=POD
=section besDeleteSymbol
=H besDeleteSymbol(X,Y,Z)

This macro calls the function R<sym_DeleteSymbol>.
=CUT
#define besDeleteSymbol(X,Y) \
  (pSt->DeleteSymbol((X),(Y),pSt->Free,pSt->pEo->pMemorySegment))

  void *(*LoadLibrary)(char *s); // load a dynamic load library
  void (*FreeLibrary)(void *);     // free the loaded library
  void *(*GetFunctionByName)(void *pLibrary,char *pszFunctionName);

=POD
=section besLOADLIBRARY
=H besLOADLIBRARY(X)

This macro calls the function R<dynlolib_LoadLibrary()>.
=CUT
#define besLOADLIBRARY(X) (pSt->LoadLibrary( (X) ))

=POD
=section besFREELIBRARY
=H besFREELIBRARY(X)

This macro calls the function R<dynlolib_FreeLibrary()>.
=CUT
#define besFREELIBRARY(X) (pSt->FreeLibrary( (X) ))

=POD
=section besGETFUNCTIONBYNAME
=H besGETFUNCTIONBYNAME(X)

This macro calls the function R<dynlolib_GetFunctionByName()>.
=CUT
#define besGETFUNCTIONBYNAME(LIB,FUN) (pSt->GetFunctionByName((LIB),(FUN)))

  FILE *(*fopen)(char *pszFileName,char *pszOpenMode);
  void (*fclose)(FILE *fp);
  long (*size)(char *pszFileName);
  long (*time_accessed)(char *pszFileName);
  long (*time_modified)(char *pszFileName);
  long (*time_created)(char *pszFileName);
  int (*isdir)(char *pszFileName);
  int (*isreg)(char *pszFileName);
  int (*exists)(char *pszFileName);
  int (*truncate)(FILE *,long lNewFileSize);
  int (*fgetc)(FILE *);
  int (*fread)(char *, int, int, FILE *);
  int (*fwrite)(char *, int, int, FILE *);
  void (*setmode)(FILE *, int);
  void (*binmode)(FILE *);
  void (*textmode)(FILE *);
  int (*ferror)(FILE *);
  int (*fputc)(int c, FILE*fp);
  int (*flock)(FILE *fp,int iLockType);
  int (*lock)(FILE *fp,int iLockType,long lStart,long lLength);
  int (*feof)(FILE *fp);
  int (*mkdir)(char *pszDirectoryName);
  int (*rmdir)(char *pszDirectoryName);
  int (*remove)(char *pszFileName);
  int (*deltree)(char *pszDirectoryName);
  int (*MakeDirectory)(char *pszDirectoryName);
  DIR *(*opendir)(char *pszDirectoryName,tDIR *pDirectory);
  struct dirent *(*readdir)(DIR *pDirectory);
  void (*closedir)(DIR *pDirectory);

=POD
=section besFOPEN
=H besFOPEN

This macro calls the function R<file_fopen>.
=CUT
#define besFOPEN(FN,OM)       (pSt->fopen( (FN),(OM) ))

=POD
=section besFCLOSE
=H besFCLOSE

This macro calls the function R<file_fclose>.
=CUT
#define besFCLOSE(FP)         (pSt->fclose( (FP) ))

=POD
=section besSIZE
=H besSIZE

This macro calls the function R<file_size>.
=CUT
#define besSIZE(FN)           (pSt->size( (FN) ))

=POD
=section besTIME_ACCESSED
=H besTIME_ACCESSED

This macro calls the function R<file_time_accessed>.
=CUT
#define besTIME_ACCESSED(FN)  (pSt->time_accessed( (FN) ))

=POD
=section besTIME_MODIFIED
=H besTIME_MODIFIED

This macro calls the function R<file_time_modified>.
=CUT
#define besTIME_MODIFIED(FN)  (pSt->time_modified( (FN) ))

=POD
=section besTIME_CREATED
=H besTIME_CREATED

This macro calls the function R<file_time_created>.
=CUT
#define besTIME_CREATED(FN)   (pSt->time_created( (FN) ))

=POD
=section besISDIR
=H besISDIR

This macro calls the function R<file_isdir>.
=CUT
#define besISDIR(FN)          (pSt->isdir( (FN) ))

=POD
=section besISREG
=H besISREG

This macro calls the function R<file_isreg>.
=CUT
#define besISREG(FN)          (pSt->isreg( (FN) ))

=POD
=section besEXISTS
=H besEXISTS

This macro calls the function R<file_fileexists>.
=CUT
#define besEXISTS(FN)         (pSt->exists( (FN) ))

=POD
=section besTRUNCATE
=H besTRUNCATE

This macro calls the function R<file_truncate>.
=CUT
#define besTRUNCATE(FN,NS)    (pSt->truncate( (FN),(NS) ))

=POD
=section besFGETC
=H besFGETC

This macro calls the function R<file_fgetc>.
=CUT
#define besFGETC(FP)          (pSt->fgetc( (FP) ))

=POD
=section besFREAD
=H besFREAD

This macro calls the function R<file_fread>.
=CUT
#define besFREAD(S,X,Y,FP)    (pSt->fread( (S),(X),(Y),(FP) ))

=POD
=section besFWRITE
=H besFWRITE

This macro calls the function R<file_fwrite>.
=CUT
#define besFWRITE(S,X,Y,FP)   (pSt->fwrite( (S),(X),(Y),(FP) ))

=POD
=section besSETMODE
=H besSETMODE

This macro calls the function R<file_setmode>.
=CUT
#define besSETMODE(FP,M)      (pSt->setmode( (FP),(M) ))

=POD
=section besBINMODE
=H besBINMODE

This macro calls the function R<file_binmode>.
=CUT
#define besBINMODE(FP)        (pSt->binmode( (FP) ))

=POD
=section besTEXTMODE
=H besTEXTMODE

This macro calls the function R<file_textmode>.
=CUT
#define besTEXTMODE(FP)       (pSt->textmode( (FP) ))

=POD
=section besFERROR
=H besFERROR

This macro calls the function R<file_ferror>.
=CUT
#define besFERROR(FP)         (pSt->ferror( (FP) ))

=POD
=section besFPUTC
=H besFPUTC

This macro calls the function R<file_fputc>.
=CUT
#define besFPUTC(C,FP)        (pSt->fputc( (C),(FP) ))

=POD
=section besFLOCK
=H besFLOCK

This macro calls the function R<file_flock>.
=CUT
#define besFLOCK(FP)          (pSt->flock( (FP) ))

=POD
=section besLOCK
=H besLOCK

This macro calls the function R<file_lock>.
=CUT
#define besLOCK(FP,LT,LS,LE)  (pSt->lock( (FP),(LT),(LS),(LE) ))

=POD
=section besFEOF
=H besFEOF

This macro calls the function R<file_feof>.
=CUT
#define besFEOF(FP)           (pSt->feof( (FP) ))

=POD
=section besMKDIR
=H besMKDIR

This macro calls the function R<file_mkdir>.
=CUT
#define besMKDIR(DN)          (pSt->mkdir( (DN) ))

=POD
=section besRMDIR
=H besRMDIR

This macro calls the function R<file_rmdir>.
=CUT
#define besRMDIR(DN)          (pSt->rmdir( (DN) ))

=POD
=section besREMOVE
=H besREMOVE

This macro calls the function R<file_remove>.
=CUT
#define besREMOVE(FN)         (pSt->remove( (FN) ))

=POD
=section besDELTREE
=H besDELTREE

This macro calls the function R<file_deltree>.
=CUT
#define besDELTREE(DN)        (pSt->deltree( (DN) ))

=POD
=section besMAKEDIRECTORY
=H besMAKEDIRECTORY

This macro calls the function R<file_MakeDirectory>.
=CUT
#define besMAKEDIRECTORY(DN)  (pSt->MakeDirectory( (DN) ))

=POD
=section besOPENDIR
=H besOPENDIR

This macro calls the function R<file_opendir>.
=CUT
#define besOPENDIR(DN,DP)     (pSt->opendir( (DN),(DP) ))

=POD
=section besREADDIR
=H besREADDIR

This macro calls the function R<file_readdir>.
=CUT
#define besREADDIR(DP)        (pSt->readdir( (DP) ))

=POD
=section besCLOSEDIR
=H besCLOSEDIR

This macro calls the function R<file_closedir>.
=CUT
#define besCLOSEDIR(DP)       (pSt->closedir( (DP) ))

  long (*GetOption)(pExecuteObject,char*);
  int (*SetOption)(pExecuteObject,char *,long);
  int (*ResetOption)(pExecuteObject,char *);

=POD
=section besOPTION
=H besOPTION(X)
Get the T<long> value of the option T<X>. This macro calls the function
R<options_Get>. The macro uses the default execution context.
=CUT
#define besOPTION(x)          (pSt->GetOption(pSt->pEo,(x)))

=POD
=section besSETOPTION
=H besSETOPTION(x,y)
Set the T<y> T<long> value of the option T<x>. This macro calls the function
R<options_Set>. The macro uses the default execution context.
=CUT
#define besSETOPTION(x,y)     (pSt->SetOption(pSt->pEo,(x),(y)))

=POD
=section besRESETOPTION
=H besRESETOPTION(X)
Reset the option T<X>. This macro calls the function R<options_Reset>.
The macro uses the default execution context.
=CUT
#define besRESETOPTION(x)     (pSt->ResetOption(pSt->pEo,(x)))

  pFixSizeMemoryObject (*Convert2String)(pExecuteObject, pFixSizeMemoryObject, pMortalList);
  pFixSizeMemoryObject (*Convert2Long)(pExecuteObject, pFixSizeMemoryObject, pMortalList);
  pFixSizeMemoryObject (*Convert2Double)(pExecuteObject, pFixSizeMemoryObject, pMortalList);
  int                  (*IsStringInteger)(pFixSizeMemoryObject);

  double               (*GetDoubleValue)(pExecuteObject, pFixSizeMemoryObject);
  long                 (*GetLongValue)  (pExecuteObject, pFixSizeMemoryObject);

=POD
=section besCONVERT2STRING
=H besCONVERT2STRING(x)

Use this macro to convert a mortal T<VARIABLE> to string. The macro calls the
function R<execute_Convert2String> and uses the global mortal list.
=CUT
#define besCONVERT2STRING(x)  (pSt->Convert2String(pSt->pEo,(x),pSt->pEo->pGlobalMortalList))

=POD
=section besCONVERT2LONG
=H besCONVERT2LONG(x)
Use this macro to convert a mortal T<VARIABLE> to T<long>. The macro calls the
function R<execute_Convert2Long> and uses the global mortal list.
=CUT
#define besCONVERT2LONG(x)    (pSt->Convert2Long(pSt->pEo,(x),pSt->pEo->pGlobalMortalList))

=POD
=section besGETLONGVALUE
=H besGETLONGVALUE(x)
Use this macro to get the long value of a variable. This macro is not the same as T<LONGVALUE>.
The macro T<LONGVALUE> simply accesses the long value of a variable and thus can also be used
as a left value assigning value to. On the other hand T<besGETLONGVALUE> is a function call that
returns a long value when the argumentum variable is T<NULL>, T<double>, string or some other value.
In such situation using T<LONGVALUE> would be erroneous.

The macro T<LONGVALUE> should be used to access the long value of a variable that is known to
hold a long value or when the long value of a variable is to be set.

The macro T<besGETLONGVALUE> has to be used in a situation when we want to use a variable
for its value being long. It is faster and consumes less memory than converting the variable to
long allocating a new mortal just to access the long value of the new mortal using T<LONGVALUE>.

The same statements hold for T<DOUBLEVALUE> and R<besGETDOUBLEVALUE>.
=CUT
#define besGETLONGVALUE(x)    (pSt->GetLongValue(pSt->pEo,(x)))

=POD
=section besCONVERT2DOUBLE
=H besCONVERT2DOUBLE(x)
Use this macro to convert a mortal T<VARIABLE> to T<double>. The macro calls the
function R<execute_Convert2Double> and uses the global mortal list.

=CUT
#define besCONVERT2DOUBLE(x)  (pSt->Convert2Double(pSt->pEo,(x),pSt->pEo->pGlobalMortalList))

=POD
=section besGETDOUBLEVALUE
=H besGETDOUBLEVALUE(x)
Use this macro to get the double value of a variable.

For comparision of T<DOUBLEVALUE> and this macro see the explanation in R<besGETLONGVALUE>.

=CUT
#define besGETDOUBLEVALUE(x)    (pSt->GetDoubleValue(pSt->pEo,(x)))

=POD
=section besISSTRINGINTEGER
=H besISSTRINGINTEGER(x)

Use this macro to decide if a string contains caharacters copnvertible to an
integer value. This macro calls the function R<execute_IsStringInteger>.
=CUT
#define besISSTRINGINTEGER(x) (pSt->IsStringInteger(x))

=POD
=section besCONVERT2ZCHAR
=H besCONVERT2ZCHAR(x)

Use this macro to convert a T<VARIABLE> that is already STRING type to zero character
terminated string. This is needed many times when a BASIC string has to be passed to
operating system functions.

The macro argument T<x> is converted to zero character terminated string and the
result will be pointed by T<y>. T<y> has to be a T<(char *)> C variable.

If there is not enough memory the macro returns from the function with the error code
T<COMMAND_ERROR_MEMORY_LOW> thus there is no need to check the value of the variable T<y>
if it is T<NULL>.

The memory allocated to store the ZCHAR string should be released by the macro R<besFREE>.

Note that the macro does not check wheter the string already contains a zero character or not.
It simply allocates a buffer that has length n+1 bytes to store the n bytes of the original
BASIC string and an extra zero character.
=CUT
#define besCONVERT2ZCHAR(x,y)   (y) = besALLOC(STRLEN( (x) )+1);\
                                if( (y) == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                memcpy((y),STRINGVALUE( (x) ),STRLEN( (x) ));\
                                (y)[STRLEN( (x) )] = (char)0;

  int (*InitModuleInterface)(pExecuteObject, int);
  int (*LoadModule)(pExecuteObject, char *, pModule **);
  int (*GetModuleFunctionByName)(pExecuteObject, char *, char *, void **, pModule **);
  int (*UnloadAllModules)(pExecuteObject);
  int (*UnloadModule)(pExecuteObject, char *);

=POD
=section besREINITINTERFACE
=H besREINITINTERFACE

This macro calls the function R<modu_Init> to reset the module interface to
point to the original functions.

External modules are allowed to alter the support function table implementing their
own functions and thus altering the behavior of other extensions. These extensions
usually set some of the entries of the support function table to point to functions
implemented in the external module. However these functions are available only so long
as long the extension is in memory. When the extension is exiting the pointers should
be restored. Otherwise it may happen that the extension is already unloaded and the function
is used by another module exiting later.

Because there is no guaranteed order of extension module unload the modules should call
this function to restore the support function table and should not rely on altered
function calls during module exit code is executed.

=CUT
#define besREINITINTERFACE (pSt->InitModuleInterface(pSt->pEo,1)

=POD
=section besLOADMODULE
=H besLOADMODULE(x,y)

This macro calls the function R<modu_LoadModule>. The execution
environment (T<ExecuteObject>) used by the macro while calling the
function is the default execution environment.
=CUT
#define besLOADMODULE(x,y) (pSt->LoadModule(pSt->pEo,(x),(y))

=POD
=section besGETMODULEFUNCTIONBYNAME
=H besGETMODULEFUNCTIONBYNAME

This macro calls the function R<modu_GetFunctionByName>. The execution
environment (T<ExecuteObject>) used by the macro while calling the
function is the default execution environment.
=CUT
#define besGETMODULEFUNCTIONBYNAME(x,y,z,w) (pSt->GetModuleFunctionByName(pSt->pEo,(x),(y),(z),(w))

=POD
=section besUNLOADALLMODULES
=H besUNLOADALLMODULES

This macro calls the function R<modu_UnloadAllModules>. This will unload all modules that are
not active. Because this macro is called by an extension that is a module itself the calling module
will not be unloaded.

A module is active if there is a function called in the module and the module did not return from the
function call yet. Typically there can be only one active module at a time unless some
modules call each other in tricky ways through the ScriptBasic module calling functions. Therefore
calling this macro practically unloads all modules but the very one using this macro.

See also R<besUNLOADMODULE>.
=CUT
#define besUNLOADALLMODULES (pSt->UnloadAllModules(pSt->pEo))

=POD
=section besUNLOADMODULE
=H besUNLOADMODULE(x)

This macro calls the function R<modu_UnloadModule> to unload the named
module. Note that the module name T<x> should be the same
string that was used to load the module and it can not be the actual module or
any other module that is active.

See also R<besUNLOADALLMODULES>.
=CUT
#define besUNLOADMODULE(x) (pSt->UnloadModule(pSt->pEo,(x))

  void (*sleep)(long);
  int  (*curdir)(char *, unsigned long);
  int  (*chdir)(char *);
  int  (*chown)(char *, char *);
  int  (*SetCreateTime)(char *, long);
  int  (*SetModifyTime)(char *, long);
  int  (*SetAccessTime)(char *, long);

=POD
=section besSLEEP
=H besSLEEP(x)

This macro calls R<file_sleep> to sleep T<x> number of seconds.
=CUT
#define besSLEEP(x)           (pSt->sleep(x))

=POD
=section besCURDIR
=H besCURDIR(x,y)

This function calls R<file_curdir> to get the current working directory.
=CUT
#define besCURDIR(x,y)        (pSt->curdir((x),(y)))

=POD
=section besCHDIR
=H besCHDIR(x)

This function calls R<file_chdir> to set the current working directory.
Be careful changing the working directory because it may prevent the extension
module being used in multi-thread variation of ScriptBasic.
=CUT
#define besCHDIR(x)           (pSt->chdir(x))

=POD
=section besCHOWN
=H besCHOWN(x,y)

This macro calls the function R<file_chown>.
=CUT
#define besCHOWN(x,y)         (pSt->chown((x),(y)))

=POD
=section besSETCREATETIME
=H besSETCREATETIME(x,y)

This macro calls the function R<file_SetCreateTime>.
=CUT
#define besSETCREATETIME(x,y) (pSt->SetCreateTime((x),(y)))

=POD
=section besSETMODIFYTIME
=H besSETMODIFYTIME(x,y)

This macro calls the function R<file_SetModifyTime>.
=CUT
#define besSETMODIFYTIME(x,y) (pSt->SetModifyTime((x),(y)))

=POD
=section besSETACCESSTIME
=H besSETACCESSTIME(x,y)

This macro calls the function R<file_SetAccessTime>.
=CUT
#define besSETACCESSTIME(x,y) (pSt->SetAccessTime((x),(y)))

  int (*GetHostName)(char *, long);
  int (*GetHost)(char *, struct hostent *);
  int (*TcpConnect)(SOCKET *, char *);
  int (*TcpSend)(SOCKET, char *, long, int);
  int (*TcpRecv)(SOCKET, char *, long, int);
  int (*TcpClose)(SOCKET);

=POD
=section besGETHOSTNAME
=H besGETHOSTNAME(x,y)

This macro calls the function R<file_GetHostName>.
=CUT
#define besGETHOSTNAME(X,Y) (pSt->GetHostName((X),(Y)))

=POD
=section besGETHOST
=H besGETHOST(x,y)

This macro calls the function R<file_GetHost>.
=CUT
#define besGETHOST(X,Y) (pSt->GetHost((X),((Y)))

=POD
=section besTCPCONNECT
=H besTCPCONNECT(x,y)

This macro calls the function R<file_TcpConnect>.
=CUT
#define besTCPCONNECT(X,Y) (pSt->TcpConnect((X),(Y)))

=POD
=section besTCPSEND
=H besTCPSEND(x,y,z)

This macro calls the function R<file_TcpSend>.
=CUT
#define besTCPSEND(X,Y,Z) (pSt->TcpSend((X),(Y),(Z)))

=POD
=section besTCPRECV
=H besTCPRECV(x,y,z)

This macro calls the function R<file_TcpRecv>.
=CUT
#define besTCPRECV(X,Y,Z) (pSt->TcpRecv((X),(Y),(Z)))

=POD
=section besTCPCLOSE
=H besTCPCLOSE(y)

This macro calls the function R<file_TcpClose>.
=CUT
#define besTCPCLOSE(Y) (pSt->TcpClose((X))

=POD
=section besKILLPROC
=H besKILLPROC(x)

This macro calls the function R<file_KillProc>.
=CUT
  int (*KillProc)(long);
#define besKILLPROC(X) (pSt->KillProc((X))

=POD
=section besGETOWNER
=H besGETOWNER(x,y,z)

This macro calls the function R<file_GetOwner>.
=CUT
  int (*GetOwner)(char *, char *, long);
#define besGETOWNER(X,Y,Z) (pSt->GetOwner((X),(Y),(Z));


=POD
=section besCRYPT
=H besCRYPT(x,y,z)

This macro calls the function R<file_fcrypt>.
=CUT
  char *(*Crypt)(char *, char *, char *);
#define besCRYPT(X,Y,Z) (pSt->Crypt((X),(Y),(Z)))

=POD
=section besMD5INIT
=H besMD5INIT(C)

This macro calls the function T<MD5Init>.
=CUT
  void (*MD5Init)(MD5_CTX *context);
#define besMD5INIT(C) (pSt->MD5Init((C)))

=POD
=section besMD5UPDATE
=H besMD5UPDATE(C,I,L)

This macro calls the function T<MD5Update>.
=CUT
  void (*MD5Update)(MD5_CTX *context, unsigned char *input, unsigned int inputLen);
#define besMD5UPDATE(C,I,L) (pSt->MD5Update((C),(I),(L)))

=POD
=section besMD5FINAL
=H besMD5FINAL(D,C)

This macro calls the function T<MD5Final>.
=CUT
  void (*MD5Final)(unsigned char digest[16], MD5_CTX *context);
#define besMD5FINAL(D,C) (pSt->MD5Final((D),(C)))

=POD
=section besCREATEPROCESS
=H besCREATEPROCESS(X)

This macro calls the function R<file_CreateProcess>.
=CUT
  long (*CreateProcess)(char *);
#define besCREATEPROCESS(X) (pSt->CreateProcess(X))

=POD
=section besCOPYCOMMANDTABLE
=H besCOPYCOMMANDTABLE(X)

This macro calls the function R<execute_CopyCommandTable>.
=CUT
  int (*CopyCommandTable)(pExecuteObject);
#define besCOPYCOMMANDTABLE (pSt->CopyCommandTable(pSt->pEo))

=POD
=section besGETCOMMANDBYNAME
=H besGETCOMMANDBYNAME(X,Y)

This macro calls the function R<execute_GetCommandByName>.
=CUT
  long (*GetCommandByName)(pExecuteObject, char *, long);
#define besGETCOMMANDBYNAME(X,Y) (pSt->GetCommandByName(pSt->pEo,(X),(Y)))

  pFixSizeMemoryObject (*DupMortalize)(pMemoryObject,pFixSizeMemoryObject,pMortalList,int *);
  pFixSizeMemoryObject (*Evaluate)(pExecuteObject,unsigned long,pMortalList,int *,int);
  pFixSizeMemoryObject *(*LeftValue)(pExecuteObject,unsigned long,pMortalList,int *,int);
  void (*Immortalize)(pFixSizeMemoryObject,pMortalList);
  void (*ReleaseMortals)(pMemoryObject,pMortalList pMortal);

=POD
=section besEVALUATEEXPRESSION
=H besEVALUATEEXPRESSION(X)

This macro evaluates an expression and returns the result T<VARIABLE>.
This should usually be used only in extension R<besCOMMAND> commands
and not in R<besFUNCTION>. The result is duplicated and mortalized
and therefore the command is free to modify the
T<VARIABLE>. 

=CUT
#define besEVALUATEEXPRESSION(x) (pSt->DupMortalize(pEo->pMo,\
                                            pSt->Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,0),\
                                            _pThisCommandMortals,\
                                            &iErrorCode))

=POD
=section _besEVALUATEEXPRESSION
=H _besEVALUATEEXPRESSION(X)

This macro evaluates an expression and returns the result T<VARIABLE>.
This should usually be used only in extension R<besCOMMAND> commands
and not in R<besFUNCTION>. The result is B<NOT> duplicated and B<NOT>
mortalized and therefore the command modifying the T<VARIABLE> may happen
to modify a BASIC variable.

This should not be done! There are other ways to modify the value of a
variable.
=CUT
#define _besEVALUATEEXPRESSION(x) (pSt->Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,0))

=POD
=section _besEVALUATEEXPRESSION_A
=H _besEVALUATEEXPRESSION_A(X)

This macro evaluates an expression and returns the result T<VARIABLE>.
This macro is the same as R<_besEVALUATEEXPRESSION> except that this
macro may result a whole array.
=CUT
#define _besEVALUATEEXPRESSION_A(x) (pSt->Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,1))

=POD
=section besEVALUATELEFTVALUE
=H besEVALUATELEFTVALUE(X)

This macro evaluates a left value. This left-value is a pointer to a T<VARIABLE>.
If this is not T<NULL> it has to be released first using the macro R<besRELEASE>
and a new immortal value may be assigned to it afterwards.
=CUT
#define besEVALUATELEFTVALUE(x) (pSt->LeftValue(pEo,x,_pThisCommandMortals,&iErrorCode,0))

=POD
=section besEVALUATELEFTVALUE_A
=H besEVALUATELEFTVALUE_A(X)

This macro evaluates a left value. This macro is the same as R<besEVALUATELEFTVALUE>
except that this macro may result a whole array.
=CUT
#define besEVALUATELEFTVALUE_A(x) (pSt->LeftValue(pEo,x,_pThisCommandMortals,&iErrorCode,1))

=POD
=section besIMMORTALIZE
=H besIMMORTALIZE(x)

This macro calls the function R<memory_Immortalize()>. Use this macro
to immortalize a mortal variable before assigning it to a BASIC variable.

=CUT
#define besIMMORTALIZE(x) (pSt->Immortalize(x,_pThisCommandMortals))

=POD
=section besDEREFERENCE
=H besDEREFERENCE(X)

This macro calls R<execute_DereferenceS>. 

=CUT
  int (*Dereference)(unsigned long, pFixSizeMemoryObject *);
// dereference an argument passed by reference
#define besDEREFERENCE(X) if( pSt->Dereference(pSt->pEo->pMo->maxderef,&(X)) )\
                            return COMMAND_ERROR_CIRCULAR;

=POD
=section besMatchIndex
=H besMatchIndex(X)

This macro calls R<match_index>.
=CUT
  unsigned long (*match_index)(char ch);
#define besMatchIndex(X) (pSt->match_index(X))

=POD
=section besMatchIniSets
=H besMatchIniSets(X)

This macro calls R<match_InitSets>.
=CUT
  void (*match_InitSets)(pMatchSets pMS);
#define besMatchIniSets(X) (pSt->matchInitSets(X))

=POD
=section besMatchModifySet
=H besMatchModifySet(X,Y,Z,W,Q)

This macro calls R<match_ModifySet>.
=CUT
  void (*match_ModifySet)(pMatchSets pMS,
                          char JokerCharacter,
                          int nChars,
                          unsigned char *pch,
                          int fAction);
#define besMatchModifySet(X,Y,Z,W,Q) (pSt->match_ModifySet((X),(Y),(Z),(W),(Q)))

=POD
=section besMatchMatch
=H besMatchMatch(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12)

This macro calls R<match_match>.
=CUT
  int (*match_match)(char *pszPattern,
                     unsigned long cbPattern,
                     char *pszString,
                     unsigned long cbString,
                     char **ParameterArray,
                     unsigned long *pcbParameterArray,
                     char *pszBuffer,
                     int cArraySize,
                     int cbBufferSize,
                     int fCase,
                     pMatchSets pThisMatchSets,
                     int *iResult);
#define besMatchMatch(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12)\
(pSt->match_match((P1),(P2),(P3),(P4),(P5),(P6),(P7),(P8),(P9),(P10),(P11),(P12)))

=POD
=section besMatchCount
=H besMatchCount(X,Y)

This macro calls R<match_count>.
=CUT
  int (*match_count)(char *pszPattern,unsigned long cbPattern);
#define besMatchCount(X,Y) (pSt->match_count((X),(Y)))

=POD
=section besMatchParameter
=H besMatchParameter(P1,P2,P3,P4,P5,P6,P7)

This macro calls R<match_parameter>.
=CUT
  int (*match_parameter)(char *pszFormat,
                         unsigned long cbFormat,
                         char **ParameterArray,
                         unsigned long *pcbParameterArray,
                         char *pszBuffer,
                         int cArraySize,
                         unsigned long *pcbBufferSize);
#define besMatchParameter(P1,P2,P3,P4,P5,P6,P7)\
(pSt->match_parameter((P1),(P2),(P3),(P4),(P5),(P6),(P7)))

=POD
=section besMatchSize
=H besMatchSize(P1,P2,P3,P4,P5)

This macro calls R<match_size()>.
=CUT
  int (*match_size)(char *pszFormat,
                    unsigned long cbFormat,
                    unsigned long *pcbParameterArray,
                    int cArraySize,
                    int *cbBufferSize);
#define besMatchSize(P1,P2,P3,P4,P5)\
(pSt->match_size((P1),(P2),(P3),(P4),(P5)))

//
//  Multi-thread functions and locking support
//

=POD
=section besCreateThread
=H besCreateThread(X,Y,Z)

This macro calls R<thread_CreateThread>.
=CUT
  int (*thread_CreateThread)(PTHREADHANDLE pThread,
                      void *pStartFunction,
                      void *pThreadParameter);
#define besCreateThread(X,Y,Z) (pSt->thread_CreateThread((X),(Y),(Z)))

=POD
=section besExitThread
=H besExitThread

This macro calls R<thread_ExitThread>.
=CUT
  void (*thread_ExitThread)();
#define besExitThread() (pSt->thread_ExitThread())

=POD
=section besInitMutex
=H besInitMutex(X)

This macro calls R<thread_InitMutex>.
=CUT
  void (*thread_InitMutex)(PMUTEX pMutex);
#define besInitMutex(X) (pSt->thread_InitMutex(X))

=POD
=section besFinishMutex
=H besFinishMutex(X)

This macro calls R<thread_FinishMutex>.
=CUT
  void (*thread_FinishMutex)(PMUTEX pMutex);
#define besFinishMutex(X) (pSt->thread_FinishMutex(X))

=POD
=section besLockMutex
=H besLockMutex(X)

This macro calls R<thread_LockMutex>.
=CUT
  void (*thread_LockMutex)(PMUTEX pMutex);
#define besLockMutex(X) (pSt->thread_LockMutex(X))

=POD
=section besUnlockMutex
=H besUnlockMutex(X)

This macro calls R<thread_UnlockMutex>.
=CUT
  void (*thread_UnlockMutex)(PMUTEX pMutex);
#define besUnlockMutex(X) (pSt->thread_UnlockMutex(X))

=POD
=section besInitSharedLock
=H besInitSharedLock(X)

This macro calls R<thread_InitLock>.
=CUT
  void (*shared_InitLock)(PSHAREDLOCK p);
#define besInitSharedLock(X) (pSt->shared_InitLock(X))

=POD
=section besFinishSharedLock
=H besFinishSharedLock(X)

This macro calls R<thread_FinishLock>.
=CUT
  void (*shared_FinishLock)(PSHAREDLOCK p);
#define besFinishSharedLock(X) (pSt->shared_FinishLock(X))

=POD
=section besLockSharedRead
=H besLockSharedRead(X)

This macro calls R<thread_LockRead>.
=CUT
  void (*shared_LockRead)(PSHAREDLOCK p);
#define besLockSharedRead(X) (pSt->shared_LockRead(X))

=POD
=section besLockSharedWrite
=H besLockSharedWrite(X)

This macro calls R<thread_LockWrite>.
=CUT
  void (*shared_LockWrite)(PSHAREDLOCK p);
#define besLockSharedWrite(X) (pSt->shared_LockWrite(X))

=POD
=section besUnlockSharedRead
=H besUnlockSharedRead(X)

This macro calls R<thread_UnlockRead>.
=CUT
  void (*shared_UnlockRead)(PSHAREDLOCK p);
#define besUnlockSharedRead(X) (pSt->shared_UnlockRead(X))

=POD
=section besUnlockSharedWrite
=H besUnlockSharedWrite(X)

This macro calls R<thread_UnlockWrite>.
=CUT
  void (*shared_UnlockWrite)(PSHAREDLOCK p);
#define besUnlockSharedWrite(X) (pSt->shared_UnlockWrite(X))


//
//  Using these callback functions a module can even start another BASIC
// program and execute synchronous or asynchronous mode
//

=POD
=section besScribaNew
=H besScribaNew(F0,F1)

This macro calls R<scriba_new>
=CUT
  pSbProgram (*scriba_new)(void * (*maf)(size_t), void (*mrf)(void *));
#define besScribaNew(F0,F1) (pSt->scriba_New((F0),(F1)))

=POD
=section besScribaDestroy
=H besScribaDestroy(F0)

This macro calls R<scriba_destroy>
=CUT
  void (*scriba_destroy)(pSbProgram pProgram);
#define besScribaDestroy(F0) (pSt->scriba_Destroy((F0)))

=POD
=section besScribaNewSbData
=H besScribaNewSbData(F0)

This macro calls R<scriba_NewSbData>
=CUT
  pSbData (*scriba_NewSbData)(pSbProgram pProgram);
#define besScribaNewSbData(F0) (pSt->scriba_NewSbData((F0)))

=POD
=section besScribaNewSbLong
=H besScribaNewSbLong(F0,F1)

This macro calls R<scriba_NewSbLong>
=CUT
  pSbData (*scriba_NewSbLong)(pSbProgram pProgram, long lInitValue);
#define besScribaNewSbLong(F0,F1) (pSt->scriba_NewSbLong((F0),(F1)))

=POD
=section besScribaNewSbDouble
=H besScribaNewSbDouble(F0,F1)

This macro calls R<scriba_NewSbDouble>
=CUT
  pSbData (*scriba_NewSbDouble)(pSbProgram pProgram, double dInitValue);
#define besScribaNewSbDouble(F0,F1) (pSt->scriba_NewSbDouble((F0),(F1)))

=POD
=section besScribaNewSbUndef
=H besScribaNewSbUndef(F0)

This macro calls R<scriba_NewSbUndef>
=CUT
  pSbData (*scriba_NewSbUndef)(pSbProgram pProgram);
#define besScribaNewSbUndef(F0) (pSt->scriba_NewSbUndef((F0)))

=POD
=section besScribaNewSbString
=H besScribaNewSbString(F0,F1)

This macro calls R<scriba_NewSbString>
=CUT
  pSbData (*scriba_NewSbString)(pSbProgram pProgram, char *pszInitValue);
#define besScribaNewSbString(F0,F1) (pSt->scriba_NewSbString((F0),(F1)))

=POD
=section besScribaNewSbBytes
=H besScribaNewSbBytes(F0,F1,F2)

This macro calls R<scriba_NewSbBytes>
=CUT
  pSbData (*scriba_NewSbBytes)(pSbProgram pProgram, unsigned long len, unsigned char *pszInitValue);
#define besScribaNewSbBytes(F0,F1,F2) (pSt->scriba_NewSbBytes((F0),(F1),(F2)))

=POD
=section besScribaDestroySbData
=H besScribaDestroySbData(F0,F1)

This macro calls R<scriba_DestroySbData>
=CUT
  void (*scriba_DestroySbData)(pSbProgram pProgram, pSbData p);
#define besScribaDestroySbData(F0,F1) (pSt->scriba_DestroySbData((F0),(F1)))

=POD
=section besScribaPurgeReaderMemory
=H besScribaPurgeReaderMemory(F0)

This macro calls R<scriba_PurgeReaderMemory>
=CUT
  void (*scriba_PurgeReaderMemory)(pSbProgram pProgram);
#define besScribaPurgeReaderMemory(F0) (pSt->scriba_PurgeReaderMemory((F0)))

=POD
=section besScribaPurgeLexerMemory
=H besScribaPurgeLexerMemory(F0)

This macro calls R<scriba_PurgeLexerMemory>
=CUT
  void (*scriba_PurgeLexerMemory)(pSbProgram pProgram);
#define besScribaPurgeLexerMemory(F0) (pSt->scriba_PurgeLexerMemory((F0)))

=POD
=section besScribaPurgeSyntaxerMemory
=H besScribaPurgeSyntaxerMemory(F0)

This macro calls R<scriba_PurgeSyntaxerMemory>
=CUT
  void (*scriba_PurgeSyntaxerMemory)(pSbProgram pProgram);
#define besScribaPurgeSyntaxerMemory(F0) (pSt->scriba_PurgeSyntaxerMemory((F0)))

=POD
=section besScribaPurgeBuilderMemory
=H besScribaPurgeBuilderMemory(F0)

This macro calls R<scriba_PurgeBuilderMemory>
=CUT
  void (*scriba_PurgeBuilderMemory)(pSbProgram pProgram);
#define besScribaPurgeBuilderMemory(F0) (pSt->scriba_PurgeBuilderMemory((F0)))

=POD
=section besScribaPurgeExecuteMemory
=H besScribaPurgeExecuteMemory(F0)

This macro calls R<scriba_PurgeExecuteMemory>
=CUT
  void (*scriba_PurgeExecuteMemory)(pSbProgram pProgram);
#define besScribaPurgeExecuteMemory(F0) (pSt->scriba_PurgeExecuteMemory((F0)))

=POD
=section besScribaSetFileName
=H besScribaSetFileName(F0,F1)

This macro calls R<scriba_SetFileName>
=CUT
  int (*scriba_SetFileName)(pSbProgram pProgram, char *pszFileName);
#define besScribaSetFileName(F0,F1) (pSt->scriba_SetFileName((F0),(F1)))

=POD
=section besScribaLoadConfiguration
=H besScribaLoadConfiguration(F0,F1)

This macro calls R<scriba_LoadConfiguration>
=CUT
  int (*scriba_LoadConfiguration)(pSbProgram pProgram, char *pszForcedConfigurationFileName);
#define besScribaLoadConfiguration(F0,F1) (pSt->scriba_LoadConfiguration((F0),(F1)))

=POD
=section besScribaInheritConfiguration
=H besScribaInheritConfiguration(F0,F1)

This macro calls R<scriba_InheritConfiguration>
=CUT
  int (*scriba_InheritConfiguration)(pSbProgram pProgram, pSbProgram pFrom);
#define besScribaInheritConfiguration(F0,F1) (pSt->scriba_InheritConfiguration((F0),(F1)))

=POD
=section besScribaSetCgiFlag
=H besScribaSetCgiFlag(F0)

This macro calls R<scriba_SetCgiFlag>
=CUT
  void (*scriba_SetCgiFlag)(pSbProgram pProgram);
#define besScribaSetCgiFlag(F0) (pSt->scriba_SetCgiFlag((F0)))

=POD
=section besScribaSetReportFunction
=H besScribaSetReportFunction(F0,F1)

This macro calls R<scriba_SetReportFunction>
=CUT
  void (*scriba_SetReportFunction)(pSbProgram pProgram, void *fpReportFunction);
#define besScribaSetReportFunction(F0,F1) (pSt->scriba_SetReportFunction((F0),(F1)))

=POD
=section besScribaSetReportPointer
=H besScribaSetReportPointer(F0,F1)

This macro calls R<scriba_SetReportPointer>
=CUT
  void (*scriba_SetReportPointer)(pSbProgram pProgram, void *pReportPointer);
#define besScribaSetReportPointer(F0,F1) (pSt->scriba_SetReportPointer((F0),(F1)))

=POD
=section besScribaSetStdin
=H besScribaSetStdin(F0,F1)

This macro calls R<scriba_SetStdin>
=CUT
  void (*scriba_SetStdin)(pSbProgram pProgram, void *fpStdinFunction);
#define besScribaSetStdin(F0,F1) (pSt->scriba_SetStdin((F0),(F1)))

=POD
=section besScribaSetStdout
=H besScribaSetStdout(F0,F1)

This macro calls R<scriba_SetStdout>
=CUT
  void (*scriba_SetStdout)(pSbProgram pProgram, void *fpStdoutFunction);
#define besScribaSetStdout(F0,F1) (pSt->scriba_SetStdout((F0),(F1)))

=POD
=section besScribaSetEmbedPointer
=H besScribaSetEmbedPointer(F0,F1)

This macro calls R<scriba_SetEmbedPointer>
=CUT
  void (*scriba_SetEmbedPointer)(pSbProgram pProgram, void *pEmbedder);
#define besScribaSetEmbedPointer(F0,F1) (pSt->scriba_SetEmbedPointer((F0),(F1)))

=POD
=section besScribaSetEnvironment
=H besScribaSetEnvironment(F0,F1)

This macro calls R<scriba_SetEnvironment>
=CUT
  void (*scriba_SetEnvironment)(pSbProgram pProgram, void *fpEnvirFunction);
#define besScribaSetEnvironment(F0,F1) (pSt->scriba_SetEnvironment((F0),(F1)))

=POD
=section besScribaLoadBinaryProgram
=H besScribaLoadBinaryProgram(F0)

This macro calls R<scriba_LoadBinaryProgram>
=CUT
  int (*scriba_LoadBinaryProgram)(pSbProgram pProgram);
#define besScribaLoadBinaryProgram(F0) (pSt->scriba_LoadBinaryProgram((F0)))

=POD
=section besScribaInheritBinaryProgram
=H besScribaInheritBinaryProgram(F0,F1)

This macro calls R<scriba_InheritBinaryProgram>
=CUT
  int (*scriba_InheritBinaryProgram)(pSbProgram pProgram, pSbProgram pFrom);
#define besScribaInheritBinaryProgram(F0,F1) (pSt->scriba_InheritBinaryProgram((F0),(F1)))

=POD
=section besScribaReadSource
=H besScribaReadSource(F0)

This macro calls R<scriba_ReadSource>
=CUT
  int (*scriba_ReadSource)(pSbProgram pProgram);
#define besScribaReadSource(F0) (pSt->scriba_ReadSource((F0)))

=POD
=section besScribaDoLexicalAnalysis
=H besScribaDoLexicalAnalysis(F0)

This macro calls R<scriba_DoLexicalAnalysis>
=CUT
  int (*scriba_DoLexicalAnalysis)(pSbProgram pProgram);
#define besScribaDoLexicalAnalysis(F0) (pSt->scriba_DoLexicalAnalysis((F0)))

=POD
=section besScribaDoSyntaxAnalysis
=H besScribaDoSyntaxAnalysis(F0)

This macro calls R<scriba_DoSyntaxAnalysis>
=CUT
  int (*scriba_DoSyntaxAnalysis)(pSbProgram pProgram);
#define besScribaDoSyntaxAnalysis(F0) (pSt->scriba_DoSyntaxAnalysis((F0)))

=POD
=section besScribaBuildCode
=H besScribaBuildCode(F0)

This macro calls R<scriba_BuildCode>
=CUT
  int (*scriba_BuildCode)(pSbProgram pProgram);
#define besScribaBuildCode(F0) (pSt->scriba_BuildCode((F0)))

=POD
=section besScribaIsFileBinaryFormat
=H besScribaIsFileBinaryFormat(F0)

This macro calls R<scriba_IsFileBinaryFormat>
=CUT
  int (*scriba_IsFileBinaryFormat)(pSbProgram pProgram);
#define besScribaIsFileBinaryFormat(F0) (pSt->scriba_IsFileBinaryFormat((F0)))

=POD
=section besScribaGetCacheFileName
=H besScribaGetCacheFileName(F0)

This macro calls R<scriba_GetCacheFileName>
=CUT
  int (*scriba_GetCacheFileName)(pSbProgram pProgram);
#define besScribaGetCacheFileName(F0) (pSt->scriba_GetCacheFileName((F0)))

=POD
=section besScribaUseCacheFile
=H besScribaUseCacheFile(F0)

This macro calls R<scriba_UseCacheFile>
=CUT
  int (*scriba_UseCacheFile)(pSbProgram pProgram);
#define besScribaUseCacheFile(F0) (pSt->scriba_UseCacheFile((F0)))

=POD
=section besScribaSaveCacheFile
=H besScribaSaveCacheFile(F0)

This macro calls R<scriba_SaveCacheFile>
=CUT
  int (*scriba_SaveCacheFile)(pSbProgram pProgram);
#define besScribaSaveCacheFile(F0) (pSt->scriba_SaveCacheFile((F0)))

=POD
=section besScribaRunExternalPreprocessor
=H besScribaRunExternalPreprocessor(F0,F1)

This macro calls R<scriba_RunExternalPreprocessor>
=CUT
  int (*scriba_RunExternalPreprocessor)(pSbProgram pProgram, char **ppszArgPreprocessor);
#define besScribaRunExternalPreprocessor(F0,F1) (pSt->scriba_RunExternalPreprocessor((F0),(F1)))

=POD
=section besScribaSaveCode
=H besScribaSaveCode(F0,F1)

This macro calls R<scriba_SaveCode>
=CUT
  int (*scriba_SaveCode)(pSbProgram pProgram, char *pszCodeFileName);
#define besScribaSaveCode(F0,F1) (pSt->scriba_SaveCode((F0),(F1)))

=POD
=section besScribaSaveCCode
=H besScribaSaveCCode(F0,F1)

This macro calls R<scriba_SaveCCode>
=CUT
  void (*scriba_SaveCCode)(pSbProgram pProgram, char *pszCodeFileName);
#define besScribaSaveCCode(F0,F1) (pSt->scriba_SaveCCode((F0),(F1)))

=POD
=section besScribaLoadSourceProgram
=H besScribaLoadSourceProgram(F0)

This macro calls R<scriba_LoadSourceProgram>
=CUT
  int (*scriba_LoadSourceProgram)(pSbProgram pProgram);
#define besScribaLoadSourceProgram(F0) (pSt->scriba_LoadSourceProgram((F0)))

=POD
=section besScribaRun
=H besScribaRun(F0,F1)

This macro calls R<scriba_Run>
=CUT
  int (*scriba_Run)(pSbProgram pProgram, char *pszCommandLineArgument);
#define besScribaRun(F0,F1) (pSt->scriba_Run((F0),(F1)))

=POD
=section besScribaNoRun
=H besScribaNoRun(F0)

This macro calls R<scriba_NoRun>
=CUT
  int (*scriba_NoRun)(pSbProgram pProgram);
#define besScribaNoRun(F0) (pSt->scriba_NoRun((F0)))

=POD
=section besScribaResetVariables
=H besScribaResetVariables(F0)

This macro calls R<scriba_ResetVariables>
=CUT
  void (*scriba_ResetVariables)(pSbProgram pProgram);
#define besScribaResetVariables(F0) (pSt->scriba_ResetVariables((F0)))

=POD
=section besScribaCall
=H besScribaCall(F0,F1)

This macro calls R<scriba_Call>
=CUT
  int (*scriba_Call)(pSbProgram pProgram, unsigned long lEntryNode);
#define besScribaCall(F0,F1) (pSt->scriba_Call((F0),(F1)))

=POD
=section besScribaCallArg
=H besScribaCallArg(F0,F1,F2,F3)

This macro calls R<scriba_CallArg>
=CUT
  int (*scriba_CallArg)(pSbProgram pProgram, unsigned long lEntryNode, char *pszFormat, ...);
#define besScribaCallArg(F0,F1,F2,F3) (pSt->scriba_CallArg((F0),(F1),(F2),(F3)))

=POD
=section besScribaDestroySbArgs
=H besScribaDestroySbArgs(F0,F1,F2)

This macro calls R<scriba_DestroySbArgs>
=CUT
  void (*scriba_DestroySbArgs)(pSbProgram pProgram, pSbData Args, unsigned long cArgs);
#define besScribaDestroySbArgs(F0,F1,F2) (pSt->scriba_DestroySbArgs((F0),(F1),(F2)))

=POD
=section besScribaNewSbArgs
=H besScribaNewSbArgs(F0,F1,F2)

This macro calls R<scriba_NewSbArgs>
=CUT
  pSbData (*scriba_NewSbArgs)(pSbProgram pProgram, char *pszFormat, ...);
#define besScribaNewSbArgs(F0,F1,F2) (pSt->scriba_NewSbArgs((F0),(F1),(F2)))

=POD
=section besScribaCallArgEx
=H besScribaCallArgEx(F0,F1,F2,F3,F4)

This macro calls R<scriba_CallArgEx>
=CUT
  int (*scriba_CallArgEx)(pSbProgram pProgram, unsigned long lEntryNode, pSbData ReturnValue, unsigned long cArgs, pSbData Args);
#define besScribaCallArgEx(F0,F1,F2,F3,F4) (pSt->scriba_CallArgEx((F0),(F1),(F2),(F3),(F4)))

=POD
=section besScribaLookupFunctionByName
=H besScribaLookupFunctionByName(F0,F1)

This macro calls R<scriba_LookupFunctionByName>
=CUT
  long (*scriba_LookupFunctionByName)(pSbProgram pProgram, char *pszFunctionName);
#define besScribaLookupFunctionByName(F0,F1) (pSt->scriba_LookupFunctionByName((F0),(F1)))

=POD
=section besScribaLookupVariableByName
=H besScribaLookupVariableByName(F0,F1)

This macro calls R<scriba_LookupVariableByName>
=CUT
  long (*scriba_LookupVariableByName)(pSbProgram pProgram, char *pszVariableName);
#define besScribaLookupVariableByName(F0,F1) (pSt->scriba_LookupVariableByName((F0),(F1)))

=POD
=section besScribaGetVariableType
=H besScribaGetVariableType(F0,F1)

This macro calls R<scriba_GetVariableType>
=CUT
  long (*scriba_GetVariableType)(pSbProgram pProgram, long lSerial);
#define besScribaGetVariableType(F0,F1) (pSt->scriba_GetVariableType((F0),(F1)))

=POD
=section besScribaGetVariable
=H besScribaGetVariable(F0,F1,F2)

This macro calls R<scriba_GetVariable>
=CUT
  int (*scriba_GetVariable)(pSbProgram pProgram, long lSerial, pSbData *pVariable);
#define besScribaGetVariable(F0,F1,F2) (pSt->scriba_GetVariable((F0),(F1),(F2)))

=POD
=section besScribaSetVariable
=H besScribaSetVariable(F0,F1,F2,F3,F4,F5,F6)

This macro calls R<scriba_SetVariable>
=CUT
  int (*scriba_SetVariable)(pSbProgram pProgram, long lSerial, int type, long lSetValue, double dSetValue, char *pszSetValue, unsigned long size);
#define besScribaSetVariable(F0,F1,F2,F3,F4,F5,F6) (pSt->scriba_SetVariable((F0),(F1),(F2),(F3),(F4),(F5),(F6)))

=POD
=section besLogState
=H besLogState(X)

This macro calls R<log_state>
=CUT
  int (*log_state)(ptLogger pLOG);
#define besLogState(X) (pSt->log_state(X))


=POD
=section besLogInit
=H besLogInit(F0,F1,F2,F3,F4,F5)

This macro calls R<log_init>
=CUT
  int (*log_init)(ptLogger pLOG,
                  void *(*memory_allocating_function)(size_t, void *),
                  void (*memory_releasing_function)(void *, void *),
                  void *pMemorySegment,
                  char *pszLogFileName,
                  int iLogType);
#define besLogInit(F0,F1,F2,F3,F4,F5) (pSt->log_init((F0),(F1),(F2),(F3),(F4),(F5)))

=POD
=section besLogPrintf
=H besLogPrintf(pLOG,FORMAT, ...)

This macro calls R<log_printf>
=CUT
  int (*log_printf)(ptLogger pLOG,
                    char *pszFormat,
                    ...);
#define besLogPrintf pSt->log_printf

=POD
=section besLogShutdown
=H besLogShutdown(pLOG)

This macro calls R<log_shutdown>
=CUT
  int (*log_shutdown)(ptLogger pLOG);
#define besLogShutdown(X) (pSt->log_shutdown(X))

=POD
=section besHandleGetHandle
=H besHandleGetHandle(X,Y)

This macro calls R<handle_GetHandle>. The memory segment used by the
macro is the default.

Example:

=verbatim
  void *H;
  int i;
    ....
  i = besHandleGetPointer(H,pointer);
=noverbatim

=CUT
  unsigned long (*handle_GetHandle)(void **pHandle,
                                    void *pMEM,
                                    void *pointer);
#define besHandleGetHandle(X,Y) (pSt->handle_GetHandle(&(X),pSt->pEo->pMemorySegment,(Y)))

=POD
=section besHandleGetPointer
=H besHandleGetPointer(X,Y)

This macro calls R<handle_GetPointer>. The memory segment used by the
macro is the default.

Example:

=verbatim
  void *H,*pointer;
    ....
  pointer = besHandleGetPointer(H,handle);
=noverbatim

=CUT
  void *(*handle_GetPointer)(void **pHandle,
                             unsigned long handle);
#define besHandleGetPointer(X,Y) (pSt->handle_GetPointer( &(X),(Y)))

=POD
=section besHandleFreeHandle
=H besHandleFreeHandle(X,Y)

This macro calls R<handle_FreeHandle>
=CUT
  void (*handle_FreeHandle)(void **pHandle,
                            unsigned long handle);
#define besHandleFreeHandle(X,Y) (pSt->handle_FreeHandle( &(X), (Y)))

=POD
=section besHandleDestroyHandleArray
=H besHandleDestroyHandleArray(X)

This macro calls R<handle_DestroyHandleArray>
=CUT
  void (*handle_DestroyHandleArray)(void **pHandle,
                                    void *pMEM);
#define besHandleDestroyHandleArray(X) (pSt->handle_DestroyHandleArray( &(X),pSt->pEo->pMemorySegment))

  int (*basext_GetArgsF)(pSupportTable pSt,
                      pFixSizeMemoryObject pParameters,
                      char *pszFormat,
                      ...
                      );

#define besGETARGS pSt->basext_GetArgsF(pSt,pParameters,
#define besGETARGE );

=POD
=section besARGUMENTS

Use this macro to declare and process the arguments of a besFUNCTION. The
macro should be placed right after the variable declaration and before
any other code, because it declares the variable T<iError>.

The macro calls the function R<basext_GetArgsF> and returns with error in case
some error has happened during the parsing of the arguments.

Example:

=verbatim
besARGUMENTS("llzz")
&my_first_long , &my_second_long, &my_string1 , &my_string2
besARGEND
=noverbatim
=CUT
#define besARGUMENTS(X) int iError; iError = pSt->basext_GetArgsF(pSt,pParameters,(X),
#define besARGEND ); if( iError )return iError;

  void *(*InitSegment)(void * (*maf)(size_t),
                            void   (*mrf)(void *));
  long (*SegmentLimit)(void *p,unsigned long NewMaxSize);
  void (*FreeSegment)(void *p);
  void (*FinishSegment)(void *p);

=POD
=section besINIT_SEGMENT
=H besINIT_SEGMENT(MAF,MRF)
This macro calls the function R<alloc_InitSegment>
=CUT
#define besINIT_SEGMENT(MAF,MRF) (pSt->InitSegment(MAF,MRF))

=POD
=section besSEGMENT_LIMIT
=H besSEGMENT_LIMIT(PMS,L)
This macro calls the function R<alloc_SegmentLimit>
=CUT
#define besSEGMENT_LIMIT(PMS,L) (pSt->SegmentLimit(PMS,L))

=POD
=section besFREE_SEGMENT
=H besFREE_SEGMENT(PMS)
This macro calls the function R<alloc_FreeSegment>
=CUT
#define besFREE_SEGMENT(PMS) (pSt->FreeSegment(PMS))

=POD
=section besFINISH_SEGMENT
=H besFINISH_SEGMENT(PMS)
This macro calls the function R<alloc_FinishSegment>
=CUT
#define besFINISH_SEGMENT(PMS) (pSt->FinishSegment(PMS))

  } SupportTable
#ifndef PSUPPORTTABLE
  , *pSupportTable
#endif
  ;

// The support table contains the function pointers and other support information
// that the modules may need. This support table is common for all modules.
// The pointer ModuleInternal points to a NULL initialized pointer. This pointer
// is unique for each module and is guaranteed to be available and guaranteed
// be unchanged. The module may use this pointer to store internal non-volatile
// data.

=POD
=section besFUNCTION
=H besFUNCTION(X)

Use this macro to start an extension module interface function. The macro argument T<X>
is the name of the function as it is used in the BASIC statement

=verbatim
declare sub ::Function alias "X" lib "module"
=noverbatim

This macro handles all system dependant function decoration declarations and
declares the argument variables. Altough it is possible to name the argument
variables in a different way it is strongly recommended that the programmer
writing external module uses this macro and thus uses the argument names, because
the T<besXXX> macros rely on these variable names.
=CUT
#define besFUNCTION(X) DLL_EXPORT int X(pSupportTable pSt, \
                          void **ppModuleInternal, \
                          pFixSizeMemoryObject pParameters, \
                          pFixSizeMemoryObject *pReturnValue){\
                          pExecuteObject pEo=NULL;

=POD
=section besASSERT_FUNCTION
=H besASSERT_FUNCTION

Use this macro to check inside a besFUNCTION that the function was called
as a sub and not command. If the include file declares the function as

=verbatim
declare command XXX alias "xxx" lib "library"
=noverbatim

instead of

=verbatim
declare sub XXX alias "xxx" lib "library"
=noverbatim

then you can not execute the rest of the code safely. This macro returns
with the error code T<COMMAND_ERROR_BAD_CALL> if the function was
declared the wrong way.
=CUT
#define besASSERT_FUNCTION if( pSt == NULL || pSt->pEo == NULL || pSt->pEo->pST != pSt ){\
                              pEo = (pExecuteObject)pSt;\
                              pEo->ErrorCode = COMMAND_ERROR_BAD_CALL;\
                              return 0;\
                              }

=POD
=section besCOMMAND
=H besCOMMAND(X)

Use this macro to start an extension module interface command. The macro argument T<X>
is the name of the command as it is used in the BASIC statement

=verbatim
declare command ::Function alias "X" lib "module"
=noverbatim

This macro handles all system dependant function decoration declarations and
declares the argument variables. Altough it is possible to name the argument
variables in a different way it is strongly recommended that the programmer
writing external module uses this macro and thus uses the argument names, because
the T<besXXX> macros rely on these variable names.

In addition to the arguments this macro also declares some mandatory local variables
thatshould be used in most of the module implemented commands and are used by some
macros.

Note that interface functions get their arguments already evaluated while interface
commands may decide if an argument is evaluated or not, or even evaluated multiple
times.
=CUT
#define besCOMMAND(x) DLL_EXPORT int x(pExecuteObject pEo,void **ppModuleInternal){\
                        MortalList _ThisCommandMortals=NULL;\
                        pMortalList _pThisCommandMortals = &_ThisCommandMortals;\
                        unsigned long _ActualNode= pEo && pEo->pST && pEo->pST->pEo == pEo ? PROGRAMCOUNTER : 0;\
                        int iErrorCode;\
                        pSupportTable pSt= pEo ? pEo->pST : NULL;

=POD
=section besASSERT_COMMAND
=H besASSERT_COMMAND

Use this macro to check inside a besCOMMAND that the command was called
as a command. If the include file declares the function as

=verbatim
declare sub XXX alias "xxx" lib "library"
=noverbatim

instead of

=verbatim
declare command XXX alias "xxx" lib "library"
=noverbatim

then you can not execute the rest of the code safely. This macro returns
with the error code T<COMMAND_ERROR_BAD_CALL> if the function was
declared the wrong way.

=CUT
#define besASSERT_COMMAND if( pEo == NULL || pEo->pST == NULL || pEo->pST->pEo != pEo )return COMMAND_ERROR_BAD_CALL;


=POD
=section besEND_COMMAND
=H besEND_COMMAND

Use this macro to finish an extension module command.

Note that this macro uses the macro T<FINISH> that is empty by default. However a command
may decide to perform some action after the mortals are released. To do so the macro
T<FINISH> can be redefined.
=CUT
#define besEND_COMMAND goto _FunctionFinishLabel;\ // this is to avoid warnings on unrefereced labels
            _FunctionFinishLabel: \
            pSt->ReleaseMortals(pEo->pMo,&_ThisCommandMortals);\
            iErrorCode = 0;\ // this is to avoid warnings on unreferenced variable
            FINISH;\
            return 0;\
            }

=POD
=section besARGNR
=H besARGNR

This macro returns the number of arguments passed to the extension module interface function.
=CUT
#define besARGNR       (pParameters ? pParameters->ArrayHighLimit : 0)

=POD
=section besARGUMENT
=H besARGUMENT(X)
To access the function arguments the module can use the macro. The argument is the
ordinal number of the argument starting from 1. You can overindex the arguments. In
that case the macro value is T<NULL>.

Note that this macro can only be used in interface functions and not in interface commands.
=CUT
#define besARGUMENT(X) ((X) <= besARGNR ? pParameters->Value.aValue[(X)-1]: NULL)

=POD
=section besPARAMETERLIST
=H besPARAMETERLIST

Get the ordinal number of the node, where the parameter list starts for the
extenal module interface command. This macro should not be used in interface
functions only in interface commands.
=CUT
#define besPARAMETERLIST (pEo->OperatorNode)

=POD
=section besLEFTVALUE
=H besLEFTVALUE(X,Y)

Use this macro to evaluate an argument as left value in an interface command.
This macro should not be used in interface functions.
=CUT
#define besLEFTVALUE(X,Y) do{if( TYPE((X)) == VTYPE_REF ){\
                               __refcount_ = pSt->pEo->pMo->maxderef;\
                               Y=(X)->Value.aValue;\
                               while( *(Y) && TYPE(*(Y))== VTYPE_REF ){\
                                 (Y=(*(Y))->Value.aValue);\
                                 if( ! __refcount_ -- ){\
                                   return COMMAND_ERROR_CIRCULAR;\
                                   }\
                                 }\
                               }else Y=NULL;}while(0)

// define the name of the function which is called to negotiate the version
#define MODULE_VERSIONER   "versmodu"
// define the name of the function which is called to initialize the module
#define MODULE_INITIALIZER "bootmodu"
// define the function name which is called when the module is not used by the
// interpreter anymore
#define MODULE_FINALIZER   "finimodu"
// define the function name which is called when the module is shut-down
#define MODULE_SHUTDOWN    "shutmodu"
// define the autoloader function which is called when a function is not
// found in the DLL or so file
#define MODULE_AUTOLOADER  "automodu"
// define the function name which is called in multi thread implementation
// to decide whether to call dynlolib_FreeLibrary or not
// thus helping to keep a module inside a process even if there is no any
// current threads using it
#define MODULE_KEEPER      "keepmodu"
// define the error message function that results an error message whenever an
// error code is returned by the module
#define MODULE_ERRMSG      "emsgmodu"

=POD
=section besVERSION_NEGOTIATE
=H besVERSION_NEGOTIATE

Use this macro to start the module interface version negotiation function. The simplest
example is:

=verbatim
besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND
=noverbatim
=CUT
#define besVERSION_NEGOTIATE int DLL_EXPORT versmodu(int Version, char *pszVariation, void **ppModuleInternal){

=POD
=section besSUB_START
=H besSUB_START

Use this macro to start the module initialization function.

=verbatim
besSUB_START
  ....
besEND
=noverbatim
=CUT
#define besSUB_START  besFUNCTION(bootmodu)

=POD
=section besSUB_FINISH
=H besSUB_FINISH

Use this macro to start the module finalization function.

=verbatim
besSUB_FINISH
  ....
besEND
=noverbatim
=CUT
#define besSUB_FINISH besFUNCTION(finimodu)

=POD
=section besSUB_ERRMSG
=H besSUB_ERRMSG
Use this macro to start the error message function of an external module.

=verbatim
besSUB_ERRMSG
  ....
besEND
=noverbatim
=CUT
#define besSUB_ERRMSG char DLL_EXPORT * emsgmodu(pSupportTable pSt, \
                                                 void **ppModuleInternal, \
                                                 int iError){
=POD
=section besSUB_PROCESS_START
=H besSUB_PROCESS_START

Use this macro to start the function that will be invoked when the module is loaded by the
operating system. This is T<_init> under UNIX and T<DllMain> under Windows NT. Using this
macro the programmer can hide the OS dependant code.
=CUT
#if STATIC_LINK
#define besSUB_PROCESS_START static int _init(){
#else
#define besSUB_PROCESS_START int _init(){
#endif

=POD
=section besSUB_PROCESS_FINISH
=H besSUB_PROCESS_FINISH

Use this macro to start the function that will be invoked when the module is unloaded by the
operating system. This is T<_fini> under UNIX and T<DllMain> under Windows NT. Using this
macro the programmer can hide the OS dependant code.
=CUT
#if STATIC_LINK
#define besSUB_PROCESS_FINISH static int _fini(){
#else
#define besSUB_PROCESS_FINISH int _fini(){
#endif

=POD
=section besSUB_KEEP
=H besSUB_KEEP

Use this macro to start the module kepper function. This function should return 1 when the
module wants to remain in memory and 0 when the module can be unloaded.
=CUT
#define besSUB_KEEP int DLL_EXPORT keepmodu(){

=POD
=section besSUB_SHUTDOWN
=H besSUB_SHUTDOWN

Use this macro to start a module shutdown function.

This shutdown function is called before a module is unloaded from the process 
space. The function is similar to R<besSUB_FINISH>. That function is called when
the interpreter finishes. When there are many interpreter threads in a single process
that uses the module the function R<besSUB_FINISH> is called each time an
interpreter finishes. The function R<besSUB_SHUTDOWN> is called only once, before the
interpreter unloads the extesion from memory.

The difference between T<besSUB_SHUTDOWN> and R<besSUB_PROCESS_FINISH> is that T<besSUB_SHUTDOWN>
is called by the interpreter, R<besSUB_PROCESS_FINISH> is called by the operating system.
T<besSUB_SHUTDOWN> can access the support functions because it gets the T<pSt> argument,
R<besSUB_PROCESS_FINISH> can not access these functions.

When a single thread interpreter finishes it first calls the function R<besSUB_FINISH> to unload
the module and after that it calls T<besSUB_SHUTDOWN>.

This is not an error if a module does not implement these functions.

The function should return T<COMMAND_ERROR_SUCCESS> if the module has no remaining activity and
is ready to be unloaded.

The function should return T<COMMAND_ERROR_STAYS_IN_MEMORY> if there are unstopped threads
that use the module code. In this case unloading the module would cause segmentation
fault that would interfere with the still running shutdown procedures. In that case the module
is not unloaded by the program, but only when the process finishes by the operating system.
=CUT
#define besSUB_SHUTDOWN besFUNCTION(shutmodu)

=POD
=section besSUB_AUTO
=H besSUB_AUTO

Use this macro to start the external module autoloader function.

=CUT
#define besSUB_AUTO int DLL_EXPORT automodu(pSupportTable pSt, \
                                                     void **ppModuleInternal, \
                                                     char *pszFunction, \
                                                     void **ppFunction){
=POD
=section besEND
=H besEND

Use this macro to close an extension module interface function.
=CUT
#define besEND return 0;}

=POD
=section besRETURNVALUE
=H besRETURNVALUE
Use this macro to access the extension function or command return value pointer. Assign allocated
mortal variable to this pointer.
=CUT
#define besRETURNVALUE (*pReturnValue)

=POD
=section besMODULEPOINTER
=H besMODULEPOINTER
Use this macro to access the extension module pointer.
=CUT
#define besMODULEPOINTER (*ppModuleInternal)

=POD
=section besALLOC_RETURN_STRING
=H besALLOC_RETURN_STRING(X)

Use this macro to allocate a string as a return value. If there is not enough space to store the
result the macro returns from the function with the error code T<COMMAND_ERROR_MEMORY_LOW>.

The argument should be the number of bytes of the return value. After using this macro the
macro T<STRINGVALUE(besRETURNVALUE)> can be used to access the byte-buffer of the return
value. Usually the program uses T<memcpy> to copy the bytes there.
=CUT
#define besALLOC_RETURN_STRING(x) do{besRETURNVALUE  = besNEWMORTALSTRING(x);\
                                     if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                    }while(0);

=POD
=section besALLOC_RETURN_POINTER
=H besALLOC_RETURN_POINTER

Use this macro to allocate a string as a return value to return a pointer. If there is not enough space to store the
result the macro returns from the function with the error code T<COMMAND_ERROR_MEMORY_LOW>.

After using this macro the macro T<STRINGVALUE(besRETURNVALUE)> can be used to access the byte-buffer of the return
value. Usually the program uses T<memcpy> to copy the bytes there.
=CUT
#define besALLOC_RETURN_POINTER do{besRETURNVALUE  = besNEWMORTALSTRING(sizeof( void *));\
                                     if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                    }while(0);

=POD
=section besALLOC_RETURN_LONG
=H besALLOC_RETURN_LONG

Use this macro to allocate a T<long> as a return value. If there is not enough space to store the
result the macro returns from the function with the error code T<COMMAND_ERROR_MEMORY_LOW>.

After using this macro the
macro T<LONGVALUE(besRETURNVALUE)> can be used to access the long value of the return
value.
=CUT
#define besALLOC_RETURN_LONG do{besRETURNVALUE = besNEWMORTALLONG;\
                                     if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                    }while(0);

=POD
=section besALLOC_RETURN_DOUBLE
=H besALLOC_RETURN_DOUBLE

Use this macro to allocate a T<double> as a return value. If there is not enough space to store the
result the macro returns from the function with the error code T<COMMAND_ERROR_MEMORY_LOW>.

After using this macro the
macro T<DOUBLEVALUE(besRETURNVALUE)> can be used to access the double value of the return
value.
=CUT
#define besALLOC_RETURN_DOUBLE do{besRETURNVALUE = besNEWMORTALDOUBLE;\
                                  if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                  }while(0);


=POD
=section besRETURN_STRING
=H besRETURN_STRING(X)

Use this program to return a string value. The argument of the macro should be a zero terminated
string. 

The macro allocates the return string, copies the content of the string to the allocated space and
returns from the function using the macro with no error (COMMAND_ERROR_SUCCESS).

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.

If the argument is T<NULL> the macro will return the BASIC value T<undef>.

The macro evaluates its argument twice.
=CUT
#define besRETURN_STRING(x) do{if( NULL == (x) ){besRETURNVALUE = NULL; return COMMAND_ERROR_SUCCESS; }\
                               besRETURNVALUE  = besNEWMORTALSTRING(strlen(x));\
                               if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                               memcpy(STRINGVALUE(besRETURNVALUE),(x),STRLEN(besRETURNVALUE));\
                               return COMMAND_ERROR_SUCCESS;\
                              }while(0);

=POD
=section besSET_RETURN_STRING
=H besSET_RETURN_STRING(X)

Use this program to return a string value. The argument of the macro should be a zero terminated
string. 

The macro allocates the return string, copies the content of the string to the allocated space.
This macro does NOT return from the function that uses it. It allows the function to execute
some extra code before returning from the function, for example to release the string
variable passed as argument to this macro.

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.

If the argument is T<NULL> the macro will set the return value to be T<NULL> (BASIC value T<undef>).

The macro evaluates its argument twice.
=CUT
#define besSET_RETURN_STRING(x) do{if( NULL == (x) ){besRETURNVALUE = NULL;}else{\
                                     besRETURNVALUE  = besNEWMORTALSTRING((unsigned long)strlen(x));\
                                     if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                     memcpy(STRINGVALUE(besRETURNVALUE),(x),STRLEN(besRETURNVALUE));\
                                     }\
                                   }while(0);


=POD
=section besRETURN_MEM
=H besRETURN_MEM(X,Y)

Use this program to return a binary string value. The arguments of the macro should be a pointer
to the binary string and the T<long> length of the binary string. 

The macro allocates the return string, copies the content of the string to the allocated space and
returns from the function using the macro with no error (COMMAND_ERROR_SUCCESS).

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.

The macro evaluates its argument twice.
=CUT
#define besRETURN_MEM(x,y)    do{besRETURNVALUE  = besNEWMORTALSTRING(y);\
                                 if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                 memcpy(STRINGVALUE(besRETURNVALUE),(x),STRLEN(besRETURNVALUE));\
                                 return COMMAND_ERROR_SUCCESS;\
                                }while(0);

=POD
=section besRETURN_POINTER
=H besRETURN_POINTER(X)

Use this macro to return a pointer. The argument of the macro should be the pointer to return. The
BASIC program will see this value as a string of four (32 bit machines) or eight (64bit machines)
characters. The BASIC program should not alter the value but pass it back to the module wherever
the program needs. In other words the program should treat the value as an abstract handle
and not try to manipulate it.

The macro allocates the return string, copies the pointer into the string and returns from the function
using the macro with no error (COMMAND_ERROR_SUCCESS).

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.

If the pointer is T<NULL> the function will return the BASIC value T<undef>. This way you can not pass a
T<NULL> pointer back to the BASIC program stored as four (or eight) zero characters in a string. On the other
hand this is usually not what you really want and the BASIC program can check T<undef>indeness of the value.
When T<undef> is passed back to the module the argument handling functions convert it back to T<NULL>.
=CUT
#define besRETURN_POINTER(x) do{if( NULL == (x) ){ besRETURNVALUE = NULL; return COMMAND_ERROR_SUCCESS; }\
                                besRETURNVALUE  = besNEWMORTALSTRING(sizeof( void *));\
                                if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                                memcpy(STRINGVALUE(besRETURNVALUE),&(x),sizeof( void *) );\
                                return COMMAND_ERROR_SUCCESS;\
                                }while(0);

=POD
=section besRETURN_LONG
=H besRETURN_LONG(X)

Use this macro to return a long value. The argument should be a T<long> value to return.

The macro allocates the BASIC variable to return the value, sets the value to it to the actual
value of the macro argument and returns from the funcion using the macro with no error (COMMAND_ERROR_SUCCESS).

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.
=CUT
#define besRETURN_LONG(X) do{besRETURNVALUE = besNEWMORTALLONG;\
                             if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                             LONGVALUE(besRETURNVALUE) = (X);\
                             return COMMAND_ERROR_SUCCESS;\
                             }while(0);

=POD
=section besRETURN_DOUBLE
=H besRETURN_DOUBLE


Use this macro to return a double value. The argument should be a T<double> value to return.

The macro allocates the BASIC variable to return the value, sets the value to it to the actual
value of the macro argument and returns from the funcion using the macro with no error (COMMAND_ERROR_SUCCESS).

If the return value can not be allocated the macro returns with COMMAND_ERROR_MEMORY_LOW.
=CUT
#define besRETURN_DOUBLE(X) do{besRETURNVALUE = besNEWMORTALDOUBLE;\
                               if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;\
                               DOUBLEVALUE(besRETURNVALUE) = (X);\
                               return COMMAND_ERROR_SUCCESS;\
                               }while(0);

=POD
=section besSETCOMMAND
=H besSETCOMMAND(X,Y)
Use this macro to alter the command table. T<X> is the command code and T<Y> is the
new function implementing the command.
=CUT
#define besSETCOMMAND(X,Y) (pSt->pEo->pCommandFunction[(X)-START_CMD] = Y)

=POD
=section besGETCOMMAND
=H besGETCOMMAND(X)
Use this macro to get the command function currently assigned to the command T<X>.
=CUT
#define besGETCOMMAND(X)   (pSt->pEo->pCommandFunction[(X)-START_CMD])

=POD
=section INTERFACE_VERSION
=H INTERFACE_VERSION
The current external module interface version. This is an integer number.
=CUT
#define INTERFACE_VERSION 11

=POD
=section besHOOK_FILE_ACCESS
=H besHOOK_FILE_ACCESS
This macro calls the function R<hook_file_access>
=CUT
#define besHOOK_FILE_ACCESS(X) (pSt->pEo->pHookers->HOOK_file_access(pSt->pEo,(X)))
=POD
=section besHOOK_FOPEN
=H besHOOK_FOPEN
This macro calls the function R<hook_fopen>
=CUT
#define besHOOK_FOPEN(X,Y) (pSt->pEo->pHookers->HOOK_fopen(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_FCLOSE
=H besHOOK_FCLOSE
This macro calls the function R<hook_fclose>
=CUT
#define besHOOK_FCLOSE(X) (pSt->pEo->pHookers->HOOK_fclose(pSt->pEo,(X)))
=POD
=section besHOOK_SIZE
=H besHOOK_SIZE
This macro calls the function R<hook_size>
=CUT
#define besHOOK_SIZE(X) (pSt->pEo->pHookers->HOOK_size(pSt->pEo,(X)))
=POD
=section besHOOK_TIME_ACCESSED
=H besHOOK_TIME_ACCESSED
This macro calls the function R<hook_time_accessed>
=CUT
#define besHOOK_TIME_ACCESSED(X) (pSt->pEo->pHookers->HOOK_time_accessed(pSt->pEo,(X)))
=POD
=section besHOOK_TIME_MODIFIED
=H besHOOK_TIME_MODIFIED
This macro calls the function R<hook_time_modified>
=CUT
#define besHOOK_TIME_MODIFIED(X) (pSt->pEo->pHookers->HOOK_time_modified(pSt->pEo,(X)))
=POD
=section besHOOK_TIME_CREATED
=H besHOOK_TIME_CREATED
This macro calls the function R<hook_time_created>
=CUT
#define besHOOK_TIME_CREATED(X) (pSt->pEo->pHookers->HOOK_time_created(pSt->pEo,(X)))
=POD
=section besHOOK_ISDIR
=H besHOOK_ISDIR
This macro calls the function R<hook_isdir>
=CUT
#define besHOOK_ISDIR(X) (pSt->pEo->pHookers->HOOK_isdir(pSt->pEo,(X)))
=POD
=section besHOOK_ISREG
=H besHOOK_ISREG
This macro calls the function R<hook_isreg>
=CUT
#define besHOOK_ISREG(X) (pSt->pEo->pHookers->HOOK_isreg(pSt->pEo,(X)))
=POD
=section besHOOK_EXISTS
=H besHOOK_EXISTS
This macro calls the function R<hook_fileexists>
=CUT
#define besHOOK_EXISTS(X) (pSt->pEo->pHookers->HOOK_exists(pSt->pEo,(X)))
=POD
=section besHOOK_TRUNCATE
=H besHOOK_TRUNCATE
This macro calls the function R<hook_truncate>
=CUT
#define besHOOK_TRUNCATE(X,Y) (pSt->pEo->pHookers->HOOK_truncate(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_FGETC
=H besHOOK_FGETC
This macro calls the function R<hook_fgetc>
=CUT
#define besHOOK_FGETC(X) (pSt->pEo->pHookers->HOOK_fgetc(pSt->pEo,(X)))
=POD
=section besHOOK_FREAD
=H besHOOK_FREAD
This macro calls the function R<hook_fread>
=CUT
#define besHOOK_FREAD(X,Y,Z,W) (pSt->pEo->pHookers->HOOK_fread(pSt->pEo,(X),(Y),(Z),(W)))
=POD
=section besHOOK_FWRITE
=H besHOOK_FWRITE
This macro calls the function R<hook_fwrite>
=CUT
#define besHOOK_FWRITE(X,Y,Z,W) (pSt->pEo->pHookers->HOOK_fwrite(pSt->pEo,(X),(Y),(Z),(W)))
=POD
=section besHOOK_FERROR
=H besHOOK_FERROR
This macro calls the function R<hook_ferror>
=CUT
#define besHOOK_FERROR(X) (pSt->pEo->pHookers->HOOK_ferror(pSt->pEo,(X)))
=POD
=section besHOOK_PUTC
=H besHOOK_PUTC
This macro calls the function R<hook_fputc>
=CUT
#define besHOOK_PUTC(X,Y) (pSt->pEo->pHookers->HOOK_fputc(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_FLOCK
=H besHOOK_FLOCK
This macro calls the function R<hook_flock>
=CUT
#define besHOOK_FLOCK(X,Y) (pSt->pEo->pHookers->HOOK_flock(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_LOCK
=H besHOOK_LOCK
This macro calls the function R<hook_lock>
=CUT
#define besHOOK_LOCK(X,Y,Z,W) (pSt->pEo->pHookers->HOOK_lock(pSt->pEo,(X),(Y),(Z),(W)))
=POD
=section besHOOK_FEOF
=H besHOOK_FEOF
This macro calls the function R<hook_feof>
=CUT
#define besHOOK_FEOF(X) (pSt->pEo->pHookers->HOOK_feof(pSt->pEo,(X)))
=POD
=section besHOOK_MKDIR
=H besHOOK_MKDIR
This macro calls the function R<hook_mkdir>
=CUT
#define besHOOK_MKDIR(X) (pSt->pEo->pHookers->HOOK_mkdir(pSt->pEo,(X)))
=POD
=section besHOOK_RMDIR
=H besHOOK_RMDIR
This macro calls the function R<hook_rmdir>
=CUT
#define besHOOK_RMDIR(X) (pSt->pEo->pHookers->HOOK_rmdir(pSt->pEo,(X)))
=POD
=section besHOOK_REMOVE
=H besHOOK_REMOVE
This macro calls the function R<hook_remove>
=CUT
#define besHOOK_REMOVE(X) (pSt->pEo->pHookers->HOOK_remove(pSt->pEo,(X)))
=POD
=section besHOOK_DELTREE
=H besHOOK_DELTREE
This macro calls the function R<hook_deltree>
=CUT
#define besHOOK_DELTREE(X) (pSt->pEo->pHookers->HOOK_deltree(pSt->pEo,(X)))
=POD
=section besHOOK_MAKEDIRECTORY
=H besHOOK_MAKEDIRECTORY
This macro calls the function R<hook_MakeDirectory>
=CUT
#define besHOOK_MAKEDIRECTORY(X) (pSt->pEo->pHookers->HOOK_MakeDirectory(pSt->pEo,(X)))
=POD
=section besHOOK_OPENDIR
=H besHOOK_OPENDIR
This macro calls the function R<hook_opendir>
=CUT
#define besHOOK_OPENDIR(X,Y) (pSt->pEo->pHookers->HOOK_opendir(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_READDIR
=H besHOOK_READDIR
This macro calls the function R<hook_readdir>
=CUT
#define besHOOK_READDIR(X) (pSt->pEo->pHookers->HOOK_readdir(pSt->pEo,(X)))
=POD
=section besHOOK_CLOSEDIR
=H besHOOK_CLOSEDIR
This macro calls the function R<hook_closedir>
=CUT
#define besHOOK_CLOSEDIR(X) (pSt->pEo->pHookers->HOOK_closedir(pSt->pEo,(X)))
=POD
=section besHOOK_SLEEP
=H besHOOK_SLEEP
This macro calls the function R<hook_sleep>
=CUT
#define besHOOK_SLEEP(X) (pSt->pEo->pHookers->HOOK_sleep(pSt->pEo,(X)))
=POD
=section besHOOK_CURDIR
=H besHOOK_CURDIR
This macro calls the function R<hook_curdir>
=CUT
#define besHOOK_CURDIR(X,Y) (pSt->pEo->pHookers->HOOK_curdir(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_CHDIR
=H besHOOK_CHDIR
This macro calls the function R<hook_chdir>
=CUT
#define besHOOK_CHDIR(X) (pSt->pEo->pHookers->HOOK_chdir(pSt->pEo,(X)))
=POD
=section besHOOK_CHOWN
=H besHOOK_CHOWN
This macro calls the function R<hook_chown>
=CUT
#define besHOOK_CHOWN(X,Y) (pSt->pEo->pHookers->HOOK_chown(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_SETCREATETIME
=H besHOOK_SETCREATETIME
This macro calls the function R<hook_SetCreateTime>
=CUT
#define besHOOK_SETCREATETIME(X,Y) (pSt->pEo->pHookers->HOOK_SetCreateTime(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_SETMODIFYTIME
=H besHOOK_SETMODIFYTIME
This macro calls the function R<hook_SetModifyTime>
=CUT
#define besHOOK_SETMODIFYTIME(X,Y) (pSt->pEo->pHookers->HOOK_SetModifyTime(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_SETACCESSTIME
=H besHOOK_SETACCESSTIME
This macro calls the function R<hook_SetAccessTime>
=CUT
#define besHOOK_SETACCESSTIME(X,Y) (pSt->pEo->pHookers->HOOK_SetAccessTime(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_GETHOSTNAME
=H besHOOK_GETHOSTNAME
This macro calls the function R<hook_GetHostName>
=CUT
#define besHOOK_GETHOSTNAME(X,Y) (pSt->pEo->pHookers->HOOK_GetHostName(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_GETHOST
=H besHOOK_GETHOST
This macro calls the function R<hook_GetHost>
=CUT
#define besHOOK_GETHOST(X,Y) (pSt->pEo->pHookers->HOOK_GetHost(pSt->pEo,(X),((Y)))
=POD
=section besHOOK_TCPCONNECT
=H besHOOK_TCPCONNECT
This macro calls the function R<hook_TcpConnect>
=CUT
#define besHOOK_TCPCONNECT(X,Y) (pSt->pEo->pHookers->HOOK_TcpConnect(pSt->pEo,(X),(Y)))
=POD
=section besHOOK_TCPSEND
=H besHOOK_TCPSEND
This macro calls the function R<hook_TcpSend>
=CUT
#define besHOOK_TCPSEND(X,Y,Z) (pSt->pEo->pHookers->HOOK_TcpSend(pSt->pEo,(X),(Y),(Z)))
=POD
=section besHOOK_TCPRECV
=H besHOOK_TCPRECV
This macro calls the function R<hook_TcpRecv>
=CUT
#define besHOOK_TCPRECV(X,Y,Z) (pSt->pEo->pHookers->HOOK_TcpRecv(pSt->pEo,(X),(Y),(Z)))
=POD
=section besHOOK_TCPCLOSE
=H besHOOK_TCPCLOSE
This macro calls the function R<hook_TcpClose>
=CUT
#define besHOOK_TCPCLOSE(Y) (pSt->pEo->pHookers->HOOK_TcpClose(pSt->pEo,(X))
=POD
=section besHOOK_KILLPROC
=H besHOOK_KILLPROC
This macro calls the function R<hook_KillProc>
=CUT
#define besHOOK_KILLPROC(X) (pSt->pEo->pHookers->HOOK_KillProc(pSt->pEo,(X))
=POD
=section besHOOK_GETOWNER
=H besHOOK_GETOWNER
This macro calls the function R<hook_GetOwner>
=CUT
#define besHOOK_GETOWNER(X,Y,Z) (pSt->pEo->pHookers->HOOK_GetOwner(pSt->pEo,(X),(Y),(Z));
=POD
=section besHOOK_CREATEPROCESS
=H besHOOK_CREATEPROCESS
This macro calls the function R<hook_CreateProcess>
=CUT
#define besHOOK_CREATEPROCESS(X) (pSt->pEo->pHookers->HOOK_CreateProcess(pSt->pEo,(X));
=POD
=section besHOOK_CALLSCRIBAFUNCTION
=H besHOOK_CALLSCRIBAFUNCTION
This macro calls the function R<hook_CallScribaFunction>
=CUT
#define besHOOK_CALLSCRIBAFUNCTION(X,Y,Z,W) (pSt->pEo->pHookers->HOOK_CallScribaFunction(pSt->pEo,(X),(Y),(Z),(W)))
=POD
=section besSETHOOK
=H besSETHOOK(X,Y)
Use this macro to alter the hook function table.
=CUT
#define besSETHOOK(X,Y) (pSt->pEo->pHookers->HOOK_##X = Y)

=POD
=section besDLL_MAIN
=H besDLL_MAIN

process header macro makes UNIX like dll loading and unloading
on Win32. This just defines a wrapper DllMain that calls _init() and
_fini() that a the default library loading and unloading functions
under UNIX.

Use of this macro may be needed for modules that serve multi thread
interpreters and share resources on the process level
=CUT
#ifdef WIN32
#define besDLL_MAIN \
BOOL __declspec(dllexport) WINAPI DllMain(\
  HINSTANCE hinstDLL,\
  DWORD fdwReason,\
  LPVOID lpvReserved\
  ){\
  int _init(void);\
  int _fini(void);\
  switch( fdwReason ){\
    case DLL_PROCESS_ATTACH:\
      _init();\
      break;\
    case DLL_THREAD_ATTACH: return TRUE;\
    case DLL_THREAD_DETACH: return TRUE;\
    case DLL_PROCESS_DETACH:\
      _fini();\
      break;\
    }\
  return TRUE;\
  }
#else
#define besDLL_MAIN
#endif

=POD
=section INITLOCK
=H INITLOCK

Whent he process first time loads an extension and the extension wants to decide whether
to unload it or keep in memory it needs a counter and a mutex to access the counter
to declare the counter the extension should use the macro SUPPORT_MULTITHREAD and
to initialize it it should use the macro INIT_MULTITHREAD

Note that the call-back functions to handle the mutexes OS independant are not available
by the time when the OS calls DllMain on NT or _init on UNIX, thus the code should call
system dependant functions directly

IsThisTheVeryFirstThreadCallingTheModule <- name of the function
=CUT
#ifdef WIN32
#define INITLOCK WaitForSingleObject(mxInit,INFINITE);
#define INITUNLO ReleaseSemaphore(mxInit,1,NULL);
#else
#define INITLOCK pthread_mutex_lock(&mxInit);
#define INITUNLO pthread_mutex_unlock(&mxInit);
#endif

#define SUPPORT_MULTITHREAD static MUTEX mxThreadCounter,mxInit; static int iFirst;static long lThreadCounter;

#ifdef WIN32
#define INIT_MULTITHREAD      mxThreadCounter = CreateSemaphore(NULL,1,1,NULL);\
                              mxInit = CreateSemaphore(NULL,1,1,NULL);\
                              iFirst = 1;
#define GET_THREAD_COUNTER(X) WaitForSingleObject(mxThreadCounter,INFINITE);\
                              (X) = lThreadCounter;\
                              ReleaseSemaphore(mxThreadCounter,1,NULL);
#define INC_THREAD_COUNTER    WaitForSingleObject(mxThreadCounter,INFINITE);\
                              lThreadCounter++;\
                              ReleaseSemaphore(mxThreadCounter,1,NULL);
#define DEC_THREAD_COUNTER    WaitForSingleObject(mxThreadCounter,INFINITE);\
                              lThreadCounter--;\
                              ReleaseSemaphore(mxThreadCounter,1,NULL);
#define FINISH_MULTITHREAD    CloseHandle(mxThreadCounter);
#else
#define INIT_MULTITHREAD      pthread_mutex_init(&mxThreadCounter,NULL);\
                              pthread_mutex_init(&mxInit,NULL);\
                              iFirst = 1;
#define GET_THREAD_COUNTER(X) pthread_mutex_lock(&mxThreadCounter);\
                              (X) = lThreadCounter;\
                              pthread_mutex_unlock(&mxThreadCounter);
#define INC_THREAD_COUNTER    pthread_mutex_lock(&mxThreadCounter);\
                              lThreadCounter++;\
                              pthread_mutex_unlock(&mxThreadCounter);
#define DEC_THREAD_COUNTER    pthread_mutex_lock(&mxThreadCounter);\
                              lThreadCounter--;\
                              pthread_mutex_unlock(&mxThreadCounter);
#define FINISH_MULTITHREAD    pthread_mutex_destroy(&mxThreadCounter);
#endif

#define START_FUNCTION_TABLE(X) SLFST X[] ={
#define EXPORT_MODULE_FUNCTION(X) { #X , X },
#define END_FUNCTION_TABLE { NULL , NULL } };

*/

#include <stdio.h>
#include <stdarg.h>
#include "basext.h"

/*POD
=H basext_GetArgsF()

This function can be used to get arguments simple and fast in extension modules.
All functionality of this function can be individually programmed using the
T<besXXX> macros. Here it is to ease the programming of extension modules for most of
the cases.

This function should be called like

=verbatim
  iError = besGETARGS "ldz",&l1,&d1,&s besGETARGE
=noverbatim

The macro T<besGETARGS> (read GET ARGument Start) hides the complexity of the
function call and the macro T<besGETARGE> (read Get ARGument End) simply closes the
function call.

The first argument is format string. Each character specifies how the next argument
should be treated.

/*FUNCTION*/
int basext_GetArgsF(pSupportTable pSt,
                    pFixSizeMemoryObject pParameters,
                    char *pszFormat,
                    ...
  ){
/*noverbatim
The following characters are recognized:

=itemize
=item T<i> the next argument of the function call should point to a T<long> variable.
           The ScriptBasic argument will be converted to T<long> using the macro
           T<besCONVERT2LONG> and will be stored in the T<long> variable.
=item T<r> the same as T<l> except that the argument should point a T<double> and the
           basic argument is converted to T<double> using T<besCONVERT2DOUBLE>.
=item T<z> the next argument should point to a T<char *> pointer. The function takes
           the next BASIC argument as string, converts it to zero terminated string
           allocating space for it. These variables SHOULD be released by the caller
           using the macro T<besFREE>.
=item T<s> the next argument should point to a T<unsigned char *> pointer. The function takes
           the next BASIC argument as string, converting it in case conversion is needed, and
           sets the T<unsigned char *> pointer to point to the string. This format character
           should be used together with the character T<l>
=item T<l> the next argument should point to a T<long> and the value of the variable
           will be the length of the last string atgument (either T<z> or T<s>).
           If there was no previous string argument the value returned will be zero.
=item T<p> the next argument should point to a T<void *> pointer. The BASIC argument value
           should be a string of T<sizeof(void *)> characters that will be copied into the
           pointer argument. If the argument is not string or has not the proper size the function
           returns T<COMMAND_ERROR_ARGUMENT_RANGE>.
=item T<[> The arguments following this character are optional. Optional arguments may be
           unspecified. This is the case when the BASIC function call has less number of
           arguments or when the actual argument value is T<undef>. In case of optional
           arguments the T<undef> values are converted to zero value of the appropriate
           type. This means 0 in case of long, 0.0 in case of double, NULL in case of pointer
           and zero length string in case of strings.
=item T<]> Arguments following this character are mandatory (are not optional). When the
           function starts to process the arguments they are mandatory by default. Using this
           notation you can enclode the optional arguments between T<[> and T<]>. For example
           the format string T<"ii[z]"> means two long arguments and an optional zero terminated
           string argument.
=item T<*> The argument is skipped. This may be used during development of a function.
=noitemize

The return value of the function is zero in case there is no error or the error code.
CUT*/
  va_list marker;
  long ArgNr;
  void *argptr;
  char *s;
  long *pL;
  double *pD;
  unsigned char **ppS;
  void **ppV;
  VARIABLE Argumentum;
  int mandatory; /* true when we process mandatory arguments (should not be undef) */
  char **ppszStrAlloc;
  long iStrArg,iArgNr;
  unsigned long lLastStrLen = 0;

  /* allocate a string array that keeps track of all allocated zchar strings
     if the memory allocation fails somewhere in between the string are freed
     and we do not loose memory */
  ppszStrAlloc = (char **)besALLOC(sizeof(char *) * (iArgNr=strlen(pszFormat)));
  if( ppszStrAlloc == NULL )return COMMAND_ERROR_MEMORY_LOW;
  for( iStrArg = 0 ; iStrArg < iArgNr ; iStrArg++ )ppszStrAlloc[iStrArg] = NULL;
  iStrArg = 0; /* the next string */
  va_start(marker,pszFormat);
  ArgNr = 1;
  mandatory = 1;
#define NEXT_ARG argptr = va_arg(marker, void * );
  NEXT_ARG
  for( s = pszFormat ; *s ; s++ ){
    switch( *s ){
      case '[': mandatory = 0;break;
      case ']': mandatory = 1;break;
      case '*': ArgNr++; break; /* Skip this argument */
      case 'l': /* Get the length of the last string either 'z' or 's' */
        pL = (long *)argptr;
        NEXT_ARG
        *pL = lLastStrLen;
        break;
      case 'i': /* Get a long */
        pL = (long *)argptr;
        NEXT_ARG
        Argumentum = besARGUMENT(ArgNr);
        besDEREFERENCE(Argumentum);
        ArgNr++;
        if( memory_IsUndef(Argumentum) && mandatory ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MANDARG;
          }
        if( memory_IsUndef(Argumentum) ){
          *pL = 0;
          }else{
          Argumentum = besCONVERT2LONG(Argumentum);
          *pL = LONGVALUE(Argumentum);
          }
        break;
      case 'r': /* Get a double */
        pD = (double *)argptr;
        NEXT_ARG
        Argumentum = besARGUMENT(ArgNr);
        ArgNr++;
        besDEREFERENCE(Argumentum);
        if( memory_IsUndef(Argumentum) && mandatory ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MANDARG;
          }
        if( memory_IsUndef(Argumentum) ){
          *pD = 0.0;
          }else{
          *pD = besGETDOUBLEVALUE(Argumentum);
          }
        break;
      case 'z': /* Get a zchar string */
        ppS = (unsigned char **)argptr;
        NEXT_ARG
        Argumentum = besARGUMENT(ArgNr);
        ArgNr++;
        besDEREFERENCE(Argumentum);
        if( memory_IsUndef(Argumentum) && mandatory ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MANDARG;
          }
        Argumentum = besCONVERT2STRING(Argumentum);
        *ppS = besALLOC( (lLastStrLen = STRLEN(Argumentum))+1);
        if( *ppS == NULL ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MEMORY_LOW;
          }
        ppszStrAlloc[iStrArg++] = *ppS;
        memcpy(*ppS,STRINGVALUE(Argumentum),lLastStrLen);
        (*ppS)[STRLEN(Argumentum)] = (char)0;
        break;
      case 'p': /* Get a pointer */
        ppV = (void **)argptr;
        NEXT_ARG
        Argumentum = besARGUMENT(ArgNr);
        ArgNr++;
        besDEREFERENCE(Argumentum);
        if( memory_IsUndef(Argumentum) && mandatory ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MANDARG;
          }
        if( ! memory_IsUndef(Argumentum) && 
           (TYPE(Argumentum) != VTYPE_STRING || 
            STRLEN(Argumentum) != sizeof( void * )) ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_ARGUMENT_RANGE;
          }
        if( memory_IsUndef(Argumentum) )
          *ppV = NULL;
        else
          memcpy(ppV, STRINGVALUE(Argumentum),sizeof( void * ) );
        break;
      case 's': /* Get a string */
        ppS = (unsigned char **)argptr;
        NEXT_ARG
        Argumentum = besARGUMENT(ArgNr);
        ArgNr++;
        besDEREFERENCE(Argumentum);
        if( memory_IsUndef(Argumentum) && mandatory ){
          while( iStrArg -- )besFREE(ppszStrAlloc[iStrArg]);
          besFREE(ppszStrAlloc);
          return COMMAND_ERROR_MANDARG;
          }
        Argumentum = besCONVERT2STRING(Argumentum);
        *ppS = STRINGVALUE(Argumentum);
        lLastStrLen = STRLEN(Argumentum);
        break;
      }
    }
  va_end( marker );
  besFREE(ppszStrAlloc);
  return 0;
  }
