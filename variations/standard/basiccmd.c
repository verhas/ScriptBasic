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
#include "../../basext.h"
#include "../../epreproc.h"
#include "../../uniqfnam.h"

int GetC(void *f){ return getc((FILE *)f); }

#if BCC32
char *_pgmptr;
#endif

#ifdef WIN32
main(int argc, char *argv[]){
#else
char **_environ;
main(int argc, char *argv[], char *env[]){
#endif
  int OptionIndex;
  LexObject MyLEX;
  ReadObject MyREAD;
  BuildObject MyBUILD;
  tConfigTree MyCONF;
  char *pszForcedConfigurationFileName;
  void *pMEM;
  eXobject MyEX;
  ExecuteObject MyEXE;
  peNODE_l CommandList;
  int iError,iErrorCounter;
  unsigned long fErrorFlags;
  char *szInputFile,
       *szOutputFile;
  char *optarg,opt;
  /* the maximal number of preprocessors that are applied in chain */
#define MAXPREPROC 100
  char *pszEPreproc[MAXPREPROC],*pszPreprocessedFileName;
  int iPreprocIndex;
  int giveusage,binarycode,nocache,iscgi,isCoutput;
  int execute;
#define FULL_PATH_BUFFER_LENGTH 256
  char CmdLinBuffer[FULL_PATH_BUFFER_LENGTH],*szCache,*s,*q;
  char Argv0[FULL_PATH_BUFFER_LENGTH];
  char CachedFileName[FULL_PATH_BUFFER_LENGTH];
  unsigned long FileTime,CacheTime;
#ifndef WIN32
  _environ = env;
#if BCC32
  _pgptr = argv[0];
#endif
#endif

#ifdef _DEBUG
#define malloc testa_Alloc
#define free testa_Free
  testa_InitSegment();
#endif

  /* default values for command line options */
  szInputFile = NULL;
  szOutputFile = NULL;
  iPreprocIndex = 0; /* no external preprocessor by default */
  pszPreprocessedFileName = NULL;
  *pszEPreproc = NULL; 
  giveusage  = 0; /* assume the command line is correct, we need not display usage and stop */
  binarycode = 0; /* input is not binary by default */
  execute    = 0; /* do not execute by default after binary format save */
  nocache   = 0; /* we use cached code if it exists */
  OptionIndex = 0;
  iscgi = 0; /* by default this is not a cgi script, not HTTP/1.0 ... when error message is sent */
  isCoutput = 0;
  pszForcedConfigurationFileName = NULL;

  while( (opt = getoptt(argc, argv, "f:pCcnvebo:",&optarg,&OptionIndex)) != ':'){
    switch( opt ){
      case 'p' :
        if( iPreprocIndex >= MAXPREPROC-1 )
          giveusage = 1;
        else{
          pszEPreproc[iPreprocIndex ++ ] = optarg;
          pszEPreproc[iPreprocIndex] = NULL;
          }
         break;
      case 'C' :
        if( isCoutput )giveusage = 1;
        isCoutput = 1;
        break;
      case 'c' :
        if( iscgi )giveusage = 1;
        iscgi = 1;
        break;
      case 'n' :
        if( nocache )giveusage = 1;
        nocache = 1;
        break;
      case 'e' :
        if( execute )giveusage = 1;
        execute = 1;
        break;
      case 'f' :
        if( pszForcedConfigurationFileName )giveusage = 1;
        pszForcedConfigurationFileName = optarg;
        break;
      case 'o' :
        if( szOutputFile || binarycode )giveusage = 1;
        szOutputFile = optarg;
        break;
      case 'b':
        if( szOutputFile || binarycode )giveusage = 1;
        binarycode =1;
        break;
      case 'v':
#define S fprintf(stderr,
#define E );
                   S "ScriptBasic v%ld.%ld\n",VERSION_HIGH,VERSION_LOW E
                   S "Variation >>%s<< build %ld\n",VARIATION,SCRIPTBASIC_BUILD E
                   S "Magic value %lu\n",build_MagicCode(NULL) E
                   S "Node size is %d\n", sizeof(cNODE) E
                   S "Extension interface version is %d\n",INTERFACE_VERSION E
                   S "Compilation: %s %s\n", __DATE__,__TIME__ E
        exit(0);
      case '!' :
        giveusage = 1;
        break;
      case '?':
        if( szInputFile )giveusage = 1;
        szInputFile = optarg;
        CmdLinBuffer[0] = (char)0;
        while( OptionIndex < argc ){
          strcat(CmdLinBuffer,argv[OptionIndex++]);
          if( OptionIndex < argc )
            strcat(CmdLinBuffer," ");
          }
        goto CmdLineFinished;
      }
    }

CmdLineFinished:
  if( execute && binarycode )giveusage=1;
  if( isCoutput && !szOutputFile )giveusage=1;

  if( szInputFile == NULL || giveusage ){
#define U(x) fprintf(stderr,"%s\n",(x));
    U("Usage: basic [options] program.bas")
    U("")
    U("options: -o file_name")
    U("            save binary format to file but don't execute")
    U("         -b file_name")
    U("            load binary format from file and execute")
    U("         -n")
    U("            do not use cache (no save, no load)")
    U("         -e")
    U("            execute after binary format was saved")
    U("         -v")
    U("            print version info and stop")
    U("         -c")
    U("            inform scriba that this is a CGI script.")
    U("         -C");
    U("            save C program output.");
    U("         -p preprocessor");
    U("            specify preprocessor.");
    U("         -f configurationfile");
    U("            specify configuration file");
    exit(1);
    }

  pMEM = alloc_InitSegment(malloc,free);
  if( pMEM == NULL ){
    fprintf(stderr,"No memory\n");
    exit(1);
    }

  cft_start(&MyCONF,alloc_Alloc,alloc_Free,pMEM,
#ifdef WIN32
            "Software\\ScriptBasic\\config",
            "WINNT\\SCRIBA.INI",
#else
            "SCRIBACONF",
            "/etc/scriba/basic.conf",
#endif
            pszForcedConfigurationFileName);

  szCache = nocache ? NULL : cft_GetString(&MyCONF,"cache");
  if( szCache ){
    strcpy(CachedFileName,szCache);
    s = CachedFileName + strlen(CachedFileName); /* point to the end of the cache directory */

#ifdef WIN32
/* under Win32 we convert the argv[0] to the full path file name */
    if( GetFullPathName(szInputFile,
                        FULL_PATH_BUFFER_LENGTH-strlen(CachedFileName),s,&q)==0 )
      goto UseNoCache;
#else
/* under UNIX we can not convert, but it usually contains the full path */
    strcpy(s,szInputFile);
#endif
    strcpy(Argv0,s);
    uniqfnam(s,s);
    FileTime  = file_time_modified(szInputFile);
    CacheTime = file_time_modified(CachedFileName);
    if( FileTime && CacheTime && CacheTime > FileTime ){
      szInputFile = CachedFileName;
      /* Check that the cache file is in correct format.
         This call will help upgrade without deleting the cache
         files. Former versions tried to load the binary code created by
         a previous version of scriba and resulted error messages
         complaining about corrupt binary file whe trying to execute
         a syntactically correct BASIC program.      */
      binarycode = build_IsFileBinaryFormat(szInputFile);
      }
    }else{/* if no cache is configured */
#ifdef WIN32
    if( GetFullPathName(szInputFile,
                      FULL_PATH_BUFFER_LENGTH,Argv0,&q) == 0 )
#endif
    strcpy(Argv0,szInputFile);
    }
UseNoCache:


  if( binarycode || build_IsFileBinaryFormat(szInputFile) ){
    /* the code is given as a compiled code */
    MyBUILD.memory_allocating_function = malloc;
    MyBUILD.memory_releasing_function  = free;
    MyBUILD.iErrorCounter = 0;
    MyBUILD.reportptr = (void *)stderr;
    MyBUILD.report   = report_report;
    MyBUILD.fErrorFlags = iscgi ? REPORT_F_CGI : 0;
    build_LoadCode(&MyBUILD,szInputFile);
    if( MyBUILD.iErrorCounter ){
#ifdef _DEBUG
      getchar();
#endif
      exit(1);
      }
	binarycode = 1;
    }else{
	binarycode = 0;/* this signals that we are reading non binary code */
    /* the code is source code basic text format */
    fErrorFlags = iscgi ? REPORT_F_CGI : 0;
    iErrorCounter = 0;
    iError = epreproc(&MyCONF,szInputFile,&pszPreprocessedFileName,pszEPreproc,malloc,free);
    if( iError ){
      report_report(stderr,"",0,iError,REPORT_ERROR,&iErrorCounter,NULL,&fErrorFlags);
#ifdef _DEBUG
      getchar();
#endif
      /* there were errors during preprocess */
      exit(1);
      }

    /* read the lines of the program */
    reader_InitStructure(&MyREAD);
    MyREAD.memory_allocating_function = alloc_Alloc;
    MyREAD.memory_releasing_function = alloc_Free;
    MyREAD.pMemorySegment = alloc_InitSegment(malloc,free);
    MyREAD.report = report_report;
    MyREAD.reportptr = (void *)stderr;
    MyREAD.iErrorCounter = 0;
    MyREAD.fErrorFlags = iscgi ? REPORT_F_CGI : 0;
    MyREAD.pConfig = &MyCONF;
    if( pszPreprocessedFileName )
      reader_ReadLines(&MyREAD,pszPreprocessedFileName);
    else
      reader_ReadLines(&MyREAD,szInputFile);

/*    reader_DumpLines(&MyREAD,stderr); */

    if( MyREAD.iErrorCounter ){
#ifdef _DEBUG
      getchar();
#endif
      /* there were errors during lexical analisys */
      exit(1);
      }

    reader_StartIteration(&MyREAD);

    MyLEX.memory_allocating_function = alloc_Alloc;
    MyLEX.memory_releasing_function = alloc_Free;
    MyLEX.pMemorySegment = alloc_InitSegment(malloc,free);
    lex_InitStructure(&MyLEX);

    MyLEX.pfGetCharacter = reader_NextCharacter;
    MyLEX.pfFileName = reader_FileName;
    MyLEX.pfLineNumber = reader_LineNumber;

    MyLEX.pNASymbols = NASYMBOLS;
    MyLEX.pASymbols  = ASYMBOLS;
    MyLEX.pCSymbols  = CSYMBOLS;
    MyLEX.report = report_report;
    MyLEX.reportptr = (void *)stderr;
    MyLEX.fErrorFlags = MyREAD.fErrorFlags;
    MyLEX.iErrorCounter = 0;
    MyLEX.pLexResult = (void *)stderr;


    MyLEX.pvInput = (void *)&MyREAD;
    lex_ReadInput(&MyLEX);

    if( MyLEX.iErrorCounter ){
#ifdef _DEBUG
      getchar();
#endif
      /* there were errors during lexical analisys */
      exit(1);
      }

    lex_RemoveComments(&MyLEX);
    lex_HandleContinuationLines(&MyLEX);

    MyEX.memory_allocating_function = malloc;
    MyEX.memory_releasing_function = free;
    MyEX.cbBuffer = 1024; /* init will allocate the space of this number of characters */
    MyEX.cbCurrentNameSpace = 1024; /* init will allocate the space of this number of characters */
    MyEX.pLex = &MyLEX;

    MyEX.Unaries  = UNARIES;
    MyEX.Binaries = BINARIES;
    MyEX.BuiltInFunctions = INTERNALFUNCTIONS;
    MyEX.MAXPREC  = MAX_BINARY_OPERATOR_PRECEDENCE;
    MyEX.PredeclaredLongConstants = PREDLCONSTS;
    MyEX.reportptr = (void *)stderr;
    MyEX.report   = report_report;
    MyEX.fErrorFlags = MyLEX.fErrorFlags;
    MyEX.iErrorCounter = 0;

    MyEX.Command = COMMANDS;

    ex_init(&MyEX);

    ex_Command_l(&MyEX,&CommandList);

    if( MyEX.iErrorCounter ){
#ifdef _DEBUG
      getchar();
#endif
      exit(1);
      }

    MyEX.pCommandList = CommandList;

    MyBUILD.memory_allocating_function = malloc;
    MyBUILD.memory_releasing_function  = free;
    MyBUILD.pEx = &MyEX;
    MyBUILD.iErrorCounter = 0;
    MyBUILD.fErrorFlags = MyEX.fErrorFlags;
    MyBUILD.FirstUNIXline = MyREAD.FirstUNIXline;

    build_Build(&MyBUILD);

/*
#ifdef _DEBUG
    build_pprint(&MyBUILD,stdout);
    getchar();
#endif
*/
    if( MyBUILD.iErrorCounter ){
#ifdef _DEBUG
      getchar();
#endif
      exit(1);
      }

    /* This is the very first place where we can relase the reader memory
       because syntax error messages point to the file name strings that are reserved
       by the reader. */
    alloc_FinishSegment(MyLEX.pMemorySegment);
    ex_free(&MyEX);

    if( szOutputFile ){
      if( isCoutput )
        build_SaveCCode(&MyBUILD,szOutputFile);
      else
        build_SaveCode(&MyBUILD,szOutputFile);
      if( !execute )exit(0);
      }
    if( szCache ){
      build_SaveCode(&MyBUILD,CachedFileName);
      }
    /* Reader memory segment is needed up to here, because FirstUNIXline still
       belons to this segment and is used by save code. */
    alloc_FinishSegment(MyREAD.pMemorySegment);
    }/* non binary code */

  MyEXE.memory_allocating_function = malloc;
  MyEXE.memory_releasing_function = free;
  MyEXE.reportptr = (void *)stderr;
  MyEXE.report   = report_report;
  if( binarycode )
	MyEXE.fErrorFlags = iscgi ? REPORT_F_CGI : 0;
  else
    MyEXE.fErrorFlags = MyEX.fErrorFlags;

  MyEXE.pConfig = &MyCONF;
/*  MyEXE.Argv0 = Argv0; */
  build_MagicCode(&(MyEXE.Ver));
  if( execute_InitStructure(&MyEXE,&MyBUILD) ){
    MyEXE.report(MyEXE.reportptr,
                 "",
                 0,
                 COMMAND_ERROR_MEMORY_LOW,
                 REPORT_ERROR,
                 &iError,
                 "",
                 &(MyEXE.fErrorFlags));
#ifdef _DEBUG
    getchar();
#endif
    exit(1);
    }
/*
  We could alter the standard input, standard output, the environment
  function and command arguments here . We do only the command
  arguments here in this variation.
*/
  MyEXE.CmdLineArgument = CmdLinBuffer;  

  execute_Execute(&MyEXE,&iError);
  alloc_FinishSegment(MyEXE.pMo->pMemorySegment);
  alloc_FinishSegment(MyEXE.pMemorySegment);
  alloc_FinishSegment(MyBUILD.pMemorySegment);
  alloc_FinishSegment(MyCONF.pMemorySegment);
  if( pszPreprocessedFileName )free(pszPreprocessedFileName);

#ifdef _DEBUG
  testa_AssertLeak();
  printf("Press any key to continue...\n");
  getchar();
#endif
  if( iError )exit(1); else exit(0);
  }

