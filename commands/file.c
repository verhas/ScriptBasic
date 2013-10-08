/*file.c

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

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if (!defined(_WIN32) && !defined(__MACOS__))
#include <sys/file.h>
#endif

#include "../command.h"
#include "../filesys.h"
#include "../match.h"
#include "../matchc.h"

#define THISFILEP  pFCO->Descriptor[FileNumber].fp
#define THISSOCKET pFCO->Descriptor[FileNumber].sp
/*
File functions and commands

This file contains the code for the commands and functions that deal with files.
*/
#if (!defined(_WIN32) && !defined(__MACOS__))
int stricmp(char *,char*);
#endif

#define MAXFILES 512
typedef struct _FileCommandObject {
  union {
    FILE *fp;  /* the file pointer to the opened file */
    SOCKET sp;
    }Descriptor[MAXFILES];
  long RecordSize[MAXFILES]; /* the length of a record */
  char mode[MAXFILES]; /* the mode the file was opened 'i', 'o', 'a', 'r' or 'b' */
                       /* 's' for client sockets */
                       /* '\0' for not opened file or socket */
  int SocketState[MAXFILES]; /*0 normal, -1 kind of EOF */
  } FileCommandObject, *pFileCommandObject;

static void close_all_files(pExecuteObject pEo){
  pFileCommandObject pFCO;
  long FileNumber;

  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  for( FileNumber = 0 ; FileNumber < MAXFILES ; FileNumber++ ){
    if( pFCO->mode[FileNumber] ){
      if( pFCO->mode[FileNumber] == 's' )
        HOOK_TCPCLOSE(THISSOCKET);
      else
        HOOK_FCLOSE(THISFILEP);
      }
    THISFILEP = NULL;
    }
  }

static int init(pExecuteObject pEo){
#define INITIALIZE init(pEo)
  pFileCommandObject pFCO;
  int i;

  /* initialize only once */
  if( PARAMPTR(CMD_OPEN) )return 0;
  PARAMPTR(CMD_OPEN) = ALLOC(sizeof(FileCommandObject));
  if( PARAMPTR(CMD_OPEN) == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  for( i=0 ; i < MAXFILES ; i++ )
    pFCO->mode[i] = (char)0;
  FINALPTR(CMD_OPEN) = close_all_files;
  return 0;
  }

/* Create the directory for a file in case it does not exists. */
static void prepare_directory(pExecuteObject pEo,char *pszFileName){
  int i;
  char *s;

  s = pszFileName + (i=strlen(pszFileName)) -1;
  while( i ){
#ifdef __MACOS__
    if( *s == ':' ){
#else
    if( *s == '/' || *s == '\\' ){
#endif
      i = *s;
      *s = (char)0;
      HOOK_MAKEDIRECTORY(pszFileName);
      *s = (char)i;
      return;
      }
    i--;
    s--;
    }
  }

/* check that a file name is secure */
static int FileIsSecure(pExecuteObject pEo,VARIABLE vFileName){
  unsigned long i;

  for( i=0 ; i < STRLEN(vFileName) ; i++ ){
    if( STRINGVALUE(vFileName)[i] == 0 )return 0;
    }
  return 1;
  }

#define SECUREFILE(x) if( ! FileIsSecure(pEo,x) )ERROR(COMMAND_ERROR_FILE_CANNOT_BE_OPENED);

/**OPEN
=section file
=title OPEN file_name FOR mode AS [ # ] i [ LEN=record_length ]
Open or create and open a file. The syntax of the line is 


=verbatim
OPEN file_name FOR mode AS [ # ] i [ LEN=record_length ]
=noverbatim

The parameters:
=itemize
=item T<file_name> if the name of the file to be opened. If the mode allows the file to be written
the file is created if it did not existed before. If needed, directory is created for the file.

=item T<mode> is the mode the file is opened. It can be:
 =itemize
 =item T<input> open the file for reading. In this mode the file is opened in read only mode and can not be altered using the file number associated with the open file. Using any function or command that tries to write the file will result in error. In this mode the file has to exist already to open successfully. If the file to be opened for T<input> does not exist the command T<OPEN> raises an error.
 =item T<output> open the file for writing. If the file existed it's content is deleted first and a freshly opened empty file is ready to accept commands and functions to write into the file. When a file is opened this way no function or command trying to read from the file can be used using the file number associated with the file. The file is opened in ASCII mode but the handling mode can be changed to binary any time.
 =item T<append> open a possibly existing file and write after the current content. The same conditions apply as in the mode T<output>, thus you can not read the file, only write. The file is opened in ASCII mode but the handling mode can be changed to binary any time.
 =item T<random> open the file for reading and writing (textual mode). When you open a file using this mode the file can be written and the content of the existing file can be read. The file pointer can be moved back and forth any time using the command R<SEEK> and thus quite complex file handling functions can be implemented. If the file did not exist it is created.
 =item T<binary> open the file for reading and writing (binary mode). This mode is the same as T<random> with the exception that the file is opened in binary mode.
 =item T<socket> open a socket. In this case the file name is NOT a file name, but rather an Internet machine name and a port separated by colon, like T<www.digital.com:80> You should not specify any method, like T<http://> in front of the machine name, as this command opens a TCP socket to the machine's port and the protocol has to be implemented by the BASIC program.
 =noitemize

=item T<#i> is the file number. After the file has been opened this number has to be used in later file handling functions and commands, like R<CLOSE> to refer to the file. The T<#> character is optional and is allowed for compatibility with other BASIC languages. The number can be between 1 and 512.
This number is quite big for most of the applications and provides compatibility with VisualBasic.

=item T<record_length> is optional and specify the length of a record in the file. The default record length is 1 byte. File pointer setting commands usually work on records, thus R<SEEK>, R<TRUNCATE> and other commands and functions accept arguments or return values as number of records. The actual record length is not recorded anywhere thus the BASIC program has to remember the actual length of a record in a file. This is not a BASIC program error to open a file with a different record size than it was created, although this may certainly be a programming error.
=noitemize

If the file number is specified as a variable and the variable value is set to integer zero then the command will automatically find a usable file number and set the variable to hold that value. Using any other expression of value integer zero is an error.
*/
COMMAND(OPEN)
#if NOTIMP_OPEN
NOTIMPLEMENTED;
#else


  long FileNumber,RecLen;
  char *FileName;
  char *ModeString;
  pFileCommandObject pFCO;
  VARIABLE Op1,FN,vLEN;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  ModeString = pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue;
  NEXTPARAMETER;
  FN = _EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;
  NEXTPARAMETER;
  vLEN = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;

  if( memory_IsUndef(vLEN) )ERROR(COMMAND_ERROR_BAD_RECORD_LENGTH);
  RecLen = LONGVALUE(vLEN); /* the default record length is 1 */
  if( RecLen < 1 )ERROR(COMMAND_ERROR_BAD_RECORD_LENGTH);

  if( memory_IsUndef(FN) )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  if( TYPE(FN) == VTYPE_LONG && LONGVALUE(FN) == 0 ){/* we have to automatically allocate the file number */

    for( FileNumber = 1 ; FileNumber < MAXFILES ; FileNumber++ )
      if( ! pFCO->mode[FileNumber] )break;
    if( FileNumber >= MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
    LONGVALUE(FN) = FileNumber;
    }
  FileNumber = LONGVALUE(CONVERT2LONG(FN));

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_NUMBER_IS_USED);

  FileName = ALLOC(STRLEN(Op1)+1);
  if( FileName == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  memcpy(FileName,STRINGVALUE(Op1),STRLEN(Op1));
  FileName[STRLEN(Op1)] = (char)0;

  if( !stricmp(ModeString,"socket") || !stricmp(ModeString,"socket_binary")){
    if( HOOK_TCPCONNECT(&(THISSOCKET),FileName) ){
      FREE(FileName);
      pFCO->mode[FileNumber] = (char)0;
      ERROR(COMMAND_ERROR_FILE_CANNOT_BE_OPENED);
      }
    pFCO->mode[FileNumber] = 's';
    FREE(FileName);
    pFCO->RecordSize[FileNumber] = RecLen;
    pFCO->SocketState[FileNumber] = 0;
    RETURN;
    }else
  if( !stricmp(ModeString,"input") ){
    pFCO->mode[FileNumber] = 'i';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else
      THISFILEP = HOOK_FOPEN(FileName,"r");
    }else
  if( !stricmp(ModeString,"input_binary") ){
    pFCO->mode[FileNumber] = 'i';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else
      THISFILEP = HOOK_FOPEN(FileName,"rb");
    }else
  if( !stricmp(ModeString,"output") ){
    pFCO->mode[FileNumber] = 'o';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"w");
      }
    }else
  if( !stricmp(ModeString,"output_binary") ){
    pFCO->mode[FileNumber] = 'o';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"wb");
      }
    }else
  if( !stricmp(ModeString,"append") ){
    pFCO->mode[FileNumber] = 'a';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"a");
      }
    }else
  if( !stricmp(ModeString,"append_binary") ){
    pFCO->mode[FileNumber] = 'a';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"ab");
      }
    }else
  if( !stricmp(ModeString,"random") ){
    pFCO->mode[FileNumber] = 'r';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"r+");
      if( THISFILEP == NULL )
        THISFILEP = HOOK_FOPEN(FileName,"w+");
      }
    }else
  if( !stricmp(ModeString,"binary") || !stricmp(ModeString,"random_binary") ){
    pFCO->mode[FileNumber] = 'b';
    if( HOOK_ISDIR(FileName) )
      THISFILEP = NULL;
    else{
      prepare_directory(pEo,FileName);
      THISFILEP = HOOK_FOPEN(FileName,"rb+");
      if( THISFILEP == NULL )
        THISFILEP = HOOK_FOPEN(FileName,"wb+");
      }
    }
  FREE(FileName);
  if( THISFILEP == NULL ){
    pFCO->mode[FileNumber] = (char)0;
    ERROR(COMMAND_ERROR_FILE_CANNOT_BE_OPENED);
    }
  pFCO->RecordSize[FileNumber] = RecLen;
#endif
END

static char *ReadFileLine(pExecuteObject pEo,
                          FILE *fp,
                          unsigned long *plCharactersRead,
                          int (*pfExtIn)(void *)){
  char *s,*r;
  unsigned long lBufferSize;
  int ch;
#define BUFFER_INCREASE 256

  s = ALLOC(BUFFER_INCREASE);
  if( s == NULL )return NULL;
  lBufferSize = BUFFER_INCREASE;
  *plCharactersRead = 0L;
  while( 1 ){
    if( (ch= (pfExtIn == NULL ? HOOK_FGETC(fp) : pfExtIn(pEo->pEmbedder) )) == EOF )break;
    if( lBufferSize <= *plCharactersRead ){
      r = ALLOC(lBufferSize+BUFFER_INCREASE);
      if( r == NULL ){
        FREE(s);
        return NULL;
        }
      memcpy(r,s,lBufferSize);
      lBufferSize += BUFFER_INCREASE;
      FREE(s);
      s = r;
      }
    s[(*plCharactersRead)++] = ch;
    if( ch == '\n' )break;
    }
  return s;
  }

static char *ReadSocketLine(pExecuteObject pEo,
                            SOCKET sp,
                            unsigned long *plCharactersRead){
  char *s,*r;
  unsigned long lBufferSize;
  char ch;
#define BUFFER_INCREASE 256

  s = ALLOC(BUFFER_INCREASE);
  if( s == NULL )return NULL;
  lBufferSize = BUFFER_INCREASE;
  *plCharactersRead = 0L;
  while( 1 ){
    if( HOOK_TCPRECV(sp,&ch,1,0) == 0 ){
      break;
      }
    if( lBufferSize <= *plCharactersRead ){
      r = ALLOC(lBufferSize+BUFFER_INCREASE);
      if( r == NULL ){
        FREE(s);
        return NULL;
        }
      memcpy(r,s,lBufferSize);
      lBufferSize += BUFFER_INCREASE;
      FREE(s);
      s = r;
      }
    s[(*plCharactersRead)++] = ch;
    if( ch == '\n' )break;
    }
  return s;
  }



