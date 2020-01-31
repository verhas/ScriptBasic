/*function.c

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
=H Function and sub commands

This code implements the start and the end of a function.

When the program execution reaches the code that was generated from the code

=verbatim
     fun(x,y,z)
=noverbatim

if saves some of the state variables, like the function result pointer, local variable pointer,
program counter, error resume and erroro goto values. The function evaluation does NOT allocate
local variables, and does not evaluate the function arguments. Instead it calls the executor
function recursively to perform execution starting at the node, which was generated from

=verbatim
        FUNCTION fname(a,b,c)
=noverbatim

The code of the command should evaluate the arguments (it gets the expression list node in a
class variable), and should allocate the local variables and put the values into the local
variables.

Note that the implementation of the command T<FUNCTION> is very sophisticated, and uses many
features of the execution system, which is usually not needed by other commands. Note that for this
reasons it accesses variables directly without using the hiding macros supplied via T<command.c>

The code implementing T<FUNCTION> should access the global variables, the local variables
of the caller and the local variables of the just starting function, at the same time.

Also note that the parameter evaluation is special. It first checks the expression. If this is a normal,
compound expression it does evaluate it the normal way and puts the result to the respective local variable.
However if the expression is a variable then it generates a referring variable. Whenever value is assigned to
a variable that holds a referring value the instruction T<LET> searches for the referred variable and the
assignment is done using that variable.

This means that all variables are passed by reference. If you want to pass a variable by value you should
apply some tricks, like:

=verbatim
a=1
call f(a+0)
print a
print
call f(a)
print a
print
function f(x)
 x=x+1
end function
=noverbatim

CUT*/

/**FUNCTION
=section misc
=title FUNCTION fun()

This command should be used to define a function. A function is a piece of code that can be called by the BASIC program from the main part or from a function or subroutine.

=verbatim
FUNCTION fun(a,b,c)
...
fun = returnvalue
...
END FUNCTION
=noverbatim

The end of the function is defined by the line containing the keywords T<END FUNCTION>.

=details

The function declaration can be placed anywhere in the code before or after the function is used. Because the functions just as well as the variables are typeless there is no such thing as function prototype and thus functions can not be declared beforehand.

You can call a function in your BASIC code that is not defined yet. The syntax analyzer seeing the T<function_name()> construct will see that this is a function call and will generate the appropriate code to call the function that is defined later. If the function is not defined until the end of the code then the interpreter will generate error before the code is started.

Functions just as well as subroutines can have argument and local variables. When the function or subroutine is called the actual values are evaluated and placed in the argument variables. It is not an error when calling a function to specify less or more number of actual arguments than the declared arguments.

If the actual arguments are too few the rest of the arguments will become T<undef> before the function starts. If there are too many arguments when calling a function or subroutine the extra arguments are evaluated and then dropped (ignored).

ScriptBasic allows recursive functions thus a function can call itself, but a function or subroutine is not allowed to be declared inside of another subroutine or function.

Functions differ from subroutines in the fact functions return value while subroutines do not. However in ScriptBasic this is not a strict rule. Subroutines declared with T<SUB> instead of T<FUNCTION> are allowed to return a value and been used just as if it was declared with the keyword T<FUNCTION>.

To return a value from a function the name of the function can be used just like a local variable. The assignment assigning a value to the function will be returned from the function. If there are more than one assignments to the function name then the last assignment executed will prevail. On the other hand you can not use the function name as a local variable and retrieve the last assigned value from the function name. Using the function name inside the function in an expression will result recursive call or will be used as a global or local variable.

If the actual value of an argument is left value then the reference to the actual argument is passed to the function or the subroutine instead of the value. In other cases the value of the expression standing in the position of the argument when calling the function is passed.

Passing a variable by reference means altering the argument variable also alters the variable that was passed to the function.

To force a variable passed by value rather than reference the operator T<ByVal> can be used. When T<ByVal> is used as a numeric operator in an expression acts as an identity operator. It means that the value of the expression is the same as the value standing on the right of the operator T<ByVal>. However this is already an expression and not a variable and as such it can not be passed by reference only the value.

The keyword T<ByVal> can also be used as a command listing all the argument variables after the keyword on a line:

=verbatim
function myfunc(a,b,c,d)
ByVal a,b,c
...
=noverbatim

In this case T<ByVal> acts as a command and replaces the references by copies of the values. After executing this command the argument values can be altered without affecting the variables that stand in the argument's place where the function is called.

Although the command T<ByVal> looks like a declaration of arguments to be passed by value instead of reference; this is not a declaration but it rather a command that has to be executed.
*/
COMMAND(FUNCTION)
#if NOTIMP_FUNCTION
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(FUNCTIONARG)

