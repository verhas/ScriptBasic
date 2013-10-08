/*if.c

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

#include "../command.h"

/*POD
=H 'if' conditional statement
CUT*/

/* Convert a variable to 0 or 1 for SLIF, IF, ELSIF commands */
static int IsTrue(VARIABLE v){

  if( memory_IsUndef(v) )return 0;/* undef is false*/
  switch( TYPE(v) ){
      case VTYPE_LONG:
        if( LONGVALUE(v) )return 1; else return 0;
      case VTYPE_DOUBLE:
        if(  DOUBLEVALUE(v) )return 1; else return 0;
      case VTYPE_STRING:
        if( * STRINGVALUE(v) == (char)0 )return 0; else return 1;
      case VTYPE_ARRAY:
        return 0;
      }
  return 0;
  }


/*POD
=section SLIF
=H Signle line 'IF' conditional statement

This command is a single line version of the IF statement
This has the form:

=verbatim
 IF expression THEN command
=noverbatim

CUT*/


/**IF
=section misc
=title IF condition THEN

Conditional execution. There are two different ways to use this command: single
line T<IF> and multi line T<IF>.

A single line T<IF> has the form

=verbatim
IF condition THEN command
=noverbatim

There is no way to specify any T<ELSE> part for the command in the single line version. If you need T<ELSE> command you have use multi line T<IF>.

The multi line T<IF> should not contain any command directly after the keyword T<THEN>. It should have the format:

=verbatim
IF condition THEN
  commands
ELSE
  commands
END IF
=noverbatim

The T<ELSE> part of the command is optional, thus the command can have the format

=verbatim
IF condition THEN
  commands
END IF
=noverbatim

as well. To be very precise the full syntax of the multi-line T<IF> command is:

=verbatim
IF condition THEN
 commands
[ ELSE IF | ELSEIF | ELSIF | ELIF 
  commands
  ... ]
[ ELSE
  commands ]
END IF | ENDIF
=noverbatim

You can use as many T<ELSE IF> branches as you like and at most one T<ELSE> branch. 

The keywords T<ELSE IF>, T<ELSEIF> and others are allowed for ease program porting from other BASIC dialect. There is no difference between the interpretation. The same is true for T<END IF> in two words and written into a single keyword T<ENDIF>.

*/
COMMAND(SLIF)/* Single line IF */
#if NOTIMP_WHEN
NOTIMPLEMENTED;
#else
  NODE nGoForward;
  VARIABLE ItemResult;

  ItemResult = EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;

  /* to the next statement */
  nGoForward = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest;
  /* to the next statement after the next statement */
  if( nGoForward ){
    nGoForward = pEo->CommandArray[nGoForward-1].Parameter.NodeList.rest;
    }

  if( ! IsTrue(ItemResult) ){
    SETPROGRAMCOUNTER(nGoForward);
    RETURN;
    }
#endif
END


/*POD
=section IF
=H 'if' conditional statement

The IF conditional statement is implemented in a very special way. This is because IF can
be very compound on several lines. If an IF expression is false then the command checks
if there is any daingling ELSEIF and in case there is any it goes on and on until there
is any ELSEIF with a true expression or there is no more ELSEIF but ELSE or ENDIF.

CUT*/
COMMAND(IF)
#if NOTIMP_IF
NOTIMPLEMENTED;
#else
  NODE nItem;
  NODE nGoForward;
  NODE nCode;
  VARIABLE ItemResult;

  while(1){
    nItem = PARAMETERNODE;
    ItemResult = EVALUATEEXPRESSION(nItem);
    ASSERTOKE;
    NEXTPARAMETER;
    /* if the condition is FALSE then we step forward to the ENDIF/ELSIF or ELSE statement */
    nGoForward = PARAMETERNODE;
    nCode = CAR(nGoForward); /* the code node of the instruction */

    switch( OPCODE(nCode) ){
      case CMD_ELSE:
      case CMD_ENDIF:
        /* go AFTER the ELSE or ENDIF statement */
        nGoForward = CDR(nGoForward);
        break;
      case CMD_ELSIF:
        break;
      default:; /*TODO: report an internal error*/
      }

    /* If the expression is true then we have to continue with the next statement. This is
       usually the default, but here we have to explicitely set the program counter because
       in case we wen onto an ELSEIF the execution would start on the command after the
       IF statement. 

       If the expression is false then we either continue the execution after the ELSE or ENDIF
       statement or set the program counter to the ELSEIF statement and loop back to execute it
       if it were an IF statement.
    */
    if( IsTrue(ItemResult) ){
      SETPROGRAMCOUNTER(pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest);
      RETURN;
      }else{
      if( OPCODE(nCode) == CMD_ELSIF ){
        pEo->ProgramCounter = nGoForward;
        _ActualNode=PROGRAMCOUNTER;
        }else{
        SETPROGRAMCOUNTER(nGoForward);
        RETURN;
        }
      }
    }
#endif
END

COMMAND(ELSE)
#if NOTIMP_ELSE
NOTIMPLEMENTED;
#else


  SETPROGRAMCOUNTER(PARAMETERNODE);

#endif
END

COMMAND(ENDIF)
#if NOTIMP_ENDIF
NOTIMPLEMENTED;
#else

  /* this command does not do anything */
#endif
END

/*POD
=section ELSIF
=H 'else if' conditional statement

Note that this piece of code does nothing. The execution gets here only when a previous
IF or ELSIF was true. In that case the expression has to be ignored and the ELSIF treated
like if it was an ELSE.

The expression after an ELSEIF is checked by the preceeding IF command.

CUT*/
COMMAND(ELSIF)
#if NOTIMP_ELSIF
NOTIMPLEMENTED;
#else

  NEXTPARAMETER;/* skip the expression */
  SETPROGRAMCOUNTER(PARAMETERNODE);

#endif
END