/**LINEINPUT
=section file
=display LINE INPUT
=title LINE INPUT
Read a line from a file or from the standard input.

The syntax of the command is
=verbatim
LINE INPUT [# i , ] variable
=noverbatim

The parameter T<i> is the file number used in the open statement. If this is not specified the standard input is read.

The T<variable> will hold a single line from the file read containing the possible new line character terminating the line. If the last line of a file is not terminated by a new line character then the T<variable> will not contain any new line character. Thus this command does return only the characters that are really in the file and does not append extra new line character at the end of the last line if that lacks it.

On the other hand you should not rely on the missing new line character from the end of the last line because it may and usually it happens to be there. Use rather the function R<EOF> to determine if a file reading has reached the end of the file or not.

See also R<CHOMP>

You can also read from sockets using this command but you should be careful because data in a socket comes from programs generated on the fly. This means that the socket pipe may not contain the line terminating new line and not finished as well unlike a file. Therefore the command may start infinitely long when trying to read from a socket until the application on the other end of the line sends a new line character or closes the socket. When you read from a file this may not happen.

*/
COMMAND(LINPUTF)
#if NOTIMP_LINPUTF
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;
  VARIABLE Result;
  LEFTVALUE LetThisVariable;
  unsigned long lCharactersRead;
  char *s;
  long refcount;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  NEXTPARAMETER;
  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(LetThisVariable)

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);

  if( pFCO->mode[FileNumber] == 's' ){
    s = ReadSocketLine(pEo,THISSOCKET,&lCharactersRead);
    if( lCharactersRead == 0 )pFCO->SocketState[FileNumber] = -1;
    }
  else
    s = ReadFileLine(pEo,THISFILEP,&lCharactersRead,NULL);
  if( s == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  Result = NEWSTRING(lCharactersRead);
  memcpy(STRINGVALUE(Result),s,lCharactersRead);
  FREE(s);

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = Result;

#endif
END

COMMAND(LINPUT)
#if NOTIMP_LINPUT
NOTIMPLEMENTED;
#else

  VARIABLE Result;
  LEFTVALUE LetThisVariable;
  unsigned long lCharactersRead;
  char *s;
  long refcount;

  INITIALIZE;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(LetThisVariable)


  s = ReadFileLine(pEo,stdin,&lCharactersRead,(int (*)(void *))pEo->fpStdinFunction);
  if( s == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  Result = NEWSTRING(lCharactersRead);
  memcpy(STRINGVALUE(Result),s,lCharactersRead);
  FREE(s);

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = Result;
#endif
END

/**EOF
=title EOF(n)
=section file

This function accepts one parameter, an opened file number. The return value is T<true> if and only if the reading has reached the end of the file.
*/
COMMAND(EOFFUN)
#if NOTIMP_EOFFUN
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;
  NODE nItem;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;
  FileNumber = LONGVALUE(CONVERT2LONG(_EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);

  if( pFCO->mode[FileNumber] == 's' ){
    RESULT = NEWMORTALLONG;
    ASSERTNULL(RESULT)
    LONGVALUE(RESULT) = (long)pFCO->SocketState[FileNumber];
    }else{
    RESULT = NEWMORTALLONG;
    ASSERTNULL(RESULT)
    if( HOOK_FEOF(THISFILEP) )
      LONGVALUE(RESULT) = -1L;
    else
      LONGVALUE(RESULT) =  0L;
    }
#endif
END

/**INPUTFUN
=display INPUT()
=section file
=title INPUT(n,fn)

This function reads records from an opened file.

Arguments:

=itemize
=item T<n> the first argument is the number of records to read. The size of the record in terms of bytes is defined as the T<LEN> parameter when the file was opened. If this was missing the function reads T<n> bytes from the file or socket.
=item T<fn> the second parameter is the file number associated to the opened file by the command R<OPEN>. If this parameter is missing the function reads from the standard input.
=noitemize

The function tries but not necessarily reads T<n> records from the file. To get the actual number of bytes (and not records!) read from the file you can use the function T<LEN> on the result string.

Note that some Basic languages allow the form

=verbatim
a = INPUT(20,#1)
=noverbatim

however this extra T<#> is not allowed in ScriptBasic. The character T<#> is an operator in ScriptBasic defined as no-operation and therefore you can use this form. On the other hand operators like T<#> are reserved for the external modules and some external module may redefine this operator. Programs using such modules may end up in trouble when using the format above therefore it is recommended not to use the above format.
*/
COMMAND(INPUTFUN)
#if NOTIMP_INPUTFUN
NOTIMPLEMENTED;
#else


  long FileNumber,BytesToRead,CharsRead;
  pFileCommandObject pFCO;
  NODE nItem;
  FILE *fp;
  VARIABLE fpN,vBTR;
  char *s;
  int ch;
  int (*pfExtIn)(void *); /* function to return a single character from the embedder standard input*/

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;
  vBTR = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(vBTR) ){
    /* we won't read anything because the number of characters is undefined, and therefore the
       result is undef, but we still evaluate the second argument if any */
    nItem = CDR(nItem);
    if( nItem ){
      _EVALUATEEXPRESSION(CAR(nItem));
      ASSERTOKE;

      }
    RESULT = NULL;
    RETURN;
    }
  BytesToRead = GETLONGVALUE(vBTR);
  nItem = CDR(nItem);
  if( nItem ){
    fpN = _EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;

    if( ! memory_IsUndef(fpN) ){
      FileNumber = GETLONGVALUE(fpN);

      if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
      FileNumber --;
      if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
      BytesToRead *= pFCO->RecordSize[FileNumber];
      fp = THISFILEP;
      }else fp = stdin;

    }else{
    if( pEo->fpStdinFunction != NULL ){
      pfExtIn = pEo->fpStdinFunction;
      RESULT = NEWMORTALSTRING(BytesToRead);
      ASSERTNULL(RESULT)
      s = STRINGVALUE(RESULT);
      CharsRead = 0;
      while( BytesToRead && ( ch = pfExtIn(pEo->pEmbedder) ) != EOF ){
        *s++ = ch;
        BytesToRead --;
        CharsRead ++;
        }
      STRLEN(RESULT) = CharsRead;
      RETURN;
      }
    fp = stdin;
    }
  RESULT = NEWMORTALSTRING(BytesToRead);
  ASSERTNULL(RESULT)
  if( pFCO->mode[FileNumber] == 's' ){
    STRLEN(RESULT) = HOOK_TCPRECV(THISSOCKET,STRINGVALUE(RESULT),BytesToRead,0);
    if( STRLEN(RESULT) == 0 )
      pFCO->SocketState[FileNumber] = -1;
    }else{
      s = STRINGVALUE(RESULT);
      CharsRead = 0;
      while( BytesToRead && ( ch = HOOK_FGETC(fp) ) != EOF ){
        *s++ = ch;
        BytesToRead --;
        CharsRead ++;
        }
      STRLEN(RESULT) = CharsRead;
    }

#endif
END

/**CLOSE
=section file
=title CLOSE [ # ] fn
Close a previously successfully opened file. The argument of the command is the file number that was used in the command R<OPEN> to open the file.

If the file number is not associated with a successfully opened file then error is raised.
*/
COMMAND(CLOSE)
#if NOTIMP_CLOSE
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] == 's' )
    HOOK_TCPCLOSE(THISSOCKET);
  else
    HOOK_FCLOSE(THISFILEP);
  pFCO->mode[FileNumber] = (char)0;
  THISFILEP = NULL;
#endif
END

/**RESET
=section file
=title RESET

This command closes all files opened by the current BASIC program. This command usually exists in most BASIC implementation. There is no need to close a file before a BASIC program finishes, because the interpreter automatically closes all files that were opened by the program.
*/
COMMAND(RESET)
#if NOTIMP_RESET
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  for( FileNumber = 0 ; FileNumber < MAXFILES ; FileNumber++ ){
    if( pFCO->mode[FileNumber] )
      if( pFCO->mode[FileNumber] == 's' )
        HOOK_TCPCLOSE(THISSOCKET);
      else
        HOOK_FCLOSE(THISFILEP);
    pFCO->mode[FileNumber] = (char)0;
    THISFILEP = NULL;
    }
#endif
END

/**SEEK
=section file
=title SEEK fn,position
Go to a specified position in an open file. You can use this command to position
the file pointer to a specific position. The next read or write operation
performed on the file will be performed on that very position that was set
using the command T<SEEK>. The first argument is the file number that was
used in the statement T<OPEN> to open the file. The second argument is the
position where the file pointer is to be set.

The position is counted from the start of the file
counting records. The actual file pointer will be set B<after> the record
T<position>. This means that if for example you want to set the file pointer
To the start of the file then you have to T<SEEK fn,0>. This will set the
File pointer before the first record.

If there was no record length specified when the file was
opened the counting takes bytes. There is no special "record" structure of
a file as it is usually under UNIX or Windows NT. The record is merely the
number of bytes treated as a single unit specified during file opening. 
*/

COMMAND(SEEK)
#if NOTIMP_SEEK
NOTIMPLEMENTED;
#else


  long FileNumber,Position;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] == 's' )ERROR(COMMAND_ERROR_SOCKET_FILE);
  NEXTPARAMETER;
  Position = pFCO->RecordSize[FileNumber] * LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;

  fflush(THISFILEP);
  fseek(THISFILEP,Position,SEEK_SET);

#endif
END

/**TRUNCATE
=section file
=title TRUNCATE fn,new_length
Truncate an opened file to the specified size. The first argument
Has to be the file number used in the R<OPEN> statement opening the
file. The second argument is the number of records to be in the file after
it is truncated.

The size of a record has to be specified when the file is opened. If the size
Of a record is not specified in number of bytes then the command T<TRUNCATE>
Does truncate the file to the number of specified bytes instead of records.
(In other words the record length is one byte.)

When the file is actually shorter than the length specified by the command argument the command T<TRUNCATE> automatically extends the file padding with bytes containing the value 0.

*/
COMMAND(TRUNCATEF)
#if NOTIMP_TRUNCATEF
NOTIMPLEMENTED;
#else


  long FileNumber,Position;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] == 's' )ERROR(COMMAND_ERROR_SOCKET_FILE);
  NEXTPARAMETER;
  Position = pFCO->RecordSize[FileNumber] * LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;

  fflush(THISFILEP);
  HOOK_TRUNCATE(THISFILEP,Position);

#endif
END
/**REWIND
=section file
=title REWIND [ # ]fn
Positions the file cursor to the start of the file.
This is the same as T<SEEK fn,0> or T<SEEK #fn,0>

The argument to the statement is the file number used in the R<OPEN> statement to open the file. The character T<#> is optional and can only be used for compatibility reasons.
*/
COMMAND(REWIND)
#if NOTIMP_REWIND
NOTIMPLEMENTED;
#else


  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] == 's' )ERROR(COMMAND_ERROR_SOCKET_FILE);

  fflush(THISFILEP);
  fseek(THISFILEP,0L,SEEK_SET);

#endif
END

