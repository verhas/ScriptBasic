/* 
FILE:   execute.c
HEADER: execute.h

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

#include "conftree.h"
#include "hookers.h"
#include "thread.h"

// This structure stores the information on loaded modules and also on modules
// that were only tried to be loaded, but were not loaded.
typedef struct _Module {
  char *pszModuleName; // the name of the module as supplied by the caller, either
                       // simple name or full path to the dll
  void *ModulePointer; // pointer returned by the system function LoadModule/dlopen
  void *ModuleInternalParameters; // the modules own pointer initialized to zero, and the module
                      // alters it as it likes
  int ModuleIsActive; // true if the module is active. It can not be unloaded if it is active.
  int ModuleIsStatic; // true if the module is statically linked to the interpreter.
  struct _Module *next; // the next module in the list of loaded modules
  }Module, *pModule;

// note that the ExecuteObject has a pointer to the SupportTable, but the SupportTable
// also has a pointer back to the ExecuteObject
typedef struct _SupportTable *pSupportTable;
#define PSUPPORTTABLE 1

typedef struct _ExecuteObject {
  void *(*memory_allocating_function)(size_t);
  void (*memory_releasing_function)(void *);
  void *pMemorySegment; //this pointer is passed to the memory allocating functions

  pReportFunction report;
  void *reportptr; // this pointer is passed to the report function. The caller should set it.
  unsigned long fErrorFlags;

  ptConfigTree pConfig; // configuration data

  char *StringTable; // all the string constants of the program zero terminated each

  unsigned long cbStringTable; // all the bytes of StringTable including the zeroes

  pcNODE CommandArray;
  unsigned long StartNode;
  unsigned long CommandArraySize;

  long cGlobalVariables;
  pFixSizeMemoryObject GlobalVariables;

  long cLocalVariables;
  pFixSizeMemoryObject LocalVariables; // this variable is always stored and restored when a locality is entered

  unsigned long ProgramCounter;
  unsigned long NextProgramCounter;
  int fNextPC; // command sets it to TRUE when the NextProgramCounter was set (like in a jump)

#define fStopRETURN 1
#define fStopSTOP   2
  int fStop;

  unsigned long lStepCounter;  // counts the program steps within the function
  unsigned long lGlobalStepCounter; // counts the program steps in the total program
  long lFunctionLevel;        // the level in function call deepness

  // maximal values or zero if no limit exists to help avoid infinite loop s
  long GlobalStepLimit;       // the max number of steps allowed for the programs
  long LocalStepLimit;        // the max number of steps inside a function
  long FunctionLevelLimit;    // the maximal function call deepness

  int fWeAreCallingFuction; // This is true, when we are calling a function

  unsigned long ErrorCode;
  int fErrorGoto; // what type of value is in the ErrorGoto variable
#define ONERROR_NOTHING    0
#define ONERROR_GOTO       1
#define ONERROR_RESUME     2
#define ONERROR_RESUMENEXT 3

  unsigned long ErrorGoto;   // where to go when an error occures
  unsigned long ErrorResume; // where did the error occures, where to resume
  unsigned long LastError;   // the code of the the last error that happened

  unsigned long OperatorNode;     // the node number of the current operator
  pFixSizeMemoryObject pOpResult; // result of the operator function
  pFixSizeMemoryObject pFunctionResult; // result of the current function
  pMortalList pGlobalMortalList;   // the actually used mortal list
  unsigned long FunctionArgumentsNode; // the node of the expression list forming the argument list

  pMemoryObject pMo;

  CommandFunctionType *pCommandFunction;

  void *CommandParameter[NUM_CMD]; // a NULL initialized pointer for each type of command or function
  void (*(Finaliser[NUM_CMD]))(struct _ExecuteObject*);
                                   // a NULL initialized pointer for each type of command or function
                                   // if a function pointer is put into this variable it is called upon
                                   // finishing the execution of the program
  void **InstructionParameter;     // a NULL initialized pointer for each cNODE

  VersionInfo Ver;

  void *fpStdinFunction;  // pointer to standard input function when embedded
  void *fpStdouFunction;  // pointer to standard output function when embedded
  void *fpEnvirFunction;  // pointer to the environment variable retrieval function
  char *CmdLineArgument;  // pointer to the command line argument

  SymbolTable OptionsTable; // the options that the program can set using the statement option

  void *pEmbedder; // this can be used by the embedding program

  pSupportTable pST; // support table for the external functions
  pSupportTable pSTI;// support table inherited from a process global program object needed
                     // only for multi-thread supporting modules
  MUTEX mxModules;   // to lock the modules list if this is a process SB object otherwise not used
  pModule modules;   // list of the the loaded modules
  struct _ExecuteObject *pEPo;//the process objects execute structure in case this interpreter runs in MT env

  char *pszModuleError; // the error message returned by a module call

  pHookFunctions pHookers; // structure containing the hooker function pointers

//  char *Argv0; // the name of the script executed
  LexNASymbol *pCSYMBOLS; // to help locate the command code based on command id string
  int fThreadedCommandTable; // true if the command table points to a copy (owned by the thread)
  char **CSymbolList;
  //unsigned long maxderef;
  }ExecuteObject
#ifndef PEXECUTEOBJECT
  , *pExecuteObject
#endif
  ;
#define GETDOUBLEVALUE(x) execute_GetDoubleValue(pEo,(x))
#define GETLONGVALUE(x)   execute_GetLongValue(pEo,(x))
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

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
#include "modumana.h"

#define REPORT(x) if( pEo->report )pEo->report(pEo->reportptr,"",0,x,REPORT_ERROR,NULL,NULL,&(pEo->fErrorFlags))

#if BCC32
#define pow10 _mypow10
#endif

static double pow10(double a)
{
   int j,i;
   double pro,k;

   for( (i= a<0.0) && (a = -a) , j=(int)a , pro=1.0 , k=10; j ;
       j%2 && (pro *=k) , j /= 2 , k *= k )
      continue;
   i && (pro=1.0/pro);
   return pro;
}


/*POD

This module contain the functions that execute the code resuled by the builder.

CUT*/

/*POD
=H execute_GetCommandByName()

The op-code of a command can easily be identified, because T<syntax.h> contains
symbolic constant for it. This function can be used by external modules to
get this opcode based on the name of the function. The argument T<pszCommandName>
should be the name of the command, for example T<"ONERRORRESUMENEXT">. The third 
argument is the hint for the function to help to find the value. It should always
be the opcode of the command. The return value is the actual opcode of the command.
For example:

=verbatim
i = execute_GetCommandByName(pEo,"ONERRORRESUMENEXT",CMD_ONERRORRESUMENEXT);
=noverbatim

will return T<CMD_ONERRORRESUMENEXT>.

I<Why is this function all about then?>

The reason is that the external module may not be sure that the code
T<CMD_ONERRORRESUMENEXT> is the same when the external module is compiled
and when it is loaded. External modules negotiate the interface version
information with the calling interpreter, but the opcodes may silently changed
from interpreter version to the next interpreter version and still supporting
the same extension interface version.

When an external module needs to know the opcode of a command of the calling
interpreter it first calls this function telling:

I<I need the code of the command ONERRORRESUMENEXT. I think that the code is
CMD_ONERRORRESUMENEXT, but is it the real code?>

The argument T<lCodeHint> is required only, because it speeds up search.

If there is no function found for the given name the returnvalue is zero.

/*FUNCTION*/
long execute_GetCommandByName(pExecuteObject pEo,
                              char *pszCommandName,
                              long lCodeHint
  ){
/*noverbatim
CUT*/
  long DownCounter,UpCounter;

  if( lCodeHint < START_CMD )lCodeHint = START_CMD;
  if( lCodeHint >= END_EXEC )lCodeHint = END_EXEC-1;

  DownCounter = UpCounter = lCodeHint;
  while( DownCounter || UpCounter ){
    if( DownCounter && !strcmp(pszCommandName,pEo->CSymbolList[DownCounter-START_CMD]) )return DownCounter;
    if( UpCounter && !strcmp(pszCommandName,pEo->CSymbolList[UpCounter-START_CMD]) )return UpCounter;
    UpCounter ++;
    if( UpCounter == END_EXEC )UpCounter = 0;
    DownCounter --;
    if( DownCounter < START_CMD )DownCounter = 0;
    }
  return 0; 
  }

