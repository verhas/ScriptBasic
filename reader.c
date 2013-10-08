/*
FILE:   reader.c
HEADER: reader.h

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

typedef struct _SourceLine {
  char *line;
  long lLineNumber;
  long LineLength;
  char *szFileName;
  struct _SourceLine *next;
  } SourceLine, *pSourceLine;

typedef struct _ImportedFileList {
  char *pszFileName;
  struct _ImportedFileList *next;
  }ImportedFileList,*pImportedFileList;

typedef struct _ReadObject {
  void * (*fpOpenFile)(char *, void *);  // open a file for reading
  int (*fpGetCharacter)(void *, void *); // get a character from the opened file
  void (*fpCloseFile)(void *, void *);   // close the opened file
  void *pFileHandleClass;

  void *(*memory_allocating_function)(size_t, void *);
  void (*memory_releasing_function)(void *, void *);
  void *pMemorySegment; // This variable is always passed to the memory functions

  ptConfigTree pConfig;

#define BUFFER_INITIAL_SIZE 1024 //bytes
#define BUFFER_INCREMENT 1024 // number of bytes to increase the buffer size, when it is too small
  char *Buffer;  // buffer to read a line
  long dwBuffer; // size of Buffer in bytes
  long cBuffer;  // the number of character actually in the buffer

  pSourceLine Result; // the lines of the file(s) read

// iteration variables
  pSourceLine CurrentLine; // the current line in the iteration
  long NextCharacterPosition; // the position of the next character to be returned during iteration
  char fForceFinalNL; // if this is TRUE then an extra new line is 
                      // added to the last line if it was terminated by EOF

  pReportFunction report;
  void *reportptr; // this pointer is passed to the report function. The caller should set it.
  int iErrorCounter;
  unsigned long fErrorFlags;

  pImportedFileList pImportList;

  char *FirstUNIXline;
  struct _PreprocObject *pPREP;
  } ReadObject, *pReadObject;

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "filesys.h"
#include "report.h"
#include "errcodes.h"
#include "conftree.h"
#include "reader.h"
#include "dynlolib.h"
#include "scriba.h"
#include "ipreproc.h"

/*POD

This module contains the functions that read a source file.

Script basic has several passes
until it can start to execute the code. The very first pass is to read the source lines from
the files. The routines in this module perform this task and build up a linked list that contains
the ascii values of the lines.

The input functions are parametrized, and the caller should support. If you have different
system dependent file reading functions, or if you have the input file in some format in memory or
in any other data holding space you can support these routines with character fetch functions.

CUT*/

#if (!defined(_WIN32) && !defined(__MACOS__))
int strnicmp(char *a, char *b, int n){
  char ca,cb;

  while( n-- ){
    ca = *a++;
    cb = *b++;
    ca = isupper(ca) ? tolower(ca) : ca;
    cb = isupper(cb) ? tolower(cb) : cb;
    if( ca == (char)0 && cb == (char)0 )return 0;
    if( ca != cb )return ca-cb;
    }
  return 0;
  }
int stricmp(char *a, char *b){
  char ca,cb;

  while( 1 ){
    ca = *a++;
    cb = *b++;
    ca = isupper(ca) ? tolower(ca) : ca;
    cb = isupper(cb) ? tolower(cb) : cb;
    if( ca == (char)0 && cb == (char)0 )return 0;
    if( ca != cb )return ca-cb;
    }
  }
#endif

#define REPORT(x1,x2,x3,x4) if( pRo->report )pRo->report(pRo->reportptr,x1,x2,x3,REPORT_ERROR,&(pRo->iErrorCounter),x4,&(pRo->fErrorFlags));

static int reader_AllocateInitialBuffer(pReadObject pRo){
  pRo->dwBuffer = BUFFER_INITIAL_SIZE;
  pRo->Buffer = (char *)pRo->memory_allocating_function(pRo->dwBuffer,pRo->pMemorySegment);
  if( pRo->Buffer )return READER_ERROR_SUCCESS; else return READER_ERROR_MEMORY_LOW;
  }

