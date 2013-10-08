/*
FILE:   builder.c
HEADER: builder.h

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

This needs expression.h

TO_HEADER:
// pragma pack(1) will help VC++ 6.0 to pack the structure to 16 bytes
#ifdef WIN32
#pragma warning( disable : 4103 ) // otherwise VC++ may complain about the pragma pack
// do not try to use pack(2) or pack(1) The node size can not be smaller than 16 bytes and
// if you use pragma(1) or pragma(2) the compiler creates unusable code (segmentation fault)
#pragma pack(push,4)
#endif
typedef struct _cNODE {
  long OpCode; // the code of operation
  union {

    // when the node is a command
    struct {
      unsigned long next;
      union {
        unsigned long pNode;  // node id of the node
        long lLongValue;
        double dDoubleValue;
        unsigned long szStringValue; // serial value of the string from the string table
        }Argument;
      }CommandArgument;

    // when the node is an operation
    struct {
      unsigned long Argument; // node id of the node list head
      }Arguments;

    // when the node is a constant
    union {
      double dValue;        
      long   lValue;        
      unsigned long sValue; // serial value of the string from the string table       
      }Constant;

    // when the node is a variable
    struct {
      unsigned long Serial; // the serial number of the variable
      }Variable;

    // when node is a user functions
    struct {
      unsigned long NodeId; // the entry point of the function
      unsigned long Argument; // node id of the node list head
      }UserFunction;

    // when the node is a node list head
    struct {
      unsigned long actualm; //car
      unsigned long rest;    //cdr
      }NodeList;


    }Parameter;
  } cNODE,*pcNODE;

#ifdef WIN32
#pragma pack(pop)
#endif

// Symbol table type where each ZCHAR symbol is assigned to a long
// the table is stored in a way so that the long value is followed
// by the ZCHAR symbol in the memory and then the next record comes.
typedef struct _SymbolLongTable {
  long value;
  char symbol[1];
  } SymbolLongTable, *pSymbolLongTable;

typedef struct _BuildObject {
  void *(*memory_allocating_function)(size_t);
  void (*memory_releasing_function)(void *);
  void *pMemorySegment; //this pointer is passed to the memory allocating functions
                        //this pointer is initialized in ex_init
  char *StringTable; // all the string constants of the program zero terminated each
  unsigned long cbStringTable; // all the bytes of StringTable including the zeroes
  unsigned long cbCollectedStrings; // the size of the strings collected so far

  int iErrorCounter;

  long cGlobalVariables;

  pcNODE CommandArray;
  unsigned long NodeCounter; // used to count the nodes and assign NodeId
  unsigned long StartNode;

  unsigned long cbFTable; // bytes of the function symbol table
  unsigned long cbVTable; // bytes of the global variables table
  pSymbolLongTable FTable; // the functions symbol table
  pSymbolLongTable VTable; // the global variable symbol table

  peXobject pEx; // the symbolic structure to build table from
  pReportFunction report;
  void *reportptr; // this pointer is passed to the report function. The caller should set it.
  unsigned long fErrorFlags;
  char *FirstUNIXline;
  struct _PreprocObject *pPREP;
  } BuildObject, *pBuildObject;

#define MAGIC_CODE   0x1A534142//this is simply BAS and ^Z on DOS to finish typing to screen
#define VERSION_HIGH 0x00000002
#define VERSION_LOW  0x00000000
#define MYVERSION_HIGH 0x00000000
#define MYVERSION_LOW  0x00000000
#define VARIATION "STANDARD"
typedef struct _VersionInfo {
  unsigned long MagicCode;
  unsigned long VersionHigh, VersionLow;
  unsigned long MyVersionHigh,MyVersionLow;
  unsigned long Build;
  unsigned long Date;
  unsigned char Variation[9];
  } VersionInfo,*pVersionInfo;

#define BU_SAVE_FTABLE 0x00000001
#define BU_SAVE_VTABLE 0x00000002
*/

/*POD

This module can and should be used to create the memory image for the
executor module from the memory structure that was created by the module
T<expression>.

The memory structure created by T<expression> is segmented, allocated
in many separate memory chunks. When the module T<expression> has been finished
the size of the memory is known. This builder creates a single memory 
chunk containing all the program code.

Note that the function names all start with the prefix T<build_> in this module.

The first argument to each function is a pointer to a T<BuildObject> structure
that contains the "global" variables for the module. This technique is used to ensure
multithread usage. There are no global variables which are really global within the
process.

The functions in this module are:

=toc

CUT*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "filesys.h"
#include "report.h"
#include "lexer.h"
#include "sym.h"
#include "expression.h"
#include "myalloc.h"
#include "builder.h"
#include "errcodes.h"
#include "buildnum.h"

#if _WIN32
#include <windows.h>
#if BCC32 || CYGWIN
extern char *_pgmptr;
#endif
#endif


#define REPORT(x1,x2,x3,x4) if( pBuild->report )pBuild->report(pBuild->reportptr,x1,x2,x3,REPORT_ERROR,&(pBuild->iErrorCounter),x4,(&pBuild->fErrorFlags))

#define CALL_PREPROCESSOR(X,Y) if( pBuild->pPREP && pBuild->pPREP->n )ipreproc_Process(pBuild->pPREP,X,Y)

/*POD
=H The structure of the string table

The string table contains all string contansts that are used in the program.
This includes the single and multi line strings as well as symbols. (note that
even the variable name after the keyword T<next> is ignored but stored in the
string table).

The strings in the string table are stored one after the other zero character
terminated. Older version of ScriptBasic v1.0b21 and before stored string
constants zero character terminated. Because of this string constants containing
zero character were truncated (note that T<\000> creates a zero character in a
string constant in ScriptBasic).

The version v1.0b22 changed the way string constants are stored and the way
string table contains the strings. Each string is stored with its length.
The length is stored as a T<long> on T<sizeof(long)> bytes. This is followed by
the string. Whenever the code refers to a string the byte offset of the first
character of the string is stored in the built code. For example the very first
string starts on the 4. byte on 32 bit machines.

Altough the string length and zero terminating characters are redundant information
both are stored to avoid higher level mistakes causing problem.
CUT*/

/*POD
=H build_AllocateStringTable()

This function allocates space for the string table. The size of the
string table is already determined during syntax analysis. The determined
size should be enough. In some cases when there are repeated string constants
the calculated sizte is bigger than the real one. In that case the larger memory
is allocated and used, but only the really used part is written to the cache file.

If the program does not use any string constants then a dummy string table of length
one byte is allocated.

/*FUNCTION*/
void build_AllocateStringTable(pBuildObject pBuild,
                          int *piFailure
  ){
/*noverbatim

The first argument is the usual pointer to the "class" structure. The second argument
is the result value. It can have two values:

=itemize
=item T<BU_ERROR_SUCCESS> which is guaranteed zero, means the function was successful.
=item T<BU_ERROR_MEMORY_LOW> means the memory allocation function could not allocate the
neccessary memory
=noitemize

The string table is allocated using the function T<alloc_Alloc>. The string table
is pointed by the class variable T<StringTable>. The size of the table is stored in
T<cStringTable>

CUT*/

  /* allocate memory for the strings */
  if( pBuild->cbStringTable == 0L )pBuild->cbStringTable=1L; /* lets allocate some space */
  pBuild->StringTable = alloc_Alloc(pBuild->cbStringTable ,pBuild->pMemorySegment);
  pBuild->cbCollectedStrings = 0L;
  if( pBuild->StringTable == NULL ){
    REPORT("",0L,BU_ERROR_MEMORY_LOW,NULL);
    *piFailure = BU_ERROR_MEMORY_LOW;
    return;
    }
  *piFailure = BU_ERROR_SUCCESS;
  return;
  }