/*POD
=H execute_CopyCommandTable()

The command table is a huge table containing pointers to functions. For example
the T<CMD_LET>-th element of the table points to the function T<COMMAND_LET>
implementing the assignment command.

This table is usually treated as constant and is not moduified during run time.
In case a module wants to reimplement a command it should alter this table.
However the table is shared all concurrently running interpreter threads in
a multi-thread variation of ScriptBasic.

To avoid altering the command table of an independent interpreter threadthe module
wanting altering the command table should call this function. This function allocates
memory for a new copy of the command table and copies the original constant
value to this new place. After the copy is done the T<ExecuteObject> will point to
the copied command table and the extension is free to alter the table.

In case the function is called more than once for the same interpreter thread
only the first time is effective. Later the function returns without creating superfluous
copies of the command table.
/*FUNCTION*/
int execute_CopyCommandTable(pExecuteObject pEo
  ){
/*noverbatim
CUT*/
  CommandFunctionType *p;

  /* it is already copied to the thread local place */
  if( pEo->fThreadedCommandTable )return COMMAND_ERROR_SUCCESS;

  p = ALLOC( sizeof( CommandFunctionType ) * (END_EXEC-START_CMD) );
  if( p == NULL )return COMMAND_ERROR_MEMORY_LOW;
  memcpy(p, pEo->pCommandFunction, sizeof( CommandFunctionType ) * (END_EXEC-START_CMD) );
  pEo->pCommandFunction = p;
  pEo->fThreadedCommandTable = 1;
  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H execute_InitStructure()

/*FUNCTION*/
int execute_InitStructure(pExecuteObject pEo,
                          pBuildObject pBo
  ){
/*noverbatim
CUT*/
  long maxmem;
  int iError;

  build_MagicCode(&(pEo->Ver));
  pEo->fpStdinFunction = NULL;
  pEo->fpStdouFunction = NULL;
  pEo->fpEnvirFunction = NULL;
  pEo->CmdLineArgument = NULL;

  if( cft_GetEx(pEo->pConfig,"maxstep",NULL,NULL,
                &(pEo->GlobalStepLimit),NULL,NULL) )
    pEo->GlobalStepLimit = 0;

  if( cft_GetEx(pEo->pConfig,"maxlocalstep",NULL,NULL,
                &(pEo->LocalStepLimit),NULL,NULL) )
    pEo->LocalStepLimit = 0;

  if( cft_GetEx(pEo->pConfig,"maxlevel",NULL,NULL,
                &(pEo->FunctionLevelLimit),NULL,NULL) )
    pEo->FunctionLevelLimit = 0;

  pEo->CommandArray = pBo->CommandArray ;
  pEo->StartNode = pBo->StartNode;
  pEo->CommandArraySize = pBo->NodeCounter;
  pEo->StringTable = pBo->StringTable;
  pEo->cbStringTable = pBo->cbStringTable;
  pEo->cGlobalVariables = pBo->cGlobalVariables;
  pEo->lGlobalStepCounter = 0L;
  pEo->LastError = 0; /* there was no run time error so 
                         far and this led formerly the 
                         BASIC 'ERROR()' function random
                         on the start */

  pEo->pCommandFunction = CommandFunction;
  pEo->CSymbolList = COMMANDSYMBOLS;
  pEo->fThreadedCommandTable = 0;
  pEo->pFunctionResult = NULL;
  pEo->pGlobalMortalList = NULL;
  pEo->pST = NULL;
  pEo->pSTI = NULL;
  pEo->pEPo = NULL;
  pEo->modules = NULL;

  pEo->pMemorySegment = alloc_InitSegment(pEo->memory_allocating_function,pEo->memory_releasing_function);

  if( pEo->pMemorySegment == NULL )return COMMAND_ERROR_MEMORY_LOW;


  pEo->pMo = alloc_Alloc(sizeof(MemoryObject),pEo->pMemorySegment);
  if( pEo->pMo == NULL )return EXE_ERROR_MEMORY_LOW;

  if( cft_GetEx(pEo->pConfig,"maxderef",NULL,NULL,
                &(pEo->pMo->maxderef),NULL,NULL) )
    pEo->pMo->maxderef = 1000;
  pEo->pMo->memory_allocating_function = pEo->memory_allocating_function;
  pEo->pMo->memory_releasing_function = pEo->memory_releasing_function;
  pEo->cbStringTable = 0L;
  pEo->OptionsTable = NULL;
  if( iError = memory_InitStructure(pEo->pMo) )return iError;

  if( cft_GetEx(pEo->pConfig,"maxmem",NULL,NULL,
                &maxmem,NULL,NULL) == CFT_ERROR_SUCCESS )
    alloc_SegmentLimit(pEo->pMo->pMemorySegment,maxmem);


  memory_RegisterTypes(pEo->pMo);
  if( hook_Init(pEo, &(pEo->pHookers)) )return 1;
  if( modu_Preload(pEo) )return 1;

  pEo->GlobalVariables = NULL;
  pEo->InstructionParameter = NULL;

  return EXE_ERROR_SUCCESS;
  }

/*POD
=H execute_ReInitStructure()

This function should be used if a code is executed repeatedly. The first
initialization call is R<execute_InitStructure()> and consecutive executions
should call this function.

/*FUNCTION*/
int execute_ReInitStructure(pExecuteObject pEo,
                            pBuildObject pBo
  ){
/*noverbatim
CUT*/

  pEo->lGlobalStepCounter = 0L;
  pEo->pFunctionResult = NULL;

  pEo->pST  = NULL;
  pEo->pSTI = NULL;
  pEo->pEPo = NULL;
  pEo->modules = NULL;
  if( modu_Preload(pEo) )return 1;

  return 0;
  }

/*POD
=H execute_Execute_r()

This function executes a program fragment. The execution starts from the class variable
T<ProgramCounter>. This function is called from the R<execute_Execute()> function which is the
main entry point to the basic main program. This function is also called recursively from
the function R<execute_Evaluate()> when a user defined function is to be executed.

/*FUNCTION*/
void execute_Execute_r(pExecuteObject pEo,
                       int *piErrorCode
  ){
/*noverbatim
CUT*/
  CommandFunctionType ThisCommandFunction;
  unsigned long CommandOpCode;
  unsigned long pc,npc;

  pEo->fStop = 0;
  pEo->lStepCounter = 0L;
  pEo->fErrorGoto = ONERROR_NOTHING; /* this is the default behaviour */
  while( pEo->ProgramCounter ){
    pEo->fNextPC = 0;
    if( pEo->ProgramCounter > pEo->CommandArraySize ){
      *piErrorCode = EXE_ERROR_INVALID_PC;
      return;
      }

    if( pEo->CommandArray[pEo->ProgramCounter-1].OpCode != eNTYPE_LST ){
      *piErrorCode = EXE_ERROR_INVALID_NODE;
      return;
      }

    pc = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm;
    if( pc > pEo->CommandArraySize ){
      *piErrorCode = EXE_ERROR_INVALID_PC1;
      return;
      }
    if( pc ){
      CommandOpCode = pEo->CommandArray[pc-1].OpCode;
      if( CommandOpCode < START_CMD || CommandOpCode > END_CMD ){
        *piErrorCode = EXE_ERROR_INVALID_OPCODE;
        return;
        }
      }
    npc = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest ;

    if( pc ){
      ThisCommandFunction = pEo->pCommandFunction[CommandOpCode - START_CMD];
      if( ThisCommandFunction == NULL ){
        *piErrorCode = EXE_ERROR_NOT_IMPLEMENTED;
        return;
        }
      }
    pEo->ErrorCode = EXE_ERROR_SUCCESS;
    if( pEo->pHookers->HOOK_ExecBefore && (*piErrorCode = pEo->pHookers->HOOK_ExecBefore(pEo)) )return;
    if( pc )
      ThisCommandFunction(pEo);
    if( pEo->pHookers->HOOK_ExecAfter && (*piErrorCode = pEo->pHookers->HOOK_ExecAfter(pEo)) )return;
    pEo->fWeAreCallingFuction = 0;
    pEo->lStepCounter++;
    if( pEo->LocalStepLimit && pEo->lStepCounter > (unsigned)pEo->LocalStepLimit ){
      *piErrorCode = EXE_ERROR_TOO_LONG_RUN;
      return;
      }
    pEo->lGlobalStepCounter++;
    if( pEo->GlobalStepLimit && pEo->lGlobalStepCounter > (unsigned)pEo->GlobalStepLimit ){
      *piErrorCode = EXE_ERROR_TOO_LONG_RUN;
      return;
      }

    /* If the error code was set by the instruction then handle the on error goto statements */
    if( pEo->ErrorCode ){
      pEo->LastError = pEo->ErrorCode;
      if( pEo->fErrorGoto == ONERROR_RESUMENEXT ){
        pEo->fErrorGoto = ONERROR_NOTHING;
        pEo->ErrorResume = 0; /* we resume the exection, we won't go back to anywhere */
        pEo->LastError = 0;   /* it is resumed, there is no error                     */
        pEo->ErrorGoto = 0;   /* no error goto node */
        }else
      if( pEo->fErrorGoto == ONERROR_RESUME ){
        if( ! pEo->ErrorGoto ){
          *piErrorCode = pEo->ErrorCode;
          return;
          }
        pEo->ErrorResume = 0; /* we resume the exection, we won't go back to anywhere */
        pEo->LastError = 0;   /* it is resumed, there is no error                     */
        pEo->fErrorGoto = ONERROR_NOTHING; /* on error is switched off by default     */
        pEo->ErrorGoto = 0;   /* no error goto node */
        pEo->fNextPC = 1;
        pEo->NextProgramCounter = pEo->ErrorGoto;
        }else
      if( pEo->fErrorGoto == ONERROR_GOTO ){
        if( ! pEo->ErrorGoto ){
          *piErrorCode = pEo->ErrorCode;
          return;
          }
        pEo->ErrorResume = pEo->ProgramCounter;
        pEo->fNextPC = 1;
        pEo->NextProgramCounter = pEo->ErrorGoto;
        pEo->ErrorGoto = 0;
        }else{
        /* fErrorGoto should be ONERROR_NOTHING if we got here */
        *piErrorCode = pEo->ErrorCode;
        return;
        }
      }

    if( pEo->fStop ){
      if( pEo->fStop == fStopRETURN )pEo->fStop = 0;
      *piErrorCode = EXE_ERROR_SUCCESS;
      return;
      }
    if( pEo->fNextPC )
      pEo->ProgramCounter = pEo->NextProgramCounter;
    else
      pEo->ProgramCounter = npc;
    }
  *piErrorCode = EXE_ERROR_SUCCESS;
  return;
  }

/*POD
=H execute_InitExecute()

/*FUNCTION*/
void execute_InitExecute(pExecuteObject pEo,
                        int *piErrorCode
  ){
/*noverbatim
CUT*/
#ifdef NULL_IS_NOT_ZERO
  unsigned long i;
#endif
  *piErrorCode = 0;
  pEo->ProgramCounter = pEo->StartNode;

  pEo->LocalVariables = NULL;
  pEo->pszModuleError = NULL;
  pEo->cLocalVariables = 0;
  pEo->ErrorGoto = 0;

  if( pEo->GlobalVariables == NULL ){
    pEo->GlobalVariables = memory_NewArray(pEo->pMo,1,pEo->cGlobalVariables);
    if( pEo->GlobalVariables == NULL ){
      *piErrorCode = EXE_ERROR_MEMORY_LOW;
      return;
      }
    }
  pEo->fWeAreCallingFuction = 0;
  pEo->lFunctionLevel = 0;
  if( pEo->InstructionParameter == NULL ){
    pEo->InstructionParameter = alloc_Alloc(pEo->CommandArraySize*sizeof(void *),pEo->pMemorySegment);
    if( pEo->InstructionParameter == NULL ){
      *piErrorCode = EXE_ERROR_MEMORY_LOW;
      return;
      }
    memset(pEo->InstructionParameter,0,pEo->CommandArraySize*sizeof(void *));
    }
  memset(pEo->CommandParameter,0,NUM_CMD*sizeof(void *));
  memset(pEo->Finaliser,0,NUM_CMD*sizeof(void *));
  }

/*POD
=H execute_FinishExecute()

/*FUNCTION*/
void execute_FinishExecute(pExecuteObject pEo,
                           int *piErrorCode
  ){
/*noverbatim
CUT*/
  unsigned long i;

  for( i=0 ; i < NUM_CMD ; i++ ){
    if( pEo->Finaliser[i] )pEo->Finaliser[i](pEo);
    }
  modu_UnloadAllModules(pEo);
  }

/*POD
=H execute_Execute()

This function was called from the basic T<main> function. This function performs inititalization
that is needed before each execution of the code and calls R<execute_Execute_r()> to perform the execution.

Note that R<execute_Execute_r()> is recursively calls itself.

This function is obsolete and is not used anymore. This is kept in the source
for the shake of old third party variations that may depend on this function.

Use of this function in new applications is discouraged.

/*FUNCTION*/
void execute_Execute(pExecuteObject pEo,
                     int *piErrorCode
  ){
/*noverbatim
CUT*/
  execute_InitExecute(pEo,piErrorCode);
  if( *piErrorCode )return;

  execute_Execute_r(pEo,piErrorCode);

  if( *piErrorCode ){
    REPORT(*piErrorCode);
    }

  execute_FinishExecute(pEo,piErrorCode);
  }

/*POD
=H execute_ExecuteFunction()

This function is used by the embedding layer (aka T<scriba_> functions) to execute a function.
This function is not directly called by the execution of a ScriptBasic program. It may be
used after the execution of the program by a special embeddign application that keeps the
code and the global variables in memory and calls functions of the program.

The function takes T<pEo> as the execution environment. T<StartNode> should be the node where the
sub or function is defined. T<cArgs> should give the number of arguments. T<pArgs> should point
to the argument array. T<pResult> will point to the result. If T<pResult> is T<NULL> the result is
dropped. Otherwise the result is a mortal variable.

Note that this code does not check the number of arguments you provide. There can be more arguments
passed to the SUB than it has declared, therefore you can initialize the local variables of the sub.
(You should know that arguments are local variables in ScriptBasic just as any other non-argument local
variable.)

The arguments should be normal immortal variables. They are passed to the SUB by reference and in case
they are modified the old variable is going to be released.

T<piErrorCode> returns the error code of the execution which is zero in case of no error.
/*FUNCTION*/
void execute_ExecuteFunction(pExecuteObject pEo,
                             unsigned long StartNode,
                             long cArgs,
                             pFixSizeMemoryObject *pArgs,
                             pFixSizeMemoryObject *pResult,
                             int *piErrorCode
  ){
/*noverbatim
CUT*/
  unsigned long nItem,pc;
  long i;
  long CommandOpCode;

  pEo->ProgramCounter = StartNode;
  pEo->pFunctionResult = NULL;
  pEo->lStepCounter = 0;
  pEo->fWeAreCallingFuction = 1;
  pEo->ErrorGoto = 0;
  pEo->ErrorResume = 0;
  pEo->fErrorGoto = ONERROR_NOTHING;
  pEo->LocalVariables = NULL;
  if( pResult )*pResult = NULL;

  if( pEo->CommandArray[pEo->ProgramCounter-1].OpCode != eNTYPE_LST ){
      *piErrorCode = EXE_ERROR_INVALID_NODE;
      return;
      }

  pc = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm;
  if( pc > pEo->CommandArraySize ){
    *piErrorCode = EXE_ERROR_INVALID_PC1;
    return;
    }
  if( pc ){
    CommandOpCode = pEo->CommandArray[pc-1].OpCode;
    if( CommandOpCode < START_CMD || CommandOpCode > END_CMD ){
      *piErrorCode = EXE_ERROR_INVALID_OPCODE;
      return;
      }
    }
  if( CommandOpCode != CMD_FUNCTION &&
      CommandOpCode != CMD_FUNCTIONARG &&
      CommandOpCode != CMD_SUB &&
      CommandOpCode != CMD_SUBARG
    ){
    *piErrorCode = COMMAND_ERROR_INVALID_CODE;
    return;
    }
  nItem = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm ;
  pEo->cLocalVariables = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;

  if( pEo->cLocalVariables ){
    pEo->LocalVariables = memory_NewArray(pEo->pMo,1,pEo->cLocalVariables);
    if( pEo->LocalVariables == NULL ){
      pEo->fStop = fStopSTOP;
      return;
      }
    }

  /* there can not be more arguments than local variables. The rest is ignored. */
  if( cArgs > pEo->cLocalVariables )cArgs = pEo->cLocalVariables;
  for( i=0 ; i < cArgs ; i++ ){
    pEo->LocalVariables->Value.aValue[i] = memory_NewRef(pEo->pMo);
    memory_SetRef(pEo->pMo,pEo->LocalVariables->Value.aValue+i,pArgs+i);
    }

  /* step over the function head */
  pEo->ProgramCounter = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest ;

  execute_Execute_r(pEo,piErrorCode);


  if( pEo->LocalVariables )/* this is null if the function did not have arguments and no local variables */
    memory_ReleaseVariable(pEo->pMo,pEo->LocalVariables);

  /* if the result is not needed by the caller we drop it. Otherwise it is the caller's responsibility to drop it. */
  if( pResult )
    *pResult = pEo->pFunctionResult;
  else 
    memory_ReleaseVariable(pEo->pMo,pEo->pFunctionResult);
  return;
  }

/*POD
=H execute_Evaluate()

This function evaluates an expression. You should not get confused! This is not syntax analysis, caring
operator precedences and grouping by nested parentheses. That has already been done during syntax analysis.
This code performs the code that was generated from an expression.

The result is usually a mortal memory value which is the final result of the expression. However this piece of
code assumes that the caller is careful enough to handle the result as read only, and sometimes the return
value is not mortal. In this case the return value is a memory object that a variable points to. Whenever the
caller needs this value to perform an operation that does not alter the value it is OK. Duplicating the structure
to create a mortal would be waste of time and memory. On the other hand sometimes operations modify their operands
assuming that they are mortal values. They should be careful.

Operators are actually created in the directory T<commands> and they use the macros defined in T<command.h> (created
by T<headerer.pl> from T<command.c>). They help to avoid pitfalls.

The argument T<iArrayAccepted> tells the function whether an array as a result is accepted or not. If a whole
array is accepted as a result of the expression evaluation the array is returned. If the array is not an
acceptable result, then the first element of the array is retuned in case the result is an array. If the result
is NOT an array this parameter has no effect.

/*FUNCTION*/
pFixSizeMemoryObject execute_Evaluate(pExecuteObject pEo,
                                      unsigned long lExpressionRootNode,
                                      pMortalList pMyMortal,
                                      int *piErrorCode,
                                      int iArrayAccepted
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject pVar;
  char *s;
  unsigned long slen,refcount;
  unsigned long SaveProgramCounter,SaveStepCounter;
  unsigned long SavefErrorGoto,SaveErrorGoto,SaveErrorResume;
  pFixSizeMemoryObject SaveLocalVariablesPointer;
  pFixSizeMemoryObject SaveFunctionResultPointer;
  pFixSizeMemoryObject ThisFunctionResultPointer;
  long OpCode;
  CommandFunctionType ThisCommandFunction;
  pMortalList pSaveMortalList;

  if( ! lExpressionRootNode ){
    *piErrorCode = EXE_ERROR_INTERNAL;
    return NULL;
    }
  if( pMyMortal == NULL ){
    *piErrorCode = EXE_ERROR_INTERNAL;
    return NULL;
    }
  *piErrorCode = EXE_ERROR_SUCCESS;
#define ASSERT_NON_NULL(x) if( (x) == NULL ){ *piErrorCode = EXE_ERROR_MEMORY_LOW; return NULL; }
  switch( OpCode = pEo->CommandArray[lExpressionRootNode-1].OpCode ){
    case eNTYPE_ARR: /* array access                */
      pVar = execute_EvaluateArray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);
      while( pVar && ( ((!iArrayAccepted) && pVar->vType == VTYPE_ARRAY) || pVar->vType == VTYPE_REF) ){
        /* when an array is referenced as scalar the first element is returned */
        while( pVar && (!iArrayAccepted) && pVar->vType == VTYPE_ARRAY )
          pVar = pVar->Value.aValue[0];
        while( pVar && pVar->vType == VTYPE_REF )
          pVar = *(pVar->Value.aValue);
        }
      return memory_SelfOrRealUndef(pVar);
    case eNTYPE_SAR: /* associative array access */
      pVar = execute_EvaluateSarray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);
      while( pVar && ( ((!iArrayAccepted) && pVar->vType == VTYPE_ARRAY) || pVar->vType == VTYPE_REF) ){
        /* when an array is referenced as scalar the first element is returned */
        while( pVar && (!iArrayAccepted) && pVar->vType == VTYPE_ARRAY )
          pVar = pVar->Value.aValue[0];
        while( pVar && pVar->vType == VTYPE_REF )
          pVar = *(pVar->Value.aValue);
        }
      return memory_SelfOrRealUndef(pVar);
    case eNTYPE_FUN: /* function                    */
      if( pEo->FunctionLevelLimit && pEo->lFunctionLevel > pEo->FunctionLevelLimit ){
        *piErrorCode = EXE_ERROR_TOO_DEEP_CALL;
        return NULL;
        }
      SaveLocalVariablesPointer = pEo->LocalVariables;
      SaveProgramCounter = pEo->ProgramCounter;
      pEo->ProgramCounter = pEo->CommandArray[lExpressionRootNode-1].Parameter.UserFunction.NodeId;
      if( pEo->ProgramCounter == 0 ){
        *piErrorCode = EXE_ERROR_USERFUN_UNDEFINED;
        return NULL;
        }
      pEo->FunctionArgumentsNode = pEo->CommandArray[lExpressionRootNode-1].Parameter.UserFunction.Argument;
      SaveFunctionResultPointer = pEo->pFunctionResult;
      pEo->pFunctionResult = NULL;
      SaveStepCounter = pEo->lStepCounter;
      pEo->lStepCounter = 0;
      pEo->fWeAreCallingFuction = 1;
      SaveErrorGoto = pEo->ErrorGoto;
      pEo->ErrorGoto = 0;
      SaveErrorResume = pEo->ErrorResume;
      pEo->ErrorResume = 0;
      SavefErrorGoto = pEo->fErrorGoto;
      pEo->fErrorGoto = ONERROR_NOTHING;
      if( pEo->pHookers->HOOK_ExecCall && (*piErrorCode = pEo->pHookers->HOOK_ExecCall(pEo)) )return NULL;
      /* function entering code needs access to the caller local variables, therefore
         WE SHOULD NOT NULL pEo->LocalVariables */
      execute_Execute_r(pEo,piErrorCode);
      if( pEo->pHookers->HOOK_ExecReturn ){
        /* if there was already an error then there is no way to handle two different errors
           one coming from the execution system and one from the hook function. This way the
           hook function generated error (if any) is ignored. */
        if( *piErrorCode )
          pEo->pHookers->HOOK_ExecReturn(pEo);
         else
          *piErrorCode = pEo->pHookers->HOOK_ExecReturn(pEo);
        }
      pEo->lStepCounter = SaveStepCounter;
      if( pEo->LocalVariables )/* this is null if the function did not have arguments and no local variables */
        memory_ReleaseVariable(pEo->pMo,pEo->LocalVariables);
      pEo->ProgramCounter = SaveProgramCounter;
      pEo->LocalVariables = SaveLocalVariablesPointer;
      ThisFunctionResultPointer = pEo->pFunctionResult;
      pEo->pFunctionResult = SaveFunctionResultPointer;
      while( ThisFunctionResultPointer &&
             (!iArrayAccepted) && 
             ThisFunctionResultPointer->vType == VTYPE_ARRAY ){
        ThisFunctionResultPointer = ThisFunctionResultPointer->Value.aValue[0];
        }
      /* Functions return their value as immortal values assigned to the very global
         variable pEo->pFunctionResult. Here this variable is restored to point to the
         saved value and the value returned should be mortalized.                   */
      if( ThisFunctionResultPointer && 
          ThisFunctionResultPointer->vType != VTYPE_ARRAY &&
          ! IsMortal(ThisFunctionResultPointer) )
        memory_Mortalize(ThisFunctionResultPointer,pMyMortal);

      pEo->ErrorGoto = SaveErrorGoto;
      pEo->fErrorGoto = SavefErrorGoto;
      pEo->ErrorResume = SaveErrorResume;
      if( *piErrorCode )return NULL;
      return memory_SelfOrRealUndef(ThisFunctionResultPointer);
    case eNTYPE_LVR: /* local variable              */
      if( pEo->LocalVariables == NULL ){
        *piErrorCode = EXE_ERROR_NO_LOCAL;
        return NULL;
        }
      pVar = pEo->LocalVariables->Value.aValue[pEo->CommandArray[lExpressionRootNode-1].Parameter.Variable.Serial-1];
      while( pVar && ( ((!iArrayAccepted) && pVar->vType == VTYPE_ARRAY) || pVar->vType == VTYPE_REF) ){
        /* when an array is referenced as scalar the first element is returned */
        while( pVar && (!iArrayAccepted) && pVar->vType == VTYPE_ARRAY )
          pVar = pVar->Value.aValue[0];
        refcount = 0;
        while( pVar && pVar->vType == VTYPE_REF ){
          pVar = *(pVar->Value.aValue);
          if( refcount++ > pEo->pMo->maxderef ){
            *piErrorCode = COMMAND_ERROR_CIRCULAR;
            return NULL;
            }
          }
        }
      return memory_SelfOrRealUndef(pVar);
    case eNTYPE_GVR: /* global variable             */
      pVar = pEo->GlobalVariables->Value.aValue[pEo->CommandArray[lExpressionRootNode-1].Parameter.Variable.Serial-1];
      while( pVar && ( ((!iArrayAccepted) && pVar->vType == VTYPE_ARRAY) || pVar->vType == VTYPE_REF) ){
        /* when an array is referenced as scalar the first element is returned */
        while( pVar && (!iArrayAccepted) && pVar->vType == VTYPE_ARRAY )
          pVar = pVar->Value.aValue[0];
        refcount = 0;
        while( pVar && pVar->vType == VTYPE_REF ){
          pVar = *(pVar->Value.aValue);
          if( refcount++ > pEo->pMo->maxderef ){
            *piErrorCode = COMMAND_ERROR_CIRCULAR;
            return NULL;
            }
          }
        }
      return memory_SelfOrRealUndef(pVar);
    case eNTYPE_DBL: /* constant double             */
      if( pEo->InstructionParameter[lExpressionRootNode-1] == NULL ){
        pVar = pEo->InstructionParameter[lExpressionRootNode-1] = memory_NewDouble(pEo->pMo);
        ASSERT_NON_NULL(pVar);
        pVar->Value.dValue = pEo->CommandArray[lExpressionRootNode-1].Parameter.Constant.dValue;
        }else
        pVar = pEo->InstructionParameter[lExpressionRootNode-1];
      return memory_SelfOrRealUndef(pVar);

    case eNTYPE_LNG: /* constant long               */
      if( pEo->InstructionParameter[lExpressionRootNode-1] == NULL ){
        pVar = pEo->InstructionParameter[lExpressionRootNode-1] = memory_NewLong(pEo->pMo);
        ASSERT_NON_NULL(pVar);
        pVar->Value.lValue = pEo->CommandArray[lExpressionRootNode-1].Parameter.Constant.lValue;
        }else
        pVar = pEo->InstructionParameter[lExpressionRootNode-1];
      return memory_SelfOrRealUndef(pVar);

    case eNTYPE_STR: /* constant string             */
      s = pEo->StringTable+pEo->CommandArray[lExpressionRootNode-1].Parameter.Constant.sValue;
      memcpy(&slen, s-sizeof(long), sizeof(long));
      pVar = memory_NewMortalCString(pEo->pMo,slen,pMyMortal);
      ASSERT_NON_NULL(pVar);
      pVar->Value.pValue = s;
      return memory_SelfOrRealUndef(pVar);

    case eNTYPE_LST: /* list member (invalid)       */
      *piErrorCode = EXE_ERROR_INVALID_EXPRESSION_NODE;
      return NULL;
    case eNTYPE_CRG: /* command arguments (invalid) */
      *piErrorCode = EXE_ERROR_INVALID_EXPRESSION_NODE1;
      return NULL;
    default: /* operators and built in functions    */
      ThisCommandFunction = pEo->pCommandFunction[OpCode - START_CMD];
      if( ThisCommandFunction == NULL ){
        *piErrorCode = EXE_ERROR_NOT_IMPLEMENTED;
        return NULL;
        }
      pEo->OperatorNode = lExpressionRootNode;
      pEo->pOpResult = NULL;
      pSaveMortalList = pEo->pGlobalMortalList;
      pEo->pGlobalMortalList = pMyMortal;
      ThisCommandFunction(pEo);
      pEo->pGlobalMortalList = pSaveMortalList;
      *piErrorCode = pEo->ErrorCode;
      return memory_SelfOrRealUndef(pEo->pOpResult);
    }
  *piErrorCode = EXE_ERROR_INTERNAL;
  return NULL;
  }

