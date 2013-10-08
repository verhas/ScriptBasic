/*mathfunc.c

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
#include <time.h>
#include <math.h>
#include <limits.h>

#include "../command.h"

/* this function is defined in mathops.c */
long *RaiseError(pExecuteObject pEo);

#define GET_ONE_ARG USE_CALLER_MORTALS; nItem = PARAMETERLIST; Op1 = EVALUATEEXPRESSION(CAR(nItem)); NONULOP(Op1)


/*POD
=H Mathematical Functions

This file defines all the mathematical functions that are implemented in ScriptBasic.
CUT*/


/**POW
=section math
=display POW()

Calculates the x-th exponent of 10. If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<POW(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(POW)
#if NOTIMP_POW
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dResult;

  GET_ONE_ARG

  RETURN_DOUBLE_VALUE_OR_LONG( pow(10.0,GETDOUBLEVALUE(Op1)) )

#endif
END


/**EXP
=section math
=display EXP()

Calculates the x-th exponent of T<e>. If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<EXP(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(EXP)
#if NOTIMP_EXP
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dResult;

  GET_ONE_ARG
  RETURN_DOUBLE_VALUE_OR_LONG( exp(GETDOUBLEVALUE(Op1)) )

#endif
END

/**LOG
=display LOG()
=section math

Calculates the natural log of the argument. If the argument is zero or less than zero the result is T<undef>.

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<LOG(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(LOG)
#if NOTIMP_LOG
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1;
  double dop1,dResult;

  GET_ONE_ARG

  dop1 = GETDOUBLEVALUE(Op1);
  if( dop1 <= 0.0 ){
    ERRORUNDEF
    }
  RETURN_DOUBLE_VALUE_OR_LONG( log(dop1) )

#endif
END

/**LOG10
=display LOG10()
=section math

Calculates the log of the argument. If the argument is zero or less than zero the result is T<undef>

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<LOG10(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(LOG10)
#if NOTIMP_LOG10
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1;
  double dop1,dResult;

  GET_ONE_ARG

  dop1 = GETDOUBLEVALUE(Op1);
  if( dop1 <= 0.0 ){
    ERRORUNDEF
    }
  RETURN_DOUBLE_VALUE_OR_LONG( log10(dop1) )

#endif
END

/**SIN
=section math
=display SIN()

Calculates the sine of the argument. If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<SIN(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(SIN)
#if NOTIMP_SIN
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dResult;

  GET_ONE_ARG

  RETURN_DOUBLE_VALUE_OR_LONG( sin(GETDOUBLEVALUE(Op1)) )

#endif
END

/**ASIN
=section math
=display ASIN()

Calculates the arcus sine of the argument, which is the inverse of the function R<SIN>. If the argument is not between (-1.0,+1.0) the return value is T<undef>.

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<ASIN(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(ASIN)
#if NOTIMP_ASIN
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dop1,dResult;

  GET_ONE_ARG

  dop1 = GETDOUBLEVALUE(Op1);
  if( dop1 < -1 || dop1 > +1 ){
    ERRORUNDEF
    }
  RETURN_DOUBLE_VALUE_OR_LONG( asin(dop1) )

#endif
END

/**ACOS
=display ACOS()
=section math

Calculates the arcus cosine of the argument, which is the inverse of the function R<COS>. If the argument is not between (-1.0,+1.0) the return value is T<undef>.

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<ACOS(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(ACOS)
#if NOTIMP_ACOS
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE Op1;
  double dop1,dResult;

  GET_ONE_ARG

  dop1 = GETDOUBLEVALUE(Op1);
  if( dop1 < -1 || dop1 > +1 ){
    ERRORUNDEF
    }
  RETURN_DOUBLE_VALUE_OR_LONG( acos(dop1) )

#endif
END

/**COS
=section math
=display COS()
Calculates the cosine of the argument.

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<COS(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(COS)
#if NOTIMP_COS
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dResult;

  GET_ONE_ARG

  RETURN_DOUBLE_VALUE_OR_LONG( cos(GETDOUBLEVALUE(Op1)) )

#endif
END

/**RND
=section math
=display RND()

Returns a random number as generated by the C function T<rand()>. Note that this random number generator usually provided by the libraries implemented for the C compiler or the operating system is not the best quality ones. If you need really good random number generator then you have to use some other libraries that implement reliable RND functions.

*/
COMMAND(RND)
#if NOTIMP_RND
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  RETURN_DOUBLE_VALUE( rand() )