/*POD
=H build_StringIndex()

In the built code all the strings are references using the offset of the string
from the string table (See R<build_AllocateStringTable()>). This function calculates this value
for the string.

This function is used repetitively during the code building. Whenever a string index is
sought that is not in the string table yet the string is put into the table and the
index is returned.

If there is not enough space in the string table the function calls the system function
T<exit> and stops the process. This is rude especially in a multithread application
but it should not ever happen. If this happens then it is a serious internal error.
/*FUNCTION*/
unsigned long build_StringIndex(pBuildObject pBuild,
                                char *s,
                                long sLen
  ){
/*noverbatim
CUT*/
  unsigned long ulIndex;
  char *r;
  long lLen;

  ulIndex = 0;
  while( ulIndex < pBuild->cbCollectedStrings ){
    /* SUN Solaris generated segmentation fault for accessing this memory with 'lLen=*(long*)ptr' assignment */
    memcpy( &lLen, pBuild->StringTable + ulIndex , sizeof(long));
    ulIndex += sizeof(long);
    if( sLen == lLen && !memcmp(s,pBuild->StringTable + ulIndex,lLen) )return ulIndex;
    ulIndex += lLen;
    ulIndex++; /* step over the extra zchar at the end of the string */
    }
  /* the string is not in the table, we have to place it */
  r = pBuild->StringTable + pBuild->cbCollectedStrings;
  if( sLen+1+pBuild->cbCollectedStrings > pBuild->cbStringTable ){
    fprintf(stderr,"String table build up. Internal error!\n");
    exit(2000);
    }
  memcpy(r,&sLen,sizeof(long));
  r += sizeof(long);
  memcpy(r,s,sLen+1);
  ulIndex = pBuild->cbCollectedStrings + sizeof(long);
  pBuild->cbCollectedStrings += sLen + sizeof(long) + 1;
  return ulIndex;
  }

/*POD
=H build_Build_l()

This function converts an T<eNODE_l> list to T<cNODE> list in a loop.
This function is called from R<build_Build()> and from R<build_Build_r()>.

/*FUNCTION*/
int build_Build_l(pBuildObject pBuild,
                  peNODE_l Result
  ){
/*noverbatim
The function returns the error code, or zero in case of success.

CUT*/
  int iFailure;

  while( Result ){
    pBuild->CommandArray[Result->NodeId-1].OpCode = eNTYPE_LST;
    pBuild->CommandArray[Result->NodeId-1].Parameter.NodeList.actualm = Result->actualm ? Result->actualm->NodeId : 0;
    pBuild->CommandArray[Result->NodeId-1].Parameter.NodeList.rest = Result->rest ? Result->rest->NodeId : 0;
    if(  iFailure = build_Build_r(pBuild,Result->actualm) )return iFailure;
    Result = Result->rest;
    }

  return BU_ERROR_SUCCESS;
  }
