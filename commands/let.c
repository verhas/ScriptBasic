/*let.c

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
#include <limits.h>

#include <math.h>
#include "../command.h"

/* this function is defined in mathops.c */
long *RaiseError(pExecuteObject pEo);

/*POD
=H Commands that assign values to variables

This file contains commands that assign value to variables.

CUT*/

/**LET
=section misc
=title v = expression
=display LET

Assign a value to a variable. 

On the left side of the T<=> a variable or some other ScriptBasic left value has to stand. On the right side an expression should be used. First the left value is evaluated and then the expression. Finally the left value's old value is replaced by the result of the expression.

The left value standing on the left side of the T<=> can be a local or global variable, array element or associative array element.

*/
COMMAND(LET)
#if NOTIMP_LET
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult,ArrayCopyTo;
  LEFTVALUE LetThisVariable;
  long refcount;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  ExpressionResult = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,1);
  ASSERTOKE;

  /* If this is not an expression then copy the value. */
  if( ExpressionResult == NULL || TYPE(ExpressionResult) != VTYPE_ARRAY ){
    memory_ReplaceVariable(pEo->pMo,LetThisVariable,ExpressionResult,_pThisCommandMortals,1);
    RETURN;
    }

  /* If this is an array then copy the array itself. All elements, but the references will remain
     references. */
  ArrayCopyTo = memory_CopyArray(pEo->pMo,ExpressionResult);
  if( ArrayCopyTo == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
  memory_ReplaceVariable(pEo->pMo,LetThisVariable,ArrayCopyTo,_pThisCommandMortals,0);
#endif
END

COMMAND(LLET)
END

/**SWAP
=section misc
=title swap a,b
=display SWAP

Planned command.

This command swaps two variables.

*/
COMMAND(SWAP)
#if NOTIMP_SWAP
NOTIMPLEMENTED;
#else
  LEFTVALUE VariableA,VariableB;
  pFixSizeMemoryObject VSWAP;
  long refcount;

  /* we get the pointer to the variable that points to the value */
  VariableA = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(VariableA);

  /* get the next parameter of the command, which is the other variable */
  NEXTPARAMETER;

  /* we get the pointer to the variable that points to the value */
  VariableB = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(VariableB);

  VSWAP = *VariableA;
  *VariableA = *VariableB;
  *VariableB = VSWAP;

#endif
END

/**REF
=section misc
=title ref v1 = v2
=display REF

Assign a variable to reference another variable. Following this command altering one of the variables alters both variables. In other words this command can be used to define a kind of alias to a variable. The mechanism is the same as local variable of a function is an alias of a variable passed to the function as actual argument. The difference is that this reference is not automatically released when some function returns, but rather it is alive so long as long the referencing variable is not undefined saying T<undef variable> in a command.

To have an alias to a variable is not something of a great value though. It becomes a real player when the 'variable' is not just an ordinary 'named' variable but rather part of an array (or associative array). Using this mechanisms the programmer can build up arbitrary complex memory structures without caring such complex things as pointers for example in C. This is a simple BASIC way of building up complex memory structures.

*/
COMMAND(REF)
#if NOTIMP_REF
NOTIMPLEMENTED;
#else
  LEFTVALUE v1,v2;
  VARIABLE NewValue;
  long refcount;
  int iError;

  /* we get the pointer to the variable that points to the value */
  v1 = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* Dereference the variable to set the variable referenced by the actual variable
     to be a reference to another variable. In case this is not what the programmer
     wants then he/she can undef the variable first. */
  DEREFERENCE(v1);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  v2 = EVALUATELEFTVALUE_A(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(v2);

  /* capture it here, this is a harmless action, no v1 was changed or left in an unusable state
     */
  if( v1 == v2 )RETURN;

  NewValue = memory_NewRef(pEo->pMo);
  if( *v1 ){
    NewValue->link.rprev = (*v1)->link.rprev;
    (*v1)->link.rprev = NULL;
    }else{
    NewValue->link.rprev = NULL;
    }
  if( *v1 )memory_ReleaseVariable(pEo->pMo,*v1);

  *v1 = NewValue;

  iError = memory_SetRef(pEo->pMo,v1,v2);

  if( iError )ERROR(iError);
#endif
END

/**UNDEF
=section misc
=title UNDEF variable
=display UNDEF

Sets the value of a variable (or some other ScriptBasic left value) to be undefined. This command can also be used to release the memory that was occupied by an array when the variable holding the array is set to T<undef>.

When this command is used as a function (with or without, but usually without parentheses), it simply returns the value T<undef>.

=details

Note that when this command is called in a function then the local variable is undefined and the caller variable passed by reference is not changed. Therefore

=verbatim

sub xx(a)
 undef a
end sub

q = 1
xx q
print q
=noverbatim

will print 1 and not T<undef>.

On the other hand

=verbatim

sub xx(a)
 a = undef
end sub

q = 1
xx q
print q
=noverbatim

does print T<undef>.
*/
COMMAND(CUNDEF)
#if NOTIMP_CUNDEF
NOTIMPLEMENTED;
#else

  NODE nItem;
  LEFTVALUE LetThisVariable;

  nItem = PARAMETERNODE;
  while( nItem ){

    LetThisVariable = EVALUATELEFTVALUE_A(CAR(nItem));
    ASSERTOKE;
    /* do NOT dereference by definition */
    if( *LetThisVariable == NULL ){
      nItem = CDR(nItem);
      continue;
      }

    memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
    *LetThisVariable = NULL;
    nItem = CDR(nItem);
    }

#endif
END

/*BYVAL
=section misc

Convert an argument variable to local. Saying

=verbatim
SUB fun(a,b,c)
BYVAL a,b

 ....

END SUB
=noverbatim

will make the variables T<a> and T<b> truly local, and assigning a value to them will not alter the variable that was passed as parameter when calling T<fun>.

This keyword can also be used as an operator, like

=verbatim
CALL fun(BYVAL a, BYVAL B)
=noverbatim

that will pass the variables T<a> and T<b> to the function by value instead of reference.

=details

T<BYVAL> as an operator actually does nothing but returns the value of its argument.

When T<BYVAL> is used as a command it assigns the value of the variable to the local variable that may have been a reference to a global variable before that.
*/
COMMAND(CBYVAL)
#if NOTIMP_CBYVAL
NOTIMPLEMENTED;
#else

  NODE nItem;
  LEFTVALUE LetThisVariable;
  VARIABLE NewValue;
  unsigned long refcount;

  nItem = PARAMETERNODE;
  while( nItem ){

    LetThisVariable = EVALUATELEFTVALUE_A(CAR(nItem));
    ASSERTOKE;

    if( *LetThisVariable == NULL || TYPE(*LetThisVariable) != VTYPE_REF ){
      nItem = CDR(nItem);
      continue;
      }
    NewValue = *LetThisVariable;
    refcount = pEo->pMo->maxderef;
    while( NewValue && TYPE(NewValue) == VTYPE_REF ){
      NewValue = *(NewValue->Value.aValue);
      if( ! refcount-- )ERROR(COMMAND_ERROR_CIRCULAR);
      }
    if( NewValue ){
      NewValue = memory_DupImmortal(pEo->pMo,NewValue,&iErrorCode);
      }
    if( *LetThisVariable )
      memory_ReleaseVariable(pEo->pMo,*LetThisVariable); /* release the ref value */
    *LetThisVariable = NewValue;
    nItem = CDR(nItem);
    }

#endif
END

/**LETM
=section misc
=title v -= expression
=display LETM

This command subtracts a value from a variable.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v - expression>. Using this short format is more readable in
some cases and generates more efficient code. However note that this kind of assignment operation is a C language
like operator and is not common in BASIC programs.
*/
COMMAND(LETM)
#if NOTIMP_LETM
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;
  int bResultIsBig;
  long Lop2;
  double dResult;
  long lResult;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  if( memory_Type(*LetThisVariable) == VTYPE_DOUBLE ){
    dResult = DOUBLEVALUE(*LetThisVariable) - GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      IMMORTALIZE(ExpressionResult);
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = ExpressionResult;
      RETURN;
      }
    DOUBLEVALUE(*LetThisVariable) = dResult;
    RETURN;
    }

  /* we assume that the result is not big so long as long it turns out to be */
  bResultIsBig = 0;
  /* If the variable is long and the expression is convertable to long (aka not double) then
     there is no reason to release the variable and allocate a new one to hold the resulting 
     long. The result is stored in the same place as the original value.
     This code could be left out but it increases the interpreter speed. I did measure. */
  if( memory_Type(*LetThisVariable) == VTYPE_LONG &&
      (memory_Type(Op2) == VTYPE_LONG || (memory_Type(Op2) == VTYPE_STRING && ISSTRINGINTEGER(Op2)) )){
    Lop2 = GETLONGVALUE(Op2);
    dResult = ((double)LONGVALUE(*LetThisVariable)) - ((double)Lop2);
    if( dResult <= ((double)LONG_MAX) && dResult >= ((double)LONG_MIN) ){
      LONGVALUE(*LetThisVariable) -= Lop2;
      RETURN;
      }
    bResultIsBig = 1;
    }

  Op1 = *LetThisVariable;

      /* if the numbers are integer, but the result overflows plus or minus */
  if( bResultIsBig ||
     /* if any of the arguments is double then the result is double */
     (memory_Type(Op1) == VTYPE_DOUBLE || memory_Type(Op2) == VTYPE_DOUBLE) ||

     /* if the first argument is string and is NOT integer */
     ( memory_Type(Op1) == VTYPE_STRING && !ISSTRINGINTEGER(Op1)) ||

     /* if the second argument is string and is NOT integer */
     ( memory_Type(Op2) == VTYPE_STRING && !ISSTRINGINTEGER(Op2))
     ){
    dResult = GETDOUBLEVALUE(Op1) - GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      }else{
      ExpressionResult = NEWMORTALDOUBLE;
      ASSERTNULL(ExpressionResult);
      DOUBLEVALUE(ExpressionResult) = dResult;
      }
    }else{
    ExpressionResult = NEWMORTALLONG;
    ASSERTNULL(ExpressionResult);
    LONGVALUE(ExpressionResult) = GETLONGVALUE(Op1) - GETLONGVALUE(Op2);
    }

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END

