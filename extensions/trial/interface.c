/*
NTLIBS:
UXLIBS:
DWLIBS:
MCLIBS:
 */

#include <stdio.h>

#include "../../basext.h"

besVERSION_NEGOTIATE

  printf("The function bootmodu was started and the requested version is %d\n",Version);
  printf("The variation is: %s\n",pszVariation);
  printf("We are returning accepted version %d\n",(int)INTERFACE_VERSION);
  return (int)INTERFACE_VERSION;

besEND

besSUB_START
  long *pL;

  besMODULEPOINTER = besALLOC(sizeof(long));
  if( besMODULEPOINTER == NULL )return 0;
  pL = (long *)besMODULEPOINTER;
  *pL = 0L;

  printf("The function bootmodu was started.\n");

besEND

besSUB_FINISH
  printf("The function finimodu was started.\n");
besEND

besFUNCTION(pprint)
  int i;
  int slen;
  char *s;
  VARIABLE Argument;

  printf("The number of arguments is: %ld\n",besARGNR);

  for( i=1 ; i <= besARGNR ; i++ ){
    Argument = besARGUMENT(i);
    besDEREFERENCE(Argument);
redo:
    switch( slen=TYPE(Argument) ){
      case VTYPE_LONG:
        printf("This is a long: %ld\n",LONGVALUE(Argument));
        break;
      case VTYPE_DOUBLE:
        printf("This is a double: %lf\n",DOUBLEVALUE(Argument));
        break;
      case VTYPE_STRING:
        printf("This is a string ");
        s = STRINGVALUE(Argument);
        slen = STRLEN(Argument);
        while( slen -- )
            putc(((int)*s++),stdout);
        printf("\n");
        break;
      case VTYPE_ARRAY:
        printf("ARRAY@#%08X\n",LONGVALUE(Argument));
        printf("ARRAY LOW INDEX: %ld\n",ARRAYLOW(Argument));
        printf("ARRAY HIGH INDEX: %ld\n",ARRAYHIGH(Argument));
        printf("The first element of the array is:\n");
        Argument = ARRAYVALUE(Argument,ARRAYLOW(Argument));
        goto redo;
        break;
      }
    }
besEND

besFUNCTION(set1)
  VARIABLE Argument;
  LEFTVALUE Lval;
  int i;
  unsigned long __refcount_;

  for( i=1 ; i <= besARGNR ; i++ ){
    Argument = besARGUMENT(i);

    besLEFTVALUE(Argument,Lval);
    if( Lval ){
      besRELEASE(*Lval);
      *Lval = besNEWLONG;
      if( *Lval )
        LONGVALUE(*Lval) = 1;
      }
    }

besEND

besFUNCTION(arbdata)
  VARIABLE Argument;
  LEFTVALUE Lval;
  static char buffer[1024];
  char *p;
  unsigned long __refcount_;

  p = buffer;
  sprintf(buffer,"%s","hohohoho\n");
  Argument = besARGUMENT(1);

  besLEFTVALUE(Argument,Lval);
  if( Lval ){
    besRELEASE(*Lval);
    *Lval = besNEWSTRING(sizeof(char*));
    memcpy(STRINGVALUE(*Lval),&p,sizeof(p));
    }

besEND

besFUNCTION(pzchar)
  int i;
  VARIABLE Argument;
  char *p;

  for( i=1 ; i <= besARGNR ; i++ ){
    Argument = besARGUMENT(i);
    besDEREFERENCE(Argument);
    memcpy(&p,STRINGVALUE(Argument),sizeof(p));
    printf("%s\n",p);
    }
besEND


besFUNCTION(trial)
  long *pL;

  printf("Function trial was started...\n");
  pL = (long *)besMODULEPOINTER;
  (*pL)++;
  besRETURNVALUE = besNEWMORTALLONG;
  LONGVALUE(besRETURNVALUE) = *pL;

  printf("Module directory is %s\n",besCONFIG("module"));
  printf("dll extension is %s\n",besCONFIG("dll"));
  printf("include directory is %s\n",besCONFIG("include"));

besEND

besFUNCTION(myicall)
  VARIABLE Argument;
  VARIABLE pArgument;
  VARIABLE FunctionResult;
  unsigned long ulEntryPoint;
  unsigned long i;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  ulEntryPoint = LONGVALUE(Argument);

  pArgument = besNEWARRAY(0,besARGNR-2);
  for( i=2 ; i <= (unsigned)besARGNR ; i++ ){
     pArgument->Value.aValue[i-2] = besARGUMENT(i);
     }

  besHOOK_CALLSCRIBAFUNCTION(ulEntryPoint,
                             pArgument->Value.aValue,
                             besARGNR-1,
                             &FunctionResult);

  for( i=2 ; i <= (unsigned)besARGNR ; i++ ){
     pArgument->Value.aValue[i-2] = NULL;
     }
  besRELEASE(pArgument);
  besRELEASE(FunctionResult);
besEND

besSUB_AUTO
  printf("autoloading %s\n",pszFunction);
  *ppFunction = (void *)trial;
besEND

besCOMMAND(iff)
  NODE nItem;
  VARIABLE Op1;
  long ConditionValue;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = besPARAMETERLIST;
  if( ! nItem ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = besEVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;

  if( Op1 == NULL )ConditionValue = 0;
  else{
    Op1 = besCONVERT2LONG(Op1);
    ConditionValue = LONGVALUE(Op1);
    }

  if( ! ConditionValue )
    nItem = CDR(nItem);

  if( ! nItem ){
    RESULT = NULL;
    RETURN;
    }
  nItem = CDR(nItem);

  RESULT = besEVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  
  RETURN;
besEND_COMMAND
