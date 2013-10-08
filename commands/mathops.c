/*mathop.c

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
#include <math.h>
/* comparision operators compare strings as well */
#include <string.h>
#include <limits.h>

#include "../command.h"

/* stringcompare two string values. The values SHOULD be string.
*/
static int STRCMP(pExecuteObject pEo,VARIABLE Op1, VARIABLE Op2, int iCase){
  unsigned long n;
  char *a,*b;
  char ca,cb;

  if( memory_IsUndef(Op1) && memory_IsUndef(Op2) )return 0;
  if( memory_IsUndef(Op1) )return 1;
  if( memory_IsUndef(Op2) )return -1;
  iCase &= 1;/* only the lowest bit is about case sensitivity */
  n = STRLEN(Op1);
  if( n > STRLEN(Op2) ) n= STRLEN(Op2);
  a = STRINGVALUE(Op1);
  b = STRINGVALUE(Op2);
  while( n-- ){
    ca = *a;
    cb = *b;
    if( iCase ){
      if( isupper(ca) )ca = tolower(ca);
      if( isupper(cb) )cb = tolower(cb);
      }
    if( ca != cb )return ( (ca)-(cb) );
    a++;
    b++;
    }
  if( STRLEN(Op1) == STRLEN(Op2) )return 0;
  if( STRLEN(Op1) > STRLEN(Op2) )return 1;
  return -1;
  }

static long longpow(long a,long b){
  long result;

  result = 1;
  while( b ){
    if( b&1 )result *= a;
    b /= 2;
    a *= a;
    }
  return result;
}

static double doublepow(double a,long b){
  double result;

  result = 1.0;
  while( b ){
    if( b&1 )result *= a;
    b /= 2;
    a *= a;
    }
  return result;
}

#define RAISEMATHERROR "raisematherror"
long *RaiseError(pExecuteObject pEo){
  long *plCache;

  plCache = (long *) PARAMPTR(CMD_DIV);
  if( plCache == NULL ){
    plCache = options_GetR(pEo,RAISEMATHERROR);
    if( plCache == NULL )
      options_Set(pEo,RAISEMATHERROR,0);
    plCache = options_GetR(pEo,RAISEMATHERROR);
    }
  return plCache;
  }

/*POD
=H Mathematical operators

This file defines all the mathematical operators that are implemented in ScriptBasic.

CUT*/

/*POD
=section MULT
=H Multiplication

This operator multiplies two numbers. If one of the arguments is double then the result is double, otherwise the result is long.

If one of the operators is undefined the result is undefined.

CUT*/
COMMAND(MULT)
#if NOTIMP_MULT
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1,Op2;
  double dResult;
  long lResult,lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op2)

  /* if any of the arguments is double then the result is double */
  if( ! ISINTEGER(Op1) || ! ISINTEGER(Op2) ){
    RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) * GETDOUBLEVALUE(Op2) )
    }
  lop1 = GETLONGVALUE(Op1);
  lop2 = GETLONGVALUE(Op2);
  lResult = lop1 * lop2;
  if( 0 == lop1 ){
    RETURN_LONG_VALUE( lResult );
    }
  if( lResult / lop1 == lop2 ){
    RETURN_LONG_VALUE( lResult );
    }
  RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) * GETDOUBLEVALUE(Op2) )
#endif
END

COMMAND(EQ)
#if NOTIMP_EQ
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOPE(Op1)

  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOPE(Op2)

  /* undef is equal to undef */
  if( memory_IsUndef(Op1) && memory_IsUndef(Op2) ){
    RETURN_LONG_VALUE(-1L);
    }

  /* undef is not equal to anything else */
  if( memory_IsUndef(Op1) || memory_IsUndef(Op2) ){
    RETURN_LONG_VALUE(0)
    }

  /* if any of the arguments is string then we compare strings */
  if( TYPE(Op1) == VTYPE_STRING || TYPE(Op2) == VTYPE_STRING ){
    Op1 = CONVERT2STRING(Op1);
    Op2 = CONVERT2STRING(Op2);
    RETURN_LONG_VALUE( STRCMP(pEo,Op1,Op2,OPTION("compare")) == 0 ? -1L : 0 )
    }

  /* if any of the arguments is double then we compare double */
  if( TYPE(Op1) == VTYPE_DOUBLE || TYPE(Op2) == VTYPE_DOUBLE ){
    RETURN_LONG_VALUE( GETDOUBLEVALUE(Op1) == GETDOUBLEVALUE(Op2) ? -1L : 0L )
    }

  RETURN_LONG_VALUE( GETLONGVALUE(Op1) == GETLONGVALUE(Op2) ? -1L : 0L )

