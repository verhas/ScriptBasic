/* hookers.c

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

#include "filesys.h"

typedef struct _ExecuteObject *pExecuteObject;
#define PEXECUTEOBJECT 1

typedef struct _HookFunctions {

  void *hook_pointer;//this can freely be used by a module that alters some of the hook functions

#define HOOK_FILE_ACCESS(X) (pEo->pHookers->HOOK_file_access(pEo,(X)))
  int (*HOOK_file_access)(pExecuteObject, char *);

#define HOOK_FOPEN(X,Y) (pEo->pHookers->HOOK_fopen(pEo,(X),(Y)))
  FILE *(*HOOK_fopen)(pExecuteObject, char *, char *);

#define HOOK_FCLOSE(X) (pEo->pHookers->HOOK_fclose(pEo,(X)))
  void (*HOOK_fclose)(pExecuteObject, FILE *);

#define HOOK_SIZE(X) (pEo->pHookers->HOOK_size(pEo,(X)))
  long (*HOOK_size)(pExecuteObject, char *);

#define HOOK_TIME_ACCESSED(X) (pEo->pHookers->HOOK_time_accessed(pEo,(X)))
  long (*HOOK_time_accessed)(pExecuteObject, char *);

#define HOOK_TIME_MODIFIED(X) (pEo->pHookers->HOOK_time_modified(pEo,(X)))
  long (*HOOK_time_modified)(pExecuteObject, char *);

#define HOOK_TIME_CREATED(X) (pEo->pHookers->HOOK_time_created(pEo,(X)))
  long (*HOOK_time_created)(pExecuteObject, char *);

#define HOOK_ISDIR(X) (pEo->pHookers->HOOK_isdir(pEo,(X)))
  int (*HOOK_isdir)(pExecuteObject, char *);

#define HOOK_ISREG(X) (pEo->pHookers->HOOK_isreg(pEo,(X)))
  int (*HOOK_isreg)(pExecuteObject, char *);

#define HOOK_EXISTS(X) (pEo->pHookers->HOOK_exists(pEo,(X)))
  int (*HOOK_exists)(pExecuteObject, char *);

#define HOOK_TRUNCATE(X,Y) (pEo->pHookers->HOOK_truncate(pEo,(X),(Y)))
  int (*HOOK_truncate)(pExecuteObject, FILE *, long);

#define HOOK_FGETC(X) (pEo->pHookers->HOOK_fgetc(pEo,(X)))
  int (*HOOK_fgetc)(pExecuteObject, FILE *);

#define HOOK_FERROR(X) (pEo->pHookers->HOOK_ferror(pEo,(X)))
  int (*HOOK_ferror)(pExecuteObject, FILE *);

#define HOOK_FREAD(X,Y,Z,W) (pEo->pHookers->HOOK_fread(pEo,(X),(Y),(Z),(W)))
  int (*HOOK_fread)(pExecuteObject, char *, int, int, FILE *);

#define HOOK_SETMODE(X,Y) (pEo->pHookers->HOOK_setmode(pEo,(X),(Y)))
  void (*HOOK_setmode)(pExecuteObject,FILE *, int);

#define HOOK_BINMODE(X) (pEo->pHookers->HOOK_binmode(pEo,(X)))
  void (*HOOK_binmode)(pExecuteObject,FILE *);

#define HOOK_TEXTMODE(X) (pEo->pHookers->HOOK_textmode(pEo,(X)))
  void (*HOOK_textmode)(pExecuteObject,FILE *);

#define HOOK_FWRITE(X,Y,Z,W) (pEo->pHookers->HOOK_fwrite(pEo,(X),(Y),(Z),(W)))
  int (*HOOK_fwrite)(pExecuteObject, char *, int, int, FILE *);

#define HOOK_PUTC(X,Y) (pEo->pHookers->HOOK_fputc(pEo,(X),(Y)))
  int (*HOOK_fputc)(pExecuteObject, int, FILE *);

#define HOOK_FLOCK(X,Y) (pEo->pHookers->HOOK_flock(pEo,(X),(Y)))
  int (*HOOK_flock)(pExecuteObject, FILE *, int);

#define HOOK_LOCK(X,Y,Z,W) (pEo->pHookers->HOOK_lock(pEo,(X),(Y),(Z),(W)))
  int (*HOOK_lock)(pExecuteObject, FILE *, int, long, long);

#define HOOK_FEOF(X) (pEo->pHookers->HOOK_feof(pEo,(X)))
  int (*HOOK_feof)(pExecuteObject, FILE *);

#define HOOK_MKDIR(X) (pEo->pHookers->HOOK_mkdir(pEo,(X)))
  int (*HOOK_mkdir)(pExecuteObject, char *);

#define HOOK_RMDIR(X) (pEo->pHookers->HOOK_rmdir(pEo,(X)))
  int (*HOOK_rmdir)(pExecuteObject, char *);

#define HOOK_REMOVE(X) (pEo->pHookers->HOOK_remove(pEo,(X)))
  int (*HOOK_remove)(pExecuteObject, char *);

#define HOOK_DELTREE(X) (pEo->pHookers->HOOK_deltree(pEo,(X)))
  int (*HOOK_deltree)(pExecuteObject, char *);

#define HOOK_MAKEDIRECTORY(X) (pEo->pHookers->HOOK_MakeDirectory(pEo,(X)))
  int (*HOOK_MakeDirectory)(pExecuteObject, char *);

#define HOOK_OPENDIR(X,Y) (pEo->pHookers->HOOK_opendir(pEo,(X),(Y)))
  DIR *(*HOOK_opendir)(pExecuteObject, char *, tDIR *);

#define HOOK_READDIR(X) (pEo->pHookers->HOOK_readdir(pEo,(X)))
  struct dirent *(*HOOK_readdir)(pExecuteObject, DIR *);

#define HOOK_CLOSEDIR(X) (pEo->pHookers->HOOK_closedir(pEo,(X)))
  void (*HOOK_closedir)(pExecuteObject, DIR *);

#define HOOK_SLEEP(X) (pEo->pHookers->HOOK_sleep(pEo,(X)))
  void (*HOOK_sleep)(pExecuteObject, long);

#define HOOK_CURDIR(X,Y) (pEo->pHookers->HOOK_curdir(pEo,(X),(Y)))
  int (*HOOK_curdir)(pExecuteObject, char *, unsigned long);

#define HOOK_CHDIR(X) (pEo->pHookers->HOOK_chdir(pEo,(X)))
  int (*HOOK_chdir)(pExecuteObject, char *);

#define HOOK_CHOWN(X,Y) (pEo->pHookers->HOOK_chown(pEo,(X),(Y)))
  int (*HOOK_chown)(pExecuteObject, char *, char *);

#define HOOK_SETCREATETIME(X,Y) (pEo->pHookers->HOOK_SetCreateTime(pEo,(X),(Y)))
  int (*HOOK_SetCreateTime)(pExecuteObject, char *, long);

#define HOOK_SETMODIFYTIME(X,Y) (pEo->pHookers->HOOK_SetModifyTime(pEo,(X),(Y)))
  int (*HOOK_SetModifyTime)(pExecuteObject, char *, long);

#define HOOK_SETACCESSTIME(X,Y) (pEo->pHookers->HOOK_SetAccessTime(pEo,(X),(Y)))
  int (*HOOK_SetAccessTime)(pExecuteObject, char *, long);

  int (*HOOK_ExecBefore)(pExecuteObject);
  int (*HOOK_ExecAfter)(pExecuteObject);
  int (*HOOK_ExecCall)(pExecuteObject);
  int (*HOOK_ExecReturn)(pExecuteObject);

#define HOOK_GETHOSTNAME(X,Y) (pEo->pHookers->HOOK_gethostname(pEo,(X),(Y)))
  int (*HOOK_gethostname)(pExecuteObject, char *, long);
#define HOOK_GETHOST(X,Y) (pEo->pHookers->HOOK_gethost(pEo,(X),(Y)))
  int (*HOOK_gethost)(pExecuteObject, char *, struct hostent *);
#define HOOK_TCPCONNECT(X,Y) (pEo->pHookers->HOOK_tcpconnect(pEo,(X),(Y)))
  int (*HOOK_tcpconnect)(pExecuteObject, SOCKET *, char *);
#define HOOK_TCPSEND(X,Y,Z,W) (pEo->pHookers->HOOK_tcpsend(pEo,(X),(Y),(Z),(W)))
  int (*HOOK_tcpsend)(pExecuteObject, SOCKET, char *, long, int);
#define HOOK_TCPRECV(X,Y,Z,W) (pEo->pHookers->HOOK_tcprecv(pEo,(X),(Y),(Z),(W)))
  int (*HOOK_tcprecv)(pExecuteObject, SOCKET, char *, long, int);
#define HOOK_TCPCLOSE(X) (pEo->pHookers->HOOK_tcpclose(pEo,(X)))
  int (*HOOK_tcpclose)(pExecuteObject, SOCKET);

  int (*HOOK_killproc)(pExecuteObject, long);
#define HOOK_KILLPROC(X) (pEo->pHookers->HOOK_killproc(pEo,(X)))
  int (*HOOK_getowner)(pExecuteObject, char *, char *, long);
#define HOOK_GETOWNER(X,Y,Z) (pEo->pHookers->HOOK_getowner(pEo,(X),(Y),(Z)))
  char *(*HOOK_fcrypt)(pExecuteObject, char *, char *, char *);
#define HOOK_FCRYPT(X,Y,Z) (pEo->pHookers->HOOK_fcrypt(pEo,(X),(Y),(Z)))
  long (*HOOK_CreateProcess)(pExecuteObject, char *);
#define HOOK_CREATEPROCESS(X) (pEo->pHookers->HOOK_CreateProcess(pEo,(X)))

  int (*HOOK_CallScribaFunction)(pExecuteObject,
                                 unsigned long,
                                 pFixSizeMemoryObject *,
                                 unsigned long,
                                 pFixSizeMemoryObject *);
#define HOOK_CALLSCRIBAFUNCTION(X,Y,Z,W) (pEo->pHookers->HOOK_CallScribaFunction(pEo,(X),(Y),(Z),(W)))

  long (*HOOK_CreateProcessEx)(pExecuteObject,
                              char *,
                              long,
                              unsigned long *,
                              unsigned long *);
#define HOOK_CREATEPROCESSEX(P,TO,PID,EXIT) (pEo->pHookers->HOOK_CreateProcessEx(pEo,(P),(TO),(PID),(EXIT)))

  int (*HOOK_waitpid)(pExecuteObject, long, unsigned long *);
#define HOOK_WAITPID(X,Y) (pEo->pHookers->HOOK_waitpid(pEo,(X),(Y)))
  } HookFunctions
#ifndef PHOOKFUNCTIONS  
  , *pHookFunctions
#endif
  ;

*/
#include <stdio.h>

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
#include "hookers.h"