/*POD
=H reader_IncreaseBuffer()

When the reader encounters a line which is longer than the currently allocated
input buffer it calls this function to increase the size of the input buffer. The
input buffer is linearly increased by T<BUFFER_INCREMENT> size (defined in the header
section of T<reader.c>

When a new buffer is allocated the bytes from the old buffer are copied to the new and
the old buffer is released. It is vital that the buffer is always referenced via the
pRo->buffer pointer because resizing buffer does change the location of the buffer.

If the memory allocation fails the function return T<READER_ERROR_MEMORY_LOW> error.
Otherwise it returns zero.
/*FUNCTION*/
int reader_IncreaseBuffer(pReadObject pRo
  ){
/*noverbatim
CUT*/
  char *s,*r,*q;

  if( ! pRo->Buffer )return reader_AllocateInitialBuffer(pRo);
  r = s = pRo->Buffer;
  pRo->dwBuffer += BUFFER_INCREMENT;
  pRo->Buffer = (char *)pRo->memory_allocating_function(pRo->dwBuffer,pRo->pMemorySegment);
  if( ! pRo->Buffer ){
    pRo->Buffer = s;
    return READER_ERROR_MEMORY_LOW;
    }
  for( q = pRo->Buffer ; *q=*s ; s++ , q++ );
  pRo->memory_releasing_function(r,pRo->pMemorySegment);
  return READER_ERROR_SUCCESS;
  }

/*POD
=H reader_gets()

This function reads a newline terminated line from the file. The file is
identified by function pRo->fpGetCharacter and the pointer T<fp>.

When the input buffer is too small it automatically increases the buffer. The
terminating new line is included in the buffer. If the last line of the file is
not terminated by newline an extra newline character is added to this last line.

The addition of this extra newline character can be switched off setting pRo->fForceFinalNL
to false. Even if this variable is false the normal newline characters which are present
in the file are included in the buffer.
/*FUNCTION*/
int reader_gets(pReadObject pRo,
                 void *fp
  ){
/*noverbatim
CUT*/
  int i,ch;

  ch = pRo->fpGetCharacter(fp,pRo->pFileHandleClass);
  if( ch == EOF )return EOF;
  for( i=0 ; ch != '\n' && ch != EOF ; i++ ){
    if( i >= pRo->dwBuffer )reader_IncreaseBuffer(pRo);
    pRo->Buffer[i] = ch;
    ch = pRo->fpGetCharacter(fp,pRo->pFileHandleClass);
    }
  if( i >= pRo->dwBuffer )reader_IncreaseBuffer(pRo);
  if( pRo->fForceFinalNL || ch == '\n' )
    pRo->Buffer[i++] = '\n'; /* the line terminating \n is included in the buffer */
  if( i >= pRo->dwBuffer )reader_IncreaseBuffer(pRo);
  pRo->Buffer[i++] = (char)0;/* Mr. Greess pointed out that here we did not count the terminating zero. Corrected by PV 2002MAY13 */
  pRo->cBuffer = i;
  return !EOF;
  }

/*POD
=H reader_ReadLines()

This function calls R<reader_ReadLines_r()> to read the lines of the file
given by the file name T<szFileName> into pRo->Result. For further
information see R<reader_ReadLines_r()>.
/*FUNCTION*/
int reader_ReadLines(pReadObject pRo,
                     char *szFileName
  ){
/*noverbatim
The function returns zero or the error code.
CUT*/
  int iResult;
  pSourceLine p;

  iResult = 0;
  if( pRo->pPREP )iResult = ipreproc_Process(pRo->pPREP,PreprocessorReadStart,pRo);
  if( iResult )return iResult;
  
  pRo->FirstUNIXline = NULL;
  iResult = reader_ReadLines_r(pRo,szFileName,&(pRo->Result));
  if( iResult )return iResult;
  if( pRo->Result == NULL )return READER_ERROR_EMPTY_INPUT;

  if( pRo->pPREP )iResult = ipreproc_Process(pRo->pPREP,PreprocessorReadDone0,pRo);
  if( iResult )return iResult;

  /* On UNIX we may start scriptbasic the usual UNIX way having the
     first line specifying the interpreter. This first line starts
     like #!/usr/bin/scriba and has to be skipped by the interpreter. 

     We do skip it on Win32 too to provide portability.

     On Win32 we may write a scriptbasic program into a .cmd file

     @goto start

     scriptbasic program lines

     rem """
     :start
     @echo off
     scriba %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
     rem """

     This first line will be skipped by ScriptBasic on Win32 and for
     portability reasons on UNIX-es as well.

     If you want a real portable selfstarting script and do not mind Win32 complaining some
     then you can write:

     #! /usr/bin/scriba
     goto start
     start:
     
     scriptbasic program lines

     rem """
     :start
     @echo off
     scriba %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
     rem """

     It will run on both OS-es.
  */
  if( strncmp(pRo->Result->line,"#!",2) == 0 ||
      strncmp(pRo->Result->line,"@goto",5) == 0 ){
    pRo->Result = (p=pRo->Result)->next;
    pRo->FirstUNIXline = p->line;
    pRo->memory_releasing_function(p,pRo->pMemorySegment);
    }else{
    pRo->FirstUNIXline = NULL;
    }

  if( pRo->pPREP )iResult = ipreproc_Process(pRo->pPREP,PreprocessorReadDone1,pRo);
  if( iResult )return iResult;

  /* include the files that has to be included or imported */
  reader_ProcessIncludeFiles(pRo,&(pRo->Result));

  iResult = ipreproc_Process(pRo->pPREP,PreprocessorReadDone2,pRo);
  if( iResult )return iResult;

  /* load the internal preprocessors and let them process the source code */
  reader_LoadPreprocessors(pRo,&(pRo->Result));

  if( pRo->pPREP )iResult = ipreproc_Process(pRo->pPREP,PreprocessorReadDone3,pRo);
  return iResult;
  }