#endif
END

/*POD
=section FUNCTIONARG
=H FUNCTIONARG

This command implements the function head.

CUT*/
void COMMAND_FUNCTIONARG(pExecuteObject pEo
  ){
#if NOTIMP_FUNCTIONARG
NOTIMPLEMENTED;
#else
  MortalList _ThisCommandMortals=NULL;
  pMortalList _pThisCommandMortals = &_ThisCommandMortals;
  unsigned long _ActualNode=pEo->ProgramCounter;
  int iErrorCode;
  NODE nItem,nExpression;
  unsigned long i;
  unsigned long NumberOfArguments;
  long Opcode;
  pFixSizeMemoryObject ItemResult;

/* note that this code should allocate and put values into the function local variables, but
   should evaluate expressions and code using the caller local variables. Therefore we store
   the caller local variables pointer in CallerLocalVariables and whenever we need a call
   using the caller local variables we swap the two pointers for the shortest time possible.
*/
  pFixSizeMemoryObject CallerLocalVariables,SwapLVP;
/* This macro is used to maintain readability when the variables are swapped. */
#define NewFunLocalVariables CallerLocalVariables
#define SWAP_LOCAL_VARIABLES SwapLVP = CallerLocalVariables; \
                             CallerLocalVariables = pEo->LocalVariables;\
                             pEo->LocalVariables = SwapLVP;

  nItem = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm ;
  Opcode = pEo->CommandArray[nItem-1].OpCode;
  pEo->cLocalVariables = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.next;
  NumberOfArguments = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.next;
  nItem = pEo->CommandArray[nItem-1].Parameter.CommandArgument.Argument.lLongValue;

  if( ! pEo->fWeAreCallingFuction ){
    SETPROGRAMCOUNTER(CDR(nItem));
    return;
    }

  CallerLocalVariables = pEo->LocalVariables;
  if( pEo->cLocalVariables ){
    pEo->LocalVariables = memory_NewArray(pEo->pMo,1,pEo->cLocalVariables);
    if( pEo->LocalVariables == NULL ){
      pEo->fStop = fStopSTOP;
      return;
      }
    }else pEo->LocalVariables = NULL; /* it should have been null anyway */

  nItem = pEo->FunctionArgumentsNode;
  i = 0;
  while( nItem ){
    i++ ;
    nExpression = CAR(nItem);
    switch( OPCODE(nExpression) ){

      case eNTYPE_ARR:
        SWAP_LOCAL_VARIABLES;
        if( i <= NumberOfArguments ){
          NewFunLocalVariables->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
          memory_SetRef(pEo->pMo,NewFunLocalVariables->Value.aValue+i-1,execute_LeftValueArray(pEo,nExpression,_pThisCommandMortals,&iErrorCode));
          }else execute_LeftValueArray(pEo,nExpression,_pThisCommandMortals,&iErrorCode);
        SWAP_LOCAL_VARIABLES;
        break;

      case eNTYPE_SAR:
        SWAP_LOCAL_VARIABLES;
        if( i <= NumberOfArguments ){
          NewFunLocalVariables->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
          memory_SetRef(pEo->pMo,NewFunLocalVariables->Value.aValue+i-1,execute_LeftValueSarray(pEo,nExpression,_pThisCommandMortals,&iErrorCode));
          }else execute_LeftValueSarray(pEo,nExpression,_pThisCommandMortals,&iErrorCode);
        SWAP_LOCAL_VARIABLES;
        break;

      case eNTYPE_LVR:
        SWAP_LOCAL_VARIABLES;
        if( i <= NumberOfArguments ){
          NewFunLocalVariables->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
          memory_SetRef(pEo->pMo,NewFunLocalVariables->Value.aValue+i-1,&(pEo->LocalVariables->Value.aValue[pEo->CommandArray[nExpression-1].Parameter.Variable.Serial-1]));
          }
        SWAP_LOCAL_VARIABLES;
        break;

      case eNTYPE_GVR:
        if( i <= NumberOfArguments ){
          pEo->LocalVariables->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
          memory_SetRef(pEo->pMo,pEo->LocalVariables->Value.aValue+i-1,&(pEo->GlobalVariables->Value.aValue[pEo->CommandArray[nExpression-1].Parameter.Variable.Serial-1]));
          }
        break;

      default:
        SWAP_LOCAL_VARIABLES;
        ItemResult = EVALUATEEXPRESSION(nExpression);
        SWAP_LOCAL_VARIABLES;
        ASSERTOKE;
        if( ItemResult)
          memory_Immortalize(ItemResult,_pThisCommandMortals);
        if( i <= NumberOfArguments )
          pEo->LocalVariables->Value.aValue[i-1] = ItemResult;
        break;
      }

    nItem = CDR(nItem);
    }

  memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);
  /* and finally we start to execute the function when executing the next command */
  pEo->lFunctionLevel++;
  /* some macros need this label */