/*POD

This file contains the hook functions that are called by the commands whenever
a command wants to access the operating system functions. The hook functions
implemented here are transparent, they call the operating system. However these hook
functions are called via the HookFunctions function pointer table and external modules
may alter this table supplying their own hook functions.

There are some hook functions, which do not exist by default. In this case the hook functions table
points to T<NULL>. These functions, if defined are called by ScriptBasic at certain points of execution.
For example the function T<HOOK_ExecBefore> is called each time before executing a command in case
an external module defines the function altering the hook function table.

The hook functions have the same arguments as the original function preceeded by the
pointer to the execution object T<pExecuteObject pEo>. For example the function T<fopen> has two arguments
to T<char *>, and therefore HOOK_fopen has three. The first should point to T<pEo> and the second and third should
point to 

CUT*/

/*POD
=H hook_Init

This function allocates a hook function table and fills the
function pointers to point to the original transparent hook functions.

/*FUNCTION*/
int hook_Init(pExecuteObject pEo,
              pHookFunctions *pHookers
  ){
/*noverbatim
CUT*/
  *pHookers = alloc_Alloc(sizeof(HookFunctions),pEo->pMemorySegment);
  if( *pHookers == NULL )return COMMAND_ERROR_MEMORY_LOW;
#define SET(x) (*pHookers)->HOOK_##x = hook_##x

  (*pHookers)->hook_pointer = NULL;

  SET(file_access);

  SET(fopen);
  SET(fclose);
  SET(size);
  SET(time_accessed);
  SET(time_modified);
  SET(time_created);
  SET(isdir);
  SET(isreg);
  SET(exists);
  SET(truncate);
  SET(fgetc);
  SET(ferror);
  SET(fread);
  SET(setmode);
  SET(binmode);
  SET(textmode);
  SET(fwrite);
  SET(fputc);
  SET(flock);
  SET(lock);
  SET(feof);
  SET(mkdir);
  SET(rmdir);
  SET(remove);
  SET(deltree);
  SET(MakeDirectory);
  SET(opendir);
  SET(readdir);
  SET(closedir);
  SET(sleep);
  SET(curdir);
  SET(chdir);
  SET(chown);
  SET(SetCreateTime);
  SET(SetModifyTime);
  SET(SetAccessTime);
  SET(gethostname);
  SET(gethost);
  SET(tcpconnect);
  SET(tcpsend);
  SET(tcprecv);
  SET(tcpclose);
  SET(killproc);
  SET(getowner);
  SET(fcrypt);
  SET(CreateProcess);
  SET(CreateProcessEx);
  SET(CallScribaFunction);
  SET(waitpid);

  (*pHookers)->HOOK_ExecBefore = NULL; /* called before executing a command       */
  (*pHookers)->HOOK_ExecAfter  = NULL; /* called after executing a command        */
  (*pHookers)->HOOK_ExecCall   = NULL; /* executed when calling a function        */
  (*pHookers)->HOOK_ExecReturn = NULL; /* executed when returning from a function */

  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H hook_file_access
@c  file_access

This function gets a file name as an argument and return an integer code that tells
the caller if the program is allowed to read, write or both read and write to the file.
The default implementation just dumbly answers that the program is allowed both read
and write. This function is called by each other hook functions that access a file via the
file name. If a module wants to restrict the basic code to access files based on the file
name the module does not need to alter all hook functions that access files via file name.

The module has to write its own T<file_access> hook function instead, alter the hook function table
to point to the module's function and all file accessing functions will ask the module's
hook function if the code may access the file.

The argument T<pszFileName> is the name of the file that the ScriptBasic program
want to do something. The actual T<file_access> hook function should decide if the
basic program is

=itemize
=item 0 not allowed to access the file
=item 1 allowed to read the file
=item 2 allowed to write the file (modify)
=item 3 allowed to read and write the file
=noitemize

The default implementation of this function just
allows the program to do anything. Any extension module may
have its own implementation and restrict the basic program to
certain files.

/*FUNCTION*/
int hook_file_access(pExecuteObject pEo,
                     char *pszFileName
  ){
/*noverbatim
CUT*/
  return 3;
  }

/*POD
=H hook_fopen
@c  fopen
/*FUNCTION*/
FILE *hook_fopen(pExecuteObject pEo,
                 char *pszFileName,
                 char *pszOpenMode
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);

  if( iAccessPermission == 0 )return NULL;
  if( (iAccessPermission&1 && *pszOpenMode == 'r') ||
      (iAccessPermission&2 && ( *pszOpenMode == 'w' || *pszOpenMode == 'a' )) )
    return file_fopen(pszFileName,pszOpenMode);
  return NULL;
  }