#endif
END

/**RANDOMIZE
=section math
=display RANDOMIZE

Seed the random number generator. If the command is presented without argument the random number generator is seed with the actual time. If argument is provided the random number generator is seed with the argument following the keyword T<RANDOMIZE>.
*/
COMMAND(RANDOMIZA)
#if NOTIMP_RANDOMIZA
NOTIMPLEMENTED;
#else
  time_t timer;
  srand(time(&timer));

#endif
END

COMMAND(RANDOMIZE)
#if NOTIMP_RANDOMIZE
NOTIMPLEMENTED;
#else
  time_t timer;
  NODE nItem;
  VARIABLE ItemResult;

  nItem = PARAMETERNODE;
  if( nItem ){
    ItemResult = _EVALUATEEXPRESSION_A(nItem);
    ASSERTOKE;
    if( ItemResult )
      srand(GETLONGVALUE(ItemResult));
    else
      srand(time(&timer));
    }else{
    srand(time(&timer));
    }

#endif
END

/*SGN
=display SGN()
=section math

Calculates the signum of the argument. This is -1 for negative numbers, +1 for positive numbers and 0 for zero. The return value is always long except that  T<SGN(undef)> may return T<undef>. If the argument is a string it is converted to long or double.

T<SGN(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(SGN)
#if NOTIMP_SGN
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dop1;
  long lop1,result;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    lop1 = GETLONGVALUE(Op1);
    if( lop1 == 0 )result = 0;
    else if( lop1 > 0 )result = 1;
    else result = -1;
    }else{
    dop1 = GETDOUBLEVALUE(Op1);
    if( dop1 == 0.0 )result = 0;
    else if( dop1 > 0 )result = 1;
    else result = -1;
    }

  RETURN_LONG_VALUE( result );

#endif
END

/**SQR
=section math
=display SQR()

Calculates the square root of the argument.

If the result is within the range of an integer value on the actual architecture then the result is returned as an integer, otherwise it is returned as a real value.

T<SQR(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

If the argument is a negative number the result of the function is T<undef> or the function raises error if the option T<RaiseMathError> has the bit T<sbMathErrDiv> set.

If the square root of the argument is an integer number then the function returns an integer number. In other cases the returned value is real even if the argument itself is integer.

Note that this function has the opposite meaning in the language PASCAL, namely the square of the number. This may cause some problem if you are experienced in PASCAL programming. In that language T<SQRT> notes the square I<root> of a number.
*/
COMMAND(SQR)
#if NOTIMP_SQR
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dResult;
  long lSquare,lop1;
  double dop1;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    lop1 = GETLONGVALUE(Op1);
    /* we return undef or raise error based on OPTION setting */
    if( lop1 < 0 ){
      ERRORUNDEF
      }
    /* calculate the square root of the argument */
    dResult = sqrt((double)lop1);
    /* round the result and convert to long to check if it can be handles as a long*/
    lSquare = (long) floor(dResult + 0.5);
    /* if the square of the rounded result is the same as the argument then it can be returned as a long */
    if( lSquare * lSquare == lop1 ){
      RETURN_LONG_VALUE(lSquare)
      }
    /* if it can not be converted to long w/o loosing precision then return the 'double' value */
    RETURN_DOUBLE_VALUE(dResult)
    }

  /* if the argument is double */
  dop1 = GETDOUBLEVALUE(Op1);
  /* we return undef or raise error based on OPTION setting */
  if( dop1 < 0 ){
    ERRORUNDEF
    }
  /* if the argument was double then the result is double */
  RETURN_DOUBLE_VALUE( sqrt(dop1) )
#endif
END