#endif
_FunctionFinishLabel: ;
  }

/*

This structure is used to maintain the return gosub stack. This structure
becomes empty when there is no any GOSUB/RETURN pairs pending. If some
of the GOSUB-s did not return when the code finishes the memory is released
when the segment is released. We do leak here memory at this level, because
upper levels release this memory before exiting the interpreter.

*/
typedef struct _GosubStack {
  struct _GosubStack *next;
  long lFunctionLevel;
  NODE nReturnNode;
  } GosubStack , *pGosubStack;

#define GosubStackRoot (*((GosubStack **)pEo->CommandParameter+CMD_GOSUB - START_CMD))

/**GOSUB
=section misc
=H Gosub commands
=title GOSUB label

This is the good old way implementation of the BASIC T<GOSUB> command. The command T<GOSUB> works similar to the command T<GOTO> with the exception that the next return command will drive the interpreter to the line following the line with the T<GOSUB>.

You can only call a code segment that is inside the actual code environment. In other words if the T<GOSUB> is in a function or subroutine then the label referenced by the T<GOSUB> should also be in the same function or subroutine. Similarly any T<GOSUB> in the main code should reference a label, which is also in the main code.

To return from the code fragment called by the command T<GOSUB> the command T<RETURN> should be used. Note that this will not break the execution of a function or a subroutine. The execution will continue on the command line following the T<GOSUB> line.

T<GOSUB> commands can follow each other, ScriptBasic will build up a stack of T<GOSUB> calls and will return to the appropriate command line following the matching T<GOSUB> command.

When a subroutine or function contains T<GOSUB> commands and the function or subroutine is finished so that one or more executed T<GOSUB> command remains without executed T<RETURN> then the T<GOSUB>/T<RATURN> stack is cleared. This is not an error.

See also R<RETURN>.
*/
COMMAND(GOSUB)
#if NOTIMP_GOSUB
NOTIMPLEMENTED;
#else

  pGosubStack pGSS;

  pGSS = ALLOC(sizeof(GosubStack));
  if( pGSS == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  pGSS->lFunctionLevel = pEo->lFunctionLevel;
  pGSS->nReturnNode = pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.rest;
  pGSS->next = GosubStackRoot;
  GosubStackRoot = pGSS;
  SETPROGRAMCOUNTER(PARAMETERNODE);

#endif
END

/**RETURN
=section misc

Return from a subroutine started with R<GOSUB>. For more information see the documentation of the command R<GOSUB>.
*/
COMMAND(RETURNC)
#if NOTIMP_RETURNC
NOTIMPLEMENTED;
#else

  pGosubStack pGSS;

  pGSS = GosubStackRoot;
  if( pGSS == NULL || pGSS->lFunctionLevel < pEo->lFunctionLevel )ERROR(COMMAND_ERROR_RETURN_WITHOUT_GOSUB);
  GosubStackRoot = GosubStackRoot->next;
  SETPROGRAMCOUNTER(pGSS->nReturnNode);
  FREE(pGSS);
#endif
END

/**POP
=section misc

Pop off one value from the GOSUB/RETURN stack. After this command a T<RETURN> will return to one
level higher and to the place where it was called from.
For more information see the documentation of the command R<GOSUB> and R<RETURN>.
*/
COMMAND(POP)
#if NOTIMP_POP
NOTIMPLEMENTED;
#else

  pGosubStack pGSS;

  pGSS = GosubStackRoot;
  if( pGSS == NULL || pGSS->lFunctionLevel < pEo->lFunctionLevel )ERROR(COMMAND_ERROR_RETURN_WITHOUT_GOSUB);
  GosubStackRoot = GosubStackRoot->next;
  /* this is what POP does not do as opposed to RETURN */
/*SETPROGRAMCOUNTER(pGSS->nReturnNode);*/
  FREE(pGSS);
#endif
END

/**EXITFUNC
=section misc
=title EXIT FUNCTION
=display EXIT FUNCTION

This function stops the execution of a function and the execution gets back to the point from where the function was called. Executing this command has the same effect as if the execution has reached the end of a function.
*/
COMMAND(EXITFUNC)
#if NOTIMP_EXITFUNC
NOTIMPLEMENTED;
#else

  pGosubStack pGSS;

  /* step back the function level because we are leaving the function */
  pEo->lFunctionLevel--;

  /* clean up the gosub stack */
  pGSS = GosubStackRoot;
  while( pGSS && pGSS->lFunctionLevel > pEo->lFunctionLevel ){
    GosubStackRoot = GosubStackRoot->next;
    FREE(pGSS);
    pGSS = GosubStackRoot;
    }
  pEo->fStop = fStopRETURN;

#endif
END

COMMAND(ENDFUNC)
#if NOTIMP_ENDFUNC
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(EXITFUNC)

#endif
END

COMMAND(FLET)
#if NOTIMP_FLET
NOTIMPLEMENTED;
#else


  VARIABLE ItemResult;

  /* here we get a mortal value as result. This should be mortal, because it is immortalized and
     the code later assumes that this is an immortal memory piece that is assigned only to this
     "variable"
  */
  ItemResult = _EVALUATEEXPRESSION_A(PARAMETERNODE);
  ASSERTOKE;

  if( ItemResult && TYPE(ItemResult) == VTYPE_ARRAY )ERROR(COMMAND_ERROR_NOARRAY);

  ItemResult = memory_DupMortalize(pEo->pMo,ItemResult,_pThisCommandMortals,&iErrorCode);

  if( pEo->pFunctionResult )
    memory_ReleaseVariable(pEo->pMo,pEo->pFunctionResult);
  if( ItemResult )IMMORTALIZE(ItemResult);
  pEo->pFunctionResult = ItemResult;

#endif
END

/**ADDRESSF
=section misc
=title ADDRESS( myFunc() )
=display ADDRESS()

Return the entry point of a function or subroutine. The returned value is to be used solely in a corresponding R<ICALL>.

The returned value is an integer value that is the internal node number of the compiled code where the function starts. The different node numbers are in complex relation with each other and simple rules can not be applied. In other words playing around with the value returned by the function T<ADDRESS> and then using it in an T<ICALL> may result interpreter crash raising internal error.

Note that in the argument of the function T<ADDRESS> the function name has to include the T<()> characters. The function is NOT called by the code when the function T<ADDRESS> is used. On the other hand forgetting the opening and closing parentheses will result erroneous value unusable by T<ICALL>.

*/
COMMAND(ADDRESSF)
#if NOTIMP_ADDRESSF
NOTIMPLEMENTED;
#else

  NODE z;

  USE_CALLER_MORTALS;
  z = PARAMETERLIST;

  if( OPCODE(CAR(z)) != eNTYPE_FUN )ERROR(COMMAND_ERROR_INVALID_ARGUMENT_FOR_FUNCTION_ADDRESS);

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = CAR(pEo->CommandArray[z-1].Parameter.UserFunction.NodeId);

#endif
END

/**ICALL
=section misc
=title ICALL n,v1,v2, ... ,vn
=display ICALL

ICALL is implicit call. The first argument of an T<ICALL> command or T<ICALL> function should be the integer value returned by the function R<ADDRESS> as the address of a user defined function.

The rest of the arguments are the arguments to be passed to the function to be called. The return value if the function T<ICALL> is the value of the implicitly called function.

=details

Whenever you call a function or subroutine you have to know the name of the subroutine or function. In some situation programmers want to call a function without knowing the name of the function. For example you want to write a sorting subroutine that sorts general elements and the caller should provide a subroutine that makes the comparison. This way the sorting algorithm can be implemented only once and need not be rewritten each time a new type of data is to be sorted. The sorting subroutine gets the comparing function as an argument and calls the function indirectly. ScriptBasic can not pass functions as arguments to other functions, but it can pass integer numbers. The function
R<ADDRESS> can be used to convert a function into integer. The result of the built-in function R<ADDRESS> is an integer number, which is associated inside the basic code with the function. You can pass this value to the T<ICALL> command or function as first argument. The T<ICALL> command is the command for indirect subroutine call. The call

=verbatim
ICALL Address(MySubroutine()),arg1,arg2,arg3
=noverbatim

is equivalent to

=verbatim
CALL MySubroutine( arg1,arg2,arg3)
=noverbatim

If you call a function that has return value use can use the T<ICALL> function instead of the T<ICALL> statement:

=verbatim
A = ICALL(Address(MyFunction()),arg1,arg2,arg3)
=noverbatim

is equivalent to

=verbatim
A = MyFunction(arg1,arg2,arg3)
=noverbatim

The real usage of the function Address and icall can be seen in the following example:

=verbatim
sub MySort(sfun,q)
local ThereWasNoChange,SwapVar
repeat
 ThereWasNoChange = 1
 for i=lbound(q) to ubound(q)-1

  if  icall(sfun,q[i],q[i+1]) > 0 then
   ThereWasNoChange = 0
   SwapVar = q[i]
   q[i] = q[i+1]
   q[i+1] = SwapVar
  endif

 next i
until ThereWasNoChange

end sub

function IntegerCompare(a,b)
  if a < b then
   cmp = -1
  elseif a = b then
   cmp = 0
  else
   cmp = 1
  endif
end function

h[0] = 2
h[1] = 7
h[2] = 1

MySort address(IntegerCompare()) , h

for i=lbound(h) to ubound(h)
 print h[i],"\n"
next i
=noverbatim

Note that the argument of the function Address is a function call. ScriptBasic allows variables and functions to share the same name. Address is a built-in function just as any other built in function, and therefore the expression

=verbatim
Address(MySub) B<THIS IS WRONG!>
=noverbatim

Is syntactically correct. The only problem is that it tries to calculate the address of the variable T<MySub>, which it can not and results a run-time error.
Instead you have to write

=verbatim
Address( MySub() )
=noverbatim

using the parentheses. In this situation the function or subroutine
T<MySub()> will not be invoked. The parentheses tell the compiler that this is a function and not a variable.
*/
COMMAND(ICALLFUN)
#if NOTIMP_ICALLFUN
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE ItemResult;
  pFixSizeMemoryObject ThisFunctionResultPointer;
  unsigned long SaveProgramCounter,SaveStepCounter;
  unsigned long SavefErrorGoto,SaveErrorGoto,SaveErrorResume;
  pFixSizeMemoryObject SaveLocalVariablesPointer;
  pFixSizeMemoryObject SaveFunctionResultPointer;
  long CommandOpCode;

  USE_CALLER_MORTALS;

  if( pEo->FunctionLevelLimit && pEo->lFunctionLevel > pEo->FunctionLevelLimit )
    ERROR(EXE_ERROR_TOO_DEEP_CALL);

  SaveLocalVariablesPointer = pEo->LocalVariables;
  SaveProgramCounter = pEo->ProgramCounter;

  nItem = PARAMETERLIST;
  ItemResult = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  pEo->ProgramCounter = LONGVALUE(ItemResult);

  CommandOpCode = pEo->CommandArray[pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm-1].OpCode;
  if( CommandOpCode < START_CMD || CommandOpCode > END_CMD ){
    ERROR(EXE_ERROR_USERFUN_UNDEFINED);
    }
  if( CommandOpCode != CMD_FUNCTION &&
      CommandOpCode != CMD_FUNCTIONARG &&
      CommandOpCode != CMD_SUB &&
      CommandOpCode != CMD_SUBARG ){
    ERROR(EXE_ERROR_USERFUN_UNDEFINED);
    }

  pEo->FunctionArgumentsNode = CDR(nItem);
  SaveFunctionResultPointer = pEo->pFunctionResult;
  pEo->pFunctionResult = NULL;
  SaveStepCounter = pEo->lStepCounter;
  pEo->lStepCounter = 0;
  pEo->fWeAreCallingFuction = 1;
  SaveErrorGoto = pEo->ErrorGoto;
  pEo->ErrorGoto = 0;
  SaveErrorResume = pEo->ErrorResume;
  pEo->ErrorResume = 0;
  SavefErrorGoto = pEo->fErrorGoto;
  pEo->fErrorGoto = ONERROR_NOTHING;
  if( pEo->pHookers->HOOK_ExecCall &&
      (iErrorCode = pEo->pHookers->HOOK_ExecCall(pEo)) )
    ERROR(iErrorCode);
  /* function entering code needs access to the caller local variables, therefore
     WE SHOULD NOT NULL pEo->LocalVariables */
  execute_Execute_r(pEo,&iErrorCode);
  if( pEo->pHookers->HOOK_ExecReturn &&
      (iErrorCode = pEo->pHookers->HOOK_ExecReturn(pEo)) )
    ERROR(iErrorCode);

  pEo->lStepCounter = SaveStepCounter;
  if( pEo->LocalVariables )/* this is null if the function did not have arguments and no local variables */
    memory_ReleaseVariable(pEo->pMo,pEo->LocalVariables);
  pEo->ProgramCounter = SaveProgramCounter;
  pEo->LocalVariables = SaveLocalVariablesPointer;
  ThisFunctionResultPointer = pEo->pFunctionResult;
  pEo->pFunctionResult = SaveFunctionResultPointer;
  /* Functions return their value as immortal values assigned to the very global
     variable pEo->pFunctionResult. Here this variable is restored to point to the
     saved value and the value returned should be mortalized.                   */
  if( ThisFunctionResultPointer && ! IsMortal(ThisFunctionResultPointer) )
    memory_Mortalize(ThisFunctionResultPointer,_pThisCommandMortals);

  pEo->ErrorGoto = SaveErrorGoto;
  pEo->fErrorGoto = SavefErrorGoto;
  pEo->ErrorResume = SaveErrorResume;
  if( iErrorCode )ERROR(iErrorCode);

  RESULT = ThisFunctionResultPointer;
#endif
END

COMMAND(ICALL)
#if NOTIMP_ICALL
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  unsigned long SaveProgramCounter,SaveStepCounter;
  unsigned long SavefErrorGoto,SaveErrorGoto,SaveErrorResume;
  pFixSizeMemoryObject SaveLocalVariablesPointer;
  pFixSizeMemoryObject SaveFunctionResultPointer;
  long CommandOpCode;

  if( pEo->FunctionLevelLimit && pEo->lFunctionLevel > pEo->FunctionLevelLimit )
    ERROR(EXE_ERROR_TOO_DEEP_CALL);

  SaveLocalVariablesPointer = pEo->LocalVariables;
  SaveProgramCounter = pEo->ProgramCounter;

  ItemResult = EVALUATEEXPRESSION(CAR(PARAMETERNODE));
  ASSERTOKE;
  pEo->ProgramCounter = GETLONGVALUE(ItemResult);

  CommandOpCode = pEo->CommandArray[pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm-1].OpCode;
  if( CommandOpCode < START_CMD || CommandOpCode > END_CMD ){
    ERROR(EXE_ERROR_USERFUN_UNDEFINED);
    }
  if( CommandOpCode != CMD_FUNCTION &&
      CommandOpCode != CMD_FUNCTIONARG &&
      CommandOpCode != CMD_SUB &&
      CommandOpCode != CMD_SUBARG ){
    ERROR(EXE_ERROR_USERFUN_UNDEFINED);
    }

  pEo->FunctionArgumentsNode = CDR(PARAMETERNODE);
  SaveFunctionResultPointer = pEo->pFunctionResult;
  pEo->pFunctionResult = NULL;
  SaveStepCounter = pEo->lStepCounter;
  pEo->lStepCounter = 0;
  pEo->fWeAreCallingFuction = 1;
  SaveErrorGoto = pEo->ErrorGoto;
  pEo->ErrorGoto = 0;
  SaveErrorResume = pEo->ErrorResume;
  pEo->ErrorResume = 0;
  SavefErrorGoto = pEo->fErrorGoto;
  pEo->fErrorGoto = ONERROR_NOTHING;
  if( pEo->pHookers->HOOK_ExecCall &&
      (iErrorCode = pEo->pHookers->HOOK_ExecCall(pEo)) )
    ERROR(iErrorCode);
  /* function entering code needs access to the caller local variables, therefore
     WE SHOULD NOT NULL pEo->LocalVariables */
  execute_Execute_r(pEo,&iErrorCode);
  if( pEo->pHookers->HOOK_ExecReturn &&
      (iErrorCode = pEo->pHookers->HOOK_ExecReturn(pEo)) )
    ERROR(iErrorCode);

  pEo->lStepCounter = SaveStepCounter;
  if( pEo->LocalVariables )/* this is null if the function did not have arguments and no local variables */
    memory_ReleaseVariable(pEo->pMo,pEo->LocalVariables);
  pEo->ProgramCounter = SaveProgramCounter;
  pEo->LocalVariables = SaveLocalVariablesPointer;
  memory_ReleaseVariable(pEo->pMo,pEo->pFunctionResult);
  pEo->pFunctionResult = SaveFunctionResultPointer;

  pEo->ErrorGoto = SaveErrorGoto;
  pEo->fErrorGoto = SavefErrorGoto;
  pEo->ErrorResume = SaveErrorResume;
  if( iErrorCode )ERROR(iErrorCode);

#endif
END

/**CALL
=section misc
=title CALL subroutine

Use this command to call a subroutine. Subroutines can be called just writing the name of the already defined subroutine and the arguments. However in situation when the code calls a function that has not yet been defined the interpreter knows that the command is a subroutine call from the keyword T<CALL>.

To be safe you can use the keyword before any subroutine call even if the subroutine is already defined.

Subroutines and functions can be called the same way. ScriptBasic does not make real distinction between subroutines and functions. However it is recommended that functions be used as functions using the return value and code segments not returning any value are implemented and called as subroutine.


*/
COMMAND(CALL)
#if NOTIMP_CALL
NOTIMPLEMENTED;
#else


  _EVALUATEEXPRESSION(PARAMETERNODE);
  ASSERTOKE;

#endif
END

/**SUB
=section misc
=title SUB fun()

This command should be used to define a subroutine. A subroutine is a piece of code that can be called by the BASIC program from the main part or from a function or subroutine.

=verbatim
SUB sub(a,b,c)
...
END SUB
=noverbatim

The end of the subroutine is defined by the line containing the keywords T<END SUB>.

Note that functions and subroutines are not really different in ScriptBasic. ScriptBasic allows you to return a value from a subroutine and to call a function using the command T<CALL>. It is just a convention to have separately T<SUB> and T<FUNCTION> declarations.

For detailed information please read the documentation of the command R<FUNCTION>
*/
COMMAND(SUB)
#if NOTIMP_SUB
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(FUNCTIONARG)

#endif
END

/*POD
=section SUBARG
=H SUBARG

Same as R<FUNCTIONARG>

CUT*/
COMMAND(SUBARG)
#if NOTIMP_SUBARG
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(FUNCTIONARG)

#endif
END


/**EXITSUB
=section misc
=title EXIT SUB
=display EXIT SUB

This function stops the execution of a subroutine and the execution gets back to the point from where the subroutine was called. Executing this command has the same effect as if the execution has reached the end of a subroutine.

Same as R<EXITFUNC>

*/
COMMAND(EXITSUB)
#if NOTIMP_EXITSUB
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(EXITFUNC)

#endif
END


COMMAND(ENDSUB)
#if NOTIMP_ENDSUB
NOTIMPLEMENTED;
#else

  IDENTICAL_COMMAND(EXITFUNC)

#endif
END