/*POD
=H hook_fclose
@c  fclose
/*FUNCTION*/
void hook_fclose(pExecuteObject pEo,
                  FILE *fp
  ){
/*noverbatim
CUT*/
  file_fclose(fp);
  }

/*POD
=H hook_size
@c  size
/*FUNCTION*/
long hook_size(pExecuteObject pEo,
               char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return -1;
  return file_size(pszFileName);
  }

/*POD
=H hook_time_accessed
@c  time_accessed

/*FUNCTION*/
long hook_time_accessed(pExecuteObject pEo,
                        char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_time_accessed(pszFileName);
  }

/*POD
=H hook_time_modified
@c  time_modified

/*FUNCTION*/
long hook_time_modified(pExecuteObject pEo,
                        char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_time_modified(pszFileName);
  }

/*POD
=H hook_time_created
@c  time_created

/*FUNCTION*/
long hook_time_created(pExecuteObject pEo,
                        char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_time_created(pszFileName);
  }

/*POD
=H hook_isdir
@c  isdir

/*FUNCTION*/
int hook_isdir(pExecuteObject pEo,
               char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_isdir(pszFileName);
  }

/*POD
=H hook_isreg
@c  isreg

/*FUNCTION*/
int hook_isreg(pExecuteObject pEo,
               char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_isreg(pszFileName);
  }