/*POD
=H reader_ReadLines_r()

This function reads the lines of a file and creates a linked list
of the read lines.
/*FUNCTION*/
int reader_ReadLines_r(pReadObject pRo,
                       char *szFileName,
                       pSourceLine *pLine
  ){
/*noverbatim
The file is identified by its name given in the string variable T<szFileName>. The
file is opened by the function pointed by pRo->fpOpenFile This function should return
a void pointer and this void pointer is passed to R<reader_gets()> (T<reader_gets>) to get a
single character.

The argument T<pLine> is a pointer to a T<SourceLine> pointer. The linked list lines read
will be chained into this pointer. The last read line will be followed by the line pointed by
T<*pLine> and T<*pLine> will point to the first line.

This design makes it easy to use and elegant to perform file inclusions. The caller has to
pass the address of the pointer field T<next> of the source line after which the file is to
be inserted.

See also T<ReadLines> that calls this function.
CUT*/
  void *fp;
  pSourceLine pL;
  long nLine;

  if( szFileName == NULL ){
    pRo->iErrorCounter++;
    return READER_ERROR_FILE_OPEN;
    }
  fp = pRo->fpOpenFile(szFileName,pRo->pFileHandleClass);
  if( fp == NULL ){
    REPORT(szFileName ,0 ,READER_ERROR_FILE_OPEN,NULL);
    return READER_ERROR_FILE_OPEN;
    }
  nLine = 1; /* the first line comes first */
  while( reader_gets(pRo,fp) != EOF ){
    pL = (pSourceLine)pRo->memory_allocating_function(sizeof(SourceLine),pRo->pMemorySegment);
    if( pL == NULL )return READER_ERROR_MEMORY_LOW;
    pL->line = (char *)pRo->memory_allocating_function(pRo->cBuffer,pRo->pMemorySegment);
    if( pL->line == NULL ){
      pRo->memory_releasing_function(pL,pRo->pMemorySegment);
      return READER_ERROR_MEMORY_LOW;
      }
    pL->szFileName = szFileName;
    pL->lLineNumber = nLine++; /* count the lines */
    pL->LineLength = pRo->cBuffer;
    /* copy the content of the buffer to the line */
    strcpy(pL->line,pRo->Buffer);
    /* link the line into the list */
    pL->next = (*pLine);
    *pLine = pL;
    pLine = &(pL->next);
    }
  pRo->fpCloseFile(fp,pRo->pFileHandleClass);
  return READER_ERROR_SUCCESS;
  }