/**LETP
=section misc
=title v += expression
=display LETP

Add a value to a variable.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v + expression>. Using this short format is more readable in some cases and generates more efficient code. However note that this kind of assignment operation is a C language like operator and is not common in BASIC programs.
*/
COMMAND(LETP)
#if NOTIMP_LETP
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;
  int bResultIsBig;
  long Lop2;
  double dResult;
  long lResult;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  if( memory_Type(*LetThisVariable) == VTYPE_DOUBLE ){
    dResult = DOUBLEVALUE(*LetThisVariable) + GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      IMMORTALIZE(ExpressionResult);
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = ExpressionResult;
      RETURN;
      }
    DOUBLEVALUE(*LetThisVariable) = dResult;
    RETURN;
    }

  /* we assume that the result is not big so long as long it turns out to be */
  bResultIsBig = 0;
  /* If the variable is long and the expression is convertable to long (aka not double) then
     there is no reason to release the variable and allocate a new one to hold the resulting 
     long. The result is stored in the same place as the original value.
     This code could be left out but it increases the interpreter speed. I did measure. */
  if( memory_Type(*LetThisVariable) == VTYPE_LONG &&
      (memory_Type(Op2) == VTYPE_LONG || (memory_Type(Op2) == VTYPE_STRING && ISSTRINGINTEGER(Op2)) )){
    Lop2 = GETLONGVALUE(Op2);
    dResult = ((double)Lop2) + ((double)LONGVALUE(*LetThisVariable));
    if( dResult <= ((double)LONG_MAX) && dResult >= ((double)LONG_MIN) ){
      LONGVALUE(*LetThisVariable) += Lop2;
      RETURN;
      }
    bResultIsBig = 1;
    }
  Op1 = *LetThisVariable;

      /* if the numbers are integer, but the result overflows plus or minus */
  if( bResultIsBig ||
     /* if any of the arguments is double then the result is double */
     (memory_Type(Op1) == VTYPE_DOUBLE || memory_Type(Op2) == VTYPE_DOUBLE) ||

     /* if the first argument is string and is NOT integer */
     ( memory_Type(Op1) == VTYPE_STRING && !ISSTRINGINTEGER(Op1)) ||

     /* if the second argument is string and is NOT integer */
     ( memory_Type(Op2) == VTYPE_STRING && !ISSTRINGINTEGER(Op2))
     ){
    dResult = GETDOUBLEVALUE(Op1) + GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      }else{
      ExpressionResult = NEWMORTALDOUBLE;
      ASSERTNULL(ExpressionResult);
      DOUBLEVALUE(ExpressionResult) = dResult;
      }
    }else{
    ExpressionResult = NEWMORTALLONG;
    ASSERTNULL(ExpressionResult);
    LONGVALUE(ExpressionResult) = GETLONGVALUE(Op1) + GETLONGVALUE(Op2);
    }

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END

