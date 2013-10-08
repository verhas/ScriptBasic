/* modumana.c

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

 Module manager. Since version 1.0 build 14 module management is moved to the execution level
 from the command level. Module management became a general service of the ScriptBasic core
 code and is handled mainly in this file.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "basext.h"
#include "sym.h"
#include "errcodes.h"
#include "report.h"
#include "lexer.h"
#include "expression.h"
#include "builder.h"
#include "memory.h"
#include "syntax.h"
#include "execute.h"
#include "myalloc.h"
#include "dynlolib.h"
#include "modumana.h"

#if BCC32
extern char *_pgmptr;
#endif
extern int GlobalDebugDisplayFlag;

/*POD
@c Module management

This file contains all the functions that handle external module management.

Note that all function names are prepended by T<modu_>

CUT*/

/*POD
=H modu_Init
@c Initialize the module management

This function allocates memory for the external module interface table and
initializes the function pointers.

If the interface already exists and the function is called again it just
silently returns.

The second argument can be zero or 1. The normal operation is zero. If T<iForce>
is true the function sets each function pointer to its initial value even if an
initialization has already occured before.

This can be used in a rare case when a module modifies the interface table and
want to reinitialize it to the original value. Be carefule with such
constructions.

/*FUNCTION*/
int modu_Init(pExecuteObject pEo,
              int iForce
  ){
/*noverbatim
CUT*/
  if( pEo->pST != NULL && !iForce ) return COMMAND_ERROR_SUCCESS;

  if( pEo->pST == NULL )
    pEo->pST = alloc_Alloc(sizeof(SupportTable),pEo->pMemorySegment);
  if( pEo->pST == NULL )return COMMAND_ERROR_MEMORY_LOW;
  /* If the process level inherited support table is NULL then we are
     in a single thread environment. In this case multi-thread modules
     may need the STI field pointing to the same support table. */
  if( pEo->pSTI == NULL )pEo->pSTI = pEo->pST;
  pEo->pST->Alloc               = alloc_Alloc;
  pEo->pST->Free                = alloc_Free;
  pEo->pST->InitSegment         = alloc_InitSegment;
  pEo->pST->SegmentLimit        = alloc_SegmentLimit;
  pEo->pST->FreeSegment         = alloc_FreeSegment;
  pEo->pST->FinishSegment       = alloc_FinishSegment;
  pEo->pST->NewMortalString     = memory_NewMortalString;
  pEo->pST->NewMortalLong       = memory_NewMortalLong;
  pEo->pST->NewMortalDouble     = memory_NewMortalDouble;
  pEo->pST->NewMortalArray      = memory_NewMortalArray;
  pEo->pST->NewMortalRef        = memory_NewMortalRef;
  pEo->pST->NewString           = memory_NewString;
  pEo->pST->NewLong             = memory_NewLong;
  pEo->pST->NewDouble           = memory_NewDouble;
  pEo->pST->NewArray            = memory_NewArray;
  pEo->pST->NewRef              = memory_NewRef;
  pEo->pST->ReleaseVariable     = memory_ReleaseVariable;
  pEo->pST->SetRef              = memory_SetRef;

  pEo->pST->ConfigData          = cft_GetString;
  pEo->pST->FindNode            = cft_FindNode;
  pEo->pST->GetEx               = cft_GetEx;
  pEo->pST->EnumFirst           = cft_EnumFirst;
  pEo->pST->EnumNext            = cft_EnumNext;
  pEo->pST->GetKey              = cft_GetKey;

  pEo->pST->NewSymbolTable      = sym_NewSymbolTable;
  pEo->pST->FreeSymbolTable     = sym_FreeSymbolTable;
  pEo->pST->TraverseSymbolTable = sym_TraverseSymbolTable;
  pEo->pST->LookupSymbol        = sym_LookupSymbol;
  pEo->pST->DeleteSymbol        = sym_DeleteSymbol;

  pEo->pST->LoadLibrary         = dynlolib_LoadLibrary;
  pEo->pST->FreeLibrary         = dynlolib_FreeLibrary;
  pEo->pST->GetFunctionByName   = dynlolib_GetFunctionByName;

  pEo->pST->fopen               = file_fopen;
  pEo->pST->fclose              = file_fclose;
  pEo->pST->size                = file_size;
  pEo->pST->time_accessed       = file_time_accessed;
  pEo->pST->time_modified       = file_time_modified;
  pEo->pST->time_created        = file_time_created;
  pEo->pST->isdir               = file_isdir;
  pEo->pST->isreg               = file_isreg;
  pEo->pST->exists              = file_exists;
  pEo->pST->truncate            = file_truncate;
  pEo->pST->fgetc               = file_fgetc;
  pEo->pST->ferror              = file_ferror;
  pEo->pST->fread               = file_fread;
  pEo->pST->fwrite              = file_fwrite;
  pEo->pST->fputc               = file_fputc;
  pEo->pST->flock               = file_flock;
  pEo->pST->lock                = file_lock;
  pEo->pST->feof                = file_feof;
  pEo->pST->mkdir               = file_mkdir;
  pEo->pST->rmdir               = file_rmdir;
  pEo->pST->remove              = file_remove;
  pEo->pST->deltree             = file_deltree;
  pEo->pST->MakeDirectory       = file_MakeDirectory;
  pEo->pST->opendir             = file_opendir;
  pEo->pST->readdir             = file_readdir;
  pEo->pST->closedir            = file_closedir;

  pEo->pST->GetOption           = options_Get;
  pEo->pST->SetOption           = options_Set;
  pEo->pST->ResetOption         = options_Reset;
  pEo->pST->Convert2String      = execute_Convert2String;
  pEo->pST->Convert2Long        = execute_Convert2LongS;
  pEo->pST->GetLongValue        = execute_GetLongValue;
  pEo->pST->Convert2Double      = execute_Convert2DoubleS;
  pEo->pST->GetDoubleValue      = execute_GetDoubleValue;
  pEo->pST->Dereference         = execute_DereferenceS;
  pEo->pST->IsStringInteger     = execute_IsStringInteger;

  pEo->pST->InitModuleInterface = modu_Init;
  pEo->pST->LoadModule          = modu_LoadModule;
  pEo->pST->GetModuleFunctionByName = modu_GetFunctionByName;
  pEo->pST->UnloadAllModules    = modu_UnloadAllModules;
  pEo->pST->UnloadModule        = modu_UnloadModule;

  pEo->pST->sleep               = sys_sleep;
  pEo->pST->curdir              = file_curdir;
  pEo->pST->chdir               = file_chdir;
  pEo->pST->chown               = file_chown;
  pEo->pST->SetCreateTime       = file_SetCreateTime;
  pEo->pST->SetModifyTime       = file_SetModifyTime;
  pEo->pST->SetAccessTime       = file_SetAccessTime;

  pEo->pST->GetHostName         = file_gethostname;
  pEo->pST->GetHost             = file_gethost;
  pEo->pST->TcpConnect          = file_tcpconnect;
  pEo->pST->TcpSend             = file_tcpsend;
  pEo->pST->TcpRecv             = file_tcprecv;
  pEo->pST->TcpClose            = file_tcpclose;

  pEo->pST->KillProc            = file_killproc;
  pEo->pST->GetOwner            = file_getowner;
  pEo->pST->Crypt               = file_fcrypt;

#pragma warning(disable:4113)
  pEo->pST->MD5Init             = MD5Init;
  pEo->pST->MD5Update           = MD5Update;
  pEo->pST->MD5Final            = MD5Final;
#pragma warning(default:4113)

  pEo->pST->CreateProcess       = file_CreateProcess;

  pEo->pST->CopyCommandTable    = execute_CopyCommandTable;
  pEo->pST->GetCommandByName    = execute_GetCommandByName;

  pEo->pST->DupMortalize        = memory_DupMortalize;
  pEo->pST->Evaluate            = execute_Evaluate;
  pEo->pST->LeftValue           = execute_LeftValue;
  pEo->pST->Immortalize         = memory_Immortalize;
  pEo->pST->ReleaseMortals      = memory_ReleaseMortals;



  pEo->pST->match_index         = match_index;
  pEo->pST->match_InitSets      = match_InitSets;
  pEo->pST->match_ModifySet     = match_ModifySet;
  pEo->pST->match_match         = match_match;
  pEo->pST->match_count         = match_count;
  pEo->pST->match_parameter     = match_parameter;
  pEo->pST->match_size          = match_size;

  pEo->pST->thread_CreateThread = thread_CreateThread;
  pEo->pST->thread_ExitThread   = thread_ExitThread;
  pEo->pST->thread_InitMutex    = thread_InitMutex;
  pEo->pST->thread_FinishMutex  = thread_FinishMutex;
  pEo->pST->thread_LockMutex    = thread_LockMutex;
  pEo->pST->thread_UnlockMutex  = thread_UnlockMutex;
  pEo->pST->shared_InitLock     = shared_InitLock;
  pEo->pST->shared_FinishLock   = shared_FinishLock;
  pEo->pST->shared_LockRead     = shared_LockRead;
  pEo->pST->shared_LockWrite    = shared_LockWrite;
  pEo->pST->shared_UnlockRead   = shared_UnlockRead;
  pEo->pST->shared_UnlockWrite  = shared_UnlockWrite;

  /* using these callback functions an extension may embed
     a ScriptBasic interpreter into the module */

  pEo->pST->scriba_new = scriba_new;
  pEo->pST->scriba_destroy = scriba_destroy;
  pEo->pST->scriba_NewSbData = scriba_NewSbData;
  pEo->pST->scriba_NewSbLong = scriba_NewSbLong;
  pEo->pST->scriba_NewSbDouble = scriba_NewSbDouble;
  pEo->pST->scriba_NewSbUndef = scriba_NewSbUndef;
  pEo->pST->scriba_NewSbString = scriba_NewSbString;
  pEo->pST->scriba_NewSbBytes = scriba_NewSbBytes;
  pEo->pST->scriba_DestroySbData = scriba_DestroySbData;
  pEo->pST->scriba_PurgeReaderMemory = scriba_PurgeReaderMemory;
  pEo->pST->scriba_PurgeLexerMemory = scriba_PurgeLexerMemory;
  pEo->pST->scriba_PurgeSyntaxerMemory = scriba_PurgeSyntaxerMemory;
  pEo->pST->scriba_PurgeBuilderMemory = scriba_PurgeBuilderMemory;
  pEo->pST->scriba_PurgeExecuteMemory = scriba_PurgeExecuteMemory;
  pEo->pST->scriba_SetFileName = scriba_SetFileName;
  pEo->pST->scriba_LoadConfiguration = scriba_LoadConfiguration;
  pEo->pST->scriba_InheritConfiguration = scriba_InheritConfiguration;
  pEo->pST->scriba_SetCgiFlag = scriba_SetCgiFlag;
  pEo->pST->scriba_SetReportFunction = scriba_SetReportFunction;
  pEo->pST->scriba_SetReportPointer = scriba_SetReportPointer;
  pEo->pST->scriba_SetStdin = scriba_SetStdin;
  pEo->pST->scriba_SetStdout = scriba_SetStdout;
  pEo->pST->scriba_SetEmbedPointer = scriba_SetEmbedPointer;
  pEo->pST->scriba_SetEnvironment = scriba_SetEnvironment;
  pEo->pST->scriba_LoadBinaryProgram = scriba_LoadBinaryProgram;
  pEo->pST->scriba_InheritBinaryProgram = scriba_InheritBinaryProgram;
  pEo->pST->scriba_ReadSource = scriba_ReadSource;
  pEo->pST->scriba_DoLexicalAnalysis = scriba_DoLexicalAnalysis;
  pEo->pST->scriba_DoSyntaxAnalysis = scriba_DoSyntaxAnalysis;
  pEo->pST->scriba_BuildCode = scriba_BuildCode;
  pEo->pST->scriba_IsFileBinaryFormat = scriba_IsFileBinaryFormat;
  pEo->pST->scriba_GetCacheFileName = scriba_GetCacheFileName;
  pEo->pST->scriba_UseCacheFile = scriba_UseCacheFile;
  pEo->pST->scriba_SaveCacheFile = scriba_SaveCacheFile;
  pEo->pST->scriba_RunExternalPreprocessor = scriba_RunExternalPreprocessor;
  pEo->pST->scriba_SaveCode = scriba_SaveCode;
  pEo->pST->scriba_SaveCCode = scriba_SaveCCode;
  pEo->pST->scriba_LoadSourceProgram = scriba_LoadSourceProgram;
  pEo->pST->scriba_Run = scriba_Run;
  pEo->pST->scriba_NoRun = scriba_NoRun;
  pEo->pST->scriba_ResetVariables = scriba_ResetVariables;
  pEo->pST->scriba_Call = scriba_Call;
  pEo->pST->scriba_CallArg = scriba_CallArg;
  pEo->pST->scriba_DestroySbArgs = scriba_DestroySbArgs;
  pEo->pST->scriba_NewSbArgs = scriba_NewSbArgs;
  pEo->pST->scriba_CallArgEx = scriba_CallArgEx;
  pEo->pST->scriba_LookupFunctionByName = scriba_LookupFunctionByName;
  pEo->pST->scriba_LookupVariableByName = scriba_LookupVariableByName;
  pEo->pST->scriba_GetVariableType = scriba_GetVariableType;
  pEo->pST->scriba_GetVariable = scriba_GetVariable;
  pEo->pST->scriba_SetVariable = scriba_SetVariable;

  pEo->pST->log_state = log_state;
  pEo->pST->log_init = log_init;
  pEo->pST->log_printf = log_printf;
  pEo->pST->log_shutdown = log_shutdown;

  pEo->pST->handle_GetHandle = handle_GetHandle;
  pEo->pST->handle_GetPointer = handle_GetPointer;
  pEo->pST->handle_FreeHandle = handle_FreeHandle;
  pEo->pST->handle_DestroyHandleArray = handle_DestroyHandleArray;
  pEo->pST->basext_GetArgsF = basext_GetArgsF;

  pEo->pST->pEo                 = pEo;

  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H modu_Preload
@c Preload the modules configured in the configuration file

/*FUNCTION*/
int modu_Preload(pExecuteObject pEo
  ){
/*noverbatim
CUT*/
  char *s;
  int iErrorCode;
  CFT_NODE Node;

  for( Node = 0;  
       ! cft_GetEx(pEo->pConfig,"preload",&Node,&s,NULL,NULL,NULL) ; 
       Node = cft_EnumNext(pEo->pConfig,Node) ){
    if( (! strcmp(cft_GetKey(pEo->pConfig,Node),"preload") ) &&
          (iErrorCode = modu_LoadModule(pEo,s,NULL)) )return iErrorCode;
    }
  return COMMAND_ERROR_SUCCESS;
  }

/* This global variable should be defined in the file generated by the program makemoduletable.pl */
extern MODLIST StaticallyLinkedModules[];

/*POD
=H modu_GetModuleFunctionByName
@c Get a function entry point from a module

This function gets the entrypoint of a module function. This module can either
be statically or dynamically linked to ScriptBasic. This function is one level higher than
R<GetStaticFunctionByName> or 
R<dynlolib_GetFunctionByName>. The first argument to this function
is not the module handle as returned by R<dynlolib_LoadLibrary> but rather the pointer to the
module description structure that holds other information on the modula. Namely the information
that the module is loaded from dll or so, or if the module is linked to the interpreter static.

/*FUNCTION*/
void *modu_GetModuleFunctionByName(
  pModule pThisModule,
  char *pszFunctionName
  ){
/*noverbatim
CUT*/

  if( pThisModule->ModuleIsStatic )
    return modu_GetStaticFunctionByName((void *)pThisModule->ModulePointer,pszFunctionName);
  else
    return dynlolib_GetFunctionByName((void *)pThisModule->ModulePointer,pszFunctionName);
  }


/*POD
=H modu_GetStaticFunctionByName
@c Get a function entry point from a statically linked library

Get the entry point of a function that was linked to the ScriptBasic environment statically.

This is the counterpart of the function T<dynlolib_GetFunctionByName> for functions in library
linked static. This function searches the T<SLFST> table for the named function and returns the
entry point or T<NULL> if there is no functions with the given name defined.

/*FUNCTION*/
void *modu_GetStaticFunctionByName(
  void *pLibrary,
  char *pszFunctionName
  ){
/*noverbatim
CUT*/
  PSLFST pM = (PSLFST)pLibrary;

  while( pM->name ){
    if( !strcmp(pM->name,pszFunctionName) )return pM->function;
    pM++;
    }
  return NULL;
  }

/*POD
=H modu_LoadModule
@c Load a module

This function loads a module and returns the module pointer to in the argument
T<pThisModule>. If the module is already loaded it just returns the module
pointer.

When the function is called first time for a module it loads the module, calls
the version negotiation function and the module initializer.

If module file name given in the argument T<pszLibrary> file name is an absolute
file name this is used as it is. Otherwise the different configured module
directories are seached for the module file, and the operating system specific
extension is also appended to the file name automatically.

If the caller does not need the pointer to the module the argument T<pThisModule>
can be T<NULL>.
/*FUNCTION*/
int modu_LoadModule(pExecuteObject pEo,
                    char *pszLibraryFile,
                    pModule **pThisModule
  ){
/*noverbatim
CUT*/
#define FNLEN 1024 /* this is the maximal length of a module name with absoule path */
  char szBuffer[FNLEN],*s;
#ifdef WIN32
  char sData[FNLEN]; /* used to store the full path name to the executable if no module dir is configured */
  DWORD i;
#endif
  int (*ModuleInitializerFunction)(int, char *, void **);
  int (*ExternalFunction)(pSupportTable, void **, pFixSizeMemoryObject, pFixSizeMemoryObject *);
  int ModuleRequestedVersion;
  pModule *ThisModule;
  void *FunctionPointer;
  int iResult;
  CFT_NODE Node;
  char *pszDllExtension;
  unsigned int cbDllExtension;
  int j;

  pszDllExtension = cft_GetString(pEo->pConfig,"dll");
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

  /* Check if this module was already loaded. */
  ThisModule = &(pEo->modules);
  while( *ThisModule && strcmp((*ThisModule)->pszModuleName,pszLibraryFile) )
    ThisModule = &( (*ThisModule)->next );

  if( pThisModule )
    *pThisModule = ThisModule;
  if( *ThisModule )return COMMAND_ERROR_SUCCESS;

  /* Initialize the function pointer table for the call-back functions if it was not
     initialized yet        ( 0 means no force) */
  if( iResult = modu_Init(pEo,0) )return iResult;

  /* store the module information in the module list */
  *ThisModule = ALLOC(sizeof(Module));
  if( *ThisModule == NULL )return COMMAND_ERROR_MEMORY_LOW;
  (*ThisModule)->pszModuleName = ALLOC( strlen(pszLibraryFile)+1);
  if( (*ThisModule)->pszModuleName == NULL )return COMMAND_ERROR_MEMORY_LOW;
  strcpy((*ThisModule)->pszModuleName,pszLibraryFile);
  (*ThisModule)->ModulePointer = NULL;
  (*ThisModule)->next = NULL;
  (*ThisModule)->ModuleInternalParameters = NULL;
  (*ThisModule)->ModuleIsStatic = 0; /* by default the code in most cases assumes that the modules are
                                        not statically linked to the interpreter */

  /* load the module into process space */
  s = (*ThisModule)->pszModuleName;

#ifdef __MACOS__
  /* On Mac, an absolute path begins with no colon, and has colons in it.
     We treat as a relative path anything starting with a colon or having no colons at all */
  if( *s != ':'  && strchr(s, ':')){
#else
  if( *s == '/' ||
      *s == '\\' ||/* If the user is perverted enough to name a file C:/ under UNIX then he deserves the consequences */
      ( s[1] == ':' && (s[2] == '\\' || s[2] == '/') )         ){
#endif
    /* this is absolute path, do not prepend or append anything */
    (*ThisModule)->ModulePointer = dynlolib_LoadLibrary( s );
    if( (*ThisModule)->ModulePointer == NULL )return COMMAND_ERROR_MODULE_LOAD;

    }else{
    /* if the module name is not given as absolute path then check if this is a statically linked modules */
    j = 0;
    while( StaticallyLinkedModules[j].name ){
      if( ! strcmp( StaticallyLinkedModules[j].name , s ) ){
        /* This is a statically linked module. */
        (*ThisModule)->ModulePointer  = StaticallyLinkedModules[j].table;
        (*ThisModule)->ModuleIsStatic = 1;
        break;
        }
      j++;
      }

    if( (*ThisModule)->ModulePointer == NULL ){
      /* relative file name, prepend ModulePath */
      if( ! cft_GetEx(pEo->pConfig,"module",&Node,&s,NULL,NULL,NULL) ){
        while( 1 ){
          if( cft_GetEx(pEo->pConfig,NULL,&Node,&s,NULL,NULL,NULL) ){
            /* if there are no more directories in the configuration */
            break;
            }
          if( ! strcmp(cft_GetKey(pEo->pConfig,Node),"module") ){
            if( strlen(s) + strlen((*ThisModule)->pszModuleName) > FNLEN )return COMMAND_ERROR_MODULE_LOAD;
            strcpy(szBuffer,s);
            strcat(szBuffer,(*ThisModule)->pszModuleName);
            if( strlen(szBuffer) + cbDllExtension > FNLEN )return COMMAND_ERROR_MODULE_LOAD;
            strcat(szBuffer,pszDllExtension);
            (*ThisModule)->ModulePointer = dynlolib_LoadLibrary( szBuffer );
            if( (*ThisModule)->ModulePointer != NULL )break;
            }
          Node = cft_EnumNext(pEo->pConfig,Node);
          }
        }
      }
#ifdef WIN32
    while( (*ThisModule)->ModulePointer == NULL ){
      /* On Windows as a last resort try to find the module file in the same directory as the executable is
         or as the very last resort in the directory  scribapath/bin/../modules 
         This will ease the installation process for the simple users, who do
         not dare to edit the registry.
      */
      s = _pgmptr;
      if( strlen(s) > FNLEN )break;
      strcpy(szBuffer,s);
      s = szBuffer;
      while( *s && ! isspace(*s) )s++;
      *s = (char)0;
      i = GetFullPathName(szBuffer,
                          FNLEN,
                          sData,
                          &s);
      *s = (char)0;
      if( strlen(sData) + strlen((*ThisModule)->pszModuleName) + cbDllExtension > FNLEN )break;
      strcpy(s,(*ThisModule)->pszModuleName);
      strcat(s,pszDllExtension);
      (*ThisModule)->ModulePointer = dynlolib_LoadLibrary( sData );
      if( (*ThisModule)->ModulePointer != NULL )break;
      /* c:\ScriptBasic\bin\scriba.exe */
      /*                 s- ^          */
      s--; /* step back to the \\*/
      if( s <= sData )break;
      s--; /*step back before the \\ */
      while( s > sData ){
        if( *s == '\\' || *s == '/' )break;
        s--;
        }
      if( s <= sData )break;
      s++; /* step after c:\ScriptBasic\ */
      *s = (char)0;
      if( strlen(sData) + 8 + strlen((*ThisModule)->pszModuleName) + cbDllExtension > FNLEN )break;
      strcpy(s,"modules\\");
      strcpy(s+8,(*ThisModule)->pszModuleName);
      strcat(s,pszDllExtension);
      (*ThisModule)->ModulePointer = dynlolib_LoadLibrary( sData );
      if( (*ThisModule)->ModulePointer != NULL )break;
    break;
    }
#endif
#ifdef __MACOS__
    if( (*ThisModule)->ModulePointer == NULL ){ /* Let MacOS see if it can find it */
      strcpy(s,(*ThisModule)->pszModuleName);
      (*ThisModule)->ModulePointer = dynlolib_LoadLibrary( s );
      }
#endif
    if( (*ThisModule)->ModulePointer == NULL )return COMMAND_ERROR_MODULE_LOAD;

    }/*'end if' when the module name is not full path */

  /* call the module version negotiate function */
  FunctionPointer = modu_GetModuleFunctionByName(*ThisModule,MODULE_VERSIONER);
  if( FunctionPointer ){
    ModuleInitializerFunction = FunctionPointer;
    (*ThisModule)->ModuleIsActive = 1;
    ModuleRequestedVersion = ModuleInitializerFunction((int)INTERFACE_VERSION,(signed char *)pEo->Ver.Variation,&((*ThisModule)->ModuleInternalParameters));
    (*ThisModule)->ModuleIsActive = 0;
    if( ModuleRequestedVersion == 0 )return COMMAND_ERROR_MODULE_INITIALIZE;
    if( ModuleRequestedVersion != INTERFACE_VERSION ){
      if( GlobalDebugDisplayFlag ){
        fprintf(stderr,"The module requests the interface version %d\n"
                       "The interpreter supports the interface version %d\n",
                       ModuleRequestedVersion , INTERFACE_VERSION);
        }
      return COMMAND_ERROR_MODULE_VERSION;
      }
    }else{
    ModuleRequestedVersion = INTERFACE_VERSION; /* and hope the best */
    }

  /* call the module initializer function */
  FunctionPointer = modu_GetModuleFunctionByName(*ThisModule,MODULE_INITIALIZER);
  if( FunctionPointer ){
    ExternalFunction = FunctionPointer;
    (*ThisModule)->ModuleIsActive = 1;
    iResult = ExternalFunction(pEo->pST,&((*ThisModule)->ModuleInternalParameters),NULL,NULL);
    (*ThisModule)->ModuleIsActive = 0;
    if( iResult != 0 )return iResult;
    }
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H modu_GetFunctionByName

This function can be called to get the entry point of a function from an external module.
If the module was not loaded yet it is automatically loaded.

/*FUNCTION*/
int modu_GetFunctionByName(pExecuteObject pEo,
                           char *pszLibraryFile,
                           char *pszFunctionName,
                           void **ppFunction,
                           pModule **pThisModule
  ){
/*noverbatim
CUT*/
  int iResult;
  pModule *pMyThisModule;
  void *(*ModuleAutoloaderFunction)(pSupportTable, void **, char *, void **);

  if( pThisModule == NULL )pThisModule = &pMyThisModule;
  if( iResult = modu_Init(pEo,0) )return iResult;
  if( iResult = modu_LoadModule(pEo,pszLibraryFile,pThisModule) )return iResult;
  *ppFunction = modu_GetModuleFunctionByName(**pThisModule,pszFunctionName);

  /* if there is no such function exported by the module try to call the autoloader */
  if( *ppFunction == NULL &&
      (ModuleAutoloaderFunction = modu_GetModuleFunctionByName(**pThisModule,MODULE_AUTOLOADER) ) )
    ModuleAutoloaderFunction(pEo->pST,&((**pThisModule)->ModuleInternalParameters),pszFunctionName,ppFunction);
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H modu_UnloadAllModules
@c Unload all loaded modules

This function unloads all modules. This is called via the command finalizer mechanizm. If ever any module
was loaded via a "declare sub" statement the command execution sets the command finalizer function
pointer to point to this function.
/*FUNCTION*/
int modu_UnloadAllModules(pExecuteObject pEo
  ){
/*noverbatim

In a multi-threaded environment this function calls the keeper function of the module and in case the
keeper returns 1 the module is kept in memory, though the module finalizer function is called. This
lets multi-thread external modules to keep themselfs in memory even those times when there is not any
interpreter thread using the very module running.

In that case the module is put on the module list of the process SB object. That list is used to shut down
the modules when the whole process is shut down.

If there is no process SB object (pEo->pEPo is NULL) then the variation is a single process single thread
implementation of ScriptBasic. In this case this function first calls the module finalizer function
that is usally called in multi-threaded environment every time an interpreter thread is about to finish and
after this the module shutdown function is called, which is called in a multi-thread environment when
the whole process is to be shut down. After that the module is unloaded even if the keeper function said
that the module wants to stay in memory.

Don't worry about this: it is not abuse. The keeper function saying 1 means that the module has to 
stay in memory after the actual interpreter thread has finished until the process finishes. However
in this very case the process also terminates.

B<Note:> A one-process one-thread implementation may also behave like a multi thread implementation
allocating a separate process SB object and a program object to run. Then it should inherit the
support table and the execution object of the process SB object to the runnable program object. After
running finish the runned program object and call the shutdown process for the process SB object.
But that is tricky for a single thread implementation.
CUT*/
  pModule ThisModule,*pThisModule,pMptr;
  void *FunctionPointer;
  int (*ExternalFunction)(pSupportTable, void **, pFixSizeMemoryObject, pFixSizeMemoryObject *);
  int iActiveModules;
  int (*KeeperFunction)(void);

  iActiveModules = 0;
  /* Check if this module was already loaded. */
  pThisModule = &(pEo->modules);
  while( *pThisModule ){
    /* call the module finalizer function */
    if( (*pThisModule)->ModulePointer ){/* avoid modules not loaded by error */
      if( (*pThisModule)->ModuleIsActive ){/* can not unload a module if it is active */
        iActiveModules ++;/* count the not unloaded modules that are active */
        pThisModule = &((*pThisModule)->next);
        }else{
        FunctionPointer = modu_GetModuleFunctionByName(*pThisModule,MODULE_FINALIZER);
        if( FunctionPointer ){
          ExternalFunction = FunctionPointer;
          ExternalFunction(pEo->pST,&((*pThisModule)->ModuleInternalParameters),NULL,NULL);
          }
        /* decide calling the keeper function whether we should call unload or not in multi-thread
           environment */
        FunctionPointer = modu_GetModuleFunctionByName(*pThisModule,MODULE_KEEPER);
        if( FunctionPointer ){
          KeeperFunction = FunctionPointer;
          if( KeeperFunction() && ! (*pThisModule)->ModuleIsStatic )
            dynlolib_FreeLibrary((*pThisModule)->ModulePointer);
          else if( pEo->pEPo ){
            thread_LockMutex( &(pEo->pEPo->mxModules) );
            pMptr = alloc_Alloc(sizeof(Module),pEo->pEPo->pMemorySegment);
            if( pMptr == NULL ){
              thread_UnlockMutex( &(pEo->pEPo->mxModules) );
              return COMMAND_ERROR_MEMORY_LOW;
              }
            memcpy(pMptr,*pThisModule,sizeof(Module));
            pMptr->next = pEo->pEPo->modules;
            pEo->pEPo->modules = pMptr;
            thread_UnlockMutex( &(pEo->pEPo->mxModules) );
            }else{
            /* the module wants to stay in memory, but the variation
               runs single thread and there is no process level object */
            if( ! modu_ShutdownModule(pEo,*pThisModule) && ! (*pThisModule)->ModuleIsStatic )
              dynlolib_FreeLibrary((*pThisModule)->ModulePointer);
            (*pThisModule)->ModulePointer = NULL;
            }
          }else{
          if( ! (*pThisModule)->ModuleIsStatic )
            dynlolib_FreeLibrary((*pThisModule)->ModulePointer);
          (*pThisModule)->ModulePointer = NULL;
          }
        ThisModule = *pThisModule;
        *pThisModule = (*pThisModule)->next;
        FREE(ThisModule->pszModuleName);
        FREE(ThisModule);
        }
      }else{
      /* if the module was not loaded by error, release the space occupied by the error indicating rec */
      ThisModule = *pThisModule;
      *pThisModule = (*pThisModule)->next;
      FREE(ThisModule->pszModuleName);
      FREE(ThisModule);
      }
    }
  if( iActiveModules )return COMMAND_ERROR_PARTIAL_UNLOAD;
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H modu_UnloadModule
@c Unload the named module

This function unloads the named module. Note that this function is not
called unless some extension module calls it to unload another module.

Currently there is no support for a module to unload itself.

/*FUNCTION*/
int modu_UnloadModule(pExecuteObject pEo,
                      char *pszLibraryFile
  ){
/*noverbatim
CUT*/
  pModule *ThisModule,pMptr;
  void *FunctionPointer;
  int (*ExternalFunction)(pSupportTable, void **, pFixSizeMemoryObject, pFixSizeMemoryObject *);
  int (*KeeperFunction)(void);

  ThisModule = &(pEo->modules);
  while( *ThisModule && strcmp((*ThisModule)->pszModuleName,pszLibraryFile) )
    ThisModule = &( (*ThisModule)->next );

  if( ! *ThisModule )return COMMAND_ERROR_MODULE_NOT_LOADED;
  if( (*ThisModule)->ModuleIsActive )return COMMAND_ERROR_MODULE_ACTIVE;
  if( *ThisModule ){
    if( (*ThisModule)->ModulePointer ){/* avoid modules not loaded by error */
      FunctionPointer = modu_GetModuleFunctionByName(*ThisModule,MODULE_FINALIZER);
      if( FunctionPointer ){
        ExternalFunction = FunctionPointer;
        ExternalFunction(pEo->pST,&((*ThisModule)->ModuleInternalParameters),NULL,NULL);
        }
      /* decide calling the keeper function whether we should call unload or not in multi-thread
         environment */
      FunctionPointer = modu_GetModuleFunctionByName(*ThisModule,MODULE_KEEPER);
      if( FunctionPointer ){
        KeeperFunction = FunctionPointer;
        if( KeeperFunction() && ! (*ThisModule)->ModuleIsStatic )
          dynlolib_FreeLibrary((*ThisModule)->ModulePointer);
        else if( pEo->pEPo ){
          thread_LockMutex( &(pEo->pEPo->mxModules) );
          pMptr = alloc_Alloc(sizeof(Module),pEo->pEPo->pMemorySegment);
          if( pMptr == NULL ){
            thread_UnlockMutex( &(pEo->pEPo->mxModules) );
            return COMMAND_ERROR_MEMORY_LOW;
            }
          memcpy(pMptr,*ThisModule,sizeof(Module));
          pMptr->next = pEo->pEPo->modules;
          pEo->pEPo->modules = pMptr;
          thread_UnlockMutex( &(pEo->pEPo->mxModules) );
          }else{
          /* the module wants to stay in memory, but the variation
             runs single thread and there is no process level object */
          if( ! modu_ShutdownModule(pEo,*ThisModule) && ! (*ThisModule)->ModuleIsStatic )
            dynlolib_FreeLibrary((*ThisModule)->ModulePointer);
          (*ThisModule)->ModulePointer = NULL;
          }
        }else{
        if( ! (*ThisModule)->ModuleIsStatic )
          dynlolib_FreeLibrary((*ThisModule)->ModulePointer);
        }
      }
    *ThisModule = (*ThisModule)->next ;
    }
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H modu_ShutdownModule
@c Shut down a module

This function calls the shutdown function of a module.

If the shutdown function performs well and returns SUCCESS this function
also returns success. If the shutdown function returns error code
it means that the module has running thread and thus can not be unloaded.

/*FUNCTION*/
int modu_ShutdownModule(pExecuteObject pEo,
                        pModule pThisModule
  ){
/*noverbatim
CUT*/
  void *FunctionPointer;
  int (*ShutdownFunction)(pSupportTable, void **, pFixSizeMemoryObject, pFixSizeMemoryObject *);
  int iError;

  FunctionPointer = modu_GetModuleFunctionByName(pThisModule,MODULE_SHUTDOWN);
  if( FunctionPointer != NULL ){
    ShutdownFunction = FunctionPointer;
    /* shutdown function gets support table, but no internal data, parameters or return value */
    iError = ShutdownFunction(pEo->pST,NULL,NULL,NULL);
    return iError;
    }
  return COMMAND_ERROR_SUCCESS;
  }