extern int GlobalDebugDisplayFlag;
/*POD
=H reader_ProcessIncludeFiles()

This function is called from R<reader_ReadLines()> after calling R<reader_ReadLines_r()>.

This function goes through all the lines and checks if there is any line
containing an include directive.

An include directive is a line starting with a word INCLUDE (case insensitive) and
is followed by the file name. The file name can be enclodes between double quotes.

Note that the processing of the include directives are done on the characters on the
line, because they are processed before any tokenization of the lexer module. This
can cause some problem only when there is an include like line inside a multiline string.
For example:

=verbatim
a = """Hey this is a multiline string
include "subfile.txt"
"""
=noverbatim

This B<will> include the file T<subfile.txt> and its content will become part of the string.
This becomes more complicated when the file T<subfile.txt> contains strings.

The file name may not be enclosed between double quotes. In this case the file is tried to be
found in predefined system directories.

If the programmer uses the command IMPORT instead of INCLUDE the file will only be included if
it was not included yet into the current program.
/*FUNCTION*/
void reader_ProcessIncludeFiles(pReadObject pRo,
                                pSourceLine *pLine
  ){
/*noverbatim

The file read is inserted into the plce where the include statement was.

CUT*/
#define FNLEN 1024
  pSourceLine p;
  char *s,*file_name;
  CFT_NODE Node;
  char szBuffer[FNLEN];
  void *fp;
  int isImport; /* true if the statement is import and not include */
  pImportedFileList pIFL;
  long IncludeCounter;

  IncludeCounter = 1000;
  cft_GetEx(pRo->pConfig,"maxinclude",&Node,&s,&IncludeCounter,NULL,NULL); 
  p = *pLine;
  while( p ){
    s = p->line;
    while( isspace(*s) )s++;
    if( (((!strnicmp(s,"include",7)) && (s+=7) && !(isImport=0) ) ||
        ((!strnicmp(s,"import" ,6)) && (s+=6) &&  (isImport=1) )) &&
        /* there should be at least one space after the keyword */
        isspace(*s) ){
      if( --IncludeCounter == 0 ){
        REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_TOOMANY_INCLUDE,NULL);
        return;
        }
      while( isspace(*s) )s++;
      if( *s == '"' ){
        /* include user files relative to the current source file */
        s++;
        file_name = s; /* start of the file name */
        while( *s && *s != '"' )s++; /* find the end of the file name */
        if( *s != '"' ){
          REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_INCLUDE_SYNTAX,NULL);
          p = p->next;
          continue;
          }
        *s = (char)0;
        s++;
        while( isspace(*s) )s++;
        if( *s && *s != '\n' ){
          REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_INCLUDE_SYNTAX,NULL);
          p = p->next;
          continue;
          }
        file_name = reader_RelateFile(pRo,p->szFileName,file_name);
        /* here we have to modify the file name to handle the ../ and other constructs to be
           relative to the actual file */
        }else{
        /* include installed standard file at standard location */
        file_name = s;
        while( *s && ! isspace(*s) )s++; /* find the end of the file name */
        if( *s ){
          *s = (char)0;
          s++;
          }else *s = (char)0;
        while( isspace(*s) )s++;
        if( *s && *s != '\n' ){
          REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_INCLUDE_SYNTAX,NULL);
          p = p->next;
          continue;
          }

        if( GlobalDebugDisplayFlag ){
          fprintf(stderr,"Searching installed module header file '%s' ...\n",file_name);
          }
        /* here we try to iterate all the configuration defines include directories and
           try if there is an openable file in one of the. If there is the first is
           used */
        fp = NULL;
        for( cft_GetEx(pRo->pConfig,"include",&Node,&s,NULL,NULL,NULL);  
             ! cft_GetEx(pRo->pConfig,NULL,&Node,&s,NULL,NULL,NULL) ; 
             Node = cft_EnumNext(pRo->pConfig,Node) ){
          if( ! strcmp(cft_GetKey(pRo->pConfig,Node),"include") ){
            if( s && strlen(s) > FNLEN )REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_INCLUDE_SYNTAX,NULL);
            if( s )strcpy(szBuffer,s); else *szBuffer = (char)0;
            strcat(szBuffer,file_name);
            fp = pRo->fpOpenFile(szBuffer,pRo->pFileHandleClass);  /* open a file for reading (just for test: can we?) */
            if( GlobalDebugDisplayFlag ){
              fprintf(stderr,"Checking installed module header file location '%s' Result=%s\n",szBuffer, fp ? "OK" : "FAILED" );
              }
            if( fp != NULL )break;
            }
          }

        if( fp == NULL ){/* if there was no file openable during the search */
          REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_INCLUDE_FILE,NULL);
          goto NotInclude;
          }
        pRo->fpCloseFile(fp,pRo->pFileHandleClass);/* close the file because it was opened */
        file_name = pRo->memory_allocating_function(strlen(szBuffer)+1,pRo->pMemorySegment);
        if( file_name == NULL )REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_MEMORY_LOW,NULL);
        strcpy(file_name,szBuffer);
        }
      /* file_name now points to a file openable by fpOpenFile */
      if( isImport ){
        /* check that this file was not included yet. */
        pIFL = pRo->pImportList;
        while( pIFL ){
          if( ! strcmp(file_name,pIFL->pszFileName) ){
            *pLine = (*pLine)->next; /* unlink the line including the import statement */
            p = *pLine;
            goto NextP;
            }
          pIFL = pIFL->next;
          }
        }
      /* if it is not import or if the file was not included yet then put the name of the file
         on the included file list. This puts an included file on this list as many times as
         it is included, but it does not matter. */
      pIFL = pRo->memory_allocating_function(sizeof(ImportedFileList),pRo->pMemorySegment);
      if( pIFL == NULL )REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_MEMORY_LOW,NULL);
      pIFL->next = pRo->pImportList;
      pIFL->pszFileName = file_name;
      pRo->pImportList = pIFL;
      *pLine = (*pLine)->next; /* unlink the line containing the 'include' statement */
      if( GlobalDebugDisplayFlag ){
        fprintf(stderr,"Including file '%s'\n",file_name);
        }
      reader_ReadLines_r(pRo,file_name,pLine);
      /* release the line containing the include statement */
      pRo->memory_releasing_function(p->line,pRo->pMemorySegment);
      pRo->memory_releasing_function(p,pRo->pMemorySegment);
      /* file_name is not released, because that buffer is
         referenced several times. That buffer is released when the
         whole reader segment is released. */
      p = *pLine;
      }else