/**LETS
=section misc
=title v *= expression
=display LETS

Multiply a variable with a value.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v * expression>. Using this short format is more readable in some cases and generates more efficient code. However note that this kind of assignment operation is a C language like operator and is not common in BASIC programs.
*/
COMMAND(LETS)
#if NOTIMP_LETS
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;
  int bResultIsBig;
  long Lop2;
  double dResult;
  long lResult;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  /* If the variable is double then the final value is double and thus there is no reason to
     release the variable and allocate a new one to hold the resulting double. The result is 
     stored in the same place as the original value. This code could be left out but it
     increases the interpreter speed. I did measure. */
  if( memory_Type(*LetThisVariable) == VTYPE_DOUBLE ){
    dResult = DOUBLEVALUE(*LetThisVariable) * GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      IMMORTALIZE(ExpressionResult);
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = ExpressionResult;
      RETURN;
      }
    DOUBLEVALUE(*LetThisVariable) = dResult;
    RETURN;
    }

  /* we assume that the result is not big so long as long it turns out to be */
  bResultIsBig = 0;
  /* If the variable is long and the expression is convertable to long (aka not double) then
     there is no reason to release the variable and allocate a new one to hold the resulting 
     long. The result is stored in the same place as the original value.
     This code could be left out but it increases the interpreter speed. I did measure. */
  if( memory_Type(*LetThisVariable) == VTYPE_LONG &&
      (memory_Type(Op2) == VTYPE_LONG || (memory_Type(Op2) == VTYPE_STRING && ISSTRINGINTEGER(Op2)) )){
    Lop2 = GETLONGVALUE(Op2);
    dResult = ((double)Lop2) * ((double)LONGVALUE(*LetThisVariable));
    if( dResult <= ((double)LONG_MAX) && dResult >= ((double)LONG_MIN) ){
      LONGVALUE(*LetThisVariable) *= Lop2;
      RETURN;
      }
    bResultIsBig = 1;
    }

  Op1 = *LetThisVariable;

      /* if the numbers are integer, but the result overflows plus or minus */
  if( bResultIsBig ||
     /* if any of the arguments is double then the result is double */
     (memory_Type(Op1) == VTYPE_DOUBLE || memory_Type(Op2) == VTYPE_DOUBLE) ||

     /* if the first argument is string and is NOT integer */
     ( memory_Type(Op1) == VTYPE_STRING && !ISSTRINGINTEGER(Op1)) ||

     /* if the second argument is string and is NOT integer */
     ( memory_Type(Op2) == VTYPE_STRING && !ISSTRINGINTEGER(Op2))
     ){
    dResult = GETDOUBLEVALUE(Op1) * GETDOUBLEVALUE(Op2);
    lResult = (long)dResult;
    if( dResult == (double)lResult ){
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = lResult;
      }else{
      ExpressionResult = NEWMORTALDOUBLE;
      ASSERTNULL(ExpressionResult);
      DOUBLEVALUE(ExpressionResult) = dResult;
      }
    }else{
    ExpressionResult = NEWMORTALLONG;
    ASSERTNULL(ExpressionResult);
    LONGVALUE(ExpressionResult) = GETLONGVALUE(Op1) * GETLONGVALUE(Op2);
    }

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END

