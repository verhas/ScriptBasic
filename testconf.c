
#include <stdio.h>
#include "conftree.h"

main(){

  tConfigTree MyCONF;
  CFT_NODE Node;
  char *pszValue;

 printf("%d\n",
  cft_start(&MyCONF,NULL,NULL,NULL,
#ifdef WIN32
            "Software\\ScriptBasic\\config",
            "WINNT\\SCRIBA.INI",
#else
            "SCRIBACONF",
            "/etc/scriba/basic.conf",
#endif
            NULL));

  for( Node = 0;  ! cft_GetEx(&MyCONF,"include",&Node,&pszValue,NULL,NULL,NULL) ; Node = cft_EnumNext(&MyCONF,Node) ){
    printf("include %s\n",pszValue);
    }

  }