/**ABS
=display ABS()
=section math
Returns the absolute value of the argument. If the argument is a string then it first converts it to integer or real value. The return value is integer or real value depending on the argument.

T<ABS(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(ABS)
#if NOTIMP_ABS
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  long lop1;
  double dop1;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    /* if the argument is long then the ABS value of it is also long*/
    lop1 = GETLONGVALUE(Op1);
    /* ... with the only exception of LONG_MAX because ABS(LONG_MIN) is LONG_MAX-1 usually (two's complement) */
    /* if ever you implement ScriptBasic on a machine that uses different signed integer not two's complement then you have to rewrite it */
    if( lop1 < LONG_MAX ){
      RETURN_LONG_VALUE( lop1 > 0 ? lop1 : -lop1 );
      }else{
      RETURN_DOUBLE_VALUE( -(double)lop1 );
      }
    }
  /* if the argument is double then the result will also be double */
  dop1 = GETDOUBLEVALUE(Op1);
  RETURN_DOUBLE_VALUE( dop1 > 0.0 ? dop1 : -dop1 );
#endif
END

/**VAL
=display VAL()
=section math

Converts a string to numeric value. If the string is integer it returns an integer value. If the string contains a number presentation which is a float number the returned value is real. In case the argument is already numeric no conversion is done.

T<VAL(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
*/
COMMAND(VAL)
#if NOTIMP_VAL
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;

  GET_ONE_ARG

  if( TYPE(Op1) == VTYPE_STRING ){
    if( ISSTRINGINTEGER(Op1) )
      RESULT = CONVERT2LONG(Op1);
    else
      RESULT = CONVERT2DOUBLE(Op1);
    }else{
    RESULT = Op1;
    }

#endif
END

/**MAXINT
=section math
=display MAXINT

This built-in constant is implemented as an argument less function. Returns the maximal number that can be stored as an integer value.
*/
COMMAND(MAXINT)
#if NOTIMP_MAXINT
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  RETURN_LONG_VALUE(LONG_MAX)

#endif
END

/**MININT
=section math
=display MININT

This built-in constant is implemented as an argument less function. Returns the minimal ("maximal negative") number that can be stored as an integer value.
*/
COMMAND(MININT)
#if NOTIMP_MININT
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  RETURN_LONG_VALUE(LONG_MIN)

#endif
END

/**PI
=section math
=display PI
This built-in constant is implemented as an argument less function. Returns the approximate value of the constant PI which is the ratio of the circumference of a circle to its diameter.

*/
COMMAND(PI)
#if NOTIMP_PI
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  RETURN_DOUBLE_VALUE(3.1415926)

#endif
END

COMMAND(UNDEF)
#if NOTIMP_UNDEF
NOTIMPLEMENTED;
#else


  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  RESULT = NULL;

#endif
END

/**TRUE
=section math

This built-in constant is implemented as an argument less function. Returns the value T<true>.
*/
COMMAND(TRUE)
#if NOTIMP_TRUE
NOTIMPLEMENTED;
#else


  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  RETURN_LONG_VALUE( -1L )

#endif
END

/**FALSE
=section math

This built-in constant is implemented as an argument less function. Returns the value T<false>.
*/
COMMAND(FALSE)
#if NOTIMP_FALSE
NOTIMPLEMENTED;
#else


  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  RETURN_LONG_VALUE( 0L )

#endif
END

/**FIX
=display FIX()
=section math

This function returns the integral part of the argument. The return value of the function is integer with the exception that T<FIX(undef)> may return T<undef>. 

T<FIX(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.


The difference between T<INT> and T<FIX> is that T<INT> truncates down while T<FIX> truncates towards zero. The two functions are identical for positive numbers. In case of negative arguments T<INT> will give a smaller number if the argument is not integer. For example:

=verbatim
  int(-3.3) = -4
  fix(-3.3) = -3
=noverbatim

See R<INT>.
*/
COMMAND(FIX)
#if NOTIMP_FIX
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dop1;
  long   lop1;
  int isneg;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    RETURN_LONG_VALUE( GETLONGVALUE(Op1) )
    }
  dop1 = GETDOUBLEVALUE(Op1);
  if( isneg = dop1 < 0 )dop1 =  -dop1;
  dop1 = floor(dop1);
  if( isneg )dop1 = - dop1;
  lop1 = (long)dop1;
  if( dop1 == (double)lop1 ){
    RETURN_LONG_VALUE( lop1 );
    }

  RETURN_DOUBLE_VALUE( dop1 );

#endif
END