/**LOC
=display LOC()
=title LOC()
=section file
Return current file pointer position of the opened file. The argument of the
function is the file number that was used by the statement R<OPEN> opening the
file.

This function is the counter part of the statement R<SEEK> that sets the file
pointer position.

The file position is counted in record size. This means that the file pointer stands after the record returned by the function. This is not necessarily stands right after the record at the start of the next record actually. It may happen that the file pointer stands somewhere in the middle of the next record. Therefore the command

=verbatim
SEEK fn,LOC(fn)
=noverbatim

may alter the actual file position and can be used to set the file pointer to a safe record boundary position.

If there was no record size defined when the file was opened the location is counted in bytes. In this case the returned value precisely defines where the file pointer is. 
*/
COMMAND(LOC)
#if NOTIMP_LOC
NOTIMPLEMENTED;
#else


  VARIABLE Op1;
  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  Op1 = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;

  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }

  FileNumber = LONGVALUE(CONVERT2LONG(Op1));
  RESULT = NULL;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )RETURN;
  if( pFCO->mode[FileNumber] == 's' )RESULT;

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = (long)(ftell(THISFILEP) / pFCO->RecordSize[FileNumber]);

#endif
END

/**LOF
=section file
=display LOF()
=title LOF()
This function returns the length of an opened file in number of records. The argument of the function has to be the file number that was used by the statement R<OPEN> to open the file.

The actual number of records is calculated using the record size specified when the command R<OPEN> was used. The returned number is the number of records that fit in the file. If the file is longer containing a fractional record at the end the fractional record is not counted.

If there was no record length specified when the file was opened the length of the file is returned in number of bytes. In this case fractional record has no meaning.
*/
COMMAND(LOF)
#if NOTIMP_LOF
NOTIMPLEMENTED;
#else


  VARIABLE Op1;
  long FileNumber,SavePosition;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  Op1 = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;

  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }

  FileNumber = LONGVALUE(CONVERT2LONG(Op1));
  RESULT = NULL;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )RETURN;
  if( pFCO->mode[FileNumber] == 's' )RETURN;

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  SavePosition = ftell(THISFILEP);
  fseek(THISFILEP,0,SEEK_END);
  LONGVALUE(RESULT) = (long)(ftell(THISFILEP) / pFCO->RecordSize[FileNumber]);
  fseek(THISFILEP,SavePosition,SEEK_SET);

#endif
END

/**FREEFILE
=section file
=title FREEFILE()
This function returns a free file number, which is currently not associated with any opened file. If there is no such file number it returns T<undef>.

The returned value can be used in a consecutive R<OPEN> statement to specify a file number. Another way to get a free file number is to set a variable to hold the integer value zero and use the variable as file number in the statement R<OPEN>. For more information on this method see the documentation of the statement R<OPEN>.
*/
COMMAND(FREEFILE)
#if NOTIMP_FREEFILE
NOTIMPLEMENTED;
#else


  VARIABLE Op1;
  long FileNumber,Range;
  pFileCommandObject pFCO;
  NODE nItem;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  if( nItem = PARAMETERLIST ){
    Op1 = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
    ASSERTOKE;
    if( memory_IsUndef(Op1) )
      Range = -1;
    else
      Range = LONGVALUE(CONVERT2LONG(Op1));
    }else Range = -1;

  if( Range == -1 ){
    for( FileNumber = 1 ; FileNumber < MAXFILES ; FileNumber++ )
      if( ! pFCO->mode[FileNumber] ){
        Range = -2;
        break;
        }
    }else
  if( Range == 0 ){
    for( FileNumber = 1 ; FileNumber < 255 ; FileNumber++ )
      if( ! pFCO->mode[FileNumber]  ){
        Range = -2;
        break;
        }
    }else{

    for( FileNumber = 255 ; FileNumber < MAXFILES ; FileNumber++ )
      if( ! pFCO->mode[FileNumber]  ){
        Range = -2;
        break;
        }
    }
  if( Range != -2 ){
    RESULT = NULL;
    RETURN;
    }

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = FileNumber+1;

#endif
END

/**PRINT
=section file
=title PRINT [ # fn , ] print_list

This command prints the elements of the T<print_list>. The argument T<print_list> is a comma separated list of expressions. The expressions are evaluated one after the other and are printed to the standard output or to the file.

The command prints the T<print_list> to an opened file given by the file number T<fn>. If T<fn> (along with the T<#> character) is not specified the command prints to the standard output. The file has to be opened to some "output" mode otherwise the command fails to print anything into the file. The command can also print into an opened socket (a file opened for mode socket). If the file is not opened then the expressions in the list T<print_list> are not evaluated and the command actually does nothing. If the file is opened, but not for a kind of "output" mode then the expressions in the T<print_list> are evaluated but the printing does not happen. Neither printing to a non-opened file number nor printing to a file opened for some read-only mode generates error.

If there is no T<print_list> specified the command prints a new line. In other words if the keyword T<PRINT> stands on the command with the optional T<#> and the file number but without anything to print then the command will print a new line character.

Note that unlike other BASIC implementations the command T<PRINT> does not accept print list formatters, like T<AT> or semicolons and does not tabify the output. The arguments are printed to the file or to the standard output one after the other without any intrinsic space or tab added. Also the print statements does not print a new line at the end of the print list unless the new line character is explicitly defined or if there is no print list at all following the command.
*/
COMMAND(FPRINT)
#if NOTIMP_FPRINT
NOTIMPLEMENTED;
#else

  char buffer[80]; /* should be enough to print a long or a double */
  NODE nItem;
  VARIABLE ItemResult;

  long FileNumber;
  pFileCommandObject pFCO;
  char *s;
  unsigned long slen;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  nItem = PARAMETERNODE;
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(nItem)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )RETURN;


  NEXTPARAMETER;
  nItem = PARAMETERNODE;

  while( nItem ){
    ItemResult = EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;


    if( memory_IsUndef(ItemResult) ){
      s = "undef";
      slen = 5;
      }
    else
    switch( TYPE(ItemResult) ){
      case VTYPE_LONG:
        sprintf(buffer,"%ld",LONGVALUE(ItemResult));
        s = buffer;
        slen = strlen(buffer);
        break;
      case VTYPE_DOUBLE:
        sprintf(buffer,"%lf",DOUBLEVALUE(ItemResult));
        s = buffer;
        slen = strlen(buffer);
        break;
      case VTYPE_STRING:
        s = STRINGVALUE(ItemResult);
        slen = STRLEN(ItemResult);
        break;
      case VTYPE_ARRAY:
        sprintf(buffer,"ARRAY@#%08X",LONGVALUE(ItemResult));
        s = buffer;
        slen = strlen(buffer);
        break;
      }

    if( pFCO->mode[FileNumber] == 's' )
      HOOK_TCPSEND(THISSOCKET,s,slen,0);
    else
      while( slen -- )HOOK_PUTC(((int)*s++),THISFILEP);

    nItem = CDR(nItem);
    }

  if( pFCO->mode[FileNumber] != 's' &&
      fflush(THISFILEP) == EOF )ERROR(COMMAND_ERROR_PRINT_FAIL);

#endif
END

COMMAND(FPRINTNL)
#if NOTIMP_FPRINTNL
NOTIMPLEMENTED;
#else


  NODE nItem;
  long FileNumber;
  pFileCommandObject pFCO;


  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  nItem = PARAMETERNODE;
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(nItem)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )RETURN;


  if( pFCO->mode[FileNumber] == 's' )
    HOOK_TCPSEND(THISSOCKET,"\n",1,0);
  else{
    HOOK_PUTC('\n',THISFILEP);
    if( fflush(THISFILEP) == EOF )ERROR(COMMAND_ERROR_PRINT_FAIL);
    }

#endif
END
#define NOCOMMAND(XXX) \
COMMAND(XXX)\
NOTIMPLEMENTED;\
END

#define FILE_FUN(XXX,yyy) \
COMMAND(XXX)\
\
  char *FileName;\
  VARIABLE Op;\
  long lRes;\
\
  USE_CALLER_MORTALS;\
\
  Op = _EVALUATEEXPRESSION(CAR(PARAMETERLIST));\
  ASSERTOKE;\
  if( memory_IsUndef(Op) ){\
    RESULT = NULL;\
    RETURN;\
    }\
\
  Op = CONVERT2STRING(Op);\
\
  FileName = ALLOC(STRLEN(Op)+1);\
  if( FileName == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);\
  memcpy(FileName,STRINGVALUE(Op),STRLEN(Op));\
  FileName[STRLEN(Op)] = (char)0;\
  lRes = yyy(FileName);\
  if( lRes == -1 ){\
/* APK 21-Nov-2003 Fix memory leak */\
    FREE(FileName);\
    RESULT = NULL;\
    RETURN;\
    }\
\
  RESULT = NEWMORTALLONG;\
  ASSERTNULL(RESULT)\
  LONGVALUE(RESULT) = lRes;\
  FREE(FileName);\
\
END

#define FILE_BFUN(XXX,yyy) \
COMMAND(XXX)\
\
  char *FileName;\
  VARIABLE Op;\
  long lRes;\
\
  USE_CALLER_MORTALS;\
\
  Op = _EVALUATEEXPRESSION(CAR(PARAMETERLIST));\
  ASSERTOKE;\
  if( memory_IsUndef(Op) ){\
    RESULT = NULL;\
    RETURN;\
    }\
\
  Op = CONVERT2STRING(Op);\
\
  FileName = ALLOC(STRLEN(Op)+1);\
  if( FileName == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);\
  memcpy(FileName,STRINGVALUE(Op),STRLEN(Op));\
  FileName[STRLEN(Op)] = (char)0;\
  lRes = yyy(FileName);\
  RESULT = NEWMORTALLONG;\
  ASSERTNULL(RESULT)\
  LONGVALUE(RESULT) = lRes;\
  FREE(FileName);\
\
END

/**FILELEN
=section file
=display FILELEN()
=title FILELEN(file_name)
Get the length of a named file. If the length of the file can not be determined (for example the file does not exists, or the process running the code does not have permission to read the file) then the return value is T<undef>.

This function can be used instead of R<LOC> when the file is not opened by the BASIC program.
*/

#if NOTIMP_FILELEN
NOCOMMAND(FILELEN)
#else
FILE_FUN(FILELEN,HOOK_SIZE)
#endif

/**FILEACCESSTIME
=section file
=display FILEACCESSTIME()

=title FILEACCESSTIME(file_name)
Get the time the file was accessed last time.

This file time is measured in number of seconds since January 1, 1970 00:00. Note that the different file systems store the file time with different precision. Also FAT stores the file time in local time while NTFS for example stores the file time as GMT. This function returns the value rounded to whole seconds as returned by the operating system. Some of the file systems may not store all three file time types: 

=itemize
=item the time when the file was created, 
=item last time the file was modified and 
=item last time the file was accessed
=noitemize

Trying to get a time not defined by the file system will result T<undef>.
*/
#if NOTIMP_FTACCESS
NOCOMMAND(FTACCESS)
#else
FILE_FUN(FTACCESS,HOOK_TIME_ACCESSED)
#endif

/**FILEMODIFYTIME
=section file
=display FILEMODIFYTIME()
=title FILEMODIFYTIME(file_name)
Get the time the file was modified last time. See also the comments on the function R<FTACCESS>.

*/
#if NOTIMP_FTMODIFY
NOCOMMAND(FTMODIFY)
#else
FILE_FUN(FTMODIFY,HOOK_TIME_MODIFIED)
#endif

/**FILECREATETIME
=section file
=display FILECREATETIME()
=title FILECREATETIME(file_name)
Get the time the file was modified last time. See also the comments on the function R<FTACCESS>.
*/
#if NOTIMP_FTCREATED
NOCOMMAND(FTCREATED)
#else
FILE_FUN(FTCREATED,HOOK_TIME_CREATED)
#endif