#endif
END

COMMAND(NE)
#if NOTIMP_NE
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOPE(Op1)
  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOPE(Op2)

  /* undef is equal to undef */
  if( memory_IsUndef(Op1) && memory_IsUndef(Op2) ){
    RETURN_LONG_VALUE( 0 )
    }

  /* undef is not equal to anything else */
  if( memory_IsUndef(Op1) || memory_IsUndef(Op2) ){
    RETURN_LONG_VALUE( -1L )
    }

  /* if any of the arguments is string then we compare strings */
  if( TYPE(Op1) == VTYPE_STRING || TYPE(Op2) == VTYPE_STRING ){
    Op1 = CONVERT2STRING(Op1);
    Op2 = CONVERT2STRING(Op2);
    RETURN_LONG_VALUE( STRCMP(pEo,Op1,Op2,OPTION("compare")) != 0 ?  -1L : 0 )
    }

  /* if any of the arguments is double then we compare double */
  if( TYPE(Op1) == VTYPE_DOUBLE || TYPE(Op2) == VTYPE_DOUBLE ){
    RETURN_LONG_VALUE( GETDOUBLEVALUE(Op1) != GETDOUBLEVALUE(Op2) ? -1L : 0L )
    }

  RETURN_LONG_VALUE( GETLONGVALUE(Op1) != GETLONGVALUE(Op2) ? -1L : 0L )

#endif
END

#define LOGOP(NAME,OP) \
COMMAND(NAME)\
  NODE nItem;\
  VARIABLE Op1,Op2;\
\
  /* this is an operator and not a command, therefore we do not have our own mortal list */\
  USE_CALLER_MORTALS;\
\
  /* evaluate the parameters */\
  nItem = PARAMETERLIST;\
  Op1 = EVALUATEEXPRESSION(CAR(nItem));\
  NONULOPE(Op1)\
  nItem = CDR(nItem);\
  Op2 = EVALUATEEXPRESSION(CAR(nItem));\
  NONULOPE(Op2)\
\
  /* undef is not comparable except for equality */\
  if( memory_IsUndef(Op1) || memory_IsUndef(Op2) ){\
    RETURN_LONG_VALUE( 0 )\
    }\
\
  /* if any of the arguments is string then we compare strings */\
  if( TYPE(Op1) == VTYPE_STRING || TYPE(Op2) == VTYPE_STRING ){\
    Op1 = CONVERT2STRING(Op1);\
    Op2 = CONVERT2STRING(Op2);\
    RETURN_LONG_VALUE( STRCMP(pEo,Op1,Op2,OPTION("compare")) OP 0 ?  -1L : 0 )\
    RETURN;\
    }\
\
  /* if any of the arguments is double then we compare double */\
  if( TYPE(Op1) == VTYPE_DOUBLE || TYPE(Op2) == VTYPE_DOUBLE ){\
    RETURN_LONG_VALUE( GETDOUBLEVALUE(Op1) OP GETDOUBLEVALUE(Op2) ? -1L : 0L )\
    }\
  RETURN_LONG_VALUE( GETLONGVALUE(Op1) OP GETLONGVALUE(Op2) ? -1L : 0L )\
END

/*POD
=section compare
=H Comparing operators

The comparing operators compare long, double and string values. Whenever any of the
arguments is string the comparisionis done stringwise. If none of the arguments are strings
but one of then is double then the comparision is done between doubles. Otherwise we compare
long values.

The comparing operators are

=itemize
=item = equality operator
=item <> non equality operator
=item < less than
=item > greather than
=item <= less than or equal
=item >= greather than or equal
=noitemize

When comparing T<undef> values the following statements should be taken into account:

=itemize
=item T<undef> is equal to T<undef>
=item T<undef> is not equal anything else than T<undef>
=item T<undef> is comparable with anything only for equality or non equality. Any other comparision
      having an operand T<undef> results an undefined value.
=noitemize


CUT*/

#define NOCOMMAND(XXX) \
COMMAND(XXX)\
NOTIMPLEMENTED;\
END

#if NOTIMP_LT
NOCOMMAND(LT)
#else
LOGOP(LT,<)
#endif

#if NOTIMP_LE
NOCOMMAND(LE)
#else
LOGOP(LE,<=)
#endif


#if NOTIMP_GT
NOCOMMAND(GT)
#else
LOGOP(GT,>)
#endif

#if NOTIMP_GE
NOCOMMAND(GE)
#else
LOGOP(GE,>=)
#endif