/**INT
=display INT()
=section math

This function returns the integral part of the argument.
T<INT(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.
Other than this the function returns integer value.

The difference between T<INT> and T<FIX> is that T<INT> truncates down 
while T<FIX> truncates towards zero. The two functions are identical for 
positive numbers. In case of negative arguments T<INT> will give a smaller
number if the argument is not integer. For example:

=verbatim
  int(-3.3) = -4
  fix(-3.3) = -3
=noverbatim

See R<FIX>.
*/
COMMAND(INT)
#if NOTIMP_INT
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dop1;
  long   lop1;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    RETURN_LONG_VALUE( GETLONGVALUE(Op1) )
    }

  dop1 = floor(GETDOUBLEVALUE(Op1));
  lop1 = (long)dop1;
  if( dop1 == (double)lop1 ){
    RETURN_LONG_VALUE( lop1 );
    }

  RETURN_DOUBLE_VALUE( dop1 );

#endif
END


/**CINT
=display CINT()
=section math

This function is the same as R<INT> and is present in ScriptBasic to be more
compatible with other BASIC language variants.
*/

/**FRAC
=display FRAC()
=section math

The function returns the fractional part of the
argument. This function always returns a double
except that T<FRAC(undef)> may return T<undef>.
T<FRAC(undef)> is T<undef> or raises an error if
the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>. 

Negative arguments return negative value (or zero if the argument is
a negative integer), positive arguments result positive values
(or zero if the argument is integer).
*/
COMMAND(FRAC)
#if NOTIMP_FRAC
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  double dop1;

  GET_ONE_ARG

  if( ISINTEGER(Op1) ){
    RETURN_DOUBLE_VALUE( 0.0 )
    }

  dop1 = GETDOUBLEVALUE(Op1);
  if( dop1 < 0 ){
    RETURN_DOUBLE_VALUE( dop1 - floor(dop1) -1 )
    }else{
    RETURN_DOUBLE_VALUE( dop1 - floor(dop1) )
    }

#endif
END


/**ROUND
=display ROUND()
=section math

This function rounds its argument. The first argument is the number to round, and the optional second argument specifies the number of fractional digits to round to.

The function rounds to integer value if the second argument is missing.

The return value is long if the number of decimal places to keep is zero, otherwise the return value is double.

Negative value for the number of decimal places results rounding to integer value.

T<ROUND(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

Also T<ROUND(val,undef)> is equivalent to T<ROUND(value)>.

See also R<INT>, R<FRAC> and R<FIX>

*/
COMMAND(ROUND)
#if NOTIMP_ROUND
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  long iNumberOfDigits,lop1;
  double Multiplier,dop1;

  GET_ONE_ARG

  nItem = CDR(nItem);

  iNumberOfDigits = 0;
  if( nItem ){
    iNumberOfDigits = GETLONGVALUE(EVALUATEEXPRESSION(CAR(nItem)));
    ASSERTOKE;
    }
  if( iNumberOfDigits < 0 )iNumberOfDigits = 0;
  if( iNumberOfDigits > 100 )iNumberOfDigits = 100;

  if( ISINTEGER(Op1) ){
    RETURN_LONG_VALUE( GETLONGVALUE(Op1) )
    }

  dop1 = GETDOUBLEVALUE(Op1);
  if( iNumberOfDigits == 0 ){
    dop1 = floor( dop1 + 0.5);
    lop1 = (long)dop1;
    if( dop1 == (double)lop1 ){
      RETURN_LONG_VALUE( lop1 )
      }else{
      RETURN_DOUBLE_VALUE( dop1 )
      }
    }
  Multiplier = 1.0;
  while( iNumberOfDigits -- )Multiplier *= 10.0;
  dop1 *= Multiplier;
  dop1 += 0.5;
  dop1 = floor(dop1);

  dop1 /= Multiplier;
  lop1 = (long)dop1;
  if( dop1 == (double)lop1 ){
    RETURN_LONG_VALUE( lop1 )
    }else{
     RETURN_DOUBLE_VALUE( dop1 )
   }
#endif
END

/**GCD
=section math
=display GCD()

This is a planned function that takes two or more integer argument and calculates the largest common divisor of them.

*/
COMMAND(GCD)
#if NOTIMP_GCD
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**LCM
=section math
=display LCM()

This is a planned function that takes two or more integer argument and calculates the least common multiple of them.

