/* FILE: ux.c

This file is a ScriptBasic interface to UNIX system calls
that are not available under Windows NT and thus are not
implemented in the core of ScriptBasic.

UXLIBS:
DWLIBS:

*/
#include <stdio.h>
#include "../../basext.h"

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START

besEND

besSUB_FINISH

besEND

/**
=H Unix specific functions
This module implements some UNIX system calls that are not implemented 
in the core of ScriptBasic but can be helpful for those who want to write 
system maintenance scripts using ScriptBasic.

The reason that these functions are not implemented inside ScriptBasic 
is that ScriptBasic itself is portable, and whenever a programmer writes a 
program in pure ScriptBasic it should execute the same way under UNIX as well 
as under Windows NT/Win98/W2K.

Programs using the module ux however are UNIX specific and will 
not run unaltered under Windows NT.

*/
/**
=section fork
=H Fork the process

=verbatim
pid = ux::fork()
if pid then
  print "The sub process pid is ",pid
else
  print "Oh I am the child process"
end if
=noverbatim

*/
besFUNCTION(uxfork)
  int i;

  besALLOC_RETURN_LONG;
  i = fork();
  LONGVALUE(besRETURNVALUE) = i;

besEND

/**
=section chmod
=H Change file access mode
=verbatim
ux::chmod("file_name",mode)
=noverbatim

This function implements the T<chmod> UNIX system call.
*/
besFUNCTION(uxchmod)
  VARIABLE Argument;
  char *pszFileName;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszFileName);

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2LONG(Argument);
  chmod(pszFileName,(mode_t)LONGVALUE(Argument));
besEND