#define LONGOP(NAME,OP) \
COMMAND(NAME)\
  NODE nItem;\
  VARIABLE Op1,Op2;\
  USE_CALLER_MORTALS;\
  nItem = PARAMETERLIST;\
  Op1 = EVALUATEEXPRESSION(CAR(nItem));\
  NONULOP(Op1)\
  nItem = CDR(nItem);\
  Op2 = EVALUATEEXPRESSION(CAR(nItem));\
  NONULOP(Op2)\
  RETURN_LONG_VALUE( GETLONGVALUE(Op1) OP GETLONGVALUE(Op2) )\
END

/*POD
=section longoperators
=H Long operators

These operators are defined only for long arguments, and they result long value.
If any of their argument is T<undef> the result is T<undef>. The operators are

=itemize
=item T<and> bitwise and
=item T<or> bitwise or
=item T<xor> bitwise xor
=noitemize

Note that the logical operators can be used to evaluate logical expressions as
well as bitwise expressions, because logical TRUE value is -1L which means all
bits set to 1. In commands that take a logical value any nonzero value is true.

CUT*/

#if NOTIMP_AND
NOCOMMAND(AND)
#else
LONGOP(AND,&)
#endif

#if NOTIMP_OR
NOCOMMAND(OR)
#else
LONGOP(OR,|)
#endif

#if NOTIMP_XOR
NOCOMMAND(XOR)
#else
LONGOP(XOR,^)
#endif

/*POD
=section mod
=H Modulo operators

This operator calculates the modulo of two numbers.
CUT*/
COMMAND(MOD)
#if NOTIMP_MOD
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  long lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op2)

  lop1 = GETLONGVALUE(Op1);
  lop2 = GETLONGVALUE(Op2);

  if( lop2 == 0 ){
    ERRORUNDEF
    }

  RETURN_LONG_VALUE( lop1 % lop2 )

#endif
END

/*POD
=section plusminus
=H unary and binary plus and minus

These functions implement the unary and binary plus and minus operands.

If any of the arguments is T<undef> then the result is T<undef>.

If any of the arguments is double or a string evaluating to a float value
then the result is double.

The result is long or double, never string.

CUT*/
COMMAND(PLUS)
#if NOTIMP_PLUS
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  double dResult;
  long lResult,lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  /* if there is second operand then this is binary operation*/
  if( nItem ){
    Op2 = EVALUATEEXPRESSION(CAR(nItem));
    NONULOP(Op2)

    /* if any of the arguments is double then the result is double */
    if( ! ISINTEGER(Op1) || !ISINTEGER(Op2) ){
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) + GETDOUBLEVALUE(Op2) )
      }

    lop1 = GETLONGVALUE(Op1);
    lop2 = GETLONGVALUE(Op2);
    lResult = lop1 + lop2;

    if( lop1 == 0 || lop2 == 0 ){
      RETURN_LONG_VALUE( lResult );
      }

    /* if operands have different sign then there can not be overflow */
    if( ( lop1 < 0 && lop2 > 0 ) || ( lop1 > 0 && lop2 < 0 ) ){
      RETURN_LONG_VALUE( lResult );
      }

    /* if operands are positive */
    if( lop1 > 0 ){
      if( LONG_MAX - lop1 >= lop2 ){
        RETURN_LONG_VALUE( lResult );
        }
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) + GETDOUBLEVALUE(Op2) )
      }

    /* if operands are negative */
    if( lop1 < 0 ){
      if( LONG_MIN - lop1 <= lop2 ){
        RETURN_LONG_VALUE( lResult );
        }
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) + GETDOUBLEVALUE(Op2) )
      }
    /* we should not ever get here */
    RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) + GETDOUBLEVALUE(Op2) )
    }

  /* we get here if this is unary */
  if( ISINTEGER(Op1) ){
    RETURN_LONG_VALUE( GETLONGVALUE(Op1) )
    }
  RETURN_DOUBLE_VALUE_OR_LONG( GETLONGVALUE(Op1) )

#endif
END