/*POD
=H hook_fileexists
@c  fileexists

/*FUNCTION*/
int hook_exists(pExecuteObject pEo,
                char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return 0;

  return file_exists(pszFileName);
  }

/*POD
=H hook_truncate
@c  truncate
/*FUNCTION*/
int hook_truncate(pExecuteObject pEo,
                  FILE *fp,
                  long lNewFileSize
  ){
/*noverbatim
CUT*/
  return file_truncate(fp,lNewFileSize);
  }

/*POD
=H hook_fgetc
@c  fgetc
/*FUNCTION*/
int hook_fgetc(pExecuteObject pEo,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return file_fgetc(fp);
  }

/*POD
=H hook_ferror
@c  ferror
/*FUNCTION*/
int hook_ferror(pExecuteObject pEo,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return file_ferror(fp);
  }

/*POD
=H hook_fread
@c  fread
/*FUNCTION*/
int hook_fread(pExecuteObject pEo,
               char *buf,
               int size,
               int count,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return file_fread(buf,size,count,fp);
  }

/*POD
=H hook_setmode
@c  Set the mode of a file stream to binary or to ASCII

/*FUNCTION*/
void hook_setmode(pExecuteObject pEo,
                  FILE *fp,
                  int mode
  ){
/*noverbatim
CUT*/
  file_setmode(fp,mode);
  return;
  }