NotInclude:
    if( p ){
      pLine = &(p->next);
      p = *pLine;
      }
NextP:;
    }
  }

/*POD
=H reader_LoadPreprocessors()

Preprocessors are not part of ScriptBasic. They can be implemented as external DLLs and
should be configured in the configuration file.

When a line contains

=verbatim
USE preprocessorname
=noverbatim

this reader module loads the preprocessor DLL or SO (dll under unix) file.

/*FUNCTION*/
void reader_LoadPreprocessors(pReadObject pRo,
                                 pSourceLine *pLine
  ){
/*noverbatim

CUT*/
#define FNLEN 1024
  pSourceLine p,*prev;
  char *s;
  int iError;
  char szBuffer[FNLEN];

  if( pRo->pPREP == NULL ){
    /* if the embedding application does not provide 
       the environment then ignore the preprocess lines 
       all we do is to unlink the 'preprocess' lines */
    p = *pLine;
    prev = pLine;
    while( p ){
      s = p->line;
      while( isspace(*s) )s++;
      if( !strnicmp(s,"use",3) )
        (*prev) = p->next; /* unlink the 'use' line */
      prev = &(p->next);
      p = p->next;
      }
    return;
    }

  p = *pLine;
  prev = pLine;
  while( p ){
    s = p->line;
    while( isspace(*s) )s++;
    if( !strnicmp(s,"use",3) ){
      s += 3;
      if( ! isspace(*s) ){
        prev = &(p->next);
        p = p->next;
        continue;
        }
      while( isspace(*s) )s++;
      if( strlen(s) > FNLEN ){
        REPORT(p->szFileName ,p->lLineNumber ,READER_ERROR_PREPROC_LONG ,s);
        continue;
        }
      strcpy(szBuffer,s);
      s = szBuffer;
      while( *s && ! isspace(*s) )s++;
      *s = (char)0;
      if( pRo->pPREP && (iError = ipreproc_LoadInternalPreprocessor(pRo->pPREP,szBuffer)) ){
        REPORT(p->szFileName ,p->lLineNumber ,iError ,szBuffer);
        }
      (*prev) = p->next; /* unlink the 'preprocess' line */
      }else
      prev = &(p->next);
    p = p->next;
    }
  }


/*POD
=H reader_StartIteration()

The package supports functions that help upper layer modules to iterate
through the lines read. This function should be called to start the iteration
and to set the internal iteration pointer to the first line.
/*FUNCTION*/
void reader_StartIteration(pReadObject pRo
  ){
/*noverbatim
CUT*/
  pRo->CurrentLine = pRo->Result;
  pRo->NextCharacterPosition = 0;
  }