COMMAND(MINUS)
#if NOTIMP_MINUS
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  double dResult;
  long lResult,lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  if( nItem ){
    Op2 = EVALUATEEXPRESSION(CAR(nItem));
    NONULOP(Op2)

    /* if any of the arguments is double then the result is double */
    if( ! ISINTEGER(Op1) || ! ISINTEGER(Op2) ){
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) - GETDOUBLEVALUE(Op2) )
      }
    
    lop1 = GETLONGVALUE(Op1);
    lop2 = GETLONGVALUE(Op2);
    lResult = lop1 - lop2;

    if( lop1 == 0 || lop2 == 0 ){
      RETURN_LONG_VALUE( lResult );
      }

    /* if operands have the same sign then there can not be overflow */
    if( ( lop1 < 0 && lop2 < 0 ) || ( lop1 > 0 && lop2 > 0 ) ){
      RETURN_LONG_VALUE( lResult );
      }

    /* if lop1 is positive and we substract a negative number from it */
    if( lop1 > 0 ){
      if( LONG_MAX - lop1 >= -lop2 ){
        RETURN_LONG_VALUE( lResult );
        }
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) - GETDOUBLEVALUE(Op2) )
      }

    /* if lop1 is negative and we substract from it */
    if( lop1 < 0 ){
      if( LONG_MIN - lop1 <= -lop2 ){
        RETURN_LONG_VALUE( lResult );
        }
      RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) - GETDOUBLEVALUE(Op2) )
      }
    /* we should never get here */
    RETURN_DOUBLE_VALUE_OR_LONG( GETDOUBLEVALUE(Op1) - GETDOUBLEVALUE(Op2) )
    }

  /* this is unary */
  if( ! ISINTEGER(Op1) ){
    RETURN_DOUBLE_VALUE_OR_LONG( - GETDOUBLEVALUE(Op1) )
    }
  RETURN_LONG_VALUE( - GETLONGVALUE(Op1) )

#endif
END

/*POD
=section NOT
=H unary NOT operator

This operator takes one argument converts it to long and inverts all bits.
If the argument is T<undef> the result is -1L which is the absolute TRUE value,
havinbg all bits set.

CUT*/
COMMAND(NOT)
#if NOTIMP_NOT
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  RETURN_LONG_VALUE( ~ GETLONGVALUE(Op1) )

#endif
END

/*POD
=section POWER
=H powering operator

This is a binary operator that calculates I<x> powered to I<y>.

If any of the arguments is T<undef> then the result is also T<undef>.

If the exponent is negative the result is double.

If any of the operators is double or is a string evaluating to a non-integer
value then the result is double.

Otherwise the operator makes integer operations calculating the power value.
CUT*/
COMMAND(POWER)
#if NOTIMP_POWER
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE vMantissa,vExponent;
  double dMantissa,dExponent,dRoot,dResult;
  long lMantissa,lExponent,lRoot;
  int bMantIsInt,bExpIsInt;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  vMantissa = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(vMantissa) ){
    RESULT = NULL;
    RETURN;
    }

  nItem = CDR(nItem);
  vExponent = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(vExponent) ){
    RESULT = NULL;
    RETURN;
    }

  bMantIsInt = ISINTEGER(vMantissa);
  bExpIsInt  = ISINTEGER(vExponent);

  if( bExpIsInt ){/* if the exponent is integer */

    lExponent = GETLONGVALUE(vExponent);
    if( bMantIsInt ){
      /* both exponent and mantissa are integer */
      lMantissa = GETLONGVALUE(vMantissa);
      /* 0 ^ 0 is undefined, because this functional has a singularity there */
      if( lMantissa == 0 && lExponent == 0 ){
        RESULT = NULL;
        RETURN;
        }
      if( lExponent < 0 ){
        if( lMantissa == 0 ){/* the result is zero */
          RETURN_LONG_VALUE(0);
          }
        if( lMantissa == 1 ){/* the result is 1 */
          RETURN_LONG_VALUE(1);
          }
        /* The result is double because it is between one and zero excluding the boundaries. */
        /* the value in the denominator is zero only if lMantissa is zero, but that was already handled above */
        RETURN_DOUBLE_VALUE( 1.0 / (double)longpow(lMantissa,-lExponent) );
        }else{
        /* if the exponent is positive (or zero) and both mantissa and exponent are integers */
        RETURN_LONG_VALUE(longpow(lMantissa,lExponent));
        }
      }else{
      /* Exponent is integer, but the mantissa is not. */
      dMantissa = GETDOUBLEVALUE(vMantissa);
      if( lExponent < 0 ){
        /* The demoninator can not be zero, because doublepow is zero only if the mantissa is zero.
           However zero is an integer value, and the mantissa is double. What about rounding errors? */
        dResult = 1.0 / doublepow(dMantissa,-lExponent);
        /* if the result is integer amd what is more: it can be stored in an integer... */
        if( dResult == floor(dResult) && fabs(dResult) <= LONG_MAX ){
          RETURN_LONG_VALUE((long)dResult);
          }else{
          RETURN_DOUBLE_VALUE(dResult);
          }
        }else{
        /* if the exponent is positive (or zero) */
        dResult = doublepow(dMantissa,lExponent);
        if( dResult == floor(dResult) && fabs(dResult) <= LONG_MAX ){
          RETURN_LONG_VALUE((long)dResult);
          }else{
          RETURN_DOUBLE_VALUE(dResult);
          }
        }
      }

    }else{ /* not bExpIsInt ***************************************************** */

    /* If the exponent is not integer then we can not use any kind of integer calculation. */
    dMantissa = GETDOUBLEVALUE(vMantissa);
    dExponent = GETDOUBLEVALUE(vExponent);

    if( dMantissa < 0.0 ){
      dRoot = 1.0 / dExponent;
      if( dRoot == floor(dRoot) && fabs(dRoot) <= LONG_MAX ){
        lRoot = ((long)dRoot);
        if( lRoot & 1 ){
          dResult = -pow(-dMantissa,dExponent);
          if( dResult == floor(dResult) && dResult >= -LONG_MAX ){
            RESULT = NEWMORTALLONG;
            ASSERTNULL(RESULT);
            LONGVALUE(RESULT) = ((long)dResult);
            RETURN;
            }
          RESULT = NEWMORTALDOUBLE;
          ASSERTNULL(RESULT);
          DOUBLEVALUE(RESULT) = dResult;
          RETURN;
          }
        }
      /* bad luck, the result is complex */
      RESULT = NULL;
      RETURN;
      }else{/* dMantissa is positive and the exponent is not integer */
      dResult = pow(dMantissa,dExponent);
      if( dResult == floor(dResult) && fabs(dResult) <= LONG_MAX ){
        RESULT = NEWMORTALLONG;
        ASSERTNULL(RESULT)
        LONGVALUE(RESULT) = ((long)dResult);
        RETURN;
        }else{
        RESULT = NEWMORTALDOUBLE;
        ASSERTNULL(RESULT)
        DOUBLEVALUE(RESULT) = dResult;
        RETURN;
        }
      }

    }