/**LETD
=section misc
=title v /= expression
=display LETD

Divide a variable by an expression.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v / expression>. Using this short format is more readable in some cases and generates more efficient code. However note that this kind of assignment operation is a C language like operator and is not common in BASIC programs.
*/
COMMAND(LETD)
#if NOTIMP_LETD
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;
  long lop2;
  double dResult;
  long lResult;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  /* If the variable is double then the final value is double and thus there is no reason to
     release the variable and allocate a new one to hold the resulting double. The result is 
     stored in the same place as the original value. This code could be left out but it
     increases the interpreter speed. I did measure. */
  if( memory_Type(*LetThisVariable) == VTYPE_DOUBLE ){
    if( memory_Type(Op2) != VTYPE_DOUBLE )Op2 = CONVERT2DOUBLE(Op2);
    if( DOUBLEVALUE(Op2) == 0.0 ){
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = NULL;
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      RETURN;
      }
    dResult = DOUBLEVALUE(*LetThisVariable) / DOUBLEVALUE(Op2);
    lResult = (long)dResult;
    /* if the result can be stored as a long then we do so */
    if( dResult == (double)lResult ){
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = NULL;
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult)
      LONGVALUE(ExpressionResult) = lResult;
      IMMORTALIZE(ExpressionResult);
      *LetThisVariable = ExpressionResult;
      RETURN;
      }
    DOUBLEVALUE(*LetThisVariable) = dResult;
    RETURN;
    }

  if( memory_Type(*LetThisVariable) == VTYPE_LONG &&
      (memory_Type(Op2) == VTYPE_LONG || (memory_Type(Op2) == VTYPE_STRING && ISSTRINGINTEGER(Op2)) )){
    lop2 = GETLONGVALUE(Op2);
    if( lop2 == 0 ){
      if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);
      *LetThisVariable = NULL;
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      RETURN;
      }
    if( LONGVALUE(*LetThisVariable) % lop2 == 0 ){
      LONGVALUE(*LetThisVariable) /= lop2;
      RETURN;
      }
    }

  Op1 = *LetThisVariable;

  /* if any of the arguments is double then the result is double */
  if( (memory_Type(Op1) == VTYPE_DOUBLE || memory_Type(Op2) == VTYPE_DOUBLE) ||

     /* if the first argument is string and is NOT integer */
     ( memory_Type(Op1) == VTYPE_STRING && !ISSTRINGINTEGER(Op1)) ||

     /* if the second argument is string and is NOT integer */
     ( memory_Type(Op2) == VTYPE_STRING && !ISSTRINGINTEGER(Op2))
     ){
    Op1 = CONVERT2DOUBLE(Op1);
    Op2 = CONVERT2DOUBLE(Op2);
    if( DOUBLEVALUE(Op2) == 0.0 ){
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      ExpressionResult = NULL;
    }else{
      ExpressionResult = NEWMORTALDOUBLE;
      ASSERTNULL(ExpressionResult);
      DOUBLEVALUE(ExpressionResult) = DOUBLEVALUE(Op1) / DOUBLEVALUE(Op2);
      }
    }else{
    Op1 = CONVERT2LONG(Op1);
    Op2 = CONVERT2LONG(Op2);
    if( LONGVALUE(Op2) == 0 ){
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      ExpressionResult = NULL;
      }else{
      if( LONGVALUE(Op1) % LONGVALUE(Op2) ){
        ExpressionResult = NEWMORTALDOUBLE;
        ASSERTNULL(ExpressionResult)
        DOUBLEVALUE(ExpressionResult) = ((double)LONGVALUE(Op1)) / ((double)LONGVALUE(Op2));
        }else{
        ExpressionResult = NEWMORTALLONG;
        ASSERTNULL(ExpressionResult)
        LONGVALUE(ExpressionResult) = LONGVALUE(Op1) / LONGVALUE(Op2);
        }
      }
    }

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END