/*POD
=H execute_LeftValue()

This function evaluate a left value. A left value is a special expression that value can be assigned, and therefore
they usually stand on the left side of the assignment operator. That is the reason for the name.

When an expression is evaluates a pointer to a memory object is returned. Whenever a left value is evaluated a pointer
to the variable is returned. If any code assignes value to the variable pointed by the return value of this function
it should release the memory object that the left value points currently.
/*FUNCTION*/
pFixSizeMemoryObject *execute_LeftValue(pExecuteObject pEo,
                                        unsigned long lExpressionRootNode,
                                        pMortalList pMyMortal,
                                        int *piErrorCode,
                                        int iArrayAccepted
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject *ppVar;
  long OpCode;

  *piErrorCode = EXE_ERROR_SUCCESS;

  switch( OpCode = pEo->CommandArray[lExpressionRootNode-1].OpCode ){

    case eNTYPE_ARR: /* array access                */
      return execute_LeftValueArray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);

    case eNTYPE_SAR: /* associative array access */
      return execute_LeftValueSarray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);

    case eNTYPE_LVR: /* local variable              */
      if( pEo->LocalVariables == NULL ){
        *piErrorCode = EXE_ERROR_NO_LOCAL;
        return NULL;
        }
      ppVar = &(pEo->LocalVariables->Value.aValue[pEo->CommandArray[lExpressionRootNode-1].Parameter.Variable.Serial-1]);
      /* when an array is referenced as scalar the first element is returned */
      while( (!iArrayAccepted) && *ppVar && (*ppVar)->vType == VTYPE_ARRAY )
        ppVar = &((*ppVar)->Value.aValue[0]);
      return ppVar;

    case eNTYPE_GVR: /* global variable             */
      ppVar = &(pEo->GlobalVariables->Value.aValue[pEo->CommandArray[lExpressionRootNode-1].Parameter.Variable.Serial-1]);
      /* when an array is referenced as scalar the first element is returned */
      while( (!iArrayAccepted) && *ppVar && (*ppVar)->vType == VTYPE_ARRAY )
        ppVar = &((*ppVar)->Value.aValue[0]);
      return ppVar;

    case eNTYPE_FUN: /* function                    */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE0;
      return NULL;

    case eNTYPE_DBL: /* constant double             */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE1;
      return NULL;

    case eNTYPE_LNG: /* constant long               */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE2;
      return NULL;

    case eNTYPE_STR: /* constant string             */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE3;
      return NULL;

    case eNTYPE_LST: /* list member (invalid)       */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE4;
      return NULL;

    case eNTYPE_CRG: /* command arguments (invalid) */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE5;
      return NULL;
    default: /* operators and built in functions    */
      *piErrorCode = EXE_ERROR_INVALID_LVALNODE6;
      return NULL;
    }
  *piErrorCode = EXE_ERROR_INTERNAL;
  return NULL;
  }

