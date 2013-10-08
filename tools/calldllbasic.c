#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../getopt.h"
#include "../variations/win32dll/basicdll.h"

#ifdef _DEBUG

int mymain();
main(){
  char *myargv[] = {
     "basic",
     "D:\\MyProjects\\sb\\source\\examples\\hello.bas",
     NULL
     };
  int myargc= 2;

  mymain(myargc, myargv);
  }
#define main mymain

#endif

int GetC(void *f){ return getc((FILE *)f); }
static MyOutput(char q){
  printf("*%c",q);
  }
static char *s = "alma van a fa alatt\n";
static int MyInput(void){

  if( *s == (char)0 )return EOF;
  return *s++;
  }

main(int argc, char *argv[]){
  char *szInputFile;
  char *optarg,opt;
  int giveusage,binarycode;
  int execute,OptionIndex;
  char CmdLinBuffer[256];

  /* default values for command line options */
  szInputFile = NULL;
  giveusage  = 0; /* assume the command line is correct, we need not display usage and stop */
  binarycode = 0; /* input is not binary by default */
  execute    = 0; /* do not execute by default after binary format save */

  OptionIndex = 0;

  while( (opt = getoptt(argc, argv, "veb",&optarg,&OptionIndex)) != ':'){
    switch( opt ){
      case 'e' :
        if( execute )giveusage = 1;
        execute = 1;
        break;
      case 'b':
        binarycode =1;
        break;
      case 'v':
#define S fprintf(stderr,
#define E );
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


  if( szInputFile == NULL || giveusage ){
#define U(x) fprintf(stderr,"%s\n",(x));
    U("Usage: basic [options] program.bas")
    U("")
    U("options: -o file_name")
    U("            save binary format to file but don't execute")
    U("         -b file_name")
    U("            load binary format from file and execute")
    U("         -e")
    U("            execute after binary format was saved")
    U("         -v")
    U("            print version info and stop")
    
    exit(1);
    }

  execute = basic(szInputFile,(void *)MyInput,(void *)MyOutput,NULL,CmdLinBuffer,NULL,binarycode);
  exit(execute);
  }