/**LETI
=section misc
=title v \= expression
=display LETI

Integer divide a variable by a value.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v \ expression>.
Using this short format is more readable in some cases and generates more efficient code.
However note that this kind of assignment operation is a C language like operator and is not common in BASIC programs.
*/
COMMAND(LETI)
#if NOTIMP_LETI
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;
  double dResult;
  long lResult;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  Op1 = *LetThisVariable;

  /* if any of the arguments is double then the result is double */
  if( (memory_Type(Op1) == VTYPE_DOUBLE || memory_Type(Op2) == VTYPE_DOUBLE) ||

     /* if the first argument is string and is NOT integer */
     ( memory_Type(Op1) == VTYPE_STRING && !ISSTRINGINTEGER(Op1)) ||

     /* if the second argument is string and is NOT integer */
     ( memory_Type(Op2) == VTYPE_STRING && !ISSTRINGINTEGER(Op2))
     ){
    Op1 = CONVERT2DOUBLE(Op1);
    Op2 = CONVERT2DOUBLE(Op2);
    if( DOUBLEVALUE(Op2) == 0.0 ){
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      ExpressionResult = NULL;
      }else{
      dResult = floor(DOUBLEVALUE(Op1) / DOUBLEVALUE(Op2));
      lResult = (long)dResult;
      if( dResult == (double)lResult ){
        ExpressionResult = NEWMORTALLONG;
        ASSERTNULL(ExpressionResult);
        LONGVALUE(ExpressionResult) = lResult;
        }else{
        ExpressionResult = NEWMORTALDOUBLE;
        ASSERTNULL(ExpressionResult);
        DOUBLEVALUE(ExpressionResult) = dResult;
        }
      }
    }else{
    Op1 = CONVERT2LONG(Op1);
    Op2 = CONVERT2LONG(Op2);
    if( LONGVALUE(Op2) == 0 ){
      if((*RaiseError(pEo))&1 ){
        ERROR(COMMAND_ERROR_DIV);
        }
      ExpressionResult = NULL;
      }else{
      ExpressionResult = NEWMORTALLONG;
      ASSERTNULL(ExpressionResult);
      LONGVALUE(ExpressionResult) = LONGVALUE(Op1) / LONGVALUE(Op2);
      }
    }

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END