/*POD
=H build_Build_r()

This function builds a single node. This actually means copiing the values
from the data structure created by the module T<expression>. The major
difference is that the pointers of the original structure are converted to
T<unsigned long>. Whenever a pointer pointed to a T<eNODE> the T<unsigned long>
will contain the T<NodeId> of the node. This ID is the same for the T<eNODE> and
for the T<cNODE> that is built from the T<eNODE>.

/*FUNCTION*/
int build_Build_r(pBuildObject pBuild,
                  peNODE Result
  ){
/*noverbatim

The node to be converted is passed by the pointer T<Result>. The return value is
the error code. It is zero (T<BU_ERRROR_SUCCESS>) in case of success.

When the node pointed by T<Result> references other nodes the function recursively
calls itself to convert the referenced nodes.

CUT*/
  pcNODE pThis;
  unsigned long *q;
  pLineSyntax pCommand;
  int iFailure;
  int j;

  if( Result == NULL )return BU_ERROR_SUCCESS;
  pThis = pBuild->CommandArray+Result->NodeId-1;
  pThis->OpCode = Result->OpCode;

  /* convert an array access node */
  if( Result->OpCode == eNTYPE_ARR || Result->OpCode == eNTYPE_SAR ){
    pThis->Parameter.Arguments.Argument = Result->Parameter.Arguments.Argument->NodeId;
    return build_Build_l(pBuild,Result->Parameter.Arguments.Argument);
    }

  /* Convert a user function node */
  if( Result->OpCode == eNTYPE_FUN ){
    if( Result->Parameter.UserFunction.pFunction->node == 0 ){
      REPORT("",0L,EX_ERROR_FUNCTION_NOT_DEFINED,Result->Parameter.UserFunction.pFunction->FunctionName);
      }
    pThis->Parameter.UserFunction.NodeId = Result->Parameter.UserFunction.pFunction->node;
    pThis->Parameter.UserFunction.Argument = Result->Parameter.UserFunction.Argument ?
                                        Result->Parameter.UserFunction.Argument->NodeId : 0;
    return build_Build_l(pBuild,Result->Parameter.UserFunction.Argument);
    }

  /* Convert a local/global variable node */
  if( Result->OpCode == eNTYPE_LVR || Result->OpCode == eNTYPE_GVR ){
    pThis->Parameter.Variable.Serial = Result->Parameter.Variable.Serial;
    return BU_ERROR_SUCCESS;
    }

  if( Result->OpCode == eNTYPE_DBL ){
    pThis->Parameter.Constant.dValue = Result->Parameter.Constant.Value.dValue;
    return BU_ERROR_SUCCESS;
    }

  if( Result->OpCode == eNTYPE_LNG ){
    pThis->Parameter.Constant.lValue = Result->Parameter.Constant.Value.lValue;
    return BU_ERROR_SUCCESS;
    }

  if( Result->OpCode == eNTYPE_STR ){
    pThis->Parameter.Constant.sValue = build_StringIndex(pBuild,Result->Parameter.Constant.Value.sValue,Result->Parameter.Constant.sLen);
    return BU_ERROR_SUCCESS;
    }

  q = pBuild->pEx->Binaries;

  while( *q && *q != (unsigned)pThis->OpCode )q+=2;
  if( *q ){
    pThis->Parameter.Arguments.Argument = Result->Parameter.Arguments.Argument->NodeId;
    return build_Build_l(pBuild,Result->Parameter.Arguments.Argument);
    }

  q = pBuild->pEx->Unaries;
  while( *q && *q != (unsigned)pThis->OpCode )q++;
  if( *q ){
    pThis->Parameter.Arguments.Argument = Result->Parameter.Arguments.Argument->NodeId;
    return build_Build_l(pBuild,Result->Parameter.Arguments.Argument);
    }

  pCommand = pBuild->pEx->Command;
  while( pCommand && pCommand->CommandOpCode != 0 && pCommand->CommandOpCode != pThis->OpCode )pCommand++;

#define NEXT_ARGUMENT if( Result->Parameter.CommandArgument.next ){\
            if( pThis->Parameter.CommandArgument.next = Result->Parameter.CommandArgument.next->NodeId ){\
              pThis = pBuild->CommandArray+Result->Parameter.CommandArgument.next->NodeId-1;\
              pThis->OpCode = eNTYPE_CRG;\
              Result = Result->Parameter.CommandArgument.next;\
              }\
            break;\
            }\
          else{\
            pThis->Parameter.CommandArgument.next = 0;\
            return BU_ERROR_SUCCESS;\
            }

  if( pCommand && pCommand->CommandOpCode ){
    for( j=0 ; j < MAX_LEXES_PER_LINE && pCommand->lexes[j].type ; j++ ){
      switch( pCommand->lexes[j].type ){
        /****************************************************************/
        /* lex types that do not generate any parameter for the command */
        /****************************************************************/
        case  EX_LEX_NSYMBOL:
        case  EX_LEX_SET_NAME_SPACE:
        case  EX_LEX_CHARACTER:
        case  EX_LEX_LOCAL:
        case  EX_LEX_LOCALL:
        case  EX_LEX_FUNCTION:
        case  EX_LEX_THIS_FUNCTION:
        case  EX_LEX_LABEL_DEFINITION:
        case  EX_LEX_STAR:
        case  EX_LEX_NOEXEC:
        case  EX_LEX_COME_FORWARD:
        case  EX_LEX_COME_BACK:
        case  EX_LEX_LOCAL_END:
          break;

        /****************************************************************/
        /*      lex types that generate parameters for the command      */
        /****************************************************************/

        case  EX_LEX_LABEL:
        case  EX_LEX_GO_FORWARD:
        case  EX_LEX_GO_BACK:
          pThis->Parameter.CommandArgument.Argument.pNode =
            Result->Parameter.CommandArgument.Argument.pLabel ?
            Result->Parameter.CommandArgument.Argument.pLabel->node : 0;
          NEXT_ARGUMENT;

        case EX_LEX_LVAL:
        case EX_LEX_EXP:
          pThis->Parameter.CommandArgument.Argument.pNode = 
            Result->Parameter.CommandArgument.Argument.pNode->NodeId;
          iFailure = build_Build_r(pBuild,Result->Parameter.CommandArgument.Argument.pNode);
          if( iFailure )return iFailure;
          NEXT_ARGUMENT;

        case EX_LEX_LVALL:
        case EX_LEX_EXPL:
          pThis->Parameter.CommandArgument.Argument.pNode = 
            Result->Parameter.CommandArgument.Argument.pNodeList->NodeId;
          iFailure = build_Build_l(pBuild,Result->Parameter.CommandArgument.Argument.pNodeList);
          if( iFailure )return iFailure;
          NEXT_ARGUMENT;

        case EX_LEX_STRING:
        case EX_LEX_SYMBOL:
        case EX_LEX_ASYMBOL:
          pThis->Parameter.CommandArgument.Argument.szStringValue = 
             build_StringIndex(pBuild,Result->Parameter.CommandArgument.Argument.szStringValue,Result->Parameter.CommandArgument.sLen);
          NEXT_ARGUMENT;

        case EX_LEX_ARG_NUM:
        case EX_LEX_LOCAL_START:
        case EX_LEX_LONG:
          pThis->Parameter.CommandArgument.Argument.lLongValue =
             Result->Parameter.CommandArgument.Argument.lLongValue;
          NEXT_ARGUMENT;

        case EX_LEX_DOUBLE:
          pThis->Parameter.CommandArgument.Argument.dDoubleValue =
             Result->Parameter.CommandArgument.Argument.dDoubleValue;
          NEXT_ARGUMENT;

        default:
          fprintf(stderr,"This is a serious internal error. STOP\n");
          exit(1000);
        }       
      }
    return BU_ERROR_SUCCESS;
    }
  /* this is some built in function */
  pThis->OpCode = Result->OpCode;
  if( Result->Parameter.Arguments.Argument )
    pThis->Parameter.Arguments.Argument = Result->Parameter.Arguments.Argument->NodeId;
  else
    pThis->Parameter.Arguments.Argument = 0; /* the function was called without argument */
  return build_Build_l(pBuild,Result->Parameter.Arguments.Argument);
  }

/*POD
=H build_Build()

This is the main entry function for this module. This function initializes the
class variable pointed by T<pBuild> and calls R<build_Build_l()> to build up the 
command list.
/*FUNCTION*/
int build_Build(pBuildObject pBuild
  ){
/*noverbatim
CUT*/
  int iFailure;

  pBuild->cbFTable = 0;
  pBuild->cbVTable = 0;
  pBuild->FTable = NULL;
  pBuild->VTable = NULL;

  pBuild->NodeCounter = pBuild->pEx->NodeCounter;
  pBuild->cGlobalVariables = pBuild->pEx->cGlobalVariables;
  pBuild->report = pBuild->pEx->report;
  pBuild->reportptr = pBuild->pEx->reportptr;
  if( pBuild->pEx->NodeCounter ==0 ){
    /* Kevin Landman [KevinL@pptvision.com] recognized that here pMemorySegment
       remains uninitialized and in a multi-thread embedding application
       calling scriba_destroy will miserably crash. */
    pBuild->pMemorySegment = NULL;
    REPORT("",0L,BU_ERROR_NO_CODE,NULL);
    return BU_ERROR_NO_CODE;
    }
  pBuild->pMemorySegment = alloc_InitSegment(pBuild->memory_allocating_function,
                                             pBuild->memory_releasing_function);
  if( pBuild->pMemorySegment == NULL ){
    REPORT("",0L,BU_ERROR_MEMORY_LOW,NULL);
    return BU_ERROR_MEMORY_LOW;
    }
  pBuild->CommandArray = alloc_Alloc(sizeof(cNODE) * pBuild->NodeCounter , pBuild->pMemorySegment );
  if( pBuild->CommandArray == NULL ){
    REPORT("",0L,BU_ERROR_MEMORY_LOW,NULL);
    return BU_ERROR_MEMORY_LOW;
    }
  pBuild->cbStringTable = pBuild->pEx->cbStringTable;
  build_AllocateStringTable(pBuild,&iFailure);
  if( iFailure )return iFailure;

  if( (iFailure = build_CreateVTable(pBuild)) != BU_ERROR_SUCCESS )return iFailure;
  if( (iFailure = build_CreateFTable(pBuild)) != BU_ERROR_SUCCESS )return iFailure;


  pBuild->StartNode = pBuild->pEx->pCommandList->NodeId;
  return build_Build_l(pBuild, pBuild->pEx->pCommandList);
  }