/**ISDIR
=section file
=display ISDIRECTORY()
=title ISDIRECTORY(file_name)
Returns T<true> if the named file is a directory and T<false> if the file is NOT a directory.
*/
#if NOTIMP_ISDIR
NOCOMMAND(ISDIR)
#else
FILE_BFUN(ISDIR,HOOK_ISDIR)
#endif

/**ISREG
=section file
=display ISFILE()
=title ISFILE(file_name)
Returns T<true> if the named file is a regular file and T<false> if it is a directory.
*/
#if NOTIMP_ISREG
NOCOMMAND(ISREG)
#else
FILE_BFUN(ISREG,HOOK_ISREG)
#endif

/**FILEEXISTS
=section file
=display FILEEXISTS()
=title FILEEXISTS(file_name)
Returns T<true> if the named file exists.
*/
#if NOTIMP_FILEXISTS
NOCOMMAND(FILEXISTS)
#else
FILE_BFUN(FILEXISTS,HOOK_EXISTS)
#endif

/**LOCKF
=section file
=display LOCK
=title LOCK # fn, mode
Lock a file or release a lock on a file. The T<mode> parameter can be T<read>, T<write> or T<release>. 

When a file is locked to T<read> no other program is allowed to write the file. This ensures that the program reading the file gets consistent data from the file. If a program locks a file to read using the lock value T<read> other programs may also get the T<read> lock, but no program can get the R<write> lock. This means that any program trying to write the file and issuing the command T<LOCK> with the parameter T<write> will stop and wait until all read locks are released.

When a program write locks a file no other program can read the file or write the file.

Note that the different operating systems and therefore ScriptBasic running on different operating systems implement file lock in different ways. UNIX operating systems implement so called advisory locking, while Windows NT implements mandatory lock.

This means that a program under UNIX can write a file while another program has a read or write lock on the file if the other program is not good behaving and does not ask for a write lock. Therefore this command under UNIX does not guarantee that any other program is not accessing the file simultaneously.

Contrary Windows NT does lock the file in a hard way, and this means that no other process can access the file in prohibited way while the file is locked.

This different behavior usually does not make harm, but in some rare cases knowing it may help in debugging some problems. Generally you should not have a headache because of this.

You should use this command to synchronize the BASIC programs running parallel and accessing the same file.

You can also use the command T<LOCK REGION> to lock a part of the file while leaving other parts of the file accessible to other programs.

If you heavily use record oriented files and file locks you may consider using some data base module to store the data in database instead of plain files.
*/
COMMAND(FLOCK)
#if NOTIMP_FLOCK
NOTIMPLEMENTED;
#else


  long FileNumber;
  char *ModeString;
  pFileCommandObject pFCO;


  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] || pFCO->mode[FileNumber] == 's' )RETURN;

  NEXTPARAMETER;
  ModeString = pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue;

  if( !stricmp(ModeString,"read") ){
    HOOK_FLOCK(THISFILEP,LOCK_SH);
    }else
  if( !stricmp(ModeString,"write") ){
    HOOK_FLOCK(THISFILEP,LOCK_EX);
    }else
  if( !stricmp(ModeString,"release") ){
    HOOK_FLOCK(THISFILEP,LOCK_UN);
    }else ERROR(COMMAND_ERROR_INVALID_LOCK);

#endif
END

/**LOCKR
=display LOCK REGION
=title LOCK REGION # fn FROM start TO end FOR mode
Lock a region of a file. The region starts with the record T<start> and ends with the record T<end> including both end positions. The length of a record in the file is given when the file is opened using the statement R<OPEN>.

The mode can be T<read>, T<write> and T<release>. The command works similar as whole file locking, thus it is recommended that you read the differences of the operating systems handling locking in the section of file locking for the command T<LOCK>.
*/
COMMAND(RLOCK)
#if NOTIMP_RLOCK
NOTIMPLEMENTED;
#else


  long FileNumber,lFrom,lTo,lSwap;
  char *ModeString;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( FileNumber < 1 || FileNumber > MAXFILES )RETURN;
  FileNumber --;
  if( ! pFCO->mode[FileNumber] || pFCO->mode[FileNumber] == 's' )RETURN;

  NEXTPARAMETER;
  lFrom = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( lFrom < 0 )RETURN;
  NEXTPARAMETER;
  lTo   = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;
  if( lTo < 0 )RETURN;
  if( lFrom > lTo ){
    lSwap = lTo;
    lTo = lFrom;
    lFrom = lSwap;
    }

  NEXTPARAMETER;
  ModeString = pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue;

  if( !stricmp(ModeString,"read") ){
    HOOK_LOCK(THISFILEP,LOCK_SH,lFrom,(lTo-lFrom+1)*pFCO->RecordSize[FileNumber]);
    }else
  if( !stricmp(ModeString,"write") ){
    HOOK_LOCK(THISFILEP,LOCK_EX,lFrom,(lTo-lFrom+1)*pFCO->RecordSize[FileNumber]);
    }else
  if( !stricmp(ModeString,"release") ){
    HOOK_LOCK(THISFILEP,LOCK_UN,lFrom,(lTo-lFrom+1)*pFCO->RecordSize[FileNumber]);
    }else ERROR(COMMAND_ERROR_INVALID_LOCK);