/**LETC
=section misc
=title v &= expression
=display LETC

Append a string to a variable.

The variable can be a global or local variable, array element or associative array element.

You can use this command as a shorthand for T<v =  v & expression>. Using this short format is more readable in some cases and generates more efficient code. However note that this kind of assignment operation is a C language like operator and is not common in BASIC programs.
*/
COMMAND(LETC)
#if NOTIMP_LETC
NOTIMPLEMENTED;
#else
  VARIABLE ExpressionResult;
  VARIABLE Op1,Op2;
  LEFTVALUE LetThisVariable;
  long refcount;

  /* we get the pointer to the variable that points to the value */
  LetThisVariable = EVALUATELEFTVALUE(PARAMETERNODE);
  ASSERTOKE;

  /* if this points to a reference value then we search the "real" variable
     to modify */
  DEREFERENCE(LetThisVariable);

  /* get the next parameter of the command, which is the expression */
  NEXTPARAMETER;

  /* Evaluate the expression, and if the expression is simple create a copy of the result
     to be sure that no two variables point to the same value */
  Op2 = execute_Evaluate(pEo,PARAMETERNODE,_pThisCommandMortals,&iErrorCode,0);
  ASSERTOKE;

  Op1 = *LetThisVariable;

  Op1 = CONVERT2STRING(Op1);
  Op2 = CONVERT2STRING(Op2);

  if( STRLEN(Op2) == 0 )RETURN;

  ExpressionResult = NEWMORTALSTRING( STRLEN(Op1) + STRLEN(Op2) );
  ASSERTNULL(ExpressionResult)
  memcpy(STRINGVALUE(ExpressionResult),STRINGVALUE(Op1),STRLEN(Op1));
  memcpy(STRINGVALUE(ExpressionResult)+STRLEN(Op1),STRINGVALUE(Op2),STRLEN(Op2));

  /* if the result of the expression is not undef then immortalize */
  if( ExpressionResult ){
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    IMMORTALIZE(ExpressionResult);
    }

  /* if this variable had value assigned to it then release that value */
  if( *LetThisVariable )memory_ReleaseVariable(pEo->pMo,*LetThisVariable);

  /* and finally assign the code to the variable */
  *LetThisVariable = ExpressionResult;

#endif
END