/*POD
=H execute_EvaluateArray()

This function should be used to evaluate an array access to get the actual
value. This is called by R<execute_Evaluate()>.

An array is stored in the expression as an operator with many operands. The first
operand is a local or global variable, the rest of the operators are the indices.

Accessing a variable holding scalar value with array indices automatically converts
the variable to array. Accessing an array variable without indices gets the "first"
element of the array.
/*FUNCTION*/
pFixSizeMemoryObject execute_EvaluateArray(pExecuteObject pEo,
                                      unsigned long lExpressionRootNode,
                                      pMortalList pMyMortal,
                                      int *piErrorCode
  ){
/*noverbatim
CUT*/

  return *execute_LeftValueArray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);
  }


/*POD
=H execute_EvaluateSarray()

This function should be used to evaluate an array access to get the actual
value. This is called by R<execute_Evaluate()>.

An array is stored in the expression as an operator with many operands. The first
operand is a local or global variable, the rest of the operators are the indices.

Associative arrays are normal arrays, only the access mode is different. When accessing
an array using the fom T<a{key}> then the access searches for the value T<key> in the 
evenly indexed elements of the array and gives the next index element of the array. This
if

=verbatim
a[0] = "kakukk"
a[1] = "birka"
a[2] = "kurta"
a[3] = "mamus"
=noverbatim

then T<a{"kakukk"}> is "birka". T<a{"birka"}> is T<undef>. T<a{"kurta"}> is "mamus".

/*FUNCTION*/
pFixSizeMemoryObject execute_EvaluateSarray(pExecuteObject pEo,
                                      unsigned long lExpressionRootNode,
                                      pMortalList pMyMortal,
                                      int *piErrorCode
  ){
/*noverbatim
CUT*/

  return *execute_LeftValueSarray(pEo,lExpressionRootNode,pMyMortal,piErrorCode);
  }