/*POD
=H hook_binmode
@c  Set a file stream to binary mode
/*FUNCTION*/
void hook_binmode(pExecuteObject pEo,
                  FILE *fp
  ){
/*noverbatim
CUT*/
  file_binmode(fp);
  return;
  }

/*POD
=H hook_textmode
@c  Set a file stream to text mode
/*FUNCTION*/
void hook_textmode(pExecuteObject pEo,
                   FILE *fp
  ){
/*noverbatim
CUT*/
  file_textmode(fp);
  return;
  }

/*POD
=H hook_fwrite
@c  fwrite
/*FUNCTION*/
int hook_fwrite(pExecuteObject pEo,
               char *buf,
               int size,
               int count,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return file_fwrite(buf,size,count,fp);
  }

/*POD
=H hook_fputc
@c  fputc

/*FUNCTION*/
int hook_fputc(pExecuteObject pEo,
               int c,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return file_fputc(c,fp);
  }

/*POD
=H hook_flock
@c  flock

/*FUNCTION*/
int hook_flock(pExecuteObject pEo,
               FILE *fp,
               int iLockType
  ){
/*noverbatim
CUT*/
  return file_flock(fp,iLockType);
  }

/*POD
=H hook_lock
@c  lock

/*FUNCTION*/
int hook_lock(pExecuteObject pEo,
              FILE *fp,
              int iLockType,
              long lStart,
              long lLength
  ){
  return file_lock(fp,iLockType,lStart,lLength);
/*noverbatim
CUT*/
  }

/*POD
=H hook_feof
@c  feof

/*FUNCTION*/
int hook_feof(pExecuteObject pEo,
              FILE *fp
  ){
/*noverbatim
CUT*/
  return file_feof(fp);
  }

