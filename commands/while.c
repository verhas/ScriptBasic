/* while.c
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
=H Implement the different looping constructs

This file implements the various looping constructs. These include the followings:

=verbatim
while expression
wend

repeat
until expression

do while expression
loop

do until expression
loop

do
loop while expression

do
loop until expression

for i=expression to expression [step expression]
next
=noverbatim

The different formats result the same, they are presented here to allow
the user to use the one that fits his or her taste.

The implementation of the 'for' loop is extremely sophisticated and tricky. Try to
understand when you are really familiar with the code structure.

CUT*/

/**WHILE
=section loop
=title WHILE condition

Implements the 'while' loop as it is usually done in most basic implementations. The loop starts with the command T<while> and finished with the line containing the keyword T<wend>. The keyword T<while> is followed by an expression and the loop is executes so long as long the expression is evaluated T<true>.

=verbatim
while expression
  ...
  commands to repeat
  ...
wend
=noverbatim

The expression is evaluated when the looping starts and each time the loop is restarted. It means that the code between the T<while> and T<wend> lines may be skipped totally if the expression evaluates to some T<false> value during the first evaluation before the execution starts the loop.

In case some condition makes it necessary to exit the loop from its middle then the command R<GOTO> can be used.

ScriptBasic implements several looping constructs to be compatible with different BASIC language dialects. Some constructs are even totally interchangeable to let programmers with different BASIC experience use the one that fit they the best. See also R<WHILE>, R<DOUNTIL>, R<DOWHILE>, R<REPEAT>, R<DO> and R<FOR>.
*/
COMMAND(WHILE)
#if NOTIMP_WHILE
NOTIMPLEMENTED;
#else

  NODE nItem;
  NODE nGoForward;
  VARIABLE ItemResult;

  nItem = PARAMETERNODE;
  ItemResult = EVALUATEEXPRESSION(nItem);
  ASSERTOKE;
  NEXTPARAMETER;
  /* we step forward to the node AFTER the while statement */
  nGoForward = CDR(PARAMETERNODE);
  if( memory_IsUndef(ItemResult) ){
    SETPROGRAMCOUNTER(nGoForward);
    RETURN;
    }

  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
      if( ! LONGVALUE(ItemResult) ){
        SETPROGRAMCOUNTER(nGoForward);
        RETURN;
        }
      break;
    case VTYPE_DOUBLE:
      if( ! DOUBLEVALUE(ItemResult) ){
        SETPROGRAMCOUNTER(nGoForward);
        RETURN;
        }
        break;
    case VTYPE_STRING:
      if( * STRINGVALUE(ItemResult) == (char)0 ){
        SETPROGRAMCOUNTER(nGoForward);
        RETURN;
        }
      break;
    case VTYPE_ARRAY:
      break;
    }

#endif
END

/**DOUNTIL
=section loop
=title DO UNTIL condition

This command implements a looping construct that loops the code between the line T<DO UNTIL> and T<LOOP> util the expression following the keywords on the loop starting line becomes T<true>.

=verbatim
DO UNTIL expression
 ...
 commands to repeat
 ...
LOOP
=noverbatim

The expression is evaluated when the looping starts and each time the loop is restarted. It means that the code between the T<DO UNTIL> and T<LOOP> lines may be skipped totally if the expression evaluates to some T<TRUE> value during the first evaluation before the execution starts the loop.

This command is practically equivalent to the construct

=verbatim
WHILE NOT expression
...
 commands to repeat
 ...
WEND
=noverbatim

You can and you also should use the construct that creates more readable code.

See also R<WHILE>, R<DOUNTIL>, R<DOWHILE>, R<REPEAT>, R<DO> and R<FOR>.
*/
COMMAND(DOUNTIL)
#if NOTIMP_DOUNTIL
NOTIMPLEMENTED;
#else


  NODE nItem;
  NODE nGoForward;
  VARIABLE ItemResult;

  nItem = PARAMETERNODE;
  ItemResult = EVALUATEEXPRESSION(nItem);
  ASSERTOKE;
  NEXTPARAMETER;

  /* we step forward to the node AFTER the loop statement */
  nGoForward = CDR(PARAMETERNODE);
  if( memory_IsUndef(ItemResult) ){
    RETURN;
    }

   switch( TYPE(ItemResult) ){
      case VTYPE_LONG:
        if( LONGVALUE(ItemResult) ){
          SETPROGRAMCOUNTER(nGoForward);
          RETURN;
          }
        break;
      case VTYPE_DOUBLE:
        if( DOUBLEVALUE(ItemResult) ){
          SETPROGRAMCOUNTER(nGoForward);
          RETURN;
          }
        break;
      case VTYPE_STRING:
        if( * STRINGVALUE(ItemResult) != (char)0 ){
          SETPROGRAMCOUNTER(nGoForward);
          RETURN;
          }
        break;
      case VTYPE_ARRAY:
        break;
      }