#endif
END

/*POD
=section IDIV
=H Integer division

This operator converts the arguments to long and divides the first argument with the second.
The result is a truncated long. The truncation is done towards zero like it is done by the function
FIX and B<unlike> by the function INT.

CUT*/
COMMAND(IDIV)
#if NOTIMP_IDIV
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1,Op2;
  double dop1,dop2;
  long lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op2)

  /* if any of the arguments is double then the result is double */
  if( !ISINTEGER(Op1) || ! ISINTEGER(Op2) ){
    dop1 = GETDOUBLEVALUE(Op1);
    dop2 = GETDOUBLEVALUE(Op2);

    if( dop2 == 0.0 ){
      ERRORUNDEF
      }
    RETURN_LONG_VALUE( ((long)(dop1 / dop2)) )
    }

  lop1 = GETLONGVALUE(Op1);
  lop2 = GETLONGVALUE(Op2);

  if( lop2  == 0 ){
     ERRORUNDEF
    }

  RETURN_LONG_VALUE( ((long)(lop1 / lop2)) )

#endif
END

/*POD
=section DIV
=H Division

This operator divides two numbers. If some of the arguments are strings then they are
converted to double or long. The result is double unless both operands are long and
the operation can be performed to result an integer value without truncation.
CUT*/
COMMAND(DIV)
#if NOTIMP_DIV
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  double dop1,dop2,dResult;
  long lop1,lop2;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op1)

  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  NONULOP(Op2)

  /* if any of the arguments is double then the result is double */
  if( ! ISINTEGER(Op1) || !ISINTEGER(Op2) ){
    dop1 = GETDOUBLEVALUE(Op1);
    dop2 = GETDOUBLEVALUE(Op2);
    if( dop2 == 0.0 ){
      ERRORUNDEF
      }
    RETURN_DOUBLE_VALUE_OR_LONG( dop1 / dop2 )
    }

  lop1 = GETLONGVALUE(Op1);
  lop2 = GETLONGVALUE(Op2);
  if( lop2 == 0 ){
    ERRORUNDEF
    }
  if( lop1 % lop2 ){
    RETURN_DOUBLE_VALUE( ((double)lop1) / ((double)lop2) )
    }
  RETURN_LONG_VALUE( lop1 / lop2)

#endif
END

/*POD
=section BYVAL
=H unary ByVal operator

This operator does nothing. Does it? It can be used to alter pass by reference
variables and help the caller to pass a variable by value. Istead of writing

=verbatim
call sub(a)
=noverbatim

it can do

=verbatim
call sub(ByVal a)
=noverbatim

and this way the subroutine can NOT alter the value of the variable T<a>.

CUT*/
COMMAND(BYVAL)
#if NOTIMP_BYVAL
NOTIMPLEMENTED;
#else


  VARIABLE Op1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  Op1 = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;

  RESULT = Op1;

#endif
END