static VersionInfo sVersionInfo;
/*POD
=H build_MagicCode()

This is a simple and magical calculation that converts any ascii date to
a single unsigned long. This is used as a magic value in the binary format
of the compiled basic code to help distinguish incompatible versions.

This function also fills in the sVersion static struct that contains the version
info.
/*FUNCTION*/
unsigned long build_MagicCode(pVersionInfo p
  ){
/*noverbatim
CUT*/
  unsigned long magic;
  unsigned char *s;

  s = (unsigned char *)__DATE__;

  magic = *s++;
  magic += *s++;
  magic += *s++;
  magic -= 199;
  if( magic > 9 )magic -= 10;
  if( magic > 15 )magic -=4;
  if( magic == 16 )magic =15;
  s++;
  magic <<= 8;
  magic |= ((unsigned long)*s++) << 4;
  magic |= ((unsigned long)*s++) << 0;
  s++;
  magic |= ((unsigned long)*s++) << 24;
  magic |= ((unsigned long)*s++) << 20;
  magic |= ((unsigned long)*s++) << 16;
  magic |= ((unsigned long)*s++) << 12;

  sVersionInfo.Build = SCRIPTBASIC_BUILD;
  sVersionInfo.Date = magic;
  sVersionInfo.MagicCode = MAGIC_CODE;
  sVersionInfo.VersionHigh = VERSION_HIGH;
  sVersionInfo.VersionLow = VERSION_LOW;
  sVersionInfo.MyVersionHigh = MYVERSION_HIGH;
  sVersionInfo.MyVersionLow = MYVERSION_LOW;
  memcpy(sVersionInfo.Variation, VARIATION, 9);
  if( p ){
    p->Build = SCRIPTBASIC_BUILD;
    p->Date = magic;
    p->MagicCode = MAGIC_CODE;
    p->VersionHigh = VERSION_HIGH;
    p->VersionLow = VERSION_LOW;
    p->MyVersionHigh = MYVERSION_HIGH;
    p->MyVersionLow = MYVERSION_LOW;
    memcpy(p->Variation, VARIATION, 9);
    }
  return magic;
  }

/*POD
=H build_SaveCCode()

This function saves the binary code of the program into the file
given by the name T<szFileName> in C programming language format.

The saved file can be compiled using a C compiler on the platform it was
saved. The generated C file is not portable.

/*FUNCTION*/
void build_SaveCCode(pBuildObject pBuild,
                    char *szFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  unsigned long i,j;
  unsigned char *s;

  fp = file_fopen(szFileName,"w");
  if( fp == NULL )return;

  fprintf(fp,"/* FILE: %s\n",szFileName);
  fprintf(fp,"   This file contains the binary code of a ScriptBasic program\n");
  fprintf(fp,"   To run this file you have to compile it to object file and\n");
  fprintf(fp,"   link it with scribast.lib or whatever the library code is\n");
  fprintf(fp,"   called on your platform.\n");
  fprintf(fp,"*/\n");

  fprintf(fp,"unsigned long ulGlobalVariables=%ld;\n",pBuild->cGlobalVariables);
  fprintf(fp,"unsigned long ulNodeCounter=%ld;\n",pBuild->NodeCounter);
  fprintf(fp,"unsigned long ulStartNode=%ld;\n",pBuild->StartNode);
  fprintf(fp,"unsigned long ulStringTableSize=%ld;\n",pBuild->cbStringTable);

  fprintf(fp,"unsigned char szCommandArray[] ={\n");
  for( i=0 ; i < pBuild->NodeCounter ; i++ ){
     s = (unsigned char *)(pBuild->CommandArray+i);
     for( j=0 ; j < sizeof(cNODE) ; j++ )
       fprintf(fp,"0x%02X, ",s[j]);
     fprintf(fp,"\n");
     }
  fprintf(fp,"0x00 };\n");

  fprintf(fp,"char szStringTable[]={\n");
  s = (unsigned char *)pBuild->StringTable;
  for( i=0 ; i < pBuild->cbStringTable ; i++ ){
    fprintf(fp,"0x%02X, ",s[i]);
    if( i%16 == 15 )fprintf(fp,"\n");
    }
  fprintf(fp,"\n0x00 };\n");
  fprintf(fp,"#ifdef WIN32\n");
  fprintf(fp,"main(int argc, char *argv[]){stndlone(argc,argv);}\n");
  fprintf(fp,"#else\n");
  fprintf(fp,"char **_environ;\n");
  fprintf(fp,"main(int argc, char *argv[], char *env[]){stndlone(argc,argv,env);}\n");
  fprintf(fp,"#endif\n");

  fprintf(fp,"/*End of file %s */",szFileName);
  file_fclose(fp);
  }

/*POD
=H build_SaveCorePart()

This function saves the binary content of the compiled file into an
already opened file. This is called from both T<build_SaveCode> and from
T<build_SaveECode>.

Arguments:
=itemize
=item T<pBuild> is the build object
=item T<fp> is the T<FILE *> file pointer to an already binary write opened (T<"wb">) file.
=noitemize

The file T<fp> is not closed even if error occures while writing the file.

/*FUNCTION*/
int build_SaveCorePart(pBuildObject pBuild,
                       FILE *fp,
                       unsigned long fFlag
  ){
/*noverbatim
The function returns T<BU_ERROR_SUCCESS> (zero) if there was no error or T<BU_ERROR_FAIL> if the function fails
writing the file.
CUT*/
  unsigned char longsize;
/* perform a file write and return error if there is some error writing the file */
#define MYFWRITE(buffer,pieces,size,fp) if( fwrite(buffer,size,pieces,fp) != pieces ){\
                                        return BU_ERROR_FAIL;\
                                        }
  longsize = sizeof(long)+0x30;
  MYFWRITE((void *)&longsize,1,1,fp)
  build_MagicCode(NULL);

  MYFWRITE((void *)&sVersionInfo,1,sizeof(sVersionInfo),fp);

  MYFWRITE((void *)&(pBuild->cGlobalVariables),1,sizeof(unsigned long),fp);
  MYFWRITE((void *)&(pBuild->NodeCounter),1,sizeof(unsigned long),fp);
  MYFWRITE((void *)&(pBuild->StartNode),1,sizeof(unsigned long),fp);
  MYFWRITE((void *)&(pBuild->cbCollectedStrings),1,sizeof(unsigned long),fp);
  MYFWRITE((void *)pBuild->CommandArray,pBuild->NodeCounter,sizeof(cNODE),fp);
  MYFWRITE((void *)pBuild->StringTable,pBuild->cbCollectedStrings,sizeof(char),fp);

  /* We put these tables here together with the counters. This lets us to strip off
     the symbolic information in case it is not needed. */
  if( fFlag & BU_SAVE_FTABLE ){
    MYFWRITE((void *)&(pBuild->cbFTable),1,sizeof(unsigned long),fp);
    MYFWRITE((void *)pBuild->FTable,pBuild->cbFTable,sizeof(char),fp);
    }
  if( fFlag & BU_SAVE_VTABLE ){
    MYFWRITE((void *)&(pBuild->cbVTable),1,sizeof(unsigned long),fp);
    MYFWRITE((void *)pBuild->VTable,pBuild->cbVTable,sizeof(char),fp);
    }