/*POD
=H hook_mkdir
@c  mkdir

/*FUNCTION*/
int hook_mkdir(pExecuteObject pEo,
               char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  return file_mkdir(pszDirectoryName);
  }

/*POD
=H hook_rmdir
@c  rmdir

/*FUNCTION*/
int hook_rmdir(pExecuteObject pEo,
               char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  return file_rmdir(pszDirectoryName);
  }

/*POD
=H hook_remove
@c  remove

/*FUNCTION*/
int hook_remove(pExecuteObject pEo,
                char *pszFileName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return -1;

  return file_remove(pszFileName);
  }

/*POD
=H hook_deltree
@c  deltree

/*FUNCTION*/
int hook_deltree(pExecuteObject pEo,
                 char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszDirectoryName);
  if( !(iAccessPermission&1) )return -1;

  return file_deltree(pszDirectoryName);
  }

/*POD
=H hook_MakeDirectory
@c  MakeDirectory

/*FUNCTION*/
int hook_MakeDirectory(pExecuteObject pEo,
                       char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszDirectoryName);
  if( !(iAccessPermission&1) )return -1;

  return file_MakeDirectory(pszDirectoryName);
  }

/*POD
=H hook_opendir
@c  opendir

/*FUNCTION*/
DIR *hook_opendir(pExecuteObject pEo,
                  char *pszDirectoryName,
                  tDIR *pDirectory
  ){
/*noverbatim
CUT*/
  return file_opendir(pszDirectoryName,pDirectory);
  }

/*POD
=H hook_readdir
@c  readdir

/*FUNCTION*/
struct dirent *hook_readdir(pExecuteObject pEo,
                            DIR *pDirectory
  ){
/*noverbatim
CUT*/
  return file_readdir(pDirectory);
  }

/*POD
=H hook_closedir
@c  closedir

/*FUNCTION*/
void hook_closedir(pExecuteObject pEo,
                   DIR *pDirectory
  ){
/*noverbatim
CUT*/

  file_closedir(pDirectory);
  }

/*POD
=H hook_sleep
@c  sleep

/*FUNCTION*/
void hook_sleep(pExecuteObject pEo,
                long lSeconds
  ){
/*noverbatim
CUT*/
  sys_sleep(lSeconds);
  }

/*POD
=H hook_curdir
@c  curdir

/*FUNCTION*/
int hook_curdir(pExecuteObject pEo,
                char *Buffer,
                unsigned long cbBuffer
  ){
/*noverbatim
CUT*/
  return file_curdir(Buffer,cbBuffer);
  }

/*POD
=H hook_chdir
@c  chdir

/*FUNCTION*/
int hook_chdir(pExecuteObject pEo,
               char *Buffer
  ){
/*noverbatim
CUT*/
  return file_chdir(Buffer);
  }

/*POD
=H hook_chown
@c  chown

/*FUNCTION*/
int hook_chown(pExecuteObject pEo,
               char *pszFileName,
               char *pszOwner
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return COMMAND_ERROR_CHOWN_SET_OWNER;

  return file_chown(pszFileName,pszOwner);
  }

/*POD
=H hook_SetCreateTime
@c  SetCreateTime

/*FUNCTION*/
int hook_SetCreateTime(pExecuteObject pEo,
                       char *pszFileName,
                       long lTime
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return COMMAND_ERROR_CREATIME_FAIL;

  return file_SetCreateTime(pszFileName,lTime);
  }

/*POD
=H hook_SetModifyTime
@c  SetModifyTime

/*FUNCTION*/
int hook_SetModifyTime(pExecuteObject pEo,
                       char *pszFileName,
                       long lTime
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return COMMAND_ERROR_MODTIME_FAIL;

  return file_SetModifyTime(pszFileName,lTime);
  }

/*POD
=H hook_SetAccessTime
@c  SetAccessTime

/*FUNCTION*/
int hook_SetAccessTime(pExecuteObject pEo,
                       char *pszFileName,
                       long lTime
  ){
/*noverbatim
CUT*/
  int iAccessPermission = HOOK_FILE_ACCESS(pszFileName);
  if( !(iAccessPermission&1) )return COMMAND_ERROR_ACCTIM_FAIL;

  return file_SetAccessTime(pszFileName,lTime);
  }