#endif
END
/**MKDIR
=section file
=title MKDIR directory_name

This command creates a new directory. If it is needed then the command attempts to create all directories automatically that are needed to create the final directory. For example if you want to create T<public_html/cgi-bin> but the directory T<public_html> does not exist then the command

=verbatim
MKDIR "public_html/cgi-bin"
=noverbatim

will first create the directory T<public_html> and then T<cgi-bin> under that directory.

If the directory can not be created for some reason an error is raised.

This is not an error if the directory does already exist.

You need not call this function when you want to create a file using the command R<OPEN>. The command R<OPEN> automatically creates the needed directory when a file is opened to be written.

The created directory can be erased calling the command R<DELETE> or calling the dangerous command R<DELTREE>.
*/
COMMAND(MKDIR)
#if NOTIMP_MKDIR
NOTIMPLEMENTED;
#else

  VARIABLE Op;
  char *s;

  Op = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  s = ALLOC(STRLEN(Op)+1);
  if( s == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  memcpy(s,STRINGVALUE(Op),STRLEN(Op));
  s[STRLEN(Op)] = (char)0;
  if( HOOK_MAKEDIRECTORY(s) == -1 ){
    FREE(s);
    ERROR(COMMAND_ERROR_MKDIR_FAIL);
    }
  FREE(s);
#endif
END

/**DELETE
=title DELETE file/directory_name
=section file

This command deletes a file or an B<empty> directory. You can not delete a directory which contains files or subdirectories.

If the file or the directory can not be deleted an error is raised. This may happen for example if the program trying to delete the file or directory does not have enough permission.

See R<DELTREE> for a more powerful and dangerous delete.
*/
COMMAND(DELETE)
#if NOTIMP_DELETE
NOTIMPLEMENTED;
#else

  VARIABLE Op;
  char *s;
  int iResult;

  Op = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  s = ALLOC(STRLEN(Op)+1);
  ASSERTOKE;
  if( s == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  memcpy(s,STRINGVALUE(Op),STRLEN(Op));
  s[STRLEN(Op)] = (char)0;
  if( ! HOOK_EXISTS(s) ){
    FREE(s);
    RETURN;
    }
  if( HOOK_ISDIR(s) ){
    iResult = HOOK_RMDIR(s);
    FREE(s);
    if( iResult == -1 )ERROR(COMMAND_ERROR_DELETE_FAIL);
    RETURN;
    }
  iResult = HOOK_REMOVE(s);
  FREE(s);
  if( iResult == -1 )ERROR(COMMAND_ERROR_DELETE_FAIL);
  RETURN;

#endif
END

/**FILECOPY
=title FILECOPY filename,filename
=section file

Copy a file. The first file is the existing one, the second is the name of the new file. If the destination file already exists then the command overwrites the file. If the destination file is to be created in a directory that does not exist yet then the directory is created automatically.

In the current version of the command you can not use wild characters to specify more than one file to copy, and you can not concatenate files using this command. You also have to specify the full file name as destination file and this is an error to specify only a directory where to copy the file.

Later versions of this command may implement these features.

If the program can not open the source file to read or the destination file can not be created then the command raises error.
*/
COMMAND(FCOPY)
#if NOTIMP_FCOPY
NOTIMPLEMENTED;
#else

  VARIABLE Op1,Op2;
  char *sfile,*dfile;
  FILE *fs,*fd;
  int ch;
  long i,LastDSPos;

  Op1 = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  CONVERT2ZCHAR(Op1,sfile);
  /* the file that we copy should exist */
  if( ! HOOK_EXISTS(sfile) ){
    FREE(sfile);
    RETURN;
    }
  NEXTPARAMETER;
  Op2 = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  CONVERT2ZCHAR(Op2,dfile);
  fs = HOOK_FOPEN(sfile,"rb");
  if( fs == NULL ){
    FREE(sfile);
    FREE(dfile);
    ERROR(COMMAND_ERROR_FILE_READ);
    }

  /* if the file is going into a directory that does not exist yet then try to create the directory */
  LastDSPos = 0;
  for( i=0 ; dfile[i] ; i++ )
#ifdef __MACOS__
    if( dfile[i] == ':' )LastDSPos = i;
#else
    if( dfile[i] == '/' || dfile[i] == '\\' )LastDSPos = i;
#endif
  if( LastDSPos ){
    i = dfile[LastDSPos];
    dfile[LastDSPos] = (char)0;
    HOOK_MAKEDIRECTORY(dfile);
    dfile[LastDSPos] = (char)i;
    }

  fd = HOOK_FOPEN(dfile,"wb");
  if( fd == NULL ){
    HOOK_FCLOSE(fs);
    FREE(sfile);
    FREE(dfile);
    ERROR(COMMAND_ERROR_FILE_WRITE);
    }
  while( (ch=HOOK_FGETC(fs)) != EOF ){
    HOOK_PUTC(ch,fd);
    }
  HOOK_FCLOSE(fs);
  HOOK_FCLOSE(fd);
  FREE(sfile);
  FREE(dfile);
  RETURN;
#endif
END

/**NAME
=title NAME filename,filename
=section file

Rename a file. The first file is the existing one, the second is the new name of the file. You can not move filed from one disk to another using this command. This command merely renames a single file. Also you can not use wild characters in the source or destination file name.

If you can not rename a file for some reason, you can try to use the command R<FileCopy> and then delete the old file. This is successful in some of the cases when T<NAME> fails, but it is a slower method.

If the file can not be renamed then the command raises error.
*/
COMMAND(NAME)
#if NOTIMP_NAME
NOTIMPLEMENTED;
#else

  VARIABLE Op1,Op2;
  char *sfile,*dfile;

  Op1 = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  CONVERT2ZCHAR(Op1,sfile);
  /* the file that we copy should exist */
  if( ! HOOK_EXISTS(sfile) ){
    FREE(sfile);
    RETURN;
    }
  NEXTPARAMETER;
  Op2 = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  CONVERT2ZCHAR(Op2,dfile);

  rename(sfile,dfile);

  FREE(sfile);
  FREE(dfile);
  RETURN;
#endif
END

/**DELTREE
=title DELTREE file/directory_name
=section file

Delete a file or a directory. You can use this command to delete a file the same way as you do use the command R<DELETE>. The difference between the two commands R<DLETE> and T<DELTREE> comes into place when the program deletes directories.

This command, T<DELTREE> forcefully tries to delete a directory even if the directory is not empty. If the directory is not empty then the command tries to delete the files in the directory and the subdirectories recursively.

If the file or the directory cannot be deleted then the command raises error. However even in this case some of the files and subdirectories may already been deleted.
*/
COMMAND(DELETEF)
#if NOTIMP_DELETEF
NOTIMPLEMENTED;
#else

  VARIABLE Op;
  char *s;
  int iResult;

  Op = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  s = ALLOC(STRLEN(Op)+1);
  if( s == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  memcpy(s,STRINGVALUE(Op),STRLEN(Op));
  s[STRLEN(Op)] = (char)0;
  if( ! HOOK_EXISTS(s) ){
    FREE(s);
    RETURN;
    }
  if( ! HOOK_ISDIR(s) ){
    iResult = HOOK_REMOVE(s);
    FREE(s);
    if( iResult == -1 )ERROR(COMMAND_ERROR_DELETE_FAIL);
    RETURN;
    }

  /* Delete the files recursively inside the directory. */
  iResult = HOOK_DELTREE(s);
  FREE(s);
  if( iResult == -1 )ERROR(COMMAND_ERROR_DELETE_FAIL);
  RETURN;

#endif
END

#define MAXDIRS 512

/* A DirList contains the file names.

   The field cbFileName points to an array.
   Each element of the array contains the length of a file name.

   SortValue is an array that stores file size, creation date or
   other parameter according to the sort condition.

   ppszFileName points to an array of char * each pointing to a
   file name. This array is altered during sort.

   cFileNames is the number of file names in the list.

   FileIndex is the index to the cbFileName array for the
   current file name during file iterations.

*/
typedef struct _DirList {
  unsigned long *cbFileName; /* file name length */
  unsigned long *SortValue;  /* sort value or string offset */
  char **ppszFileName;       /* file name */
  unsigned long cFileNames;  /* number of file names */
  unsigned long FileIndex;   /* current file index */
  } DirList, *pDirList;

#define FILES_INCREMENT 10
static int store_file_name(pExecuteObject pEo,
                           pDirList p,
                           char *buffer,
                           unsigned long ThisSortValue

  ){
  unsigned long ulNewSize;
  unsigned long *plNewCbFileName,*NewSortValue;
  char **ppszNewppszFileName;
  unsigned long i;

  /* first of all check that there is enough space to store the file name on the current index location. */
  if( p->FileIndex >= (ulNewSize = p->cFileNames) ){
    while( p->FileIndex >= ulNewSize )/* this while is redundant unless there is a programming error somwhere else */
      ulNewSize += FILES_INCREMENT;
    plNewCbFileName = ALLOC( ulNewSize * sizeof(long) );
    if( plNewCbFileName == NULL )return 1;
    NewSortValue = ALLOC( ulNewSize * sizeof(long));
    if( NewSortValue == NULL )return 1;
    ppszNewppszFileName = ALLOC( ulNewSize * sizeof(char *));
    if( ppszNewppszFileName == NULL )return 1;
    for( i=0 ; i < p->cFileNames ; i++ ){
       plNewCbFileName[i] = p->cbFileName[i];
       NewSortValue[i] = p->SortValue[i];
       ppszNewppszFileName[i] = p->ppszFileName[i];
       }
    if( p->cbFileName )FREE(p->cbFileName);
    if( p->SortValue )FREE(p->SortValue);
    if( p->ppszFileName )FREE(p->ppszFileName);
    p->cbFileName = plNewCbFileName;
    p->SortValue = NewSortValue;
    p->ppszFileName = ppszNewppszFileName;
    p->cFileNames = ulNewSize;
    }

  if( (p->ppszFileName[p->FileIndex] = ALLOC( (p->cbFileName[p->FileIndex] = strlen(buffer)) )) == NULL )
    return 1;
  memcpy(p->ppszFileName[p->FileIndex],buffer,p->cbFileName[p->FileIndex]);
  p->SortValue[p->FileIndex] = ThisSortValue;
  p->FileIndex++;
  return 0;
  }

typedef struct _DirCommandObject {
  pDirList dp[MAXDIRS];
  } DirCommandObject, *pDirCommandObject;

static void close_directory_list(pExecuteObject pEo,
                                 unsigned long i
  ){
  pDirCommandObject pDCO;
  unsigned long j;

  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);
  if( pDCO == NULL )return;

 	/* APK 21-Nov-2003 Don't manipulate empty structures */
 	if( NULL == pDCO->dp[i] )return;
 	/* APK 21-Nov-2003 ---- */

  for( j=0 ; j<pDCO->dp[i]->cFileNames ; j++ )
     FREE(pDCO->dp[i]->ppszFileName[j]);
  /* the following two pointers may be null if the directory list contained no file name*/
  if( pDCO->dp[i]->cbFileName )FREE( pDCO->dp[i]->cbFileName);
  if( pDCO->dp[i]->ppszFileName )FREE( pDCO->dp[i]->ppszFileName);
  /* APK 21-Nov-2003 Add missing FREE of structure item */
  if( pDCO->dp[i]->SortValue )FREE( pDCO->dp[i]->SortValue);

  /* this should never be null when we get here */
  if( pDCO->dp[i] )FREE( pDCO->dp[i]);
  pDCO->dp[i] = NULL;
  return;
  }

static void close_all_dirs(pExecuteObject pEo){
  pDirCommandObject pDCO;
  long i;

  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);
  if( pDCO == NULL )return;
  for( i = 0 ; i < MAXDIRS ; i++ )
    if( pDCO->dp[i] )
      close_directory_list(pEo,i);
  }

static int initdir(pExecuteObject pEo){
#define INITDIR initdir(pEo)
  pDirCommandObject pDCO;
  int i;

  /* initialize only once */
  if( PARAMPTR(CMD_OPENDIR) )return 0;

  PARAMPTR(CMD_OPENDIR) = ALLOC(sizeof(DirCommandObject));
  if( PARAMPTR(CMD_OPENDIR) == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);
  for( i=0 ; i < MAXDIRS ; i++ )
    pDCO->dp[i] = NULL;

  FINALPTR(CMD_OPENDIR) = close_all_dirs;
  return 0;
  }


#define COLLECT_DIRS   0x0001 /* collect directories as well */
#define COLLECT_DOTS   0x0002 /* collect the dot directories */
#define COLLECT_RECU   0x0004 /* collect recursively */
#define SORTBY_SIZE    0x0008 /* sort by file size */
#define SORTBY_CRETI   0x0010 /* sort by creation time */
#define SORTBY_ACCTI   0x0020 /* sort by access time */
#define SORTBY_MODTI   0x0040 /* sort by modification time */
#define SORTBY_NAME    0x0080 /* sort by name (no directory part is compared) */
#define SORTBY_FNAME   0x0100 /* sort by full name */
#define COLLECT_FULLP  0x0200 /* collect and return "full" path names including the original directory name in
                                 each file name retuned (can directly be used in file handling statements */
#define SORTBY_DESCEN  0x0400 /* sort descending order */

#define MAX_FNLEN 1024

/* This function recursively collects
*/
static int collect_dirs_r(pExecuteObject pEo,
                          char * buffer,
                          unsigned long fAction,
                          pDirList pThisDirList,
                          char *pattern,
                          unsigned long StartCharIndex
  ){
  tDIR DL;
  DIR *pDL;
  struct dirent *pD;
  int dirlen;

  unsigned long sL,pL,i;
  unsigned long cArraySize;
  pPatternParam pLastResult;
  int iError;
  unsigned long ulSortValue;

  pLastResult = (pPatternParam)PARAMPTR(CMD_LIKEOP);

  dirlen=strlen(buffer);
#ifdef __MACOS__
  if( buffer[dirlen-1] != ':' ){
    dirlen++;
    if( dirlen >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen-1,":");
    }
#else
  if( buffer[dirlen-1] != '/' ){
    dirlen++;
    if( dirlen >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen-1,"/");
    }
#endif
  pDL = HOOK_OPENDIR(buffer,&DL);
  if( pDL == NULL )return -1;
  while( pD = HOOK_READDIR(pDL) ){
    /* skip . and .. directories */
    if( pD->d_name[0] == '.' && 
       ( pD->d_name[1] == (char)0 ||
         ( pD->d_name[1] == '.' && pD->d_name[2] == (char)0 ) ) ){
      if( fAction&COLLECT_DOTS ){
        if( dirlen+strlen(pD->d_name) >= MAX_FNLEN )return -1;
        strcpy(buffer+dirlen,pD->d_name);
        if( fAction & SORTBY_SIZE )ulSortValue = HOOK_SIZE(buffer); else
        if( fAction & SORTBY_CRETI )ulSortValue = HOOK_TIME_CREATED(buffer); else
        if( fAction & SORTBY_MODTI )ulSortValue = HOOK_TIME_MODIFIED(buffer); else
        if( fAction & SORTBY_ACCTI )ulSortValue = HOOK_TIME_ACCESSED(buffer); else
        if( fAction & SORTBY_NAME  )ulSortValue = dirlen - StartCharIndex; else
        ulSortValue = 0; /* SORTBY_FNAME */
        if( store_file_name(pEo,pThisDirList,buffer+StartCharIndex,ulSortValue) )return -1;
        }
      continue;
      }
    if( dirlen+(sL=strlen(pD->d_name)) >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen,pD->d_name);
    if( *pattern ){
      pL = strlen(pattern);
      cArraySize = match_count(pattern,pL);
      if( cArraySize > pLastResult->cArraySize ){
        if( pLastResult->pcbParameterArray )FREE(pLastResult->pcbParameterArray);
        if( pLastResult->ParameterArray)FREE(pLastResult->ParameterArray);
        pLastResult->cArraySize = 0;
        pLastResult->pcbParameterArray = ALLOC(cArraySize*sizeof(unsigned long));
        if( pLastResult->pcbParameterArray == NULL )return -1;
        pLastResult->ParameterArray    = ALLOC(cArraySize*sizeof(char *));
        if( pLastResult->ParameterArray == NULL ){
          FREE(pLastResult->pcbParameterArray);
          pLastResult->pcbParameterArray = NULL;
          return -1;
          }
        pLastResult->cArraySize = cArraySize;
        }else{
        /* If the array is long enough then delete the previous result otherwise
           fake data may remain in it and it may cause trouble. */
        for( i=0 ; i < pLastResult->cArraySize ; i++ ){
          pLastResult->pcbParameterArray[i] = 0;
          pLastResult->ParameterArray[i] = NULL;
          }
        }
      pLastResult->cAArraySize = cArraySize;

      if( pLastResult->cbBufferSize < sL ){
        pLastResult->cbBufferSize = 0;
        if( pLastResult->pszBuffer )FREE(pLastResult->pszBuffer);
        pLastResult->pszBuffer = ALLOC(sL*sizeof(char));
        if( pLastResult->pszBuffer == NULL )return -1;
        pLastResult->cbBufferSize = sL;
        }

      iError = match_match(pattern,
                           pL,
                           buffer+dirlen,
                           sL,
                           pLastResult->ParameterArray,
                           pLastResult->pcbParameterArray,
                           pLastResult->pszBuffer,
                           pLastResult->cArraySize,
                           pLastResult->cbBufferSize,
#ifdef WIN32
                           0,
#else
                           !(OPTION("compare")&1),
#endif
                           pLastResult->pThisMatchSets,
                           &(pLastResult->iMatches));
      }
    if( (!*pattern) || pLastResult->iMatches ){
      if( fAction & SORTBY_SIZE )ulSortValue = HOOK_SIZE(buffer); else
      if( fAction & SORTBY_CRETI )ulSortValue = HOOK_TIME_CREATED(buffer); else
      if( fAction & SORTBY_MODTI )ulSortValue = HOOK_TIME_MODIFIED(buffer); else
      if( fAction & SORTBY_ACCTI )ulSortValue = HOOK_TIME_ACCESSED(buffer); else
      if( fAction & SORTBY_NAME  )ulSortValue = dirlen - StartCharIndex; else
      ulSortValue = 0;
      if( (fAction & COLLECT_DIRS) || (!HOOK_ISDIR(buffer)) )
      	/* APK 21-Nov-2003 Add test of return condition */
        if( store_file_name(pEo,pThisDirList,buffer+StartCharIndex,ulSortValue) )return -1;
        /* APK 21-Nov-2003 ---- */
      }
    pLastResult->iMatches = 0; /* no joker() after file pattern match */
    if( HOOK_ISDIR(buffer) && (fAction & COLLECT_RECU) )
      collect_dirs_r(pEo,buffer,fAction,pThisDirList,pattern,StartCharIndex);
    }
  HOOK_CLOSEDIR(pDL);
  dirlen--;
  buffer[dirlen] = (char)0;
  return 0;
  }

static int collect_dirs(pExecuteObject pEo,
                        unsigned long fAction,
                        pDirList pThisDirList,
                        char *Directory,
                        unsigned long cDirectory,
                        char *pattern,
                        unsigned long c_pattern
  ){
  char buffer[MAX_FNLEN];
  char puffer[MAX_FNLEN];
  unsigned long StartCharIndex;

  if( initialize_like(pEo) )return -1;
  memcpy(buffer,Directory,cDirectory);
  buffer[cDirectory] = (char)0;
#ifdef __MACOS__
  if( buffer[cDirectory-1] != ':' ){
    cDirectory++;
    if( cDirectory >= MAX_FNLEN )return -1;
    strcpy(buffer+cDirectory-1,":");
    }
#else
  if( buffer[cDirectory-1] != '/' ){
    cDirectory++;
    if( cDirectory >= MAX_FNLEN )return -1;
    strcpy(buffer+cDirectory-1,"/");
    }
#endif
  if( pattern )
    memcpy(puffer,pattern,c_pattern);
  puffer[c_pattern] = (char)0;

  StartCharIndex = strlen(buffer);
  if( fAction & COLLECT_FULLP )StartCharIndex = 0;
  if( collect_dirs_r(pEo,buffer,fAction,pThisDirList,puffer,StartCharIndex) == -1 )return -1;
  pThisDirList->cFileNames = pThisDirList->FileIndex;
  pThisDirList->FileIndex = 0;
  return 0;
  }

static int sort_dirs(pExecuteObject pEo,
                        unsigned long fAction,
                        pDirList p
  ){
  unsigned long i,j;
  unsigned long lSwap;
  char *pszSwap;
  unsigned long CompareLength,Leni,Lenj;
  int CompareSult;

#define SWAP do{\
                lSwap = p->cbFileName[i];\
                p->cbFileName[i] = p->cbFileName[j];\
                p->cbFileName[j] = lSwap;\
                lSwap = p->SortValue[i];\
                p->SortValue[i] = p->SortValue[j];\
                p->SortValue[j] = lSwap;\
                pszSwap = p->ppszFileName[i];\
                p->ppszFileName[i] = p->ppszFileName[j];\
                p->ppszFileName[j] = pszSwap;\
                }while(0)

  /* if there is nothing to sort by */
  if( !(fAction & ( SORTBY_SIZE | SORTBY_CRETI | SORTBY_ACCTI | SORTBY_MODTI | SORTBY_NAME | SORTBY_FNAME)) )
    return 0;

  if( fAction & (SORTBY_NAME | SORTBY_FNAME) ){
    /* string type of comparision */
    for( i=1 ; i < p->cFileNames ; i++ )
     for( j=0 ; j < i ; j++ ){
       CompareLength = Leni = p->cbFileName[i] - p->SortValue[i];
       if( CompareLength > (Lenj=p->cbFileName[j] - p->SortValue[j]) )
         CompareLength = Lenj;
       CompareSult = memcmp(p->ppszFileName[i]+p->SortValue[i],p->ppszFileName[j]+p->SortValue[j],CompareLength);
       CompareSult = CompareSult > 0 || (CompareSult == 0 && Leni > Lenj);
       if( fAction & SORTBY_DESCEN )CompareSult = !CompareSult;
       if( CompareSult )
         SWAP;
       }
    }else{
    /* numeric comparision based on collected value */
    for( i=1 ; i < p->cFileNames ; i++ )
     for( j=0 ; j < i ; j++ )
       if( (fAction & SORTBY_DESCEN) ? p->SortValue[i] < p->SortValue[j] : p->SortValue[i] > p->SortValue[j] )
         SWAP;
    }
  return 0;
  }

/**OPENDIR
=display OPEN DIRECTORY
=title OPEN DIRECTORY dir_name PATTERN pattern OPTION option AS dn

Open a directory to retrieve the list of files.

=itemize
=item T<dir_name> is the name of the directory.
=item T<pattern> is a wild card pattern to filter the file list.
=item T<option> is an integer value that can be composed AND-ing some of the following values
 =itemize
 =item T<SbCollectDirectories>	Collect the directory names as well as file names into the file list.
 =item T<SbCollectDots>	Collect the virtual . and .. directory names into the list.
 =item T<SbCollectRecursively>	Collect the files from the directory and from all the directories below.
 =item T<SbCollectFullPath>	The list will contain the full path to the file names. This means that the file names returned by the function R<NextFile> will contain the directory path specified in the open directory statement and therefore can be used as argument to file handling commands and functions.
 =item T<SbCollectFiles>	Collect the files. This is the default behavior.
 =item T<SbSortBySize>	The files will be sorted by file size.
 =item T<SbSortByCreateTime>	The files will be sorted by creation time.
 =item T<SbSortByAccessTime>	The files will be sorted by access time.
 =item T<SbSortByModifyTime>	The files will be sorted by modify time.
 =item T<SbSortByName>	The files will be sorted by name. The name used for sorting is the bare file name without any path. 
 =item T<SbSortByPath>	The files will be sorted by name including the path. The path is the relative to the directory, which is currently opened. This sorting option is different from the value T<sbSortByName> only when the value T<sbCollectRecursively> is also used.
 =item T<SbSortAscending>	Sort the file names in ascending order. This is the default behavior.
 =item T<SbSortDescending>	Sort the file names in descending order.
 =item T<SbSortByNone>	Do not sort. Specify this value if you do not need sorting. In this case directory opening can be much faster especially for large directories.
 =noitemize
=item T<dn> is the directory number used in later references to the opened directory.
=noitemize

Note that this command can execute for a long time and consuming a lot of memory especially when directory listing is requested recursively. When the command is executed it collects the names of the files in the directory or directories as requested and builds up an internal list of the file names in the memory. The command R<NEXTFILE> uses the list to retrieve the next file name from the list.

This implies to facts:

=itemize
=item The function T<NEXTFILE> will not ever return a file name that the file was created after, and did not exist when the command T<OPEN DIRECTORY> was executed.
=item Using R<CLOSEDIR> after the list of the files is not needed as soon as possible is a good idea.
=noitemize

Using a directory number that was already used and not released calling R<CLOSEDIR> raises an error.

If the list of the files in the directory can not be collected the command raises error.

See also R<CLOSEDIR> and R<NEXTFILE>.

*/
COMMAND(OPENDIR)
#if NOTIMP_OPENDIR
NOTIMPLEMENTED;
#else

  long DirNumber;
/* APK 21-Nov-2003 Storage for unused conversion removed
  char *DirName;
   APK 21-Nov-2003 ---- */

  unsigned long i;
  pDirCommandObject pDCO;
  VARIABLE vDirName,vPattern,vDirNumber,vOption;
  unsigned long fAction;

  INITDIR;
  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);

  /* get the directory name */
  vDirName = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  vPattern = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  vOption = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  vDirNumber = _EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;
  if( memory_IsUndef(vDirNumber) )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  if( TYPE(vDirNumber) == VTYPE_LONG && LONGVALUE(vDirNumber) == 0 ){/* we have to automatically allocate the file number */
    for( i = 1 ; i < MAXFILES ; i++ )
      if( pDCO->dp[i] == NULL )break;
    if( i >= MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
    LONGVALUE(vDirNumber) = i;
    }
  DirNumber = LONGVALUE(CONVERT2LONG(vDirNumber));

  if( DirNumber <1 || DirNumber >= MAXDIRS )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  if( pDCO->dp[DirNumber] )ERROR(COMMAND_ERROR_FILE_NUMBER_IS_USED);

  /* copy the dir name to DirName zchar terminated. */
  if( memory_IsUndef(vDirName) )ERROR(COMMAND_ERROR_INV_DNAME);
  SECUREFILE(vDirName)

/* APK 21-Nov-2003 Stop memory leak - unused conversion without FREE
  CONVERT2ZCHAR(vDirName,DirName);
   APK 21-Nov-2003 ---- */

  pDCO->dp[DirNumber] = ALLOC( sizeof(DirList) );
  if( pDCO->dp[DirNumber] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  pDCO->dp[DirNumber]->cFileNames = 0;
  pDCO->dp[DirNumber]->FileIndex  = 0;
  pDCO->dp[DirNumber]->cbFileName = NULL;
  pDCO->dp[DirNumber]->SortValue = NULL;
  pDCO->dp[DirNumber]->ppszFileName = NULL;

  if( memory_IsUndef(vOption) )ERROR(COMMAND_ERROR_INV_DO_OPTION);
  fAction = ~ (GETLONGVALUE(vOption));

#define SORTBY_SIZE    0x0008 /* sort by file size */
#define SORTBY_CRETI   0x0010 /* sort by creation time */
#define SORTBY_ACCTI   0x0020 /* sort by access time */
#define SORTBY_MODTI   0x0040 /* sort by modification time */
#define SORTBY_NAME    0x0080 /* sort by name (no directory part is compared) */
#define SORTBY_FNAME   0x0100 /* sort by full name */

  /* If the user specifies more than one sorting criuteria, we have to correct it,
     because the underlying layers assume that only one bit is set. Former version
     crashed when open directory option was sloppyly set to 0. */
  if( fAction & SORTBY_SIZE  )
    fAction &= ~(    0      |SORTBY_CRETI|SORTBY_ACCTI|SORTBY_MODTI|SORTBY_NAME|SORTBY_FNAME); else
  if( fAction & SORTBY_CRETI )
    fAction &= ~(SORTBY_SIZE|       0    |SORTBY_ACCTI|SORTBY_MODTI|SORTBY_NAME|SORTBY_FNAME); else
  if( fAction & SORTBY_ACCTI )
    fAction &= ~(SORTBY_SIZE|SORTBY_CRETI|      0     |SORTBY_MODTI|SORTBY_NAME|SORTBY_FNAME); else
  if( fAction & SORTBY_MODTI )
    fAction &= ~(SORTBY_SIZE|SORTBY_CRETI|SORTBY_ACCTI|      0     |SORTBY_NAME|SORTBY_FNAME); else
  if( fAction & SORTBY_NAME  )
    fAction &= ~(SORTBY_SIZE|SORTBY_CRETI|SORTBY_ACCTI|SORTBY_MODTI|      0    |SORTBY_FNAME); else
  if( fAction & SORTBY_FNAME )
    fAction &= ~(SORTBY_SIZE|SORTBY_CRETI|SORTBY_ACCTI|SORTBY_MODTI|SORTBY_NAME|      0     );

  if( collect_dirs(pEo,
                   fAction,
                   pDCO->dp[DirNumber],
                   STRINGVALUE(vDirName),
                   STRLEN(vDirName),
                   vPattern ? STRINGVALUE(vPattern) : NULL ,
                   vPattern ? STRLEN(vPattern) : 0 ) == -1 ){
    close_directory_list(pEo,DirNumber);
/*    FREE(DirName);
*/
    ERROR(COMMAND_ERROR_DIR_NO_OPEN); 
    }
  sort_dirs(pEo,
            fAction,
            pDCO->dp[DirNumber]);
/*
  FREE(DirName);
*/
#endif
END

/**NEXTFILE
=section file
=title NEXTFILE(dn)
=display NEXTFILE()

Retrieve the next file name from an opened directory list. If there is no more file names it returns T<undef>.

See also R<OPENDIR> and R<CLOSEDIR>.

*/
COMMAND(NEXTFILE)
#if NOTIMP_NEXTFILE
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  pDirCommandObject pDCO;
  unsigned long DirNumber;

  INITDIR;
  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  DirNumber = LONGVALUE(Op1);
  if( DirNumber <1 || DirNumber >= MAXDIRS )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);

  if( pDCO->dp[DirNumber]->FileIndex >= pDCO->dp[DirNumber]->cFileNames ){
    RESULT = NULL;
    RETURN;
    }

  RESULT = NEWMORTALSTRING(pDCO->dp[DirNumber]->cbFileName[pDCO->dp[DirNumber]->FileIndex]);
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),pDCO->dp[DirNumber]->ppszFileName[pDCO->dp[DirNumber]->FileIndex],
         (STRLEN(RESULT)=pDCO->dp[DirNumber]->cbFileName[pDCO->dp[DirNumber]->FileIndex]));
  pDCO->dp[DirNumber]->FileIndex++;