/*POD
=H reader_NextLine()

This function returns a string which is the next line during iteration.
This function does NOT read anything from any file, only returns a pointer to
a string that was already read.

This function can be used together with R<reader_NextCharacter()>. When a line was partially
passed to an upper layer that uses T<reader_NextCharacter> this function
will only return the rest of the line.

/*FUNCTION*/
char *reader_NextLine(pReadObject pRo
  ){
/*noverbatim
CUT*/
  long ThisCharacter;

  if( ! pRo->CurrentLine )return NULL;
  if( pRo->CurrentLine->line[pRo->NextCharacterPosition] == (char)0 ){
    pRo->CurrentLine = pRo->CurrentLine->next;
    pRo->NextCharacterPosition = 0;
    }
  if( ! pRo->CurrentLine )return NULL;
  ThisCharacter = pRo->NextCharacterPosition;
  pRo->NextCharacterPosition = pRo->CurrentLine->LineLength; /* we have returned all the characters */
  return pRo->CurrentLine->line+ThisCharacter;
  }

/*POD
=H reader_NextCharacter()

This function gets the next character from the actual line, or gets the
first character of the next line.

This function does NOT read anything from any file, only returns a character from a string that
was already read.

When the last character of the last line was passed it return T<EOF>
/*FUNCTION*/
int reader_NextCharacter(void *p
  ){
/*noverbatim
CUT*/
  pReadObject pRo = (pReadObject)p;
  if( ! pRo->CurrentLine )return EOF;
  if( pRo->CurrentLine->line[pRo->NextCharacterPosition] == (char)0 ){
    pRo->CurrentLine = pRo->CurrentLine->next;
    pRo->NextCharacterPosition = 0;
    }
  if( ! pRo->CurrentLine )return EOF;
  return (int)pRo->CurrentLine->line[pRo->NextCharacterPosition++];
  }


/*POD
=H reader_FileName()

This function returns the file name of the actual line. This is the string that
was used to name the file when it was opened. This can be different for different
lines when the reader is called several times to resolve the "include" statements.
/*FUNCTION*/
char *reader_FileName(void *p
  ){
/*noverbatim
CUT*/
  pReadObject pRo = (pReadObject)p;
  if( ! pRo || ! pRo->CurrentLine )return "No-File";
  return pRo->CurrentLine->szFileName;
  }

/*POD
=H reader_LineNumber()

This function returns the line number of the current line durig iteration.
This number identifies the line in the file where it was read from.

/*FUNCTION*/
long reader_LineNumber(void *p
  ){
/*noverbatim
CUT*/
  pReadObject pRo = (pReadObject)p;
  if( !pRo || ! pRo->CurrentLine )return 0;
  return pRo->CurrentLine->lLineNumber;
  }

/* two functions to call the posix standard memory allocation functions */
static void *reader_malloc(size_t n, void *pMemorySegment){
  return malloc(n);
  }
static void reader_free(void *p, void *pMemorySegment){
  free(p);
  }

/* three functions to call the posix standard file open, read, close functions */
static void *_MyOpenFile(char *FileName, void *p){
  return (void *)file_fopen(FileName,"r");
  }
static int _MyGetCharacter(void *fp, void *p){
  return file_fgetc( (FILE *)fp);
  }
static void _MyCloseFile(void *fp, void *p){
  file_fclose((FILE *)fp);
  }

/*POD
=H reader_InitStructure()

This function should be called to initialize the reader structure. It sets the
file handling routines to the standard T<fopen>, T<fclose> and T<getc> functions,
and also sets the function pointers so that the module uses T<malloc> and T<free>.

/*FUNCTION*/
void reader_InitStructure(pReadObject pRo
  ){
/*noverbatim
CUT*/
  /* set the file handling functions to the posix standard functions */
  pRo->fpOpenFile = _MyOpenFile;
  pRo->fpGetCharacter = _MyGetCharacter;
  pRo->fpCloseFile = _MyCloseFile;
  pRo->pFileHandleClass = NULL;

  pRo->memory_allocating_function = reader_malloc;
  pRo->memory_releasing_function = reader_free;
  pRo->pMemorySegment = NULL;

  pRo->pImportList = NULL;

  /* this should be NULL and zero. the buffer will be allocated during the first call to reader_gets */
  pRo->Buffer = NULL;
  pRo->dwBuffer = 0;

  pRo->Result = NULL;
  pRo->fForceFinalNL = 1; /* it is better to have the last line terminated by a new line character even
                             if the file itself has a truncated last line without line feed at the end. */
  pRo->pPREP = NULL; /* The caller has to set an appropriate SbProgram object to handle the internal
                        preprocessors */
  }