#undef MYFWRITE
  return BU_ERROR_SUCCESS;
  }

/*POD
=H build_SaveCore()

This function saves the binary content of the compiled file into an
already opened file. This is called from both T<build_SaveCode> and from
T<build_SaveECode>.

Arguments:
=itemize
=item T<pBuild> is the build object
=item T<fp> is the T<FILE *> file pointer to an already binary write opened (T<"wb">) file.
=noitemize

The file T<fp> is not closed even if error occures while writing the file.

/*FUNCTION*/
int build_SaveCore(pBuildObject pBuild,
                   FILE *fp
  ){
/*noverbatim
The function returns T<BU_ERROR_SUCCESS> (zero) if there was no error or T<BU_ERROR_FAIL> if the function fails
writing the file.
CUT*/
  return build_SaveCorePart(pBuild,fp,BU_SAVE_FTABLE|BU_SAVE_VTABLE);
  }

/*POD
=H build_SaveCode()

This function saves the binary code of the program into the file
given by the name T<szFileName>.

This version is hard wired saving the code into an operating system
file because it uses T<fopen>, T<fclose> and T<fwrite>. Later versions
may use other compatible functions passed as argument and thus allowing
output redirection to other storage media (a database for example).

However I think that this code is quite simple and therefore it is easier
to rewrite the whole function along with R<build_LoadCode()> for other storage
media than writing an interface function.

The saved binary code is NOT portable. It saves the internal values
as memory image to the disk. It means that the size of the code depends
on the actual size of long, char, int and other types. The byte ordering
is also system dependant.

The saved binary code can only be loaded with the same version, and build of
the program, therefore it is vital to distinguish each compilation of
the program. To help the recognition of the different versions, the code starts
with a version structure.

The very first byte of the code contains the size of the long on the target machine.
If this is not correct then the code was created on a different processor and the code
is incompatible.

The version info structure has the following fileds:
=itemize
=item T<MagicCode> is a magic constant. This contains the characters BAS and a character 1A that
stops output to screen on DOS operating systems.
=item T<VersionHigh> The high part of the version of the STANDARD version.
=item T<VersionLow> The low part of the version of the STANDARD version.
=item T<MyVersionHigh> The high part of the version of the variation.
This is always zero for the STANDARD version.
=item T<MyVersionLow>  The low part of the version of the variation.
This is always zero for the STANDARD version.
=item T<Build> A build code which is automatically calculated from the compilation date.
=item T<Variation> 8 characters (NOT ZERO TERMINATED!) naming the version "STANDARD" for the
STANDARD version (obvious?)
=noitemize

/*FUNCTION*/
int build_SaveCode(pBuildObject pBuild,
                   char *szFileName
  ){
/*noverbatim
The function returns zero on success (T<BU_ERROR_SUCCESS>) and T<BU_ERROR_FAIL>
if the code could not be saved.
CUT*/
  FILE *fp;

  /* we just do not like a zero length string table */
  if( ! pBuild->cbCollectedStrings )pBuild->cbCollectedStrings = 1;

  fp = file_fopen(szFileName,"wb");
  if( fp == NULL )return BU_ERROR_FAIL;

  if( pBuild->FirstUNIXline )fprintf(fp,pBuild->FirstUNIXline);

  build_SaveCore(pBuild,fp);
  file_fclose(fp);
  return BU_ERROR_SUCCESS;
  }

/*POD
=H build_SaveECode()

This function saves the binary code of the program into the file
given by the name T<szFileName> in exe format.

This is actually nothing but the copy of the original interpreter file and
the binary code of the BASIC program appended to it and some extra information
at the end of the file to help the reader to find the start of the binary 
BASIC program when it tries to read the exe file.

/*FUNCTION*/
void build_SaveECode(pBuildObject pBuild,
                     char *pszInterpreter,
                     char *szFileName
  ){
/*noverbatim
CUT*/
  FILE *fp,*fi;
  int ch;
  long lCodeStart;
  /* SCRIPTBASIC */
  char magics[11+sizeof(long)];

#if _WIN32
  /* under WIN32 we know it better, igore the argument */
  pszInterpreter = _pgmptr;
#endif

  /* copy the original exe file to the output */
  fi = file_fopen(pszInterpreter,"rb");

  if( fi == NULL ){
    REPORT("",0L,BU_ERROR_ECODE_INPUT,NULL);
    return;
    }

  /* if the interpreter can be opened then open the output file */
  fp = file_fopen(szFileName,"wb");
  if( fp == NULL ){
    file_fclose(fi);
    REPORT("",0L,BU_ERROR_FAIL,NULL);
    return;
    }

  while( EOF != (ch=getc(fi)) ){
    putc(ch,fp);
    }
  file_fclose(fi);

  lCodeStart = ftell(fp);

  build_SaveCore(pBuild,fp);

  /* print this string there to be sure that this is a scriptbasic code */
  strcpy(magics,"SCRIPTBASIC");
  /* this is a long telling where the binary code starts */
  memcpy(magics+11,&lCodeStart,sizeof(long));

  file_fwrite(magics,1,11+sizeof(long),fp);

  file_fclose(fp);
  }

/*POD
=H build_GetExeCodeOffset()

This function checks that the actually running exe contains the binary BASIC program
attached to its end. It returns zero if not, otherwise it returns 1.

=itemize
=item The argument T<pszInterpreter> should be T<argv[0]> thus the code can open the executable
file and check if it really contains the BASIC code
=item T<plOffset> should point to a long variable ready to recieve the file offset where the BASIC
code starts
=item T<plEOFfset> should point to a long variable ready to receive the file offset where the
BASIC code finishes. This is the position of the last byte belonging to the BASIC code, thus if
T<ftell(fp) >>T< *plEOFfset> means the file pointer is after the code and should treat it as EOF
condition when reading the BASIC program code.
=noitemize

It is guaranteed that both T<*plOffset> and T<*plEOFfset> will be set to T<0> (zero) if the file
proves to be a standard BASIC interpreter without appended BASIC code.

/*FUNCTION*/
int build_GetExeCodeOffset(char *pszInterpreter,
                            long *plOffset,
                            long *plEOFfset
  ){
/*noverbatim
CUT*/
  FILE *fp;
  char magics[11+sizeof(long)];
  long lOf;

#if _WIN32
  /* under WIN32 we know it better, igore the argument */
  pszInterpreter = _pgmptr;
#endif
  /* they are guaranteed to be zero in case the exe is a simple interpreter */
  *plOffset = *plEOFfset = 0L;
  /* open the executable file */
  fp = file_fopen(pszInterpreter,"rb");
  /* if it can not be read then this can not be, well... this is some weird error */
  if( fp == NULL )return 0;
  /* The executable file created by the option -Eo finishes with the 
     characters 'SCRIPTBASIC' (11 characters)  and a long number containing
     the offset where the code start */
  lOf = 11 + sizeof(long);
  /* seek it to where the magic characters are expected to start */
  fseek(fp,-lOf,SEEK_END);
  *plEOFfset = ftell(fp) - 1;
  /* read the 11 characters 'SCRIPTBASIC' and the long containing the offset */
  file_fread(magics,1,11+sizeof(long),fp);
  file_fclose(fp);
  if( memcmp(magics,"SCRIPTBASIC",11) ){
    *plEOFfset = 0L;
    return 0L;
    }
  memcpy(plOffset,magics+11,sizeof(long));
  return 1;
  }