/*POD
=H execute_LeftValueArray()

This function evaluates an array access left value. This function is also called by R<execute_EvaluateArray()>
and the result pointer is dereferenced.

/*FUNCTION*/
pFixSizeMemoryObject *execute_LeftValueArray(pExecuteObject pEo,
                                             unsigned long lExpressionRootNode,
                                             pMortalList pMyMortal,
                                             int *piErrorCode
  ){
/*noverbatim
CUT*/

  long OpCode;
  long lIndex,lMinIndex,lMaxIndex;
  unsigned long nVariable, nIndex;
  unsigned long nHead;
  pFixSizeMemoryObject *ppVar,pVar;
  unsigned long __refcount_;

  nHead = pEo->CommandArray[lExpressionRootNode-1].Parameter.Arguments.Argument;
  nVariable = pEo->CommandArray[nHead-1].Parameter.NodeList.actualm;
  OpCode = pEo->CommandArray[nVariable-1].OpCode;

  nHead = pEo->CommandArray[nHead-1].Parameter.NodeList.rest;

  switch( OpCode ){
    case eNTYPE_LVR: /* local variable              */
      if( pEo->LocalVariables == NULL ){
        *piErrorCode = EXE_ERROR_NO_LOCAL;
        return NULL;
        }
      ppVar = &(pEo->LocalVariables->Value.aValue[pEo->CommandArray[nVariable-1].Parameter.Variable.Serial-1]);
      break;
    case eNTYPE_GVR: /* global variable             */
      ppVar = &(pEo->GlobalVariables->Value.aValue[pEo->CommandArray[nVariable-1].Parameter.Variable.Serial-1]);
      break;
    case eNTYPE_ARR:
      ppVar = execute_LeftValueArray(pEo,nVariable,pMyMortal,piErrorCode);
      break;
    case eNTYPE_SAR:
      ppVar = execute_LeftValueSarray(pEo,nVariable,pMyMortal,piErrorCode);
      break;
    default:
      /* the syntax analyzer should not generate anything else than this */
      *piErrorCode =EXE_ERROR_INTERNAL;
      return NULL;
    }

  /* if this value is a reference value then */
  __refcount_ = pEo->pMo->maxderef;
  while( *ppVar && (*ppVar)->vType == VTYPE_REF && __refcount_-- )
    ppVar = (*ppVar)->Value.aValue;
  if( *ppVar && (*ppVar)->vType == VTYPE_REF ){
    *piErrorCode = COMMAND_ERROR_CIRCULAR;
    return NULL;
    }
  while( nHead ){
    nIndex = pEo->CommandArray[nHead-1].Parameter.NodeList.actualm;
    nHead  = pEo->CommandArray[nHead-1].Parameter.NodeList.rest;
    pVar = execute_Evaluate(pEo,
                            nIndex,
                            pMyMortal,
                            piErrorCode,0);
    if( *piErrorCode )return NULL;

    if( pVar )
      lIndex = GETLONGVALUE(pVar);
    else
      lIndex = 0;

    /* If the variable is NOT an array the covert it to array on the fly.
       The referenced index is the only one and gets the scalar value. */
    if( !*ppVar || (*ppVar)->vType != VTYPE_ARRAY ){
      if( *ppVar ){
        if( 0 < lIndex )lMinIndex = 0; else lMinIndex = lIndex;
        if( 0 > lIndex )lMaxIndex = 0; else lMaxIndex = lIndex;
        }else lMinIndex = lMaxIndex = lIndex;
      pVar = memory_NewArray(pEo->pMo, /* memory class */
                             lMinIndex,   /* min index */
                             lMaxIndex);  /* max index */
      if( pVar == NULL ){
        *piErrorCode = EXE_ERROR_MEMORY_LOW;
        return NULL;
        }
      if( *ppVar ){/* if the variable was defined */
        pVar->Value.aValue[-lMinIndex] = *ppVar;
        }
      *ppVar = pVar;
      }
    memory_CheckArrayIndex(pEo->pMo,(*ppVar),lIndex);
    ppVar = (*ppVar)->Value.aValue+lIndex-(*ppVar)->ArrayLowLimit;

    }
  return ppVar;
  }

/* stringcompare two string values. The values SHOULD be string.
*/
static int STRCMP(VARIABLE Op1, VARIABLE Op2, int iCase){
  unsigned long n;
  char *a,*b;
  char ca,cb;

  if( memory_IsUndef(Op1) && memory_IsUndef(Op2) )return 0;
  if( memory_IsUndef(Op1) )return 1;
  if( memory_IsUndef(Op2) )return -1;
  iCase &= 1;/* only the lowest bit is about case sensitivity */
  n = STRLEN(Op1);
  if( n > STRLEN(Op2) ) n= STRLEN(Op2);
  a = STRINGVALUE(Op1);
  b = STRINGVALUE(Op2);
  while( n-- ){
    ca = *a;
    cb = *b;
    if( iCase ){
      if( isupper(ca) )ca = tolower(ca);
      if( isupper(cb) )cb = tolower(cb);
      }
    if( ca != cb )return ( (ca)-(cb) );
    a++;
    b++;
    }
  if( STRLEN(Op1) == STRLEN(Op2) )return 0;
  if( STRLEN(Op1) > STRLEN(Op2) )return 1;
  return -1;
  }