/*POD
=H hook_gethostname
@c  gethostname

/*FUNCTION*/
int hook_gethostname(pExecuteObject pEo,
                     char *pszBuffer,
                     long cbBuffer
  ){
/*noverbatim
CUT*/
  return file_gethostname(pszBuffer,cbBuffer);
  }

/*POD
=H hook_gethost
@c  gethost
/*FUNCTION*/
int hook_gethost(pExecuteObject pEo,
                 char *pszBuffer,
                 struct hostent *pHost
  ){
/*noverbatim
CUT*/
  return file_gethost(pszBuffer,pHost);
  }

/*POD
=H hook_tcpconnect
@c  tcpconnect
/*FUNCTION*/
int hook_tcpconnect(pExecuteObject pEo,
                    SOCKET *sClient,
                    char *pszRemoteSocket
  ){
/*noverbatim
CUT*/
  return file_tcpconnect(sClient,pszRemoteSocket);
  }

/*POD
=H hook_tcpsend
@c  tcpsend
/*FUNCTION*/
int hook_tcpsend(pExecuteObject pEo,
                 SOCKET sClient,
                 char *pszBuffer,
                 long cbBuffer,
                 int iFlags
  ){
/*noverbatim
CUT*/
  return file_tcpsend(sClient,pszBuffer,cbBuffer,iFlags);
  }

/*POD
=H hook_tcprecv
@c  tcprecv
/*FUNCTION*/
int hook_tcprecv(pExecuteObject pEo,
                 SOCKET sClient,
                 char *pszBuffer,
                 long cbBuffer,
                 int iFlags
  ){
/*noverbatim
CUT*/
  return file_tcprecv(sClient,pszBuffer,cbBuffer,iFlags);
  }

/*POD
=H hook_tcpclose
@c  tcpclose
/*FUNCTION*/
int hook_tcpclose(pExecuteObject pEo,
                  SOCKET sClient
  ){
/*noverbatim
CUT*/
  return file_tcpclose(sClient);
  }

/*POD
=H hook_killproc
@c  killproc
/*FUNCTION*/
int hook_killproc(pExecuteObject pEo,
                  long pid
  ){
/*noverbatim
CUT*/
  return file_killproc(pid);
  }

/*POD
=H hook_getowner
@c  getowner
/*FUNCTION*/
int hook_getowner(pExecuteObject pEo,
                  char *pszFileName,
                  char *pszOwnerBuffer,
                  long cbOwnerBuffer
 ){
/*noverbatim
CUT*/
  return file_getowner(pszFileName,pszOwnerBuffer,cbOwnerBuffer);
  }

/*POD
=H hook_fcrypt
@c  fcrypt
/*FUNCTION*/
char *hook_fcrypt(pExecuteObject pEo,
                  char *buf,
                  char *salt,
                  char *buff
  ){
/*noverbatim
CUT*/
  return file_fcrypt(buf,salt,buff);
  }

/*POD
=H hook_CreateProcess
@c  CreateProcess
/*FUNCTION*/
long hook_CreateProcess(pExecuteObject pEo,
                         char *pszCommandLine
  ){
/*noverbatim
CUT*/
  return file_CreateProcess(pszCommandLine);
  }

/*POD
=H hook_CreateProcessEx
@c  CreateProcessEx
/*FUNCTION*/
long hook_CreateProcessEx(pExecuteObject pEo,
                          char *pszCommandLine,
                          long lTimeOut,
                          unsigned long *plPid,
                          unsigned long *plExitCode
  ){
/*noverbatim
CUT*/
  return file_CreateProcessEx(pszCommandLine,lTimeOut,plPid,plExitCode);
  }

/*POD
=H hook_waitpid
@c  waitpid
/*FUNCTION*/
int hook_waitpid(pExecuteObject pEo,
                 long pid,
                 unsigned long *plExitCode
  ){
/*noverbatim
CUT*/
  return file_waitpid(pid,plExitCode);
  }