/*POD
=H build_LoadCore()

This function loads the binary code from an opened file.

Arguments:

=itemize
=item T<pBuild> is the build object
=item T<szFileName> is the name of the file that is opened. Needed for reporting purposes.
=item T<fp> opened T<FILE *> file pointer opened for binary reading (aka T<"rb">), and positioned where the
BASIC code starts.
=item T<lEOFfset> should be the position of the last byte that belongs to the BASIC code so that T<ftell(fp)>>T<lEOFfset>
is treated as EOF condition. If this value is zero that means that the BASIC code is contained in the file until the
physical end of file.
=noitemize

/*FUNCTION*/
void build_LoadCore(pBuildObject pBuild,
                    char *szFileName,
                    FILE *fp,
                    long lEOFfset
  ){
/*noverbatim
Note that the program does not return error code, but calls the reporting function to report error. The file T<fp> is not closed in the
function even if error has happened during reading.
CUT*/
  unsigned long mc;
  unsigned long longsize;
  int ch;

#define CORRUPTFILE {REPORT(szFileName,0L,BU_ERROR_FILE_CORRUPT,NULL);return;}
#define CHECKEOF() feof(fp) || (lEOFfset && lEOFfset < ftell(fp))
#define ASSERTEOF if( CHECKEOF() )CORRUPTFILE

  ASSERTEOF
  longsize = ch = fgetc(fp);
  if( CHECKEOF() )CORRUPTFILE
  /* If the first character of the file is # then it should start as
     text file and it should be something like #!/usr/bin/scriba
     This lasts until \n on UNIX */
  if( longsize == '#' ){
    ch = fgetc(fp);
    if( ch != '!' )CORRUPTFILE
    while( ch != EOF && ch != '\n' )ch = fgetc(fp);
    if( ch == '\n' )ch = fgetc(fp);
    ASSERTEOF
    longsize = ch;
    }
  if( longsize != sizeof(long)+0x30 )CORRUPTFILE

  mc = build_MagicCode(NULL);

  fread((void *)&sVersionInfo,1,sizeof(sVersionInfo),fp);
  if( 
  sVersionInfo.Build != SCRIPTBASIC_BUILD ||
  sVersionInfo.MagicCode != MAGIC_CODE ||
  sVersionInfo.VersionHigh != VERSION_HIGH ||
  sVersionInfo.VersionLow != VERSION_LOW ||
  sVersionInfo.MyVersionHigh != MYVERSION_HIGH ||
  sVersionInfo.MyVersionLow != MYVERSION_LOW ||
  strncmp(sVersionInfo.Variation, VARIATION, 8) 
  )CORRUPTFILE

  fread((void *)&(pBuild->cGlobalVariables),sizeof(unsigned long),1,fp);
  ASSERTEOF
  fread((void *)&(pBuild->NodeCounter),sizeof(unsigned long),1,fp);
  ASSERTEOF
  fread((void *)&(pBuild->StartNode),sizeof(unsigned long),1,fp);
  ASSERTEOF

  pBuild->CommandArray = alloc_Alloc(sizeof(cNODE) * pBuild->NodeCounter , pBuild->pMemorySegment );
  if( pBuild->CommandArray == NULL ){
    REPORT(szFileName,0L,BU_ERROR_MEMORY_LOW,NULL);
    return;
    }

  fread((void *)&(pBuild->cbStringTable),1,sizeof(unsigned long),fp);
  ASSERTEOF

  pBuild->StringTable = alloc_Alloc(pBuild->cbStringTable ? pBuild->cbStringTable : 1 ,pBuild->pMemorySegment);
  if( pBuild->StringTable == NULL ){
    REPORT(szFileName,0L,BU_ERROR_MEMORY_LOW,NULL);
    return;
    }
  fread((void *)pBuild->CommandArray,pBuild->NodeCounter,sizeof(cNODE),fp);
  ASSERTEOF
  if( pBuild->cbStringTable )
    fread((void *)pBuild->StringTable,pBuild->cbStringTable,sizeof(char),fp);

  /* the file may close here in case the user defined function and global variable tables are not present */
  if( feof(fp) )return;

  fread((void *)&(pBuild->cbFTable),1,sizeof(unsigned long),fp);
  if( feof(fp) ){
    pBuild->cbFTable = 0;
    return;
    }
  if( pBuild->cbFTable ){
    pBuild->FTable = alloc_Alloc(pBuild->cbFTable,pBuild->pMemorySegment);
    if( pBuild->FTable == NULL ){
      REPORT(szFileName,0L,BU_ERROR_MEMORY_LOW,NULL);
      return;
      }
    longsize=fread((void *)pBuild->FTable,sizeof(char),pBuild->cbFTable,fp);
    if( longsize != pBuild->cbFTable )CORRUPTFILE
    if( feof(fp) )return;
    }else pBuild->FTable = NULL;

  fread((void *)&(pBuild->cbVTable),1,sizeof(unsigned long),fp);
  if( pBuild->cbVTable ){
    if( feof(fp) )return;
    pBuild->VTable = alloc_Alloc(pBuild->cbVTable,pBuild->pMemorySegment);
    if( pBuild->VTable == NULL ){
      REPORT(szFileName,0L,BU_ERROR_MEMORY_LOW,NULL);
      return;
      }
    }else pBuild->VTable = NULL;
  if( fread((void *)pBuild->VTable,sizeof(char),pBuild->cbVTable,fp) != pBuild->cbVTable)CORRUPTFILE

  }

/*POD
=H build_LoadCodeWithOffset()

For detailed definition of the binary format see the code and the documentation of
R<build_SaveCode()>

In case the file is corrupt the function reports error.

/*FUNCTION*/
void build_LoadCodeWithOffset(pBuildObject pBuild,
                              char *szFileName,
                              long lOffset,
                              long lEOFfset
  ){
/*noverbatim
CUT*/
  FILE *fp;

  pBuild->pMemorySegment = alloc_InitSegment(pBuild->memory_allocating_function,
                                             pBuild->memory_releasing_function);
  if( pBuild->pMemorySegment == NULL ){
    REPORT(szFileName,0L,BU_ERROR_MEMORY_LOW,NULL);
    return;
    }

  fp = file_fopen(szFileName,"rb");

  if( NULL == fp ){
    /* borrow an error code from the reader */
    REPORT(szFileName,0L,READER_ERROR_FILE_OPEN,NULL);
    return;
    }
  fseek(fp,lOffset,SEEK_SET);
  if( fp == NULL )CORRUPTFILE
  build_LoadCore(pBuild,szFileName,fp,lEOFfset);
  file_fclose(fp);
  return;
  }