/*POD
=H execute_LeftValueSarray()

This function evaluates an associative array access left value.
This function is also called by R<execute_EvaluateSarray()> and the result
pointer is dereferenced.

/*FUNCTION*/
pFixSizeMemoryObject *execute_LeftValueSarray(pExecuteObject pEo,
                                              unsigned long lExpressionRootNode,
                                              pMortalList pMyMortal,
                                              int *piErrorCode
  ){
/*noverbatim
CUT*/

  long OpCode;
  long lIndex;
  int KeyIsFound;
  unsigned long nVariable, nIndex;
  unsigned long nHead;
  pFixSizeMemoryObject *ppVar,pVar,vIndex,vCurrentKey;
  unsigned long __refcount_;
  int iCase = options_Get(pEo,"compare")&1;

  nHead = pEo->CommandArray[lExpressionRootNode-1].Parameter.Arguments.Argument;
  nVariable = pEo->CommandArray[nHead-1].Parameter.NodeList.actualm;
  OpCode = pEo->CommandArray[nVariable-1].OpCode;

  nHead = pEo->CommandArray[nHead-1].Parameter.NodeList.rest;

  switch( OpCode ){
    case eNTYPE_LVR: /* local variable              */
      if( pEo->LocalVariables == NULL ){
        *piErrorCode = EXE_ERROR_NO_LOCAL;
        return NULL;
        }
      ppVar = &(pEo->LocalVariables->Value.aValue[pEo->CommandArray[nVariable-1].Parameter.Variable.Serial-1]);
      break;
    case eNTYPE_GVR: /* global variable             */
      ppVar = &(pEo->GlobalVariables->Value.aValue[pEo->CommandArray[nVariable-1].Parameter.Variable.Serial-1]);
      break;
    case eNTYPE_ARR:
      ppVar = execute_LeftValueArray(pEo,nVariable,pMyMortal,piErrorCode);
      break;
    case eNTYPE_SAR:
      ppVar = execute_LeftValueSarray(pEo,nVariable,pMyMortal,piErrorCode);
      break;
    default:
      /* the syntax analyzer should not generate anything else than this */
      *piErrorCode =EXE_ERROR_INTERNAL;
      return NULL;
    }

  /* if this value is a reference value then */
  __refcount_ = pEo->pMo->maxderef;
  while( *ppVar && (*ppVar)->vType == VTYPE_REF &&__refcount_-- )
    ppVar = (*ppVar)->Value.aValue;

  if( *ppVar && (*ppVar)->vType == VTYPE_REF ){
    *piErrorCode = COMMAND_ERROR_CIRCULAR;
    return NULL;
    }

  while( nHead ){
    nIndex = pEo->CommandArray[nHead-1].Parameter.NodeList.actualm;
    nHead  = pEo->CommandArray[nHead-1].Parameter.NodeList.rest;
    vIndex = execute_Evaluate(pEo,
                            nIndex,
                            pMyMortal,
                            piErrorCode,0);
    if( *piErrorCode )return NULL;

    /* If the variable is NOT an array the covert it to array on the fly. */
    if( !*ppVar || (*ppVar)->vType != VTYPE_ARRAY ){
      if( *ppVar ){/* if the variable has some value */
        pVar = memory_NewArray(pEo->pMo, /* memory class */
                               0,        /* min index */
                               3);       /* max index */
        if( pVar == NULL ){
          *piErrorCode = EXE_ERROR_MEMORY_LOW;
          return NULL;
          }
        /* the value is stored on the index 0 associated with the value undef */
        pVar->Value.aValue[0] = *ppVar;
        pVar->Value.aValue[1] = NULL;
        pVar->Value.aValue[2] = memory_DupVar(pEo->pMo,vIndex,pMyMortal,piErrorCode);
        memory_Immortalize(pVar->Value.aValue[2],pMyMortal);
        if( *piErrorCode )return NULL;
        pVar->Value.aValue[3] = NULL;
        lIndex = 3;
        }else{/* if the variable does not have any value */
        pVar = memory_NewArray(pEo->pMo, /* memory class */
                               0,        /* min index */
                               1);       /* max index */
        if( pVar == NULL ){
          *piErrorCode = EXE_ERROR_MEMORY_LOW;
          return NULL;
          }
        pVar->Value.aValue[0] = memory_DupVar(pEo->pMo,vIndex,pMyMortal,piErrorCode);
        memory_Immortalize(pVar->Value.aValue[0],pMyMortal);
        if( *piErrorCode )return NULL;
        pVar->Value.aValue[1] = NULL;
        lIndex = 1;
        }
      *ppVar = pVar;
      }else{/* the variable is already an array */
      KeyIsFound = 0;
      for( lIndex = (*ppVar)->ArrayLowLimit ; lIndex < (*ppVar)->ArrayHighLimit ; lIndex += 2 ){
         vCurrentKey = (*ppVar)->Value.aValue[lIndex-(*ppVar)->ArrayLowLimit];
         /* if this value is a reference value then *//*TODO: limit dereferencing */
         while( vCurrentKey && vCurrentKey->vType == VTYPE_REF )vCurrentKey = *vCurrentKey->Value.aValue;
         if( memory_IsUndef(vCurrentKey) && memory_IsUndef(vIndex) ){
           lIndex++;
           KeyIsFound = 1;
           goto KEY_IS_FOUND;
           }
         if( memory_IsUndef(vCurrentKey) )continue;
         if( memory_IsUndef(vIndex) )continue;
         if( vCurrentKey->vType != vIndex->vType )continue;
         switch( vIndex->vType ){
           case VTYPE_LONG:
             if( vIndex->Value.lValue == vCurrentKey->Value.lValue ){
               lIndex++;
               KeyIsFound = 1;
               goto KEY_IS_FOUND;
               }
               break;
           case VTYPE_DOUBLE:
             if( vIndex->Value.dValue == vCurrentKey->Value.dValue ){
               lIndex++;
               KeyIsFound = 1;
               goto KEY_IS_FOUND;
               }
               break;
           case VTYPE_STRING:
             if( !STRCMP(vIndex,vCurrentKey,iCase) ){
               lIndex++;
               KeyIsFound = 1;
               goto KEY_IS_FOUND;
               }
               break;
           case VTYPE_ARRAY:
               /* if this is an array it will not match anything */
               break;
           default:
             *piErrorCode = EXE_ERROR_INTERNAL;
             return NULL;
           }
         }
KEY_IS_FOUND:
      if( ! KeyIsFound ){
        /* The key was not found in the array. Append it to the array with undef value. */
        memory_CheckArrayIndex(pEo->pMo,*ppVar,(*ppVar)->ArrayHighLimit+2);
        /* note that CheckArrayIndex also modifies the ArrayXxxLimit values */
        (*ppVar)->Value.aValue[(*ppVar)->ArrayHighLimit-(*ppVar)->ArrayLowLimit] = NULL;
        (*ppVar)->Value.aValue[(*ppVar)->ArrayHighLimit-(*ppVar)->ArrayLowLimit-1] = memory_DupVar(pEo->pMo,vIndex,pMyMortal,piErrorCode);
        memory_Immortalize((*ppVar)->Value.aValue[(*ppVar)->ArrayHighLimit-(*ppVar)->ArrayLowLimit-1],pMyMortal);
        lIndex = (*ppVar)->ArrayHighLimit;
        }
      }
    memory_CheckArrayIndex(pEo->pMo,(*ppVar),lIndex);
    ppVar = (*ppVar)->Value.aValue+lIndex-(*ppVar)->ArrayLowLimit;
    }
  return ppVar;
  }


/*POD
=H execute_Convert2String()

This functionconverts a variable to string. When the variable is already a string then it returns the pointer to the
variable. When the variable is long or double T<sprintf> is used to convert the number to string.

When the conversion from number to string is done the result is always a newly allocated mortal. In other words
this conversion routine is safe, not modifying the argument memory object.
/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2String(pExecuteObject pEo,
                                          pFixSizeMemoryObject pVar,
                                          pMortalList pMyMortal
  ){
/*noverbatim
CUT*/
  char buffer[256]; /* this size should be enough to represent a number in string format */

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  /* undef is converted to a zero length string */  
  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalString(pEo->pMo,0,pMyMortal);
    if( pVar == NULL )return NULL;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG: 
      sprintf(buffer,"%ld",pVar->Value.lValue);
      break;
    case VTYPE_STRING:
      return pVar;
    case VTYPE_DOUBLE:
      sprintf(buffer,"%lf",pVar->Value.dValue);
      break;
    }
  pVar = memory_NewMortalString(pEo->pMo,strlen(buffer),pMyMortal);
  if( pVar == NULL )return NULL;
  memcpy(pVar->Value.pValue,buffer,strlen(buffer));
  return pVar;
  }


/*POD
=H execute_Convert2Long()

This function should be used to convert a variable to long. The conversion is
usually done in place. However strings can not be converted into long in place, because
they have different size. In such a case a new variable is created. If the mortal list T<pMyMortal>
is T<NULL> then the new variable in not mortal. In such a case care should be taken
to release the original variable.

Usually there is a mortal list and a new mortal variable is generated. In such a case
the original value is also a mortal and is automatically released after the command
executing the conversion is finished.

Note that strings are converted to long in two steps. The first step converts the string to
T<double> and then this value is converted to long in-place.


/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2Long(pExecuteObject pEo,
                                          pFixSizeMemoryObject pVar,
                                          pMortalList pMyMortal
  ){
/*noverbatim
CUT*/
  char *s;
  int lintpart;
  double intpart,fracpart,exppart,man;
  int i,esig,isig;
  unsigned long sLen;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalLong(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.lValue = 0;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG: return pVar; /* it is already long */
    case VTYPE_STRING:
      s = (char *)pVar->Value.pValue;
      sLen = pVar->Size;
      while( isspace( *s ) && sLen ){
        s++; /*leading spaces don't matter*/
        sLen--;
        }
      isig = 1; esig =1;
      if( *s == '-' )isig = -1;
      if( sLen )
        if( *s == '-' || *s == '+' ){ s++; sLen--; }
      for( lintpart = 0 ; sLen && isdigit(*s) ; s++,sLen-- ){
        lintpart *= 10;
        lintpart += *s -'0';
        }
      if( (!sLen) || (*s != '.' && *s != 'e' && *s != 'E') ){
        pVar = memory_NewMortalLong(pEo->pMo,pMyMortal);
        if( pVar == NULL )return NULL;
        pVar->Value.lValue = isig*lintpart;
        return pVar;
        }
      intpart = lintpart;
      fracpart = 0.0;
      if( sLen && *s == '.' ){
        s++;     /* step over the decimal dot */
        sLen --;
        i = 0; /* this is not an integer anymore */
        fracpart = 0.0; /* fractional part */
        man = 1.0;      /* actual mantissa */
        for(  ; isdigit(*s) && sLen ; s++, sLen-- )
          fracpart += (man *= 0.1) * (*s-'0');
        }
      if( sLen && (*s == 'E' || *s == 'e') ){
        i = 0; /* this is not an integer anymore if it has exponential part */
        s++; /* step over the character E */
        sLen --;
        if( *s == '-' )esig=-1; else esig = 1;
        if( sLen )
          if( *s == '+' || *s == '-'){ s++; sLen--; } /* step over the exponential sign */
        for( exppart=0.0 , i = 0 ; sLen && isdigit(*s) ; s++, sLen-- )
          exppart = 10*exppart + *s-'0';
        }else exppart = 0.0;
      pVar = memory_NewMortalLong(pEo->pMo,pMyMortal);
      if( pVar == NULL )return NULL;
      pVar->Value.lValue = (long)(isig*(intpart + fracpart)*pow10(esig*exppart));
      return pVar;
    case VTYPE_DOUBLE:
      pVar->vType = VTYPE_LONG;
      pVar->Value.lValue = (long)pVar->Value.dValue;
      return pVar;
    }
  }