*/
COMMAND(LCM)
#if NOTIMP_LCM
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**ODD
=section math test
=display ODD()

Return T<true> if the argument is an odd number. T<ODD(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

See also R<EVEN>
*/
COMMAND(ODD)
#if NOTIMP_ODD
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;

  GET_ONE_ARG

  RESULT = NEWMORTALLONG;

  if( GETLONGVALUE(Op1) & 1 )
    LONGVALUE(RESULT) = -1;
  else
    LONGVALUE(RESULT) = 0;

#endif
END

/**EVEN
=display EVEN()
=section math test
Return T<true> if the argument is an even number. T<EVEN(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

See also R<ODD>.
*/
COMMAND(EVEN)
#if NOTIMP_EVEN
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;

  GET_ONE_ARG

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)

  if( GETLONGVALUE(Op1) & 1 )
    LONGVALUE(RESULT) = 0;
  else
    LONGVALUE(RESULT) = -1;

#endif
END

/**LBOUND
=section array
=display LBOUND()

This function can be used to determine the lowest occupied index of an array. Note that arrays are increased in addressable indices automatically, thus it is not an error to use a lower index that the value returned by the function T<LBOUND>. On the other hand all the element having index lower than the returned value are T<undef>.

The argument of this function has to be an array. If the argument is an ordinary value, or a variable that is not an array the value returned by the function will be T<undef>.

T<LBOUND(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

See also R<UBOUND>.
*/
COMMAND(LBOUND)
#if NOTIMP_LBOUND
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  NONULOP(ItemResult)

  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
    case VTYPE_DOUBLE:
    case VTYPE_STRING:
      RESULT = NULL;
      RETURN;
    case VTYPE_ARRAY:
      RESULT = NEWMORTALLONG;
      ASSERTNULL(RESULT)
      LONGVALUE(RESULT) = ItemResult->ArrayLowLimit;
      RETURN;
    }

#endif
END

/**UBOUND
=section array
=display UBOUND()

This function can be used to determine the highest occupied index of an array. Note that arrays are increased in addressable indices automatically, thus it is not an error to use a higher index that the value returned by the function T<UBOUND>. On the other hand all the element having index larger than the returned value are T<undef>.

The argument of this function has to be an array. If the argument is an ordinary value, or a variable that is not an array the value returned by the function will be T<undef>.

T<UBOUND(undef)> is T<undef> or raises an error if the option T<RaiseMatherror> is set in bit T<sbMathErrUndef>.

See also R<LBOUND>.

*/
COMMAND(UBOUND)
#if NOTIMP_UBOUND
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  NONULOP(ItemResult)

  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
    case VTYPE_DOUBLE:
    case VTYPE_STRING:
      RESULT = NULL;
      RETURN;
    case VTYPE_ARRAY:
      RESULT = NEWMORTALLONG;
      ASSERTNULL(RESULT)
      LONGVALUE(RESULT) = ItemResult->ArrayHighLimit;
      RETURN;
    }

#endif
END