/*POD
=H build_LoadCode()

For detailed definition of the binary format see the code and the documentation of
R<build_SaveCode()>

In case the file is corrupt the function reports error.

/*FUNCTION*/
void build_LoadCode(pBuildObject pBuild,
                    char *szFileName
  ){
/*noverbatim
CUT*/
  build_LoadCodeWithOffset(pBuild,szFileName,0L,0L);
  }

/*POD
=H build_IsFileBinaryFormat()

This function test a file reading its first few characters and decides
if the file is binary format of a basic program or not.

/*FUNCTION*/
int build_IsFileBinaryFormat(char *szFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  unsigned long mc;
  int ret,ch;
  unsigned char longsize;

  if( szFileName == NULL )return 0;/*no file is not binary*/
  ret = 1;
  fp = file_fopen(szFileName,"rb");
  if( fp == NULL )return 0;

  longsize = fgetc(fp);
  if( longsize == '#' ){
    ch = fgetc(fp);
    if( ch != '!' )ret = 0; else {
      while( ch != EOF && ch != '\n' )ch = fgetc(fp);
      if( ch == '\n' )ch = fgetc(fp);
      if( ch == EOF )ret = 0;
      longsize = ch;
      }
    }
  if( longsize != sizeof(long)+0x30 )ret=0;

  mc = build_MagicCode(NULL);

  fread((void *)&sVersionInfo,1,sizeof(sVersionInfo),fp);
  if( 
  sVersionInfo.Build != SCRIPTBASIC_BUILD ||
  sVersionInfo.MagicCode != MAGIC_CODE ||
  sVersionInfo.VersionHigh != VERSION_HIGH ||
  sVersionInfo.VersionLow != VERSION_LOW ||
  sVersionInfo.MyVersionHigh != MYVERSION_HIGH ||
  sVersionInfo.MyVersionLow != MYVERSION_LOW ||
  strncmp(sVersionInfo.Variation, VARIATION, 8) 
  )ret = 0;

  file_fclose(fp);
  return ret;
  }
/*POD
=H build_pprint()

This is a debug function that prints the build code into a file.

This function is not finished and the major part of it is commented out using T<#if 0> construct.
/*FUNCTION*/
void build_pprint(pBuildObject pBuild,
                  FILE *f
  ){
/*noverbatim
CUT*/
  unsigned long lIndex;
  pcNODE pThis;
  int i;
  extern LexNASymbol CSYMBOLS[];

  fprintf(f,"Start node is %d\n",pBuild->StartNode);

  for( lIndex = 0 ; lIndex < pBuild->NodeCounter ; lIndex ++ ){
    pThis = pBuild->CommandArray+lIndex;
    fprintf(f,"%d ",lIndex+1 );
    /* convert an array access node */
    if( pThis->OpCode == eNTYPE_ARR ){
      fprintf(f,"Array access\n");
      continue;
      }
    if( pThis->OpCode == eNTYPE_SAR ){
      fprintf(f,"Associative array access\n");
      continue;
      }

  /* this is a list node */
  if( pThis->OpCode == eNTYPE_LST ){
    fprintf(f,"List node\n");
    fprintf(f," car=%ld\n",pThis->Parameter.NodeList.actualm);
    fprintf(f," cdr=%ld\n",pThis->Parameter.NodeList.rest);
    continue;
    }

  /* Convert a user function node */
  if( pThis->OpCode == eNTYPE_FUN ){
    fprintf(f,"User function\n");
    fprintf(f," Starts at node %ld\n",pThis->Parameter.UserFunction.NodeId);
    fprintf(f," Actual argument list root node %ld\n",pThis->Parameter.UserFunction.Argument);
    continue;
    }

  /* Convert a local/global variable node */
  if( pThis->OpCode == eNTYPE_LVR || pThis->OpCode == eNTYPE_GVR ){
    fprintf(f,"%s variable serial=%d\n", (pThis->OpCode == eNTYPE_LVR ? "local" : "global"),pThis->Parameter.Variable.Serial);
    continue;
    }

  for( i = 0 ; CSYMBOLS[i].Symbol ; i++ )
    if( CSYMBOLS[i].Code == pThis->OpCode )break;
  if( CSYMBOLS[i].Code == pThis->OpCode ){
    fprintf(f,"  %s\n",CSYMBOLS[i].Symbol);
    continue;
    }
  if( pThis->OpCode == eNTYPE_DBL ){
    fprintf(f," Double value %lf\n",pThis->Parameter.Constant.dValue);
    continue;
    }

  if( pThis->OpCode == eNTYPE_LNG ){
    fprintf(f," Long value %ld\n",pThis->Parameter.Constant.lValue);
    continue;
    }

  if( pThis->OpCode == eNTYPE_STR ){
    fprintf(f," Constant string node id=%d\n", pThis->Parameter.Constant.sValue);
    continue;
    }

  switch( pThis->OpCode ){
    case  eNTYPE_ARR: fprintf(f,", ARRAY ACCESS\n"); break;
    case  eNTYPE_SAR: fprintf(f,", SARAY ACCESS\n"); break;
    case  eNTYPE_FUN: fprintf(f,", FUNCTION CALL\n"); break;
    case  eNTYPE_LVR: fprintf(f,", LOCAL VAR\n"); break;
    case  eNTYPE_GVR: fprintf(f,", GLOBAL VAR\n"); break;
    case  eNTYPE_DBL: fprintf(f,", DOUBLE\n"); break;
    case  eNTYPE_LNG: fprintf(f,", LONG\n"); break;
    case  eNTYPE_STR: fprintf(f,", STRING\n"); break;
    case  eNTYPE_LST: fprintf(f,", LIST\n"); break;
    case  eNTYPE_CRG: fprintf(f,", COMMAND ARG %ld -> %ld\n",  pThis->Parameter.CommandArgument.Argument.pNode,pThis->Parameter.CommandArgument.next);
                      break;

      default:
      fprintf(f,", %d\n",pThis->OpCode);
      }
    }
  }

/*Mitchell Greess [m.greess@solutions-atlantic.com]:
This is a utility function which correctly determines the size of a
table item accounting for alignment issues on Solaris.
*/
static long build_TableItemBytes(char *SymbolName){
  long len;

  /* Watch for alignment on Solaris */
  len = strlen(SymbolName) + 1 + sizeof(long);
  if (len % sizeof(long)) len += sizeof(long) - (len % sizeof(long));
  return len;
  }