/*POD
=H execute_Convert2LongS()

This is the safe version of the conversion function R<execute_Convert2Long()>.

This function ALWAYS create a new variable and does NOT convert a
double to long in place. This function is called by the extensions,
because extensions tend to be more laisy regarding conversion and
many converts arguments in place and thus introduce side effect.

To solve this problem we have introduced this function and have
set the support table to point to this function.
/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2LongS(pExecuteObject pEo,
                                           pFixSizeMemoryObject pVar,
                                           pMortalList pMyMortal
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject pVarr;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalLong(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.lValue = 0;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG: return pVar; /* it is already long */
    case VTYPE_STRING:
      /* strings are NOT converted to long in place by
         the original functions as well, so we can use it */
      return execute_Convert2Long(pEo,pVar,pMyMortal);

    case VTYPE_DOUBLE:
      pVarr = memory_NewMortalLong(pEo->pMo,pMyMortal);
      if( pVarr == NULL )return NULL;
      pVarr->vType = VTYPE_LONG;
      pVarr->Value.lValue = (long)pVar->Value.dValue;
      return pVarr;
    }
  }


/*POD
=H execute_Convert2Double()

This function should be used to convert a variable to double. The conversion is
usually done in place. However strings can not be converted into double in place, because
they have different size. In such a case a new variable is created. If the mortal list
is T<NULL> then the new variable in not mortal. In such a case care should be taken
to release the original variable.

Usually there is a mortal list and a new mortal variable is generated. In such a case
the original value is also a mortal and is automatically released after the command
executing the conversion is finished.

/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2Double(pExecuteObject pEo,
                                            pFixSizeMemoryObject pVar,
                                            pMortalList pMyMortal
  ){
/*noverbatim
CUT*/
  char *s;
  double intpart,fracpart,exppart,man;
  int i,esig,isig;
  unsigned long sLen;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalDouble(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.dValue = 0.0;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG:
      pVar->vType = VTYPE_DOUBLE;
      pVar->Value.dValue = (double)pVar->Value.lValue;
      return pVar;
    case VTYPE_DOUBLE:
      return pVar;
    case VTYPE_STRING:
      s = (char *)pVar->Value.pValue;
      sLen = pVar->Size;
      while( isspace( *s ) && sLen ){
        s++; /*leading spaces don't matter*/
        sLen--;
        }
      isig = 1; esig =1;
      if( *s == '-' )isig = -1;
      if( sLen )
        if( *s == '-' || *s == '+' ){ s++; sLen--; }
      for( intpart = 0 ; sLen && isdigit(*s) ; s++,sLen-- ){
        intpart *= 10;
        intpart += *s -'0';
        }
      fracpart = 0.0;
      if( sLen && *s == '.' ){
        s++;     /* step over the decimal dot */
        sLen --;
        i = 0; /* this is not an integer anymore */
        fracpart = 0.0; /* fractional part */
        man = 1.0;      /* actual mantissa */
        for(  ; isdigit(*s) && sLen ; s++, sLen-- )
          fracpart += (man *= 0.1) * (*s-'0');
        }
      if( sLen && (*s == 'E' || *s == 'e') ){
        i = 0; /* this is not an integer anymore if it has exponential part */
        s++; /* step over the character E */
        sLen --;
        if( *s == '-' )esig=-1; else esig = 1;
        if( sLen )
          if( *s == '+' || *s == '-'){ s++; sLen--; } /* step over the exponential sign */
        for( exppart=0.0 , i = 0 ; sLen && isdigit(*s) ; s++, sLen-- )
          exppart = 10*exppart + *s-'0';
        }else exppart = 0.0;
    pVar = memory_NewMortalDouble(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.dValue = isig*(intpart + fracpart)*pow10(esig*exppart);
    return pVar;
    }
  }

/*POD
=H execute_Convert2DoubleS()

This is the safe version of the conversion function R<execute_Convert2Double()>.

This function ALWAYS create a new variable and does NOT convert a
long to double in place. This function is called by the extensions,
because extensions tend to be more laisy regarding conversion and
many converts arguments in place and thus introduce side effect.

To solve this problem we have introduced this function and have
set the support table to point to this function.

/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2DoubleS(pExecuteObject pEo,
                                             pFixSizeMemoryObject pVar,
                                             pMortalList pMyMortal
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject pVarr;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalDouble(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.dValue = 0.0;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG:
      pVarr = memory_NewMortalDouble(pEo->pMo,pMyMortal);
      if( pVarr == NULL )return NULL;
      pVarr->vType = VTYPE_DOUBLE;
      pVarr->Value.dValue = (double)pVar->Value.lValue;
      return pVarr;
    case VTYPE_DOUBLE:
      return pVar;
    case VTYPE_STRING:
      /* strings are not converted in place by default */
      return execute_Convert2Double(pEo,pVar,pMyMortal);
    }
  }


/*POD
=H execute_Convert2Numeric()


This function should be used to convert a variable to numeric type.

The conversion results a double or long variable. If the source variable
was already a long or double the function does nothing but results the
source variable.

T<undef> is converted to long zero.

The function calls R<execute_Convert2Long> and R<execute_Convert2Double> thus
all other parameters are treated according to that.

/*FUNCTION*/
pFixSizeMemoryObject execute_Convert2Numeric(pExecuteObject pEo,
                                             pFixSizeMemoryObject pVar,
                                             pMortalList pMyMortal
  ){
/*noverbatim
CUT*/

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) ){
    pVar = memory_NewMortalLong(pEo->pMo,pMyMortal);
    if( pVar == NULL )return NULL;
    pVar->Value.lValue = 0;
    return pVar;
    }

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return NULL;
    case VTYPE_LONG:
      return pVar;
    case VTYPE_DOUBLE:
      return pVar;
    case VTYPE_STRING:
      if( ISSTRINGINTEGER(pVar) )
        return execute_Convert2Long(pEo,pVar,pMyMortal);
      else
        return execute_Convert2Double(pEo,pVar,pMyMortal);
    }
  }

/*POD
=H execute_Dereference()

This function recursively follows variable references and returns
the original variable that was referenced by the original variable.

A reference variable is a special variable that does not hold value
itself but rather a pointer to another variable. Such reference variables
are used when arguments are passed by reference to BASIC subroutines.

Calling this function the caller can get the original variable and the
valéue of the original variable rather than a reference.
/*FUNCTION*/
pFixSizeMemoryObject execute_Dereference(pExecuteObject pEo,
                                         pFixSizeMemoryObject p,
                                         int *piErrorCode
  ){
/*noverbatim
See also R<execute_DereferenceS()>.
CUT*/
  unsigned long refcount;

  if( *piErrorCode )return p;
  refcount = pEo->pMo->maxderef;
  while( p && TYPE(p) == VTYPE_REF ){
    p = *(p->Value.aValue);
    if( ! refcount-- ){
      *piErrorCode = COMMAND_ERROR_CIRCULAR;
      return NULL;
      }
    }
  return p;  
  }

/*POD
=H execute_DereferenceS()

This function does the same as R<execute_Dereference()> except that it has
different arguments fitted to support external modules and T<besXXX>
macros.

/*FUNCTION*/
int execute_DereferenceS(unsigned long refcount,
                         pFixSizeMemoryObject *p
  ){
/*noverbatim
See also R<execute_Dereference()>.

If the argument is referencing an T<undef> value then this function
converts the argument to be a real T<NULL> to allow external modules
to compare T<besDEREFERENCE>d variables against T<NULL>.

The subroutine is also error prone handling T<NULL> pointer as argument,
though it should never be happen if the external module programmer
uses the macro T<besDEREFERENCE>.
CUT*/
  while( p && *p && TYPE(*p) == VTYPE_REF ){
    *p = *((*p)->Value.aValue);
    if( ! refcount-- )return COMMAND_ERROR_CIRCULAR;
    }
  /* extension modules like to chack undef against NULL pointer, let them do */
  if( p && *p && TYPE(*p) == VTYPE_UNDEF )*p = NULL;
  return EXE_ERROR_SUCCESS;  
  }