/**ISARRAY
=section array test
=display ISARRAY()

This function can be used to determine whether a variable holds array value or ordinary value. If the variable passed as argument to the function is an array then the function returns T<true>, otherwise the function returns T<false>.

See also R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/
COMMAND(ISARRAY)
#if NOTIMP_ISARRAY
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
    case VTYPE_DOUBLE:
    case VTYPE_STRING:
      RETURN_FALSE
    case VTYPE_ARRAY:
      RETURN_TRUE
    }

#endif
END

/**ISSTRING
=section string test
=display ISSTRING()

This function can be used to determine whether an expression is string or some other type of value. If the argument is a string then the function returns T<true>, otherwise the function returns T<false>.

See also R<ISARRAY>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/
COMMAND(ISSTRING)
#if NOTIMP_ISSTRING
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
    case VTYPE_DOUBLE:
    case VTYPE_ARRAY:
      RETURN_FALSE
    case VTYPE_STRING:
      RETURN_TRUE
    }

#endif
END

/**ISINTEGER
=section test
=display ISINTEGER()

This function can be used to determine whether an expression is integer or some other type of value. If the argument is an integer then the function returns T<true>, otherwise the function returns T<false>.

See also R<ISARRAY>, R<ISSTRING>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/
COMMAND(ISLONG)
#if NOTIMP_ISLONG
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_DOUBLE:
    case VTYPE_ARRAY:
    case VTYPE_STRING:
      RETURN_FALSE
    case VTYPE_LONG:
      RETURN_TRUE
    }

#endif
END

/**ISREAL
=section test
=display ISREAL()

This function can be used to determine whether an expression is real or some other type of value. If the argument is a real then the function returns T<true>, otherwise the function returns T<false>.

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/

COMMAND(ISDOUBLE)
#if NOTIMP_ISDOUBLE
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_ARRAY:
    case VTYPE_STRING:
    case VTYPE_LONG:
      RETURN_FALSE
    case VTYPE_DOUBLE:
      RETURN_TRUE
    }

#endif
END

/**ISNUMERIC
=section test
=display ISNUMERIC()

This function can be used to determine whether an expression is numeric (real or integer) or some other type of value. If the argument is a real or an integer then the function returns T<true>, otherwise the function returns T<false>.

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/

COMMAND(ISNUMERIC)
#if NOTIMP_ISNUMERIC
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_ARRAY:
    case VTYPE_STRING:
      RETURN_FALSE
    case VTYPE_LONG:
    case VTYPE_DOUBLE:
      RETURN_TRUE
    }
#endif
END

/**ISDEFINED
=section test
=display IsDefined()

This function can be used to determine whether an expression is defined or undefined (aka T<undef>). If the argument is a defined value then the function returns T<true>, otherwise the function returns T<false>.

This function is the counter function of R<ISUNDEF>.

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISUNDEF>, R<ISEMPTY>, R<TYPE>.
*/

COMMAND(ISDEF)
#if NOTIMP_ISDEF
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_FALSE
    }
  RETURN_TRUE
#endif
END

/**ISUNDEF
=section test
=display ISUNDEF()

This function can be used to determine whether an expression is defined or undefined (aka T<undef>). If the argument is a defined value then the function returns T<false>, otherwise the function returns T<true>.

This function is the counter function of R<ISDEFINED>.

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISEMPTY>, R<TYPE>.
*/

COMMAND(ISUNDEF)
#if NOTIMP_ISUNDEF
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ){
    RETURN_TRUE
    }
  RETURN_FALSE
#endif
END

/**ISEMPTY
=section test
=display ISEMPTY()

This function can be used to determine whether an expression holds an empty string. Because programmers tend to use the value T<undef> where empty string would be more precise the function returns T<true> if the argument is T<undef>. Precisely:

The function returns true if the argument is T<undef> or a string containing zero characters. Otherwise the function returns T<false>.

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<TYPE>.
*/

COMMAND(ISEMPTY)
#if NOTIMP_ISEMPTY
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  if( memory_IsUndef(ItemResult) ||
     ( TYPE(ItemResult) == VTYPE_STRING && STRLEN(ItemResult) == 0 ) ){
    RETURN_TRUE
    }
  RETURN_FALSE
#endif
END

/**TYPE
=section test
=display TYPE()

This function can be used to determine the type of an expression.
The function returns a numeric value that describes the type of the argument. Although the numeric values are guaranteed to be the one defined here it is recommended that you use the predefined symbolic constant values to compare the return value of the function against. The function return value is the following 

=itemize
=item T<SbTypeUndef>   0 if the argument is T<undef>.
=item T<SbTypeString>  1 if the argument is string.
=item T<SbTypeReal>    2 if the argument is real.
=item T<SbTypeInteger> 3 if the argument is integer.
=item T<SbTypeArray>   4 if the argument is an array.
=noitemize

See also R<ISARRAY>, R<ISSTRING>, R<ISINTEGER>, R<ISREAL>, R<ISNUMERIC>, R<ISDEFINED>, R<ISUNDEF>, R<ISEMPTY>.
*/

COMMAND(TYPE)
#if NOTIMP_TYPE
NOTIMPLEMENTED;
#else

  VARIABLE ItemResult;
  NODE nItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  ItemResult = _EVALUATEEXPRESSION_A(CAR(nItem));
  ASSERTOKE;

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)

  if( memory_IsUndef(ItemResult) ){
    LONGVALUE(RESULT) = 0;
    RETURN;
    }
  switch( TYPE(ItemResult) ){
    case VTYPE_LONG:
      LONGVALUE(RESULT) = 3;
      break;
    case VTYPE_DOUBLE:
      LONGVALUE(RESULT) = 2;
      break;
    case VTYPE_STRING:
      LONGVALUE(RESULT) = 1;
      break;
    case VTYPE_ARRAY:
      LONGVALUE(RESULT) = 4;
      break;
    }
