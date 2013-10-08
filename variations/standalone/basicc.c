#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../../getopt.h"
#include "../../report.h"
#include "../../lexer.h"
#include "../../sym.h"
#include "../../expression.h"
#include "../../syntax.h"
#include "../../reader.h"
#include "../../myalloc.h"
#include "../../builder.h"
#include "../../memory.h"
#include "../../execute.h"
#include "../../buildnum.h"
#include "../../conftree.h"
#include "../../filesys.h"
#include "../../errcodes.h"
#include "../../testalloc.h"
#include "../../modumana.h"

int GetC(void *f){ return getc((FILE *)f); }

#ifdef WIN32
main(int argc, char *argv[]){
#else
char **_environ;
main(int argc, char *argv[], char *env[]){
#endif
  extern unsigned char szCommandArray[] ;
  extern unsigned long ulStartNode,
                       ulNodeCounter,
                       ulStringTableSize,
                       ulGlobalVariables;
  void *pMEM;
  extern unsigned char szStringTable[];

  tConfigTree MyCONF;
  ExecuteObject MyEXE;
  int iError;
#define FULL_PATH_BUFFER_LENGTH 256
  char CmdLinBuffer[FULL_PATH_BUFFER_LENGTH],*s;
#ifndef WIN32
  _environ = env;
#endif

  pMEM = alloc_InitSegment(malloc,free);
  if( pMEM == NULL ){
    fprintf(stderr,"No memory\n");
    exit(1);
    }
  cft_start(&MyCONF,alloc_Alloc,alloc_Free,pMEM,
#ifdef WIN32
            "Software\\ScriptBasic\\config",
            "SCRIBA.INI",
#else
            "SCRIBACONF",
            "/etc/scriba/basic.conf",
#endif
            NULL);

  MyEXE.memory_allocating_function = malloc;
  MyEXE.memory_releasing_function = free;
  MyEXE.reportptr = (void *)stderr;
  MyEXE.report   = report_report;
  MyEXE.fErrorFlags = 0;

  MyEXE.pConfig = &MyCONF;
  build_MagicCode(&(MyEXE.Ver));

  build_MagicCode(&(MyEXE.Ver));
  MyEXE.fpStdinFunction = NULL;
  MyEXE.fpStdouFunction = NULL;
  MyEXE.fpEnvirFunction = NULL;
  MyEXE.CmdLineArgument = NULL;

  s = cft_GetString(MyEXE.pConfig,"maxstep");
  MyEXE.GlobalStepLimit = s ? atol(s) : 0;
  s = cft_GetString(MyEXE.pConfig,"maxlocalstep");
  MyEXE.LocalStepLimit  = s ? atol(s) : 0;
  s = cft_GetString(MyEXE.pConfig,"maxlevel");
  MyEXE.FunctionLevelLimit = s ? atol(s) : 0;

  MyEXE.CommandArray = (pcNODE)szCommandArray ;
  MyEXE.StartNode = ulStartNode;
  MyEXE.CommandArraySize = ulNodeCounter;
  MyEXE.StringTable = szStringTable;
  MyEXE.cbStringTable = ulStringTableSize;
  MyEXE.cGlobalVariables = ulGlobalVariables;
  MyEXE.lGlobalStepCounter = 0L;

  MyEXE.pCommandFunction = CommandFunction;
  MyEXE.CSymbolList = COMMANDSYMBOLS;
  MyEXE.fThreadedCommandTable = 0;
  MyEXE.pFunctionResult = NULL;

  MyEXE.pST = NULL;
  MyEXE.modules = NULL;

  MyEXE.pMemorySegment = alloc_InitSegment(MyEXE.memory_allocating_function,MyEXE.memory_releasing_function);

  if( MyEXE.pMemorySegment == NULL )return 1;


  MyEXE.pMo = alloc_Alloc(sizeof(MemoryObject),MyEXE.pMemorySegment);
  if( MyEXE.pMo == NULL )return 1;
  MyEXE.pMo->memory_allocating_function = MyEXE.memory_allocating_function;
  MyEXE.pMo->memory_releasing_function = MyEXE.memory_releasing_function;
  MyEXE.cbStringTable = 0L;
  MyEXE.OptionsTable = NULL;
  memory_InitStructure(MyEXE.pMo);

  s = cft_GetString(MyEXE.pConfig,"maxmem");
  if( s )alloc_SegmentLimit(MyEXE.pMo->pMemorySegment,atol(s));


  memory_RegisterTypes(MyEXE.pMo);
  if( hook_Init(&MyEXE, &(MyEXE.pHookers)) )return 1;
  if( modu_Preload(&MyEXE) )return 1;

/*
  We could alter the standard input, standard output, the environment
  function and command arguments here . We do only the command
  arguments here in this variation.
*/
  MyEXE.CmdLineArgument = CmdLinBuffer;  

  execute_Execute(&MyEXE,&iError);
  alloc_FinishSegment(MyEXE.pMo->pMemorySegment);
  alloc_FinishSegment(MyEXE.pMemorySegment);
  alloc_FinishSegment(MyCONF.pMemorySegment);
#ifdef _DEBUG
  printf("Press any key ...\n");
  getchar();
#endif
  exit(0);
  }

