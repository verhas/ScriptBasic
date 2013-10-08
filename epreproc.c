/*
FILE: epreproc.c
HEADER: epreproc.h

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <process.h>
#else
#ifndef __MACOS__ /* No such thing as command line parameters on classic Mac */
#include <sys/types.h>
#include <sys/wait.h>
#endif
#endif

#include "errcodes.h"
#include "conftree.h"
#include "myalloc.h"
#include "uniqfnam.h"
#include "epreproc.h"

/*POD
=H External preprocessor handling

This module starts the external preprocessors.

=toc

CUT*/

/*POD
=section epreproc
=H Execute external preprocessors

This function executes the external preprocessors that are
needed to be executed either by the command line options or
driven by the extensions.

The command line option preprocessors are executed as listed
in the character array T<ppszArgPreprocessor>. These preprocessors
are checked to be run first.

If there is no preprocessors defined on the command line then the
preprocessors defined in the config file for the extensions are
executed. The input file name is also modified by the code.
The input file name is modified so that it will contain the source
code file name after the preprocessing.

The return value of the function is the error code. This is T<PREPROC_ERROR_SUCCESS>
if the preprocessing was successful. This value is zero. If the return
value is positive this is one of the error codes defined in the file T<errcodes.def>
prefixed by T<PREPROC_>.

/*FUNCTION*/
int epreproc(ptConfigTree pCONF,
             char *pszInputFileName,
             char **pszOutputFileName,
             char **ppszArgPreprocessor,
             void *(*thismalloc)(unsigned int),
             void (*thisfree)(void *)
  ){
/*noverbatim

The first argument T<pCONF> is the configuration data pointer which is passed to the
configuration handling routines.

The second argument T<pszInputFileName> is the pointer to the pointer to the input file name.

The third argument is an output variable. This will point to the output file name upon success
or to NULL. If this variable is NULL then an error has occured or the file needed no preprocessing.
The two cases can be separated based on the return value of the function. If the file needed
preprocessing and the preprocessing was successfully executed then this variable will point
to a ZCHAR string allocated via the function T<thismalloc>. This is the responsibility of the
caller to deallocate this memory space after use calling the function pointed by T<thisfree>.

The fourth argument T<ppszArgPreprocessor> is an array of preprocessors to be used
on the input file. This array contains pointers that point to ZCHAR strings.
The ZCHAR strings contain the symbolic names of the external preprocessors that
are defined in the configuration file. The configuration file defines
the actual executable for the preprocessor and the temporary directory where the
preprocessed file is stored. The final element of this pointer array should be T<NULL>.
If the pointer T<ppszArgPreprocessor> is T<NULL> or the pointer array pointed by this
contains only the terminating T<NULL> pointer then the extensions of the file name are
used to determine what preprocessors are to be applied. Preprocessors are applied from
left to right order of the file extensions.

The arguments T<thismalloc> and T<thisfree> should point to T<malloc> and T<free> or to
a similar functioning function pair. These functions will be used via the T<myalloc.c> module
and also to allocate the new T<pszOutputFileName> string in case of success. This means that the
caller should use the function pointed by T<thisfree> to release the string pointed by
T<pszOutputFileName> after the function has returned.

CUT*/
  char *pszEPreprocExe,
       *pszEPreprocDir,
#define MAXPPARGS 40
       *ppa[MAXPPARGS];
#define PCKL 40
  char szPreprocConfigKey[PCKL]; /* this is epreproc$ext where ext is the extension */
#define FULL_PATH_BUFFER_LENGTH 256
  char *PreprocessedFileName;
  char EPreProcCmdLin[FULL_PATH_BUFFER_LENGTH];
  int slen;
  void *pMemorySegment;
  char *s,*q;
  char **ppszPreprocessor = ppszArgPreprocessor;
  int iPreprIndex,i;
  char *pszLastFileName;
#if (!defined(WIN32) && !defined(__MACOS__))
  pid_t pid;
  int exit_code;
#endif

  *pszOutputFileName = NULL; /* This is the default return value in case of error or in case of
                                null operation (aka: no preprocessing needed). */
  if( pszInputFileName == NULL )return COMMAND_ERROR_SUCCESS;
  pMemorySegment = alloc_InitSegment(thismalloc,thisfree);
  if( pMemorySegment == NULL )return COMMAND_ERROR_MEMORY_LOW;

  if( ppszPreprocessor == NULL || *ppszPreprocessor == NULL ){
    /* if no preprocessor was specified on the command line then
       try to figure out what preprocessors we have to use based
       on the extension of the source file. Build the ppszPreprocessor
       array and the later is going to use it as if it were given
       as argument. */

    /* Calculate the number of extensions. */
    s = pszInputFileName;
    slen = 0; /* count the extensions in this variable */
    while( *s ){
      if( *s == '.' )slen++;
      s++;
      }
    /* if there is no preprocessor defined on the command line and
       there is no extension of the file name */
    if( slen == 0 ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_SUCCESS;
      }

    slen ++; /* count the final NULL, which is the terminating element of the array */

    /* Allocate space for the preprocessor array. */
    ppszPreprocessor = alloc_Alloc(slen*sizeof(char *),pMemorySegment);
    if( ppszPreprocessor == NULL ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_MEMORY_LOW;
      }
    for( i = 0 ; i < slen ; i++ )ppszPreprocessor[i] = NULL;

    s = pszInputFileName;
    iPreprIndex = 0;
    while( *s ){
      /* find the first/next extension */
      while( *s && *s != '.' )s++;
      if( ! *s )break; /* there is no more extensions */
      s++; /* step over the dot before the extension */
      if( ! *s )break; /* there is no more extensions */

      strcpy(szPreprocConfigKey,"preproc.extensions.");
      q = szPreprocConfigKey + 19; /* 19 = strlen("preproc.extensions.") */
      slen = 19;/* we have copied 19 characters so far */
      /* copy the extension after the "epreproc$" string */
      while( *s && *s != '.' ){
        /* if the extension is such long (30 chars or more) then there
           is surely no preprocessor configured for it */
        if( slen >= PCKL ){
          /* reset the extension copiing */
          q = szPreprocConfigKey + 19; /* 19 = strlen("preproc.extensions.") */
          break;
          }
        /* copy the character */
        *q++ = *s++;
        /* count the character copied */
        slen++;
        }
      *q = (char)0;

      /* get the symbolic name of the preprocessor assigned to the extension */
      ppszPreprocessor[iPreprIndex] = cft_GetString(pCONF,szPreprocConfigKey);
      /* this is OK if there is no preprocessor configured for an extension, but if there is
         then have the pointer to the configuration constant string and step with the index.
         we do not need to check the index value. It can not step over the allocated space,
         because we have allocated space for as many preprocessort symbolic name string pointer
          as many dots there are in the file name and there can only be less or equal number of
         preprocessors. Over indexing would be something serious internal error. */
      if( ppszPreprocessor[iPreprIndex] )iPreprIndex++;
      }
    }

  /* ----------------------------------------------------------------------------------
     At this point of exection we have all the preprocessor symbolic names in the
     array ppszPreprocessor. This was given either by argument or built up based on the
     extensions. We only have to apply it.
     ---------------------------------------------------------------------------------- */

  /* This pointer points to the actual input file of the preprocessor. At the first
     preprocessing this is the input file name. On the next runs this is the output
     of the previous preprocessors. */
  pszLastFileName = pszInputFileName;

  for( iPreprIndex = 0 ;  ppszPreprocessor[iPreprIndex] ; iPreprIndex++ ){
    pszEPreprocExe = pszEPreprocDir = NULL;
    if( strlen(ppszPreprocessor[iPreprIndex]) < PCKL - 10 ){/* 10 = strlen("eprep$xxx$") */
      /* figure out the exe of the external preprocessor */
      strcpy(szPreprocConfigKey,"preproc.external.");
      strcat(szPreprocConfigKey,ppszPreprocessor[iPreprIndex]);
      strcat(szPreprocConfigKey,".executable");
      pszEPreprocExe = cft_GetString(pCONF,szPreprocConfigKey);
      /* figure out the directory for the preprocessed file */
      strcpy(szPreprocConfigKey,"preproc.external.");
      strcat(szPreprocConfigKey,ppszPreprocessor[iPreprIndex]);
      strcat(szPreprocConfigKey,".directory");
      pszEPreprocDir = cft_GetString(pCONF,szPreprocConfigKey);
      }
    /* if there is no configured executable or temporary file directory for the preprocessor
     then this is a preprocessor error (or maybe the specified preprocessor symbolic name
     is too long. */
    if( pszEPreprocExe == NULL ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_CONFIG_EXE;
      }
    if( pszEPreprocDir == NULL ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_CONFIG_DIR;
      }

    /* Allocate space to hold the full path of the preprocessor output file name. */
    PreprocessedFileName = alloc_Alloc(strlen(pszEPreprocDir)+UNIQ_FILE_NAME_LENGTH,pMemorySegment);
    if( PreprocessedFileName == NULL ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_MEMORY_LOW;
      }
    strcpy(PreprocessedFileName,pszEPreprocDir);
    s = PreprocessedFileName + strlen(PreprocessedFileName); /* point to the end of the directory */
    /* create a unique file name from the full path of the source and append it after the directory. */
    uniqfnam(pszLastFileName,s);

    /* create the ppa argumentum list for the preprocessor */
#define CHKARGN if( i >= MAXPPARGS-1 ){\
                  alloc_FinishSegment(pMemorySegment);\
                  return PREPROC_ERROR_CONFIG_EXE;\
                  }

    strcpy(EPreProcCmdLin,pszEPreprocExe);/* copy it not to destroy the config string */
    q = EPreProcCmdLin;
    i = 1;
    ppa[0] = q;
    while( *q ){
      if( isspace(*q) ){
        CHKARGN
        *q = (char)0;
        ppa[i] = q+1;
        if( *(ppa[i]) )i++;
        }
      q++;
      }
    CHKARGN
    ppa[i++] = pszLastFileName;
    pszLastFileName = PreprocessedFileName;
    CHKARGN
    ppa[i++] = PreprocessedFileName;
    CHKARGN
    ppa[i++] = NULL;
#ifdef WIN32
    if( spawnvp(_P_WAIT,EPreProcCmdLin,ppa) ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_FAIL;
      }
#elif defined(__MACOS__)
    return PREPROC_ERROR_FAIL;
#else
    if( ! (pid = fork()) ){
      /* we are the child process */
      execvp(EPreProcCmdLin,ppa);
      /* because execvp does not return executing this exit(1) is surely an error */
      exit(1);
      }
    waitpid(pid,&exit_code,0);
    if( exit_code ){
      alloc_FinishSegment(pMemorySegment);
      return PREPROC_ERROR_FAIL;
      }
#endif
    }
  *pszOutputFileName = thismalloc(strlen(pszLastFileName)+1);
  if( *pszOutputFileName == NULL ){
    alloc_FinishSegment(pMemorySegment);
    return PREPROC_ERROR_MEMORY_LOW;
    }
  strcpy(*pszOutputFileName,pszLastFileName);
  alloc_FinishSegment(pMemorySegment);
  return PREPROC_ERROR_SUCCESS;
  }