#endif
END

/**ATN
=section planned
=display ATN()

This is a planned function to calculate the arcus tangent of the argument.
*/
COMMAND(ATN)
#if NOTIMP_ATN
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**ATAN
=section planned
=display ATAN()

This is a planned function to calculate the arcus tangent of the argument.
*/
COMMAND(ATAN)
#if NOTIMP_ATAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**TAN
=section planned
=display TAN()

This is a planned function to calculate the tangent of the argument.
*/
COMMAND(TAN)
#if NOTIMP_TAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**TAN2
=section planned
=display TAN2()

This is a planned function to calculate the tangent of the ratio of the two arguments.
*/
COMMAND(TAN2)
#if NOTIMP_TAN2
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**COTAN
=section planned
=display COTAN()

This is a planned function to calculate the cotangent of the argument.
*/
COMMAND(COTAN)
#if NOTIMP_COTAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**COTAN2
=section planned
=display COTAN2()

This is a planned function to calculate the cotangent of the ratio of the two arguments.
*/
COMMAND(COTAN2)
#if NOTIMP_COTAN2
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**ACTAN
=section planned
=display ACTAN()

This is a planned function to calculate the arcus cotangent of the argument.
*/
COMMAND(ACTAN)
#if NOTIMP_ACTAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**SECANT
=section planned
=display SECANT()

This is a planned function to calculate the secant of the argument.
*/
COMMAND(SECANT)
#if NOTIMP_SECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**COSECANT
=section planned
=display COSECANT()

This is a planned function to calculate the cosecant of the argument.
*/
COMMAND(COSECANT)
#if NOTIMP_COSECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**ASECANT
=section planned
=display ASECANT()

This is a planned function to calculate the arcus secant of the argument.
*/
COMMAND(ASECANT)
#if NOTIMP_ASECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**ACOSECANT
=section planned
=display ACOSECANT()

This is a planned function to calculate the arcus cosecant of the argument.
*/
COMMAND(ACOSECANT)
#if NOTIMP_ACOSECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HSIN
=section planned
=display HSIN()

This is a planned function to calculate the sinus hyperbolicus of the argument.
*/
COMMAND(HSIN)
#if NOTIMP_HSIN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HCOS
=section planned
=display HCOS()

This is a planned function to calculate the cosinus hyperbolicus of the argument.
*/
COMMAND(HCOS)
#if NOTIMP_HCOS 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HTAN
=section planned
=display HTAN()

This is a planned function to calculate the tangent hyperbolicus of the argument.
*/
COMMAND(HTAN)
#if NOTIMP_HTAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HCTAN
=section planned
=display HCTAN()

This is a planned function to calculate the cotangent hyperbolicus of the argument.
*/
COMMAND(HCTAN)
#if NOTIMP_HCTAN 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HSECANT
=section planned
=display HSECANT()

This is a planned function to calculate the secant hyperbolicus of the argument.
*/
COMMAND(HSECANT)
#if NOTIMP_HSECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END


/**HCOSECANT
=section planned
=display HCOSECANT()

This is a planned function to calculate the cosecant hyperbolicus of the argument.
*/
COMMAND(HCOSECANT)
#if NOTIMP_HCOSECANT 
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MAX
=section planned
=display MAX()

This is a planned function to select and return the maximum of the arguments.
*/
COMMAND(MAX)
#if NOTIMP_MAX
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MIN
=section planned
=display MIN()

This is a planned function to select and return the minimum of the arguments.
*/
COMMAND(MIN)
#if NOTIMP_MIN
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**IMAX
=section planned
=display IMAX()

This is a planned function to select and return the index of the maximum of the arguments.
*/
COMMAND(IMAX)
#if NOTIMP_IMAX
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**IMIN
=section planned
=display IMIN()

This is a planned function to select and return the index of the minimum of the arguments.
*/
COMMAND(IMIN)
#if NOTIMP_IMIN
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