#endif
END

COMMAND(WEND)
#if NOTIMP_WEND
NOTIMPLEMENTED;
#else


  SETPROGRAMCOUNTER(PARAMETERNODE);

#endif
END

/**DOWHILE
=section loop
=title DO WHILE condition

This command implements a looping construct that loops the code between the line T<DO WHILE> and T<LOOP> util the expression following the keywords on the loop starting line becomes T<false>.

Practically this command is same as the command R<WHILE> with a different syntax to be compatible with different BASIC implementations.

=verbatim
do while
 ...
loop
=noverbatim

You can use the construct that creates more readable code.

See also R<WHILE>, R<DOUNTIL>, R<DOWHILE>, R<REPEAT>, R<DO> and R<FOR>.
*/
COMMAND(DOWHILE)
#if NOTIMP_DOWHILE
NOTIMPLEMENTED;
#else
 IDENTICAL_COMMAND(WHILE)
#endif
END

COMMAND(LOOP)
#if NOTIMP_LOOP
NOTIMPLEMENTED;
#else
 IDENTICAL_COMMAND(WEND)
#endif
END

/**REPEAT
=section loop

This command implements a loop which is repeated so long as long the expression standing after the loop closing line T<UNTIL> becomes T<true>. The loop starts with a line containing the keyword T<REPEAT> and finishes with the line T<UNTIL expression>.

=verbatim
repeat
  ...
  commands to repeat
  ...
until expression
=noverbatim

The expression is evaluated each time after the loop is executed. This means that the commands inside the loop are executed at least once.

This kind of loop syntax is not usual in BASIC dialects but can be found in languages like PASCAL. Implementing this loop in ScriptBasic helps those programmers, who have PASCAL experience.

See also R<WHILE>, R<DOUNTIL>, R<DOWHILE>, R<REPEAT>, R<DO> and R<FOR>.
*/
COMMAND(REPEAT)
#if NOTIMP_REPEAT
NOTIMPLEMENTED;
#else

 /* we do not do anything just start a loop */

#endif
END

/**DO
=section loop

This command is a looping construct that repeats commands so long as long the condition following the keyword T<UNTIL> becomes T<true> or the condition following the keyword T<WHILE> becomes T<false>. 

The format of the command is
=verbatim
DO
 ...
 commands to repeat
 ...
LOOP WHILE expression
=noverbatim

or

=verbatim
DO
 ...
 commands to repeat
 ...
LOOP UNTIL expression
=noverbatim

The condition expression is evaluated every time after the loop commands were executed. This means that the loop body is executed at least once.

A T<DO>/T<LOOP> construct should be closed with a T<LOOP UNTIL> or with a T<LOOP WHILE> command but not with both.

The T<DO>/T<LOOP UNTIL> is practically equivalent to the T<REPEAT>/T<UNTIL> construct.

See also R<WHILE>, R<DOUNTIL>, R<DOWHILE>, R<REPEAT>, R<DO> and R<FOR>.
*/
COMMAND(DO)
#if NOTIMP_DO
NOTIMPLEMENTED;
#else

 /* we do not do anything just start a loop */
#endif
END

COMMAND(LOOPWHILE)
#if NOTIMP_LOOPWHILE
NOTIMPLEMENTED;
#else
  IDENTICAL_COMMAND(DOUNTIL)
#endif
END

COMMAND(UNTIL)
#if NOTIMP_UNTIL
NOTIMPLEMENTED;
#else
  IDENTICAL_COMMAND(WHILE)
#endif
END

COMMAND(LOOPUNTIL)
#if NOTIMP_LOOPUNTIL
NOTIMPLEMENTED;
#else
  IDENTICAL_COMMAND(WHILE)
#endif
END