#endif
END

/**EOD
=display EOD()
=title EOD(dn)
=section file

Checks if there is still some file names in the directory opened for reading using the directory number T<dn>.

See also R<NEXTFILE>.

*/
COMMAND(EODFUN)
#if NOTIMP_EODFUN
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  pDirCommandObject pDCO;
  unsigned long DirNumber;

  INITDIR;
  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  DirNumber = LONGVALUE(Op1);
  if( DirNumber <1 || DirNumber >= MAXDIRS )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  if( pDCO->dp[DirNumber]->FileIndex >= pDCO->dp[DirNumber]->cFileNames ){
    RESULT = NEWMORTALLONG;
    ASSERTNULL(RESULT)
    LONGVALUE(RESULT) = -1;
    RETURN;
    }
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = 0;
#endif
END

/**RESETDIR
=section file
=title RESET DIRECTORY [#] dn
=display RESET DIRECTORY

Reset the directory file name list and start from the first file name when the next call to R<NEXTFILE> is performed.

See also R<OPENDIR>, R<CLOSEDIR>, R<NEXTFILE>, R<EOD>.

*/
COMMAND(RESETDIR)
#if NOTIMP_RESETDIR
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  pDirCommandObject pDCO;
  unsigned long DirNumber;

  INITDIR;
  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  DirNumber = LONGVALUE(Op1);
  if( DirNumber <1 || DirNumber >= MAXDIRS )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  pDCO->dp[DirNumber]->FileIndex = 0;