/*POD
=H reader_RelateFile()

This function gets a file name, which is either absolute or relative
to the current working directory and another file name which is absolute or
relative to the first one.

The return value of the function is a file name which is either absolute or
relative to the current woring directory.

The return value is dynamically allocated and is to be release by the caller.
The allocation function is taken from the class function and the segment is
T<pMemorySegment>.
/*FUNCTION*/
char *reader_RelateFile(pReadObject pRo,
                       char *pszBaseFile,
                       char *pszRelativeFile
  ){
/*noverbatim
CUT*/
  long lLen;
  char *pszBuffer,*s,*r,*q;

#ifdef __MACOS__
#define ischardsep(s) ((s) == ':')

  if( !ischardsep(*pszRelativeFile)
#else

  /* note that \\ is a valid directory separator in scriba include statement 
     on UNIX as well, because stupid DOS programmers may use it and their code
     should run on UNIX boxes as well. */
#define ischardsep(s) ((s) == '/' || (s) == '\\')

  if( ischardsep(*pszRelativeFile)
#ifdef WIN32
    /* if a DOS programmer uses an absolute path including drive character, I can't help
       (laughing) */
      || pszRelativeFile[1] == ':'
#endif
#endif
      ){
    /* this is an absolute file name */
    lLen = strlen(pszRelativeFile)+1;
    pszBuffer = pRo->memory_allocating_function(lLen,pRo->pMemorySegment);
    if( pszBuffer == NULL ){
      REPORT("" ,0 ,READER_ERROR_MEMORY_LOW,NULL);
      return NULL;
      }
    strcpy(pszBuffer,pszRelativeFile);
#ifndef __MACOS__
    /* this code aids brain dead DOS programmers using \ as directory
       separator to run their code on UNIX. */
    s = pszBuffer;
    while( *s ){
      if( *s == '\\' )*s = '/';
      s++;
      }
#endif
    return pszBuffer;
    }

  lLen = strlen(pszBaseFile) + strlen(pszRelativeFile) +1;
  pszBuffer = pRo->memory_allocating_function(lLen,pRo->pMemorySegment);
  if( pszBuffer == NULL ){
    REPORT("" ,0 ,READER_ERROR_MEMORY_LOW,NULL);
    return NULL;
    }
  strcpy(pszBuffer,pszBaseFile);
  r = s = pszBuffer;
  /* find the last directory separator */
  while( *s ){
    if( ischardsep(*s) )r = s;
    s++;
    }
  if( ischardsep(*r) )r++;
  /* append the file name after the directory of the base file */
  strcpy(r,pszRelativeFile);
  s = pszBuffer;
  while( *s ){
    r = s+1;
    while( *r && ! ischardsep(*r) )r++;
    if(  (ischardsep(*r) && r[1] == '.' && r[2] == '.' && ischardsep(r[3])) &&
       /* this extra condition is to prevent "../../" to be removed */
        !( s[0] == '.' && s[1] == '.' && ischardsep(s[2]) ) ){
      /* this is a name/../ structure */
      q = s;
      r += 4;
      while( *q++ = *r++ );
      }else
      s = r+1;
    }
#ifndef __MACOS__
  /* this code aids brain dead DOS programmers using \ as directory
     separator to run their code on UNIX. */
  s = pszBuffer;
  while( *s ){
    if( *s == '\\' )*s = '/';
    s++;
    }
#endif
  return pszBuffer;
  }

/*POD
=H reader_DumpLines()

This is a debug function that prints the lines into a debug file.
/*FUNCTION*/
void reader_DumpLines(pReadObject pRo,
                      FILE *fp
  ){
/*noverbatim

CUT*/
  pSourceLine p;
  p = pRo->Result;
  while( p ){
    fprintf(fp,"%s",p->line);
    p = p->next;
    }
  }