/**FOR
=section loop
=title FOR var=exp_start TO exp_stop [ STEP exp_step ]

Implements a FOR loop. The variable T<var> gets the value of the start expression T<exp_start>, and after each execution of the loop body it is incremented or decrement by the value T<exp_step> until it reaches the stop value T<exp_stop>.

=verbatim
FOR var= exp_start TO exp_stop [ STEP exp_step]
 ...
 commands to repeat
 ...
NEXT var
=noverbatim

The T<STEP> part of the command is optional. If this part is missing then the default value to increment the variable is 1.

If 
=itemize
=item the expression T<exp_start> is larger than the expression T<exp_stop> and T<exp_step> is positive or if
=item the expression T<exp_start> is smaller than the expression T<exp_stop> and T<exp_step> is negative
=noitemize
then the loop body is not executed even once and the variable retains its old value.

When the loop is executed at least once the variable gets the values one after the other and after the loop exists the loop variable holds the last value for which the loop already did not execute. Thus 

=verbatim
for h= 1 to 3
next
print h
stop
=noverbatim

prints T<4>.

The expression T<exp_start> is evaluated only once when the loop starts. The other two expressions T<exp_stop> and T<exp_step> are evaluated before each loop. Thus

=verbatim
j = 1
k = 10
for h= 1 to k step j
print h,"\n"
j += 1
k -= 1
next
print k," ",j,"\n"
stop
=noverbatim

will print

=verbatim
1
3
6
7 4
=noverbatim

To get into more details the following example loop

=verbatim
STEP_v = 5
for z= 1 to 10 step STEP_v
 print z,"\n"
 STEP_v -= 10
next z
=noverbatim

executes only once. This is because the step value changes its sign during the evaluation and the new value being negative commands the loop to terminate as the loop variable altered value is smaller then the end value. In other words the comparison also depends on the actual value of the step expression.

These are not only the expressions that are evaluated before each loop, but the variable as well. If the loop variable is a simple variable then this has not too much effect. However if the loop variable is an array member then this really has to be taken into account. For example:

=verbatim
for j=1 to 9
  A[j] = 0
next

j = 1
for A[j]= 1 to 9

 for k=1 to 9
  print A[k]
 next k
 print

 j += 1
 if j > 9 then STOP

next
=noverbatim

prints

=verbatim
100000000
110000000
111000000
111100000
111110000
111111000
111111100
111111110
111111111
=noverbatim

so you can see that the loop takes, evaluates, compares and increments the actual array element as the variable T<j> in the sample code above is incremented.

The loop variable or some other left value has to stand between the keyword T<FOR> and the sign T<=> on the start line of the loop but this is optional following the keyword T<NEXT>. ScriptBasic optionally allow you to write the variable name after the keyword T<NEXT> but the interpreter does not check if the symbol is a variable of the loop. The role of this symbol is merely documentation of the BASIC code. However, you can not write an array element following the keyword T<NEXT>, only a simple variable name.

If the expression T<exp_step> is zero then the loop variable is not altered and the loop is re-executed with the same loop variable value. This way you can easily get into infinite loop.

These are fine tuning details of the command T<FOR> that you may need to be aware when you read some tricky code. On the other hand you should never create any code that depends on these features. The loop variable is recommended to be a simple variable and the expressions in the loop head should evaluate the same for each execution of the loop. If you need something more special that may depend on some of the features discussed above then you have to consider using some other looping construct to get more readable code.
*/
/*
THIS IS DETAILS FOR C PROGRAMMERS We discuss here the specialties how FOR is implemented in the syntax analyzer!

This command implements a 'for' loop. The 'for' loop is implemented with a slight trick. The syntax definition system of ScriptBasic does not allow optional parts of a command, like the STEP part of the for command. In such a case two different command should be defined, like

=verbatim
FOR: 'for' lval '=' expression 'to' expression nl
FORS: 'for' lval '=' expression 'to' expression 'step' expression nl
=noverbatim

in the T<syntax.def> file. B<This does not work!>

The reason is that an T<expression> is a terminal symbol which can not be parsed without side effects. Therefore the syntax of a line should be designed in a way which can assure that the line contains the very command by the time the parser starts to evaluate an expression.

Assume that we use the syntax definition above and scriba tries to analyze a line

=verbatim
for i=1 to 100 step 2
=noverbatim

The syntax analyzer analyzed the left value before the '=' sign, two expressions when it realizes that
the command does not fit the current syntax definition line, because a predefined symbol (step) is coming
instead of the new line. At this stage the syntax analyzer drops the line and treats it syntactically
incorrect, because the left value analysis and the expression analysis generated a lot of code which, 
in the current of scriba can not be undone.

I<What do we do then to implement a FOR loop?>

A FOR loop head is defined as three consecutive commands:

=itemize
=item a T<FOR> command
=item a T<FORTO> command
=item and a T<STEP> command
=noitemize
(And yes, we have a NEXT command as well, but that is implemented normal.)

The syntax definition lines in T<syntax.def> are:
=verbatim
FOR:     'for' lval '=' expression come_back(FORTO)
FORTO:   'to' expression go_back(FORTO) go_forward(FOR) come_back(FOR)
FORSTEP: 'step' expression nl
=noverbatim

This means that a

=verbatim
for i=1 to 100
print
=noverbatim

code fragment is analyzed as

=verbatim
for i=1
=noverbatim

a for command,

=verbatim
        to 100
=noverbatim

a to command. Then a single new line character comes, which is analyzed as an EMPTY command (see the syntax.def file!). And finally a
print command.

This also generates two or three commands in case you do or do not have a 'step' part. This helps implementing the initialization
code, the increment code into two different command and therefore it is easy to distinguish between starting the loop and
jumping from the end to the start. (Note that the other looping construct do not have initialization code inside them,
and therefore they do not care if the execution is the first one entering the loop or we just jumped back to the loop head from
the end.)

When we jump back from the command T<NEXT> we always jump back to the command T<FORTO>.

The sorry side of this solution is that one can write the following or similar code:

=verbatim
print "hello"
for i=10
print "here I am"
=noverbatim

or

=verbatim
print "hello"
to 63
print "here I am"
next
=noverbatim

without getting syntax error. The error occurs when the execution starts.

Let us hope two things:

=itemize
=item People doing nasty things with scriba using this "feature" won't exist.
=item Unintentional syntax errors uncovered by this "feature" are very rare.
=noitemize

For more details see the code.
*/
COMMAND(FOR)
#if NOTIMP_FOR
NOTIMPLEMENTED;
#else

  VARIABLE vStartExpression,
           vEndExpression,
           vStepExpression;
  LEFTVALUE LetThisVariable;
  NODE nToStatement;
  NODE nStepStatement;
  NODE nLoopStart,nLoopEnd;
  int iGoingUp; /* are we stepping up or down */
  long refcount;
  int bEnterTheLoop;

  /* there should be a to statement following the for statement */
  nToStatement = CDR(pEo->ProgramCounter);

  if( nToStatement == 0 ){
    /* If this is the last statement then we can stop. */
    SETPROGRAMCOUNTER(0);
    RETURN;
    }

  nStepStatement = CDR(nToStatement);
  nToStatement = CAR(nToStatement);

  if( nStepStatement ){
    if( pEo->CommandArray[CAR(nStepStatement)-1].OpCode == CMD_FORSTEP ){
      nLoopStart = CDR(nStepStatement);
      nStepStatement = CAR(nStepStatement);
      }else{
      nLoopStart = nStepStatement;
      nStepStatement = 0; /* there is no step statement */
      }
    }else{
    nLoopStart = 0;
    }

  if( OPCODE(nToStatement) != CMD_FORTO ){
    pEo->fStop = fStopSTOP; /* at least at runtime we recognize the mistake */
    RETURN;
    }

  nLoopEnd = pEo->CommandArray[nToStatement-1].Parameter.CommandArgument.next;/* come_back(FORTO) */
  nLoopEnd = pEo->CommandArray[nLoopEnd    -1].Parameter.CommandArgument.next;
  nLoopEnd = pEo->CommandArray[nLoopEnd-1].Parameter.CommandArgument.Argument.pNode;
  if( nLoopEnd )nLoopEnd = CDR(nLoopEnd);

  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(LetThisVariable)

  NEXTPARAMETER;
  vStartExpression = EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;
  /* is the start value if undef then do NOT enter the loop by definition */
  if( memory_IsUndef(vStartExpression) ){
    SETPROGRAMCOUNTER(nLoopEnd);
    RETURN;
    }
  vStartExpression = CONVERT2NUMERIC(vStartExpression);

  vEndExpression = EVALUATEEXPRESSION(pEo->CommandArray[nToStatement-1].Parameter.CommandArgument.Argument.pNode);
  ASSERTOKE;
  vEndExpression = CONVERT2NUMERIC(vEndExpression);

  if( nStepStatement && OPCODE(nStepStatement) == CMD_FORSTEP ){/* if there is STEP part */
    vStepExpression = EVALUATEEXPRESSION(pEo->CommandArray[nStepStatement-1].Parameter.CommandArgument.Argument.pNode);
    ASSERTOKE;
    vStepExpression = CONVERT2NUMERIC(vStepExpression);
    switch( TYPE(vStepExpression) ){
      case VTYPE_LONG:
        iGoingUp = 0 < LONGVALUE(vStepExpression);
        break;
      case VTYPE_DOUBLE:
        iGoingUp = 0 < DOUBLEVALUE(vStepExpression);
        break;
      default: fprintf(stderr,"Internal error 667"); exit(667);
      }
    }else{
    iGoingUp = 1; /* by default, if there is no step statement we are going upward */
    }

  /*
     NOW HERE WE HAVE TO DECIDE IF WE HAVE TO START THE LOOP OR JUST JUMP OVER IT 
   */
  bEnterTheLoop = 0;
  if( TYPE(vStartExpression) == VTYPE_LONG && TYPE(vEndExpression) == VTYPE_LONG )
    bEnterTheLoop =  iGoingUp ? LONGVALUE(vStartExpression) <= LONGVALUE(vEndExpression)
                              : LONGVALUE(vStartExpression) >= LONGVALUE(vEndExpression);

  if( TYPE(vStartExpression) == VTYPE_LONG && TYPE(vEndExpression) == VTYPE_DOUBLE )
    bEnterTheLoop = iGoingUp ? ((double)LONGVALUE(vStartExpression)) <= DOUBLEVALUE(vEndExpression)
                             : ((double)LONGVALUE(vStartExpression)) >= DOUBLEVALUE(vEndExpression);

  if( TYPE(vStartExpression) == VTYPE_DOUBLE && TYPE(vEndExpression) == VTYPE_DOUBLE )
    bEnterTheLoop = iGoingUp ? DOUBLEVALUE(vStartExpression) <= DOUBLEVALUE(vEndExpression)
                             : DOUBLEVALUE(vStartExpression) >= DOUBLEVALUE(vEndExpression);

  if( TYPE(vStartExpression) == VTYPE_DOUBLE && TYPE(vEndExpression) == VTYPE_LONG )
    bEnterTheLoop =  iGoingUp ? DOUBLEVALUE(vStartExpression) <= (double)LONGVALUE(vEndExpression)
                              : DOUBLEVALUE(vStartExpression) >= (double)LONGVALUE(vEndExpression);

  if( bEnterTheLoop ){
    /* assign the start value to the loop variable only if we enter the loop, be definition */
    memory_ReplaceVariable(pEo->pMo,LetThisVariable,vStartExpression,_pThisCommandMortals,1);
    /* start the loop */
    SETPROGRAMCOUNTER(nLoopStart);
    RETURN;
    }
  /* We do not even step into the loop */
  SETPROGRAMCOUNTER(nLoopEnd);