/*
This is a callback function used to traverse over the symbol table of user functions
and of global variables. The void pointer points to a long variable that is used to calculate
the final size of the table used to store the compacted symbol table saved to file.

For each symbol the function counts the length of the string plus the terminating zero plus the
sizeof(long).
*/
static void build_CountSymbolBytes(char *SymbolName, void *SymbolValue, void *f){
  long *pL;
  pL = (long *)f;
  *pL += build_TableItemBytes(SymbolName);
  }

/*
This is a callback function that is used to put the strings and the values of the
user function symbol table entries into the memory allocated to store the functions
and their entry point value in a single memory space.
*/
static void build_PutFTableItem(char *SymbolName, void *SymbolValue, void *f){
  pSymbolUF pF;
  pSymbolLongTable pT;
  char **pC;

  pC = (char **)f;
  pF = (pSymbolUF)SymbolValue;
  pT = (pSymbolLongTable)*pC;
  pT->value = pF->node;
  strcpy(pT->symbol,SymbolName);
  *pC += build_TableItemBytes(SymbolName);
  }

/*
This is a callback function that is used to put the strings and the values of the
global variables symbol table entries into the memory allocated to store the variables
and their serial number in a single memory space.
*/
static void build_PutVTableItem(char *SymbolName, void *SymbolValue, void *f){
  pSymbolVAR pV;
  pSymbolLongTable pT;
  char **pC;

  pC = (char **)f;
  pV = (pSymbolVAR)SymbolValue;
  pT = (pSymbolLongTable)*pC;
  pT->value = pV->Serial;
  strcpy(pT->symbol,SymbolName);
  *pC += build_TableItemBytes(SymbolName);
  }

/*POD
=H build_CreateFTable()

When the binary code of the BASIC program is saved to disk the symbol table of the user
defined functions and the symbol table of global variables is also saved. This may be needed
by some applications that embed ScriptBasic and want to call specific function or alter global variables
of a given name from the embedding C code. To do this they need the serial number of the global variable
or the entry point of the function. Therefore ScriptBasic v1.0b20 and later can save these two tables into
the binary code.

The format of the tables is simple optimized for space and for simplicity of generation. They are stored
first in a memory chunk and then written to disk just as a series of bytes.

The format is

=verbatim
long      serial number of variable or entry point of the function
zchar     zero character terminated symbol
=noverbatim

This is easy to save and to load. Searching for it is a bit slow. Embedding applications usually
have to search for the values only once, store the serial number/entry point value
in their local variable and use the value.

The function T<CreateFTable> converts the symbol table of user defined function
collected by symbolic analysis into a single memory chunk.

The same way R<build_CreateVTable()> converts the symbol table of global variables
collected by symbolic analysis into a single memory chunk.

/*FUNCTION*/
int build_CreateFTable(pBuildObject pBuild
  ){
/*noverbatim
CUT*/
  char *p;

  pBuild->cbFTable = 0;
  sym_TraverseSymbolTable(pBuild->pEx->UserFunctions,
                          build_CountSymbolBytes,
                          &(pBuild->cbFTable));
  if( pBuild->cbFTable == 0 ){
    pBuild->FTable = NULL;
    return BU_ERROR_SUCCESS;
    }
  pBuild->FTable = alloc_Alloc(pBuild->cbFTable,pBuild->pMemorySegment);
  if( pBuild->FTable == NULL ){
    pBuild->cbFTable = 0;/* just to be safe */
    return BU_ERROR_MEMORY_LOW;
    }

  p = (char *)pBuild->FTable;
  sym_TraverseSymbolTable(pBuild->pEx->UserFunctions,
                          build_PutFTableItem,
                          &p);

  return BU_ERROR_SUCCESS;
  }

/*POD
=H build_CreateVTable()

When the binary code of the BASIC program is saved to disk the symbol table of the user
defined functions and the symbol table of global variables is also saved. This may be needed
by some applications that embed ScriptBasic and want to call specific function or alter global variables
of a given name from the embedding C code. To do this they need the serial number of the global variable
or the entry point of the function. Therefore ScriptBasic v1.0b20 and later can save these two tables into
the binary code.

The format of the tables is simple optimized for space and for simplicity of generation. They are stored
first in a memory chunk and then written to disk just as a series of bytes.

The format is

=verbatim
long      serial number of variable or entry point of the function
zchar     zero character terminated symbol
=noverbatim

This is easy to save and to load. Searching for it is a bit slow. Embedding applications usually
have to search for the values only once, store it in their local variable and use the value.

The function R<build_CreateFTable()> converts the symbol table of user defined function
collected by symbolic analysis into a single memory chunk.

The same way T<CreateVTable> converts the symbol table of global variables
collected by symbolic analysis into a single memory chunk.

/*FUNCTION*/
int build_CreateVTable(pBuildObject pBuild
  ){
/*noverbatim
CUT*/
  char *p;

  pBuild->cbVTable = 0;
  sym_TraverseSymbolTable(pBuild->pEx->GlobalVariables,
                          build_CountSymbolBytes,
                          &(pBuild->cbVTable));
  if( pBuild->cbVTable == 0 ){
    pBuild->VTable = NULL;
    return BU_ERROR_SUCCESS;
    }
  pBuild->VTable = alloc_Alloc(pBuild->cbVTable,pBuild->pMemorySegment);
  if( pBuild->VTable == NULL ){
    pBuild->cbVTable = 0;/* just to be safe */
    return BU_ERROR_MEMORY_LOW;
    }

  p = (char *)pBuild->VTable;
  sym_TraverseSymbolTable(pBuild->pEx->GlobalVariables,
                          build_PutVTableItem,
                          &p);

  return BU_ERROR_SUCCESS;
  }

/* 
This function is used to search the long value for a function or for a global variable.

Mitchell Greess [m.greess@solutions-atlantic.com] modified the code April 20, 2002.
to take the alignment issues into account on operating systems like Solaris.
*/
static long build_LookupFunctionOrVariable(pSymbolLongTable Table,
                                           unsigned long cbTable,
                                           char *s){
  char *p;
  char *SymbolName;
  long TableItemLen;

  p = (char *)Table;
  while( cbTable ){
    SymbolName = p + sizeof(long);
    if( ! strcmp(s,SymbolName) ) return Table->value;
    TableItemLen = build_TableItemBytes(SymbolName);
    p += TableItemLen;
    cbTable -= TableItemLen;
    Table = (pSymbolLongTable)p;
    }
  return 0;
  }

/*POD
=H build_LookupFunctionByName()
/*FUNCTION*/
long build_LookupFunctionByName(pBuildObject pBuild,
                          char *s
  ){
/*noverbatim
CUT*/
  return build_LookupFunctionOrVariable(pBuild->FTable,pBuild->cbFTable,s);
  }

/*POD
=H build_LookupVariableByName()
/*FUNCTION*/
long build_LookupVariableByName(pBuildObject pBuild,
                          char *s
  ){
/*noverbatim
CUT*/
  return build_LookupFunctionOrVariable(pBuild->VTable,pBuild->cbVTable,s);
  }