/*POD
=H hook_CallScribaFunction
@c  Start to execute a scriba function

This is a hook function that performs its operation itself without
calling underlying T<file_> function. This function is called
by external modules whenever the external module wants to execute
certain ScriptBasic function.

The external module has to know the entry point of the ScriptBasic
function.

/*FUNCTION*/
int hook_CallScribaFunction(pExecuteObject pEo,
                            unsigned long lStartNode,
                            pFixSizeMemoryObject *pArgument,
                            unsigned long NumberOfPassedArguments,
                            pFixSizeMemoryObject *pFunctionResult
  ){
/*noverbatim
CUT*/

  int iError;
  unsigned long SaveProgramCounter,SaveStepCounter;
  unsigned long SavefErrorGoto,SaveErrorGoto,SaveErrorResume;
  pFixSizeMemoryObject SaveLocalVariablesPointer;
  pFixSizeMemoryObject SaveFunctionResultPointer;
  MortalList _ThisCommandMortals=NULL;
  pMortalList _pThisCommandMortals = &_ThisCommandMortals;
  unsigned long _ActualNode=pEo->ProgramCounter;
  int iErrorCode;
  NODE nItem;
  unsigned long i;
  unsigned long NumberOfArguments;
  long Opcode;


  SaveLocalVariablesPointer = pEo->LocalVariables;
  SaveProgramCounter = pEo->ProgramCounter;
  pEo->ProgramCounter = lStartNode;
  if( pEo->ProgramCounter == 0 )return EXE_ERROR_USERFUN_UNDEFINED;

  SaveFunctionResultPointer = pEo->pFunctionResult;
  pEo->pFunctionResult = NULL;
  SaveStepCounter = pEo->lStepCounter;
  pEo->lStepCounter = 0;
  SaveErrorGoto = pEo->ErrorGoto;
  pEo->ErrorGoto = 0;
  SaveErrorResume = pEo->ErrorResume;
  pEo->ErrorResume = 0;
  SavefErrorGoto = pEo->fErrorGoto;
  pEo->fErrorGoto = ONERROR_NOTHING;

  nItem = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm ;
  Opcode = pEo->CommandArray[nItem-1].OpCode;
  pEo->cLocalVariables = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.next;
  NumberOfArguments = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.next;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;

  if( pEo->cLocalVariables ){
    pEo->LocalVariables = memory_NewArray(pEo->pMo,1,pEo->cLocalVariables);
    if( pEo->LocalVariables == NULL )return COMMAND_ERROR_MEMORY_LOW;
    }else pEo->LocalVariables = NULL; /* it should have been null anyway */

  for( i=0 ; pArgument && i < NumberOfPassedArguments && i < NumberOfArguments ; i++ ){
     pEo->LocalVariables->Value.aValue[i]
        = memory_DupVar(pEo->pMo,
                        pArgument[i],
                        _pThisCommandMortals,
                        &iError);
     if( iError )return iError;
     }
  while( i < (unsigned)pEo->cLocalVariables ){
     pEo->LocalVariables->Value.aValue[i] = NULL;
     i++;
     }

  /* and finally we start to execute the function when executing the next command */
  pEo->lFunctionLevel++;
  /* some macros need this label */
// _FunctionFinishLabel: ;
  pEo->ProgramCounter = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest;
  execute_Execute_r(pEo,&iErrorCode);

  /* restore variables */

  pEo->lStepCounter = SaveStepCounter;
  if( pEo->LocalVariables )/* this is null if the function did not have arguments and no local variables */
    memory_ReleaseVariable(pEo->pMo,pEo->LocalVariables);
  pEo->ProgramCounter = SaveProgramCounter;
  pEo->LocalVariables = SaveLocalVariablesPointer;
  (*pFunctionResult) = pEo->pFunctionResult;
  pEo->pFunctionResult = SaveFunctionResultPointer;

  pEo->ErrorGoto = SaveErrorGoto;
  pEo->fErrorGoto = SavefErrorGoto;
  pEo->ErrorResume = SaveErrorResume;
  return iErrorCode;
  }
