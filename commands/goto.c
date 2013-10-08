/*goto.c

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

/**GOTO
=section misc
=title GOTO label
Go to a label and continue program execution at that label. Labels are local within functions and subroutines. You can not jump into a subroutine or jump out of it.


Use of GOTO is usually discouraged and is against structural programming. Whenever you feel the need to use the T<GOTO> statement (except T<ON ERROR GOTO>) thin it twice whether there is a better solution without utilizing the statement T<GOTO>.

Typical use of the T<GOTO> statement to jump out of some error condition to the error handling code or jumping out of some loop on some condition.
*/
COMMAND(GOTO)
#if NOTIMP_GOTO
NOTIMPLEMENTED;
#else


  SETPROGRAMCOUNTER(PARAMETERNODE);

#endif
END

/**RESUME
=section error
=title RESUME [ label | next ]

Resume the program execution after handling the error. T<RESUME> without argument tries to execute the same line again that caused the error. T<RESUME NEXT> tries to continue execution after the line that caused the error. T<RESUME label> tries to continue execution at the specified label.

See also R<ONERRORGOTO>, R<ONERRORRESUME> and R<ERROR>.
*/
COMMAND(RESUMELABEL)
#if NOTIMP_RESUMELABEL
NOTIMPLEMENTED;
#else


  SETPROGRAMCOUNTER(PARAMETERNODE);
  pEo->LastError = 0;
  pEo->ErrorGoto = 0;
  pEo->fErrorGoto = 0;

#endif
END
/**ONERRORRESUME
=section error
=title ON ERROR RESUME [ label | next ]
=display ON ERROR RESUME

Setting T<ON ERROR RESUME> will try to continue execution on the label or on the next statement when an error occurs without any error handling code.

See also R<ONERRORGOTO>, R<RESUME> and R<ERROR>.

*/
COMMAND(ONERRORRESUMELABEL)
#if NOTIMP_ONERRORRESUMELABEL
NOTIMPLEMENTED;
#else


  pEo->ErrorGoto = PARAMETERNODE;
  pEo->fErrorGoto = ONERROR_RESUME;

#endif
END

/**ONERRORGOTO
=section error
=title ON ERROR GOTO [ label | NULL ]
=display ON ERROR GOTO

Set the entry point of the error handling routine. If the argument is T<NULL> then the error handling is switched off.
*/
COMMAND(ONERRORGOTONULL)
#if NOTIMP_ONERRORGOTONULL
NOTIMPLEMENTED;
#else


  pEo->ErrorGoto = 0;
  pEo->fErrorGoto = 0;

#endif
END

COMMAND(ONERRORGOTO)
#if NOTIMP_ONERRORGOTO
NOTIMPLEMENTED;
#else


  pEo->ErrorGoto = PARAMETERNODE;
  pEo->fErrorGoto = ONERROR_GOTO;

#endif
END

COMMAND(RESUME)
#if NOTIMP_RESUME
NOTIMPLEMENTED;
#else


  pEo->LastError = 0;
  if( !pEo->ErrorResume )ERROR(COMMAND_ERROR_NO_RESUME);
  SETPROGRAMCOUNTER(pEo->ErrorResume);

#endif
END

COMMAND(ONERRORRESUMENEXT)
#if NOTIMP_ONERRORRESUMENEXT
NOTIMPLEMENTED;
#else


  pEo->fErrorGoto = ONERROR_RESUMENEXT;

#endif
END

COMMAND(RESUMENEXT)
#if NOTIMP_RESUMENEXT
NOTIMPLEMENTED;
#else


  pEo->LastError = 0;
  if( ! pEo->ErrorResume )ERROR(COMMAND_ERROR_NO_RESUME);
  SETPROGRAMCOUNTER( CDR(pEo->ErrorResume) );

#endif
END

/**STOP
=section misc error

This command stops program execution. There is no possibility to restart program execution after this command was executed.

See also R<END>.
*/
COMMAND(STOP)
#if NOTIMP_STOP
NOTIMPLEMENTED;
#else

  pEo->fStop = fStopSTOP;

#endif
END

/**END
=section misc

End of the program. Stops program execution.

You should usually use this command to signal the end of a program. Although you can use R<STOP> and T<END> interchangeably this is the convention in BASIC programs to use the command T<END> to note the end of the program and T<STOP> to stop the program execution based on some extra condition inside the code.
*/
COMMAND(END)
#if NOTIMP_END
NOTIMPLEMENTED;
#else

  pEo->fStop = fStopSTOP;

#endif
END

/**ERROR
=section error
=title ERROR() or ERROR n

The keyword T<ERROR> can be used as a command as well as a built-in function. When used as a function it returns the error code that last happened. The error codes are defined in the header file T<error.bas> that can be included with the command T<import>. The error code is a vital value when an error happens and is captured by some code defined after the label referenced by the command T<ON ERROR GOTO>. This helps the programmer to ensure that the error was really the one that the error handling code can handle and not something else.

If the keyword is used as a command then it has to be followed by some numeric value. The command does not ever return but generates an error of the code given by the argument.

Take care when composing the expression following the keyword T<ERROR>. It may happen that the expression itself can not be evaluated due to some error conditions during the evaluation of the expression. The best practice is to use a constant expression using the symbolic constants defined in the include file T<error.bas>.

Note that the codes defined in the include file are version dependant.

If an error is not captured by any T<ON ERROR GOTO> specified error handler then the interpreter stops. The command line variation passes the error code to the executing environment as exit code. In other word you can exit from a BASIC program
*/
COMMAND(ERROR)
#if NOTIMP_ERROR
NOTIMPLEMENTED;
#else


  RESULT = NEWMORTALLONG;
/*  ASSERTNULL(RESULT) we already raise an error */
  if( RESULT ){
    LONGVALUE(RESULT) = pEo->LastError;
    }

#endif
END

/**ERRORD
=section errord
=title ERROR$() or ERROR$(n)

Returns the English sentence describing the last error if the argument is not defined or returns the English sentence describing the error for the error code T<n>.

If the error code T<n> provided as argument is negative or is above all possible errors then the result of the function is T<undef>.
*/
COMMAND(ERRORDOLLAR)
#if NOTIMP_ERRORDOLLAR
NOTIMPLEMENTED;
#else
  VARIABLE vErrCode;
  long lErrCode;

  USE_CALLER_MORTALS;

  vErrCode = NULL;
  if( CAR(PARAMETERLIST) ){
    vErrCode = _EVALUATEEXPRESSION(CAR(PARAMETERLIST));
    ASSERTOKE;
    }
  if( vErrCode == NULL ){
    lErrCode = pEo->LastError;
    }else lErrCode = GETLONGVALUE(vErrCode);

  if( lErrCode < 0 || lErrCode >= MAX_ERROR_CODE ){
    RESULT = NULL;
    RETURN;
    }

  RESULT = NEWMORTALSTRING(strlen(en_error_messages[lErrCode]));
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),en_error_messages[lErrCode],STRLEN(RESULT));

#endif
END

COMMAND(CERROR)
#if NOTIMP_CERROR
NOTIMPLEMENTED;
#else


  ERROR(pEo->LastError=LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE))));
  /* this is a bit weird, but it can happen that the user wants to create an error and
     an error happens while the expression following the keyword ERROR is evaluated */
  ASSERTOKE;

#endif
END