#endif
END

/**CLOSEDIR
=section file
=title CLOSE DIRECTORY [#] dn
=display CLOSE DIRECTORY

Close an opened directory and release all memory that was used by the file list.

See also R<OPENDIR>.
*/
COMMAND(CLOSEDIR)
#if NOTIMP_CLOSEDIR
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  pDirCommandObject pDCO;
  unsigned long DirNumber;

  INITDIR;
  pDCO = (pDirCommandObject)PARAMPTR(CMD_OPENDIR);

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  DirNumber = LONGVALUE(Op1);
  if( DirNumber <1 || DirNumber >= MAXDIRS )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  close_directory_list(pEo,DirNumber);
#endif
END

/**SLEEP
=section misc
=title SLEEP(n)
=display SLEEP

Suspend the execution of the interpreter (process or thread) for T<n> seconds.

Whenever the program has to wait for a few seconds it is a good idea to execute this command.
Older BASIC programs originally designed for old personal computers like Atari, Amiga, ZX Spectrum
intend to use empty loop to wait time to elapse. On modern computers this is a bad idea and should not be done.

If you execute an empty loop to wait you consume CPU. Because the program does not access any resource to wait for it
actually consumes all the CPU time slots that are available. This means that the computer slows down, does not respond to user actions timely.

Different computers run with different speed and an empty loop consuming 20sec on one machine may
run 2 minutes on the other or just 10 millisec. You can not reliably tell how much
time there will be during the empty loop runs.

When you execute T<SLEEP n> the operating system is called telling it that the
code does not need the CPU for T<n> seconds. During this time the program is
suspended and the operating system executes other programs as needed. The code is
guaranteed to return from the function T<SLEEP> not sooner than T<n> seconds, but
usually it does return before the second T<n+1> starts.
*/
COMMAND(SLEEP)
#if NOTIMP_SLEEP
NOTIMPLEMENTED;
#else

  VARIABLE Op1;

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  if( ! memory_IsUndef(Op1) )
    sys_sleep(LONGVALUE(Op1));

#endif
END

/**PAUSE
=section misc
=display PAUSE

This is a planned command.

=verbatim
PAUSE n
=noverbatim

Suspend the execution of the interpreter (process or thread) for T<n>
milliseconds.
*/
COMMAND(PAUSE)
#if NOTIMP_PAUSE
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**HOSTNAME
=title HOSTNAME()
=display HOSTNAME()
=section misc

This function accepts no argument and returns the host name of the machine executing the BASIC program. This host name is the TCP/IP network host name of the machine.
*/
COMMAND(HOSTNAME)
#if NOTIMP_HOSTNAME
NOTIMPLEMENTED;
#else

  char *pszBuffer;
  long cbBuffer;
  int err;

  cbBuffer = 256;
  pszBuffer = ALLOC(cbBuffer);
  err = HOOK_GETHOSTNAME(pszBuffer,cbBuffer);
  if( err == 0 ){
    cbBuffer = strlen(pszBuffer);
    RESULT = NEWMORTALSTRING(cbBuffer);
    ASSERTNULL(RESULT)
    memcpy(STRINGVALUE(RESULT),pszBuffer,cbBuffer);
    }else RESULT = NULL;
#endif
END

/**CURDIR
=title CURDIR()
=displax CURDIR()
=section misc

This function does not accept argument and returns the current working directory as a string.
*/
COMMAND(CURDIR)
#if NOTIMP_CURDIR
NOTIMPLEMENTED;
#else

 char *Buffer;
 long cBuffer;

  USE_CALLER_MORTALS;

  cBuffer = 256;
  Buffer = ALLOC(cBuffer);
  while( HOOK_CURDIR(Buffer,cBuffer) == -1 ){
    FREE(Buffer);
    cBuffer += 256;
/* APK 21-Nov-2003 Stop Memory leak */
    if( cBuffer > 1024 ){
      FREE(Buffer);
      ERROR(COMMAND_ERROR_CURDIR);
      }

    Buffer = ALLOC(cBuffer);
    }
  cBuffer = strlen(Buffer);
  RESULT = NEWMORTALSTRING(cBuffer);
  ASSERTNULL(RESULT)

  memcpy(STRINGVALUE(RESULT),Buffer,cBuffer);

/* APK 21-Nov-2003 Stop Memory leak */
	FREE(Buffer);
/* APK 21-Nov-2003 ---- */

#endif
END

/**CHDIR
=section misc
=title CHDIR directory

Change the current working directory (CWD). This command accepts one argument, the directory which has to be the CWD after the command is executed. If the CWD can not be changed to that directory then an error is raised.

Pay careful attention when you use this command in your code. Note that there is only one CWD for each process and not one for each thread. When an application embeds the BASIC interpreter in a multi-thread environment, like in the Eszter SB Application Engine this command may alter the CWD for all the threads. 

For this reason the Eszter SB Application Engine switches off this command, raising error if ever a program executed in the engine calls this command whatever argument is given.

Thus usually BASIC programs should avoid calling this command unless the programmer is certain that the BASIC program will only be executed in a single thread environment (command line).

*/
COMMAND(CHDIR)
#if NOTIMP_CHDIR
NOTIMPLEMENTED;
#else

  VARIABLE Op;
  char *Buffer;
  int i;

  Op = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  if( memory_IsUndef(Op) )ERROR(COMMAND_ERROR_UNDEF_DIR);

  SECUREFILE(Op)
  CONVERT2ZCHAR(Op,Buffer);

  i = HOOK_CHDIR(Buffer);
  FREE(Buffer);
  if( i )ERROR(COMMAND_ERROR_CHDIR);
  
#endif
END

/**SETFILE
=section file
=title SET FILE filename parameter=value
=display SET FILE

Set some of the parameters of a file. The parameter can be:

=itemize
=item T<owner> set the owner of the file. This operation requires T<root> permission on UNIX or T<Administrator> privileges on Windows NT. The value should be the string representation of the UNIX user or the Windows NT domain user.
=item T<createtime> 
=item T<modifytime> 
=item T<accesstime>
=item Set the time of the file. The value should be the file time in seconds since January 1,1970. 00:00GMT.
=noitemize


If the command can not be executed an error is raised. Note that setting the file owner also depends on the file system. For example FAT file system does not store the owner of a file and thus can not be set.

Also setting the file time on some file system may be unsuccessful for values that are successful under other file systems. This is because different file systems store the file times using different possible start and end dates and resolution. For example you can set a file to hold the creation time to be January 1, 1970 0:00 under NTFS, but not under FAT.

The different file systems store the file times with different precision. Thus the actual time set will be the closest time not later than the specified in the command argument. For this reason the values returned by the functions T<File***Time> may not be the same that was specified in the T<SET FILE> command argument.
*/
COMMAND(SETFILE)
#if NOTIMP_SETFILE
NOTIMPLEMENTED;
#else

  VARIABLE vAttribute,vFile;
  long iErrorC;
  char *pszAttribute,*pszFile,*pszAttributeSymbol;

  vFile = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  pszAttributeSymbol = pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue;
  NEXTPARAMETER;
  vAttribute = EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;

  if( memory_IsUndef(vAttribute) )ERROR(COMMAND_ERROR_SETFILE_INVALID_ATTRIBUTE);
  if( memory_IsUndef(vFile) )ERROR(COMMAND_ERROR_INVALID_FILE_NAME);


  SECUREFILE(vFile)
  CONVERT2ZCHAR(  vFile  , pszFile  );
  if( !stricmp(pszAttributeSymbol,"owner") ){
    vAttribute = CONVERT2STRING(vAttribute);
    CONVERT2ZCHAR(  vAttribute , pszAttribute );
    iErrorC = HOOK_CHOWN(pszFile,pszAttribute);
    FREE(pszAttribute);
    }
  else if( !stricmp(pszAttributeSymbol,"createtime") ){
    CONVERT2LONG(vAttribute);
    iErrorC = HOOK_SETCREATETIME(pszFile,LONGVALUE(vAttribute));
    }
  else if( !stricmp(pszAttributeSymbol,"modifytime") ){
    CONVERT2LONG(vAttribute);
    iErrorC = HOOK_SETMODIFYTIME(pszFile,LONGVALUE(vAttribute));
    }
  else if( !stricmp(pszAttributeSymbol,"accesstime") ){
    CONVERT2LONG(vAttribute);
    iErrorC = HOOK_SETACCESSTIME(pszFile,LONGVALUE(vAttribute));
    }

  else{
    FREE(pszFile);
    ERROR(COMMAND_ERROR_SETFILE_INVALID_ATTRIBUTE);
    }
  FREE(pszFile);
  if( iErrorC )ERROR(iErrorC);
#endif
END