#endif
END

COMMAND(FORTO)
#if NOTIMP_FORTO
NOTIMPLEMENTED;
#else

  LEFTVALUE LetThisVariable;
  VARIABLE vEndExpression;
  VARIABLE vStepExpression;
  VARIABLE vNewLoopValue;
  NODE nStepStatement;
  NODE nForStatement;
  NODE nLoopStart,nLoopEnd;
  int iGoingUp;
  int bContinueLoop;
  long refcount;
  int iError;

  vEndExpression = EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;
  vEndExpression = CONVERT2NUMERIC(vEndExpression);

  NEXTPARAMETER;
  nForStatement = CAR(PARAMETERNODE);
  NEXTPARAMETER;
  nLoopEnd = CDR(PARAMETERNODE);

  LetThisVariable = EVALUATELEFTVALUE( pEo->CommandArray[nForStatement-1].Parameter.CommandArgument.Argument.pNode );
  ASSERTOKE;
  DEREFERENCE(LetThisVariable)
  if( memory_IsUndef( *LetThisVariable) ||
      ( TYPE(*LetThisVariable) != VTYPE_LONG && TYPE(*LetThisVariable) != VTYPE_DOUBLE )  ){
    /* this may happen when the user sets the loop variable to be undef inside the loop */
    vNewLoopValue = CONVERT2NUMERIC(*LetThisVariable);
    if( vNewLoopValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    iError = memory_ReplaceVariable(pEo->pMo,LetThisVariable,vNewLoopValue,_pThisCommandMortals,0);
    if( iError )ERROR(iError);
    }
  nStepStatement = CDR(pEo->ProgramCounter);

  if( nStepStatement ){
    if( pEo->CommandArray[CAR(nStepStatement)-1].OpCode == CMD_FORSTEP ){
      nLoopStart = CDR(nStepStatement);
      nStepStatement = CAR(nStepStatement);
      }else{
      nLoopStart = nStepStatement;
      nStepStatement = 0; /* there is no step statement */
      }
    }else{
    nLoopStart = 0;
    }

  if( nStepStatement && OPCODE(nStepStatement) == CMD_FORSTEP ){
    vStepExpression = EVALUATEEXPRESSION(pEo->CommandArray[nStepStatement-1].Parameter.CommandArgument.Argument.pNode);
    ASSERTOKE;
    vStepExpression = CONVERT2NUMERIC(vStepExpression);
    switch( TYPE(vStepExpression) ){
      case VTYPE_LONG:
        iGoingUp = 0 < LONGVALUE(vStepExpression);
        switch( TYPE( (*LetThisVariable) ) ){
          case VTYPE_LONG:
            LONGVALUE( (*LetThisVariable) )+= LONGVALUE(vStepExpression);
            break;
          case VTYPE_DOUBLE:
            DOUBLEVALUE( (*LetThisVariable) )+= (double)LONGVALUE(vStepExpression);
          break;
          }
      break;
    case VTYPE_DOUBLE:
      iGoingUp = 0 < DOUBLEVALUE(vStepExpression);
        switch( TYPE( (*LetThisVariable) ) ){
          case VTYPE_LONG:
            /* this is the only tricky case when the loop variable was LONG and the step value is double
               in this case we have to convert the loop variable to double */
            (*LetThisVariable)->vType = VTYPE_DOUBLE;
            (*LetThisVariable)->Value.dValue = (double)(*LetThisVariable)->Value.lValue;
            /* do not break; here, flo on to the next case */
          case VTYPE_DOUBLE:
            DOUBLEVALUE( (*LetThisVariable) )+= DOUBLEVALUE(vStepExpression);
          break;
          }
      break;
      }
    }else{/* if there is no STEP at all */
    iGoingUp = 1; /* by default, if there is no step statement we are going upward */
    switch( TYPE( (*LetThisVariable) ) ){
      case VTYPE_LONG:
        LONGVALUE( (*LetThisVariable) )+= 1;
      break;
      case VTYPE_DOUBLE:
        DOUBLEVALUE( (*LetThisVariable) )+= 1.0;
        break;
        }
    }

  if( iGoingUp ){
    bContinueLoop = (TYPE(*LetThisVariable) == VTYPE_LONG ? LONGVALUE(*LetThisVariable) : DOUBLEVALUE(*LetThisVariable) )
          <=
                    (TYPE(vEndExpression) == VTYPE_LONG ? LONGVALUE(vEndExpression) : DOUBLEVALUE(vEndExpression) );
    }else{
    bContinueLoop = (TYPE(*LetThisVariable) == VTYPE_LONG ? LONGVALUE(*LetThisVariable) : DOUBLEVALUE(*LetThisVariable) )
          >=
                    (TYPE(vEndExpression) == VTYPE_LONG ? LONGVALUE(vEndExpression) : DOUBLEVALUE(vEndExpression) );
    }
  if( bContinueLoop ){ 
       /* continue the loop */
       SETPROGRAMCOUNTER(nLoopStart);
       }else{
       /* finish the loop  */
       SETPROGRAMCOUNTER(nLoopEnd);
       }

#endif
END

COMMAND(FORSTEP)
#if NOTIMP_FORSTEP
NOTIMPLEMENTED;
#else


 /* we should never ever get here */

#endif
END

COMMAND(NEXT)
#if NOTIMP_NEXT
NOTIMPLEMENTED;
#else
  IDENTICAL_COMMAND(WEND)
#endif
END

COMMAND(NEXTI)
#if NOTIMP_NEXTI
NOTIMPLEMENTED;
#else
  NEXTPARAMETER; /* we have to step over the symbol */
  SETPROGRAMCOUNTER(PARAMETERNODE);
#endif
END