/*POD
=H execute_GetDoubleValue()

Use this function whenever you want to access the B<value> of a variable as a T<double>.
Formerly ScriptBasic in such situation converted the variable to double calling
R<execute_Convert2Double()> and then used the macro T<DOUBLEVALUE>. This method is faster
because this does not create a new mortal variable but returns directly the
double value.

The macro T<GETDOUBLEVALUE> can be used to call this function with the default
execution environment variable T<pEo>

Note however that the macro T<GETDOUBLEVALUE> and T<DOUBLEVALUE> are not 
interchangeable. T<GETDOUBLEVALUE> is returnig a T<double> while 
T<DOUBLEVALUE> is a left value available to store a T<double>.

/*FUNCTION*/
double execute_GetDoubleValue(pExecuteObject pEo,
                              pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  char *s;
  double intpart,fracpart,exppart,man;
  int i,esig,isig;
  unsigned long sLen;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) )return 0.0;

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return 0.0;
    case VTYPE_LONG:
      return (double)pVar->Value.lValue;
    case VTYPE_DOUBLE:
      return pVar->Value.dValue;
    case VTYPE_STRING:
      s = (char *)pVar->Value.pValue;
      sLen = pVar->Size;
      while( isspace( *s ) && sLen ){
        s++; /*leading spaces don't matter*/
        sLen--;
        }
      isig = 1; esig =1;
      if( *s == '-' )isig = -1;
      if( sLen )
        if( *s == '-' || *s == '+' ){ s++; sLen--; }
      for( intpart = 0 ; sLen && isdigit(*s) ; s++,sLen-- ){
        intpart *= 10;
        intpart += *s -'0';
        }
      fracpart = 0.0;
      if( sLen && *s == '.' ){
        s++;     /* step over the decimal dot */
        sLen --;
        i = 0; /* this is not an integer anymore */
        fracpart = 0.0; /* fractional part */
        man = 1.0;      /* actual mantissa */
        for(  ; isdigit(*s) && sLen ; s++, sLen-- )
          fracpart += (man *= 0.1) * (*s-'0');
        }
      if( sLen && (*s == 'E' || *s == 'e') ){
        i = 0; /* this is not an integer anymore if it has exponential part */
        s++; /* step over the character E */
        sLen --;
        if( *s == '-' )esig=-1; else esig = 1;
        if( sLen )
          if( *s == '+' || *s == '-'){ s++; sLen--; } /* step over the exponential sign */
        for( exppart=0.0 , i = 0 ; sLen && isdigit(*s) ; s++, sLen-- )
          exppart = 10*exppart + *s-'0';
        }else exppart = 0.0;
    return isig*(intpart + fracpart)*pow10(esig*exppart);
    }
  }

/*POD
=H execute_GetLongValue()

Use this function whenever you want to access the B<value> of a variable as a T<long>.
Formerly ScriptBasic in such situation converted the variable to long calling
R<execute_Convert2Long()> and then used the macro T<LONGVALUE>. This method is faster
because this does not create a new mortal variable but returns directly the
long value.

The macro T<GETLONGVALUE> can be used to call this function with the default
execution environment variable T<pEo>

Note however that the macro T<GETLONGVALUE> and T<LONGVALUE> are not 
interchangeable. T<GETLONGVALUE> is returnig a T<long> while 
T<LONGVALUE> is a left value available to store a T<long>.

/*FUNCTION*/
long execute_GetLongValue(pExecuteObject pEo,
                          pFixSizeMemoryObject pVar
  ){
/*noverbatim

Please also note that the result of converting a string variable to LONG and then
accessing its longvalue may not result the same number as calling this function.
The reason is that conversion of a string to a LONG variable is done in two steps.
First it converts the string to a T<double> and then it rounds the T<double> value
to T<long>. On the other hand this function converts a string diretly to T<long>.

For example the string T<"3.7"> becomes 4 when converted to long and 3 when getting the
value as a long.

CUT*/
  char *s;
  long lintpart;
  double intpart,fracpart,exppart,man;
  int i,esig,isig;
  unsigned long sLen;

  while( pVar && pVar->vType == VTYPE_ARRAY )
    pVar = pVar->Value.aValue[pVar->ArrayLowLimit];

  if( memory_IsUndef(pVar) )return 0;

  execute_DereferenceS(pEo->pMo->maxderef,&pVar);

  switch( pVar->vType ){
    default: return 0;
    case VTYPE_LONG: return pVar->Value.lValue;
    case VTYPE_STRING:
      s = (char *)pVar->Value.pValue;
      sLen = pVar->Size;
      while( isspace( *s ) && sLen ){
        s++; /*leading spaces don't matter*/
        sLen--;
        }
      isig = 1;
      if( *s == '-' )isig = -1;
      if( sLen )
        if( *s == '-' || *s == '+' ){ s++; sLen--; }
      for( lintpart = 0 ; sLen && isdigit(*s) ; s++,sLen-- ){
        lintpart *= 10;
        lintpart += *s -'0';
        }
      /* if there is no fractional part then return it */
      if( (!sLen) || (*s != '.' && *s != 'e' && *s != 'E') )return isig*lintpart;
      intpart = lintpart;
      fracpart = 0.0;
      if( sLen && *s == '.' ){
        s++;     /* step over the decimal dot */
        sLen --;
        i = 0; /* this is not an integer anymore */
        fracpart = 0.0; /* fractional part */
        man = 1.0;      /* actual mantissa */
        for(  ; isdigit(*s) && sLen ; s++, sLen-- )
          fracpart += (man *= 0.1) * (*s-'0');
        }
      if( sLen && (*s == 'E' || *s == 'e') ){
        i = 0; /* this is not an integer anymore if it has exponential part */
        s++; /* step over the character E */
        sLen --;
        if( *s == '-' )esig=-1; else esig = 1;
        if( sLen )
          if( *s == '+' || *s == '-'){ s++; sLen--; } /* step over the exponential sign */
        for( exppart=0.0 , i = 0 ; sLen && isdigit(*s) ; s++, sLen-- )
          exppart = 10*exppart + *s-'0';
        }else exppart = 0.0;
    return (long)(isig*(intpart + fracpart)*pow10(esig*exppart));
    case VTYPE_DOUBLE:
      return (long)pVar->Value.dValue;
    }
  }


/*POD
=H execute_IsStringInteger()

This function should be used to check a string before converting it to numeric value.
If the string contains only digits it should be converted to T<long>. If the string contains
other characters then it should be converted to double. This function decides what characters
the string contains.

/*FUNCTION*/
int execute_IsStringInteger(pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  char *s;
  double mantissa,fracpart,frac;
  long sLen;
  long fraclen,fracreallen;
  long exponent;
  int sig,esig;

  if( memory_IsUndef(pVar) || pVar->vType != VTYPE_STRING )return 0;
  s = (char *)pVar->Value.pValue;
  sLen = pVar->Size;
  while( isspace(*s) && sLen ){
    s++; /* leading spaces dont matter */
    sLen--;
    }
  sig = 1;
  if( sLen )
    if( *s == '+' || *s == '-' ){
      sig = *s == '+';
      s++;
      sLen--;
      }
  /* calculate the mantissa, to check the actual size against LONG_MAX and LONG_MIN*/
  mantissa = 0.0;
  fracreallen = 0;
  while( sLen && isdigit(*s) ){
    mantissa *= 10;
    mantissa += (double)*s - '0';
    /* Calculate the number of zeroes before the fractional dot. This was 1000.0E-3 will be recognized as integer. */
    if( '0' == *s )fracreallen --;
    s++;
    sLen--;
    }

  /* if there are no more charaters after the digits */
  if( sLen == 0 )
    return sig ? mantissa <= LONG_MAX : mantissa <= -(LONG_MIN);

  /* if this is not a correct number */
  if( *s != '.' && *s != 'e' && *s != 'E' )return 1;

  fraclen = 0;     /* the number of digits in the fractional part */
  fracpart = 0.0;  /* the fractional part of the number. we need this to check the final result against LONG_MAX and LONG_MIN*/
  frac = 0.1;      /* the magnitude of the actual fractional digit. This is divided by 10 for each new digit */
  if( *s == '.' ){
    s++;
    sLen --;
    while( sLen && isdigit(*s) ){
      fracpart += frac * ( *s - '0' );
      frac /= 10.0;
      fraclen ++;
      if( *s != '0' )fracreallen = fraclen;
      s++;
      sLen--;
      }
    }
  esig = 1;
  exponent = 0;
  if( sLen && ( *s == 'e' || *s == 'E' ) ){
    sLen--;
    s++;
    if( sLen && ( *s == '+' || *s == '-' ) ){
      if( *s == '-' )esig = -1;
      sLen--;
      s++;
      }
    while( sLen && isdigit(*s) ){
      exponent *= 10;
      exponent += *s -'0';
      s++;
      sLen--;
      }
    }
  /* if there is non-zero fractional part */
  /* note that fracreallen can be negative */
  if( fracreallen > esig * exponent )return 0;
  
  /* here we can be sure that the string is an integer number, but still it may be larger than LONG_MAX or smaller than LONG_MIN */
  mantissa += fracpart;
  mantissa *= pow10(esig *exponent);
  return sig ? mantissa <= LONG_MAX : mantissa <= -(LONG_MIN);
  }

/*POD
=H execute_IsInteger()

This function checks that a variable being long, double or string can be
converted to long without loosing information.

/*FUNCTION*/
int execute_IsInteger(pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  long lTest;

  /* Although the function GetLongValue converts an undef value to zero 
     it should not actually.   */
  if( memory_IsUndef(pVar) )return 0;

  switch( TYPE(pVar) ){
    case VTYPE_LONG: return 1; /* obvious */

    case VTYPE_DOUBLE: /* return floor(DOUBLEVALUE(pVar)) == DOUBLEVALUE(pVar);*/
      lTest = (long)DOUBLEVALUE(pVar);
      return DOUBLEVALUE(pVar) == (double)lTest;

    case VTYPE_STRING: return ISSTRINGINTEGER(pVar);
    default: return 0;
    }

  }