/**KILL
=section process
=display KILL()
=title KILL(pid)

This function kills (terminates) a process given by the T<pid> and returns true if the process was successfully killed. Otherwise it returns false.

Programs usually want to kill other processes that were started by themselves (by the program I mean) and do not stop. For example you can start an external program using the BASIC command R<EXECUTE> to run up to a certain time. If the program does not finish its work and does not stop during this time then that program that started it can assume that the external program failed and got into an infinite loop. To stop this external program the BASIC program should use the function T<KILL>.

The BASIC program however can try to kill just any process that runs on the system not only those that were started by the program. It can be successful if the program has the certain permissions to kill the given process.

You can use this function along with the functions R<SYSTEM> and T<EXECUTE>. You can list the processes currently running on an NT box using some of the functions of the module T<NT>.
*/
COMMAND(KILL)
#if NOTIMP_KILL
NOTIMPLEMENTED;
#else

  long pid;
  NODE nItem;

  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;
  pid = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  if( HOOK_KILLPROC(pid) )
    LONGVALUE(RESULT) = 0L;
  else
    LONGVALUE(RESULT) = -1L;

#endif
END

/**FOWNER
=section file
=title FILEOWNER(FileName)

This function returns the name of the owner of a file as a string. If the file does not exist or for some other reason the owner of the file can not be determined then the function returns T<undef>.
*/
#define MAXOWNERLEN 512
COMMAND(FOWNER)
#if NOTIMP_FOWNER
NOTIMPLEMENTED;
#else

  char *pszOwnerBuffer,*pszFileName;

  long cbOwnerBuffer;
  VARIABLE vFileName;

  USE_CALLER_MORTALS;

  vFileName = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  SECUREFILE(vFileName)
  CONVERT2ZCHAR(vFileName,pszFileName);
  pszOwnerBuffer = ALLOC( MAXOWNERLEN );
  if( pszOwnerBuffer == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  cbOwnerBuffer = MAXOWNERLEN;
  if( HOOK_GETOWNER(pszFileName,pszOwnerBuffer,cbOwnerBuffer) ){
    RESULT = NULL;
    RETURN;
    }
  FREE(pszFileName);
  RESULT = NEWMORTALSTRING(cbOwnerBuffer=strlen(pszOwnerBuffer));
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),pszOwnerBuffer,cbOwnerBuffer);

#endif
END

/**CRYPT
=section file misc
=display CRYPT()
=title CRYPT(string,salt)

This function returns the encoded DES digest of the string using the salt
as it is used to encrypt passwords under UNIX.

Note that only the first 8 characters of the string are taken into account.
*/
COMMAND(FCRYPT)
#if NOTIMP_FCRYPT
NOTIMPLEMENTED;
#else

  char *pszString,*pszSalt;
  char szResult[13];
  VARIABLE vString,vSalt;
  NODE nItem;

  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;

  vString = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  vSalt = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;

  if( memory_IsUndef(vString) || memory_IsUndef(vSalt) ){
    RESULT = NULL;
    RETURN;
    }

  CONVERT2ZCHAR(vString,pszString);
  CONVERT2ZCHAR(vSalt,pszSalt);

  HOOK_FCRYPT(pszString,pszSalt,szResult);

  FREE(pszString);
  FREE(pszSalt);

  RESULT = NEWMORTALSTRING(12);
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),szResult,12);

#endif
END

/**FORK
=section process
=display FORK()
=title FORK()
=subtitle NOT IMPLEMENTED

This function is supposed to perform process forking just as the native UNIX function T<fork> does. However this function is not implemented in ScriptBasic (yet). Until this function is implemented in ScriptBasic you can use the UX module fork function.

*/
COMMAND(FORK)
#if NOTIMP_FORK
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**SYSTEM
=section process
=title SYSTEM(executable_program)
=display SYSTEM()

This function should be used to start an external program in a separate process in asynchronous mode. 
In other words you can start a process and let it run by itself and not wait for the process to finish. 
After starting the new process the BASIC program goes on parallel with the started external program. 

The return value of the function is the PID of the newly created process.

If the program specified by the argument can not be started then the return value is zero. Under UNIX
the program may return a valid PID even in this case. This is because UNIX first makes a copy of the
process that wants to start another and then replaces the new process image with the program image to be
started. In this case the new process is created and the command T<SYSTEM> has no information on the fact
that the new process was not able to replace the executable image of itself. In this case, however, the
child process has a very short life.
*/
COMMAND(CREATEPROCESS)
#if NOTIMP_CREATEPROCESS
NOTIMPLEMENTED;
#else

  char *pszCommandLine;
  VARIABLE vCommandLine;
  long lPid;

  USE_CALLER_MORTALS;

  vCommandLine = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(vCommandLine) ){
    RESULT = NULL;
    RETURN;
    }

  SECUREFILE(vCommandLine)
  CONVERT2ZCHAR(vCommandLine,pszCommandLine);

  lPid = HOOK_CREATEPROCESS(pszCommandLine);

  FREE(pszCommandLine);

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = lPid;

#endif
END

/**EXECUTE
=section process
=title EXECUTE("executable_program", time_out,pid_v)
=display EXECUTE()

This function should be used to start an external program and wait for it to finish. 

The first argument of the function is the executable command line to start.
The second argument is the number of seconds that the BASIC program should wait for 
the external program to finish. If the external program finishes during this period
the function returns and the return value is the exit code of the external program.
If the argument specifying how many seconds the BASIC program has to wait is T<-1> 
then the BASIC program will wait infinitely.

If the program does not finish during the specified period then the function alters the 
third argument, which has to be a variable and raises error. In this case the argument T<pid_v> will 
hold the PID of the external program. This value can be used in the error handling code to terminate 
the external program.

=details

The function can be used to start a program synchronous and asynchronous mode as well. When the timeout value passed to the function is zero the function starts the new process, but does not wait it to finish, but raises an error. In this case the BASIC program can catch this error using the T<ON ERROR GOTO> structure and get the pid of the started process from the variable T<pid_v>. In this case the function does not "return" any value because a BASIC error happened. For example:

=verbatim
ON ERROR GOTO NoError
a = EXECUTE("ls",0,PID)
NoError:
print "The program 'ls' is running under the pid: ",PID,"\n"
=noverbatim

If the argument T<time_out> is T<-1> the function will wait for the subprocess to finish whatever long it takes to run. For example:

=verbatim
a = EXECUTE("ls",-1,PID)
print "ls was executed and the exit code was:",a
=noverbatim

Note that the string passed as first argument containing the executable program name and the arguments (the command line) should not contain zero character (a character with ASCII code 0) for security reasons. If the command line string contains zero character an error is raised.
*/
COMMAND(CREATEPROCESSEX)
#if NOTIMP_CREATEPROCESSEX
NOTIMPLEMENTED;
#else

  char *pszCommandLine;
  VARIABLE vCommandLine,vTimeOut;
  unsigned long lPid,lExitCode;
  NODE nItem;
  LEFTVALUE LetThisVariable;
  long refcount;
  int iError;

  nItem = PARAMETERLIST;

  USE_CALLER_MORTALS;

  vCommandLine = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( memory_IsUndef(vCommandLine) ){
    RESULT = NULL;
    RETURN;
    }

  /* check that the command string does not contain zero character */
  SECUREFILE(vCommandLine)
  CONVERT2ZCHAR(vCommandLine,pszCommandLine);

  nItem = CDR(nItem);
  vTimeOut = CONVERT2LONG(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;

  nItem = CDR(nItem);
  LetThisVariable = EVALUATELEFTVALUE(CAR(nItem));
  ASSERTOKE;
  DEREFERENCE(LetThisVariable);

  iError = HOOK_CREATEPROCESSEX(pszCommandLine,LONGVALUE(vTimeOut),&lPid,&lExitCode);

  FREE(pszCommandLine);
  if( iError == FILESYSE_TIMEOUT ){
    /* if this variable had value assigned to it then release that value */
    if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
    *LetThisVariable = NEWLONG;
    if( *LetThisVariable == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    LONGVALUE(*LetThisVariable) = lPid;
    ERROR(COMMAND_ERROR_TIMEOUT);
    }

  if( iError == FILESYSE_SUCCESS ){
    RESULT = NEWMORTALLONG;
    ASSERTNULL(RESULT)
    LONGVALUE(RESULT) = lExitCode;
    }
  ERROR(iError);
#endif
END

/**WAITPID
=section process
=title WAITPID(PID,ExitCode)
=display WAITPID()

This function should be used to test for the existence of a process.

The return value of the function is 0 if the process is still running.
If the process has exited (or failed in some way) the return value is T<1> and
the exit code of the process is stored in T<ExitCode>.
*/
COMMAND(WAITPID)
#if NOTIMP_WAITPID
NOTIMPLEMENTED;
#else

  VARIABLE vPid;
  NODE nItem;
  LEFTVALUE LetThisVariable;
  long refcount;
  long Result;
  long lExitCode;

  nItem = PARAMETERLIST;

  USE_CALLER_MORTALS;

  vPid = CONVERT2LONG(_EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(vPid) ){
    RESULT = NULL;
    RETURN;
    }

  nItem = CDR(nItem);
  LetThisVariable = EVALUATELEFTVALUE(CAR(nItem));
  ASSERTOKE;
  DEREFERENCE(LetThisVariable);

  Result = HOOK_WAITPID(LONGVALUE(vPid),&lExitCode);

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
  *LetThisVariable = NEWLONG;
  if( *LetThisVariable == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  if( Result == 0) lExitCode = 0;
  LONGVALUE(*LetThisVariable) = lExitCode;

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = Result;

#endif
END

/**BINMODE
=section file
=title BINMODE [ # fn ] | input | output

Set an opened file handling to binary mode.

The argument is either a file number with which the file was opened or one of keywords T<input> and T<output>. In the latter case the standard input or output is set.

See also R<TEXTMODE>
*/
COMMAND(BINMF)
#if NOTIMP_BINMF
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);
  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] != 's' )/* sockets are binary and binary only */
    HOOK_BINMODE(THISFILEP);
#endif
END

/**TEXTMODE
=section file
=title TEXTMODE [ # fn] | input | output
Set an opened file handling to text mode. 

The argument is either a file number with which the file was opened or one of keywords T<input> and T<output>. In the latter case the standard input or output is set.

See also R<BINMODE>
*/
COMMAND(TXTMF)
#if NOTIMP_TEXTMF
NOTIMPLEMENTED;
#else

  long FileNumber;
  pFileCommandObject pFCO;

  INITIALIZE;
  pFCO = (pFileCommandObject)PARAMPTR(CMD_OPEN);

  FileNumber = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE)));
  ASSERTOKE;

  if( FileNumber < 1 || FileNumber > MAXFILES )ERROR(COMMAND_ERROR_BAD_FILE_NUMBER);
  FileNumber --;
  if( ! pFCO->mode[FileNumber] )ERROR(COMMAND_ERROR_FILE_IS_NOT_OPENED);
  if( pFCO->mode[FileNumber] != 's' )/* sockets are binary and binary only */
    HOOK_TEXTMODE(THISFILEP);
#endif
END

COMMAND(BINMO)
#if NOTIMP_BINMO
NOTIMPLEMENTED;
#else
  HOOK_BINMODE(stdout);
#endif
END

COMMAND(BINMI)
#if NOTIMP_BINMI
NOTIMPLEMENTED;
#else
  HOOK_BINMODE(stdin);
#endif
END

COMMAND(TXTMO)
#if NOTIMP_TEXTMO
NOTIMPLEMENTED;
#else
  HOOK_TEXTMODE(stdout);
#endif
END

COMMAND(TXTMI)
#if NOTIMP_TEXTMI
NOTIMPLEMENTED;
#else
  HOOK_TEXTMODE(stdin);
#endif
END
