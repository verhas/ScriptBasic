/* environ.c

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

#include "../command.h"

#ifdef __DARWIN__
#define _environ environ
#endif

extern char **_environ;

/**ENVIRON
=section misc
=title ENVIRON("envsymbol") or ENVIRON(n)
=display ENVIRON()

This function returns the value of an environment variable. Environment variables are string values associated to names that are provided by the executing environment for the programs. The executing environment is usually the operating system, but it can also be the Web server in CGI programs that alters the environment variables provided by the surrounding operating system specifying extra values.

This function can be used to get the string of an environment variable in case the program knows the name of the variable or to list all the environment variables one by one.

If the environment variable name is known then the name as a string has to be passed to this function as argument. In this case the return value is the value of the environment variable.

If the program wants to list all the environment variables the argument to the function T<ENVIRON> can be an integer number T<n>. In this case the function returns a string containing the name and the value joined by a T<=> sign of the T<n>-th environment variable. The numbering starts with T<n=0>.

If the argument value is integer and is out of the range of the possible environment variable ordinal numbers (negative or larger or equal to the number of the available environment variables) then the function returns T<undef>.

If the argument to the function is T<undef> then the function also returns the T<undef> value.

Note that ScriptBasic provides an easy way for the embedding applications to redefine the underlying function that returns the environment variable. Thus an embedding application can "fool" a BASIC program providing its own environment variable. For example the Eszter SB Application Engine provides an alternative environment variable reading function and thus BASIC applications can read the environment using the function T<ENVIRON> as if the program was running in a real CGI environment.

=details

The following sample code prints all environment variables:

=verbatim
i=0
do
  e$ = environ(i)
  if IsDefined(e$) then
    print e$
    print
  endif
  i = i + 1
loop while IsDefined(e$)
=noverbatim


*/
COMMAND(ENVIRON)
#if NOTIMP_ENVIRON
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1;
#define MAX_ENVIRV_LEN 256
  char *s,buffer[MAX_ENVIRV_LEN];
  size_t sLen;
  char **EnvironmentPointer;
  long index;
  char *(*EnvirFunc)(void *, char *, long );
  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }

  EnvirFunc = pEo->fpEnvirFunction;
  if( TYPE(Op1) == VTYPE_LONG ){
    index = LONGVALUE(Op1);
    if( EnvirFunc ){
      s = EnvirFunc(pEo->pEmbedder,NULL,index);
      }else{
      EnvironmentPointer = _environ;
      if( index < 0 ){
        RESULT = NULL;
        RETURN;
        }
      /* we have to walk one by one, because index may be too large and _environ is terminated with NULL and
         we do not have the length */
      while( index && *EnvironmentPointer ){
        index--;
        EnvironmentPointer++;
        }
      s = *EnvironmentPointer;
      }
    }else{

    Op1 = CONVERT2STRING(Op1);
    if( (sLen=STRLEN(Op1)) > MAX_ENVIRV_LEN -1 )sLen = MAX_ENVIRV_LEN-1;
    memcpy(buffer,STRINGVALUE(Op1),sLen);
    buffer[sLen] = (char)0;
    if( EnvirFunc )
      s = EnvirFunc(pEo->pEmbedder,buffer,0);
    else
      s = getenv( buffer );
    }

  if( s == NULL ){
    RESULT = NULL;
    RETURN;
    }
  sLen=strlen(s);
  RESULT = NEWMORTALSTRING(sLen);
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),s,sLen);

#endif
END

/**COMMANDF
=section misc
=title COMMAND()
=display COMMAND()

This function returns the command line arguments of the program in a single string. This does not include the name of the interpreter and the name of the BASIC program, only the arguments that are to be passed to the BASIC program. For example the program started as

=verbatim
# scriba test_command.sb arg1 arg2  arg3
=noverbatim

will see T<"arg1 arg2 arg3"> string as the return value of the function T<COMMAND()>.
*/
COMMAND(COMMANDF)
#if NOTIMP_COMMANDF
NOTIMPLEMENTED;
#else

  int q;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  if( pEo->CmdLineArgument == NULL ){
    RESULT = NULL;
    RETURN;
    }

  RESULT = NEWMORTALSTRING(q=strlen(pEo->CmdLineArgument));
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),pEo->CmdLineArgument,q);

#endif
END
