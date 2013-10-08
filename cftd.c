#include <stdio.h>
#include <stdlib.h>
#include "conftree.h"
#include "confpile.h"


main(int argc,char *argv[]){
  int iError;
  tConfigTree MyCONF;

  if( argc < 3 ){
    fprintf(stderr,"Usage: cfgc binaryinput textoutput\n"
                   "\ndump config information.\n");
    exit(1);
    }

  cft_init(&MyCONF,NULL,NULL,NULL);
  iError = cft_ReadConfig(&MyCONF,argv[1]);
  if( iError != 0 ){
    fprintf(stderr,"Input file %s can not be processed.\n",argv[1]);
    fprintf(stderr,"The error code is %d\n",iError);
    exit(iError);
    }


  iError = cft_DumpConfig(&MyCONF,argv[2]);
  if( iError != 0 ){
    fprintf(stderr,"Output file %s can not be processed.\n",argv[1]);
    fprintf(stderr,"The error code is %d\n",iError);
    exit(iError);
    }

  exit(0);
  }
