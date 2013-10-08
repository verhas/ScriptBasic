/*string.c

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
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "../command.h"
#include "../match.h"
#include "../matchc.h"

#define FMT_xMIN	1e-8		/* lowest limit to use the exp. format  */
#define FMT_xMAX	1e+9		/* highest limit to use the exp. format */
#define FMT_RND		9			  /* rounding on x digits                 */
#define FMT_xRND	1e+9		/* 1 * 10 ^ FMT_RND                     */
#define FMT_xRND2	1e+8		/* 1 * 10 ^ (FMT_RND-1)                 */

static double nfta_eplus[]=
{ 
    1e+8,   1e+16,  1e+24,  1e+32,  1e+40,  1e+48,  1e+56,  1e+64,	/* 8  */
    1e+72,  1e+80,  1e+88,  1e+96,  1e+104, 1e+112, 1e+120, 1e+128,	/* 16 */
    1e+136, 1e+144, 1e+152, 1e+160, 1e+168, 1e+176, 1e+184, 1e+192,	/* 24 */
    1e+200, 1e+208, 1e+216, 1e+224, 1e+232, 1e+240, 1e+248, 1e+256,	/* 32 */
    1e+264, 1e+272, 1e+280, 1e+288, 1e+296, 1e+304                  /* 38 */
};

static double nfta_eminus[]=
{ 
    1e-8,   1e-16,  1e-24,  1e-32, 1e-40,   1e-48,  1e-56,  1e-64,  /* 8 */
    1e-72,  1e-80,  1e-88,  1e-96, 1e-104,  1e-112, 1e-120, 1e-128, /* 16 */
    1e-136, 1e-144, 1e-152, 1e-160, 1e-168, 1e-176, 1e-184, 1e-192, /* 24 */
    1e-200, 1e-208, 1e-216, 1e-224, 1e-232, 1e-240, 1e-248, 1e-256, /* 32 */
    1e-264, 1e-272, 1e-280, 1e-288, 1e-296, 1e-304				          /* 38 */
};


/*
String operator and functions

This file conatins the commands for the string operator concatendate and
string handling functions. When string parameters are needed conversion is done
automatic as usually.

*/

/* stringcompare two sub strings case sensitiveor case insensitive
   return 0 if they match and return non zero otherwise (used in InStr)
*/
static int SUBSTRCMP(char *a, char *b, long length, int iCase){
  char ca,cb;

  iCase &= 1;/* only the lowest bit is about case sensitivity */
  while( length-- ){
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
  return 0;
  }


/**CONCATENATE
=section string
=display &amp;
=title Concatenate operator &amp;

This operator concatenates two strings. The resulting string will contain the characters of the string standing on the left side of the operator followed by the characters of the string standing on the right hand side of the operator. The ScriptBasic interpreter automatically allocates the resulting string.

*/
COMMAND(CONCATENATE)
#if NOTIMP_CONCATENATE
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  long lFinalStringLength,lLen;
  char *s,*r;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  Op2 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;

  lFinalStringLength  = Op1 ? STRLEN(Op1) : 0;
  lFinalStringLength += Op2 ? STRLEN(Op2) : 0;

  RESULT = NEWMORTALSTRING(lFinalStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);

  /* copy the characters of the strings to the new location */
  s = Op1 ? STRINGVALUE(Op1) : NULL;
  lLen = Op1 ? STRLEN(Op1) : 0;
  while( s && lLen ){
    *r++ = *s++;
    lLen--;
    }
  s = Op2 ? STRINGVALUE(Op2) : NULL;
  lLen = Op2 ? STRLEN(Op2) : 0;
  while( s && lLen ){
    *r++ = *s++;
    lLen--;
    }

#endif
END

/**LEN
=section string
=display LEN()
=title LEN()

This function interprets its argument as a string and returns the length of the string. In ScriptBasic strings can hold any value thus the length of the string is the number of characters contained in the string containing any binary characters, even binary zero.

If the argument is not a string it is converted to string automatically and the length of the converted string is returned. The only exception is T<undef> for which the result is also T<undef>.

*/
COMMAND(LEN)
#if NOTIMP_LEN
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = STRLEN(Op1);

#endif
END

/**UCASE
=title UCASE()
=display UCASE()
=section string
Uppercase a string.
*/
COMMAND(UCASE)
#if NOTIMP_UCASE
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  char *r;
  unsigned long lLen;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  RESULT = Op1;
  r = STRINGVALUE(RESULT);
  lLen = STRLEN(RESULT);

  while( lLen-- ){
    if( islower( *r ) )*r = toupper( *r );
    r++;
    }

#endif
END

/**LCASE
=title LCASE()
=display LCASE()
=section string
Lowercase a string.
*/
COMMAND(LCASE)
#if NOTIMP_LCASE
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  char *r;
  unsigned long lLen;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  RESULT = Op1;
  r = STRINGVALUE(RESULT);
  lLen = STRLEN(RESULT);

  while( lLen-- ){
    if( isupper( *r ) )*r = tolower( *r );
    r++;
    }

#endif
END

/**LTRIM
=section string
=title LTRIM()
=display LTRIM()
Remove the space from the left of the string.
*/
COMMAND(LTRIM)
#if NOTIMP_LTRIM
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  char *r,*s;
  unsigned long lStringLength,lLen;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  r = STRINGVALUE(Op1);
  lLen = STRLEN(Op1);

  while( lLen && isspace(*r) )r++,lLen--;
  s = r;
  lStringLength = 0;
  while( lLen ){
    lStringLength++;
    r++;
    lLen--;
    }
  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);

  while( lStringLength-- )*r++ = *s++;

#endif
END

/**RTRIM
=section string
=title RTRIM()
=display RTRIM()
Remove the space from the right of the string.
*/
COMMAND(RTRIM)
#if NOTIMP_RTRIM
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  char *r,*s;
  unsigned long lStringLength;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  r = STRINGVALUE(Op1);
  lStringLength = STRLEN(Op1);
  while( lStringLength && isspace(r[lStringLength-1]) )lStringLength--;
  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  s = STRINGVALUE(Op1);
  while( lStringLength ){
    *r++ = *s++;
    lStringLength--;
    }

#endif
END

/**TRIM
=section string
=title TRIM()
=display TRIM()
Remove the space from both ends of the string.
*/
COMMAND(TRIM)
#if NOTIMP_TRIM
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  char *r,*s;
  unsigned long lStringLength,lLen;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  r = STRINGVALUE(Op1);
  lLen = STRLEN(Op1);
  lStringLength = STRLEN(Op1);
  while( lLen && isspace( *r ) )r++,lLen--,lStringLength--;
  s = r;
  if( lStringLength ){
    lStringLength --;      /* convert length to char array index */
    while( lStringLength && isspace(r[lStringLength]) )lStringLength--;
    lStringLength++; /* convert char array index back to length */
    }

  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lStringLength ){
    *r++ = *s++;
    lStringLength--;
    }

#endif
END

/**INSTR
=section string
=title INSTR(base_string,search_string [ ,position ] )
=display INSTR()
This function can be used to search a sub-string in a string. 
The first argument is the string we are searching in.
The second argument is the string that we actually want to find in the 
first argument. The third optional argument is the position where the 
search is to be started. If this argu-ment is missing the search starts 
with the first character position of the string.
The function returns the position where the sub-string 
can be found in the first string. If the searched sub-string 
is not found in the string then the return value is undef.

See R<INSTRREV>
*/
COMMAND(INSTR)
#if NOTIMP_INSTR
NOTIMPLEMENTED;
#else
  NODE nItem;
  VARIABLE Op1,Op2,Op3;
  long lStart,lLength,lStringLength;
  char *r,*s;
  int iCase = OPTION("compare")&1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;

  /* this is the base string that we are searching in */
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  lLength = STRLEN(Op1);
  r = STRINGVALUE(Op1);

  /* this is the string that we search */
  Op2 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op2) ){
    RESULT = NULL;
    RETURN;
    }
  Op2 = CONVERT2STRING(Op2);
  nItem = CDR(nItem);
  lStringLength = STRLEN(Op2);
  s = STRINGVALUE(Op2);

  Op3 = NULL;
  if( nItem ){
    Op3 = EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;
    }

  if( memory_IsUndef(Op3) )
    lStart = 1;
  else
    lStart = LONGVALUE(CONVERT2LONG(Op3));

  if( lStart < 1 )lStart = 1;

  if( lLength < lStringLength ){
    RESULT = NULL;
    RETURN;
    }

  while( lStart-1 <= lLength - lStringLength ){
    if( ! SUBSTRCMP(r+lStart-1,s, lStringLength,iCase ) ){
      RESULT = NEWMORTALLONG;
      ASSERTNULL(RESULT)
      LONGVALUE(RESULT) = lStart;
      RETURN;
      }
    lStart ++;
    }
  RESULT = NULL;
  RETURN;
#endif
END

/**INSTRREV
=section string
=display INSTRREV()
=title INSTRREV(base_string,search_string [ ,position ] )
This function can be used to search a sub-string in a string in reverse 
order starting from the end of the string. The first argument is the 
string we are searching in. The second argument is the string that 
we actually want to find in the first argument. The third optional argument 
is the position where the search is to be started. If this argument is 
missing the search starts with the last character position of the string.
The function returns the position where the sub-string can be found in the 
first string. If the searched sub-string is not found in the string then the 
return value is undef.

See R<INSTR>
*/
COMMAND(INSTRREV)
#if NOTIMP_INSTRREV
NOTIMPLEMENTED;
#else
  NODE nItem;
  VARIABLE Op1,Op2,Op3;
  long lStart,lLength,lStringLength;
  char *r,*s;
  int iCase = OPTION("compare")&1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;

  /* this is the base string that we are searching in */
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  lLength = STRLEN(Op1);
  r = STRINGVALUE(Op1);

  /* this is the string that we search */
  Op2 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op2) ){
    RESULT = NULL;
    RETURN;
    }
  Op2 = CONVERT2STRING(Op2);
  nItem = CDR(nItem);
  lStringLength = STRLEN(Op2);
  s = STRINGVALUE(Op2);

  Op3 = NULL;
  if( nItem ){
    Op3 = EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;
    }

  if( lLength < lStringLength ){
    RESULT = NULL;
    RETURN;
    }

  if( memory_IsUndef(Op3) )
    lStart = lLength - lStringLength+1;
  else
    lStart = LONGVALUE(CONVERT2LONG(Op3));

  if( lStart > lLength - lStringLength+1)lStart = lLength - lStringLength+1;

  while( lStart >= 1 ){
    if( ! SUBSTRCMP(r+lStart-1,s, lStringLength,iCase ) ){
      RESULT = NEWMORTALLONG;
      ASSERTNULL(RESULT)
      LONGVALUE(RESULT) = lStart;
      RETURN;
      }
    lStart --;
    }
  RESULT = NULL;
  RETURN;
#endif
END

/**REPLACE
=section string
=title REPLACE(base_string,search_string,replace_string [,number_of_replaces] [,position])
=display REPLACE()

This function replaces one or more occurrences of a sub-string in a string.
T<REPLACE(a,b,c)> searches the string T<a> seeking for occurrences of sub-string T<b> 
and replaces each of them with the string T<c>.

The fourth and fifth arguments are optional. The fourth argument specifies the number of 
replaces to be performed. If this is missing or is T<undef> then all occurrences of string
T<b> will be replaced. The fifth argument may specify the start position of the operation.
For example the function call 

=verbatim
REPLACE("alabama mama", "a","x",3,5)
=noverbatim

will replace only three occurrences of string T<"a"> starting at position 5.
The result is T<"alabxmx mxma">.
*/
COMMAND(REPLACE)
#if NOTIMP_REPLACE
NOTIMPLEMENTED;
#else
  NODE nItem;
  VARIABLE Op1,Op2,Op3,Op4,Op5;
  long lRepetitions;
  long lCalculatedRepetitions;
  int ReplaceAll;
  long l_start,lStart,lLength,lSearchLength,lReplaceLength,lResult;
  char *r,*s,*q,*w;
  int iCase = OPTION("compare")&1;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;

  /* this is the base string that we are searching in */
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  lLength = STRLEN(Op1);
  r = STRINGVALUE(Op1);
  /* this is the string that we search to replace */
  Op2 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op2) ){
    RESULT = NULL;
    RETURN;
    }
  Op2 = CONVERT2STRING(Op2);
  nItem = CDR(nItem);
  lSearchLength = STRLEN(Op2);
  s = STRINGVALUE(Op2);
  /* this is the string that we put into the place of the searched string */
  Op3 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op3) ){
    RESULT = NULL;
    RETURN;
    }
  Op3 = CONVERT2STRING(Op3);
  lReplaceLength = STRLEN(Op3);
  nItem = CDR(nItem);
  w = STRINGVALUE(Op3);

  Op4 = NULL;
  if( nItem ){
    Op4 = EVALUATEEXPRESSION(CAR(nItem));
    nItem = CDR(nItem);
    ASSERTOKE;
    }

  if( memory_IsUndef(Op4) ){
    lRepetitions = 0;
    ReplaceAll = 1;
    }else{
    lRepetitions = GETLONGVALUE(Op4);
    ReplaceAll = 0;
    }
  if( lRepetitions < 0 )lRepetitions = 0;

  Op5 = NULL;
  if( nItem ){
    Op5 = EVALUATEEXPRESSION(CAR(nItem));
    nItem = CDR(nItem);
    ASSERTOKE;
    }

  if( memory_IsUndef(Op5) )
    l_start = 1;
  else{
    l_start = GETLONGVALUE(Op5);
    }
  if( l_start < 1 )l_start = 1;
  lStart = l_start;

  /* first calculate the repeat actions */
  lCalculatedRepetitions = 0;
  while( lStart-1 <= lLength - lSearchLength ){
    if( ! SUBSTRCMP(r+lStart-1,s, lSearchLength,iCase ) ){
      lCalculatedRepetitions++;
      lStart += lSearchLength;
      }else lStart ++;
    }
  if( ! ReplaceAll && lCalculatedRepetitions > lRepetitions )lCalculatedRepetitions = lRepetitions;
  /* calculate the length of the new string */
  lResult = STRLEN(Op1) + lCalculatedRepetitions * (lReplaceLength-lSearchLength);

  /* allocate space for the result */
  RESULT = NEWMORTALSTRING(lResult);
  ASSERTNULL(RESULT)

  /* perform the replacements */
  lStart = l_start;

  q = STRINGVALUE(RESULT);
  if( lStart > 1 ){
    memcpy(q,r,lStart-1);
    q+=lStart-1;
    }
  while( lStart <= lLength ){
    if( lCalculatedRepetitions && ! SUBSTRCMP(r+lStart-1,s, lSearchLength,iCase ) ){
      memcpy(q,w,lReplaceLength);
      q += lReplaceLength;
      lStart += lSearchLength;
      lCalculatedRepetitions--;
      }else{
      *q++ = r[lStart-1];
      lStart ++;
      }
    }
#endif
END

/**MID
=section string
=display MID()
=title MID(string,start [ ,len ])

Return a subpart of the string. The first argument is the string, the second argument is the start position.
The third argument is the length of the sub-part in terms of characters. 
If this argument is missing then the
subpart lasts to the last character of the argument T<string>.

See also R<LEFT>, R<RIGHT>.

=details
T<mid(x,y,[z])> cuts out a sub-string from the string T<x>.
If the first argument of the function is undefined the result is T<undef>.
Otherwise the first argument is converted to string and the second and third
arguments are converted to numeric value. The third argument is optional.

The second argument specifies the start position of the resulting 
substring in the original string x; and the last argument specifies
the number of characters to take from the original string T<x>. 
If the third argument is missing the substring lasts from the start 
position to the end of the string. If the second argu-ment is 
not defined the start of the substring is at the start of the 
original string. In other words if the second argument is missing 
it is the same as value 1. If the second argument is zero or negative 
it will specify the start position counting the 
characters from the end of the string.

If the staring position T<y> points beyond the end of the string the result 
is empty string. If the length of the substring is larger than the number 
of characters between the starting position and end of the original string then the 
result will be the substring between the start position and the end of the original string. 

If the length of the substring is negative the characters before the starting position 
are taken. No more than the available characters can be taken in this 
case either. In other words if the length is negative and is larger in 
absolute value than the starting position the resulting sub-string is the character 
between the position specified by the second argument and the start of the string.

Note that the order of the characters is never changed even if some position or length 
parameters are negative.

For compatibility reasons you can append a dollar (T<$>) sign to the end of the function
identifier.

Example:
=verbatim
a$ = "superqualifragilisticexpialidosys"
print mid(a$,undef)
print mid(a$,1,5)
print mid(a$,undef,6)
print mid(a$,6,5)
print mid(a$,"-3")
print "*",mid(a$,0),"*"
print mid(undef,"66")
print mid(a$,6,-3)
print mid(a$,6,3)
print mid(a$,-4,-3)
print mid(a$,-4,3)
=noverbatim

will print

=verbatim
superqualifragilisticexpialidosys
super
superq
quali
sys
**
undef
erq
qua
ido
osy
=noverbatim

*/
COMMAND(MID)
#if NOTIMP_MID
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2,Op3;
  long lStart,lLength,lStringLength;
  char *r,*s;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;

  /* we need not duplicate the argument in case it is a left value, because we don't
     alter it. (And convert to string anyway that duplicates.) */
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  Op2 = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op2) )
    lStart = 1;
  else
    lStart = LONGVALUE(CONVERT2LONG(Op2));

  /* if the start value is negative then it is 
     the number of characters from the end of the string */
  if( lStart <= 0 ){
    lStart += STRLEN(Op1) + 1;
    if( lStart < 0 )lStart = 1;
    }
  nItem = CDR(nItem);
  if( nItem ){
    Op3 = EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;
    if( memory_IsUndef(Op3) )
      lLength = -1;
    else{
      lLength = LONGVALUE(CONVERT2LONG(Op3));
      /* if the length is negative then it counts the substring backward */
      if( lLength < 0 ){
        if( lStart < lLength ){
          lLength = lStart;
          lStart = 1;
          }else{
          lStart += lLength +1;
          lLength = -lLength;
          }
        }
      }
    }else
    lLength = -1;

  lStart --; /* normalize to zero */

  lStringLength = STRLEN(Op1);
  if( lStart < lStringLength ){
    r = STRINGVALUE(Op1) + lStart;
    lStringLength -= lStart;
    }else{
    r = STRINGVALUE(Op1) + lStringLength;
    lStringLength = 0L;
    }
  s = r;
  if( lLength != -1 && lLength < lStringLength )lStringLength = lLength;
  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lStringLength ){
    *r++ = *s++;
    lStringLength--;
    }

#endif
END

/**LEFT
=section string
=display LEFT()
=title LEFT(string,len)

Creates the left of a string. The first argument is the string. The second argument is the
number of characters that should be put into the result. If this value is larger than the
number of characters in the string then all the string is returned.

See also R<MID>, R<RIGHT>

=details
T<left(x,y)> cuts out a substring of T<y> characters from the left of the string T<x>.
If the first argument is not defined the result is also T<undef>. Otherwise the first 
argument is converted to string and the second ar-gument is converted to integer value.

If the second argument is not defined or has negative value it is treated as numeric zero 
and as such the result string will be empty string.

For compatibility reasons you can append a dollar (T<$>) sign to the end of the function
identifier.

Example

=verbatim
a$ = _
"superqualifragilisticexpialidosys"
print "*",left(a$,undef),"*"
print "*",left(a$,7),"*"
print "*",left(a$,-6),"*"
print "*",left(a$,0),"*"
print left(undef,"66")
=noverbatim

will print

=verbatim
**
*superqu*
**
**
undef
=noverbatim

*/
COMMAND(LEFT)
#if NOTIMP_LEFT
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  long lLength,lStringLength;
  char *r,*s;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  lLength = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;
  if( lLength < 0 )lLength = 0;

  s = STRINGVALUE(Op1);
  lStringLength = STRLEN(Op1);
  if( lLength < lStringLength )lStringLength = lLength;
  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lStringLength ){
    *r++ = *s++;
    lStringLength--;
    }

#endif
END

/**RIGHT
=section string
=display RIGHT()
=title RIGHT(string,len)

Creates the right of a string. The first argument is the string. The second argument is the
number of characters that should be put into the result. If this value is larger than the
number of characters in the string then all the string is returned.

See also R<MID>, R<LEFT>.

=details
T<RIGHT(x,y)> cuts out a substring of T<y> characters from the right of the string T<x>.
If the first argument is not defined the result is also T<undef>. Otherwise the first argument 
is converted to string and the second argument is converted to integer value.

If the second argument is not defined or has negative value it is treated as numeric zero 
and as such the result string will be empty string.

For compatibility reasons you can append a dollar (T<$>) sign to the end of the function
identifier.
*/
COMMAND(RIGHT)
#if NOTIMP_RIGHT
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  long lLength,lStringLength;
  char *r,*s;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);
  nItem = CDR(nItem);
  lLength = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;
  if( lLength < 0 )lLength = 0;

  s = STRINGVALUE(Op1);
  lStringLength = STRLEN(Op1);
  if( lStringLength > lLength ){
    s += lStringLength - lLength;
    lStringLength = lLength;
    }

  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lStringLength ){
    *r++ = *s++;
    lStringLength--;
    }

#endif
END

/**SPACE
=section string
=display SPACE()
=title SPACE(n)
Return a string of length T<n> containing spaces.
*/
COMMAND(SPACE)
#if NOTIMP_SPACE
NOTIMPLEMENTED;
#else


  NODE nItem;
  long lLength;
  char *r;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  lLength = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;
  if( lLength < 0 )lLength = 0;

  RESULT = NEWMORTALSTRING(lLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lLength ){
    *r++ = ' ';
    lLength--;
    }

#endif
END

/**STRING
=section string
=title STRING(n,code)
=display STRING()

Create a string of length T<n> containing characters T<code>. If T<code> is a string then the
first character of the string is used to fill the result. Otherwise T<code> is converted to 
long and the ASCII code is used.
*/
COMMAND(STRING)
#if NOTIMP_STRING
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op;
  long lLength;
  char cFill;
  char *r;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  lLength = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem))));
  ASSERTOKE;
  if( lLength < 0 )lLength = 0;
  nItem = CDR(nItem);
  Op = EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( Op == NULL )
    cFill = 0;
  else
  if( TYPE(Op) == VTYPE_STRING ){
    cFill = *(STRINGVALUE(Op));
    }else{
    cFill = (char)(LONGVALUE(CONVERT2LONG(Op)));
    }

  RESULT = NEWMORTALSTRING(lLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lLength ){
    *r++ = cFill;
    lLength--;
    }

#endif
END

/**CHR
=section string
=display CHR()
=title CHR(code)
Return a one character string containing a character of ASCII code T<code>.
*/
COMMAND(CHR)
#if NOTIMP_CHR
NOTIMPLEMENTED;
#else


  long lCharCode;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  lCharCode = LONGVALUE(CONVERT2LONG(EVALUATEEXPRESSION(CAR(PARAMETERLIST))));
  ASSERTOKE;
  lCharCode %= 256;
  if( lCharCode < 0 )lCharCode += 256;

  RESULT = NEWMORTALSTRING(1);
  ASSERTNULL(RESULT)
  *(STRINGVALUE(RESULT)) = (char)lCharCode;
#endif
END

/**ASC
=section string
=title ASC(string)
=display ASC()

Returns the ASCII code of the first character of the argument string.
*/
COMMAND(ASC)
#if NOTIMP_ASC
NOTIMPLEMENTED;
#else


  unsigned long lCharCode;
  VARIABLE Op;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op = _EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;
  if( Op == NULL ){
    RESULT = NULL;
    RETURN;
    }
  Op = CONVERT2STRING(Op);
  if( STRLEN(Op) == 0 ){
    RESULT = NULL;
    RETURN;
    }
  /* evaluate the parameters */
  lCharCode = (unsigned char)*(STRINGVALUE(Op));

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = lCharCode;
#endif
END

/**STRREVERTE
=section string
=display STRREVERSE()
=title STRREVERSE(string)
Return the reversed string (aka. all the characters in the string in reverse order).
*/
COMMAND(STRREVERSE)
#if NOTIMP_STRREVERSE
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1;
  long lStringLength;
  char *r,*s;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  Op1 = _EVALUATEEXPRESSION(CAR(nItem));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  Op1 = CONVERT2STRING(Op1);

  s = STRINGVALUE(Op1);
  lStringLength = STRLEN(Op1);
  s += lStringLength-1;

  RESULT = NEWMORTALSTRING(lStringLength);
  ASSERTNULL(RESULT)
  r = STRINGVALUE(RESULT);
  while( lStringLength ){
    *r++ = *s--;
    lStringLength--;
    }

#endif
END

/**STR
=section string
=title STR(n)
=display STR()
Converts a number to string. This function is rarely needed, because conversion is done automatically.
=details
Converts a number to string. This function is rarely needed, because conversion is done automatically. 
However you may need

=verbatim
 STRING(13,STR(a))
=noverbatim

to be sure that the value T<a> is interpreted as string value.
*/
COMMAND(STR)
#if NOTIMP_STR
NOTIMPLEMENTED;
#else

  VARIABLE Op;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op = _EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;
  if( Op == NULL ){
    RESULT = NULL;
    RETURN;
    }
  /* evaluate the parameters */
  RESULT = CONVERT2STRING(Op);

#endif
END

/**HEX
=section string
=display HEX()
=title HEX(n)

Take the argument as a long value and convert it to a string that represents the value in hexadecimal form.
The hexadecimal form will contain upper case alpha character if there is any alpha character in the
hexadecimal representation of the number.
*/
COMMAND(HEX)
#if NOTIMP_HEX
NOTIMPLEMENTED;
#else


  unsigned long lCode;
  unsigned long lLength,lStore;
  VARIABLE Op;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  Op = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;
  if( Op == NULL ){
    RESULT = NULL;
    RETURN;
    }
  /* evaluate the parameters */
  lCode = LONGVALUE(CONVERT2LONG(Op));
  lStore = lCode;
  lLength = 0;
  if( lCode == 0 )lLength = 1;
  while( lCode ){
    lCode /= 16;
    lLength ++;
    }

  /*
     Note that there is a little hack in this code dealing with the terminating ZCHAR.
     Strings in BASIC are NOT ZCHAR terminated, but sprintf puts a ZCHAR at the end
     of the printed string. To avoid segmentation fault we have to allocate a one byte longer
     string to accomodate the terminating ZCHAR. After the sprintf has put the ZCHAR into the
     buffer we set the correct size of the buffer. This actually removes the ZCHAR from the
     buffer. If the buffer is small, then the actual size is larger than the string length
     anyway. If the buffer is LARGE_BLOCK_TYPE then we will release it when not in use.
  */
  RESULT = NEWMORTALSTRING(lLength+1);
  ASSERTNULL(RESULT)
  sprintf(STRINGVALUE(RESULT),"%*X",lLength,lStore);
  STRLEN(RESULT) = lLength;

#endif
END

/**OCT
=section string
=title OCT(n)
=display OCT()

Take the argument as a long value and convert it to a string that represents the value in octal form.
*/
COMMAND(OCT)
#if NOTIMP_OCT
NOTIMPLEMENTED;
#else


  unsigned long lCode;
  unsigned long lLength,lStore;
  char *s;
  VARIABLE Op;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  Op = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;
  if( Op == NULL ){
    RESULT = NULL;
    RETURN;
    }
  /* evaluate the parameters */
  lCode = LONGVALUE(CONVERT2LONG(Op));
  lStore = lCode;
  lLength = 0;
  if( lCode == 0 )lLength = 1;
  while( lCode ){
    lCode /= 8;
    lLength ++;
    }
  RESULT = NEWMORTALSTRING(lLength);
  ASSERTNULL(RESULT)
  s = STRINGVALUE(RESULT) + lLength -1;
  while( lStore ){
    *s-- = (char)(lStore%8)+'0';
    lStore /= 8;
    }

#endif
END

/**SPLITAQ
=section string
=title SPLITAQ string BY string QUOTE string TO array

Split a string into an array using the second string as delimiter.
The delimited fields may optionally be quoted with the third string.
If the string to be split has zero length the array becomes undefined.
When the delimiter is a zero length string each array element will contain a
single character of the string.

Leading and trailing delimiters are accepted and return an empty element
in the array. For example :-

=verbatim
   SPLITAQ ",'A,B',C," BY "," QUOTE "'" TO Result
=noverbatim
   will generate
=verbatim
                 Result[0] = ""
                 Result[1] = "A,B"
                 Result[2] = "C"
                 Result[3] = ""
=noverbatim

Note that this kind of handling of trailing and leading empty elements is different
from the handling of the same by the command R<SPLIT> and R<SPLITA> which do ignore
those empty elements. This command is useful to handle lines exported as CSV from
Excel or similar application.

The QUOTE string is really a string and need not be a single character. If there is an
unmatched quote string in the string to be split then the rest of the string until its end
is considered quoted.

If there is an unmatched 

See also R<SPLITA>

This command was suggested and implemented by Andrew Kingwell
(T<Andrew.Kingwell@idstelecom.co.uk>)
*/
COMMAND(SPLITAQ)
#if NOTIMP_SPLITAQ
NOTIMPLEMENTED;
#else

  VARIABLE WholeString,Delimiter,Quoter,ResultArray;
  LEFTVALUE Array;
  unsigned long i,lChunkCounter,iStart,iCount;
  long refcount;
  char *Temp;

  WholeString = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Delimiter = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Quoter = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  /* we get the pointer to the variable that points to the value */
  Array = EVALUATELEFTVALUE_A(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(Array)

  /* if the string to split is empty then the result is undef */
  if( memory_IsUndef(WholeString) || STRLEN(WholeString) == 0L ){
    if( *Array )memory_ReleaseVariable(pEo->pMo,*Array);
    *Array = NULL;
    RETURN;
    }

  if( memory_IsUndef(Delimiter) || STRLEN(Delimiter) == 0 ){
    /* empty delimiter splits the string into characters */
    lChunkCounter = STRLEN(WholeString) - 1;
    }else{
    /* calculate the size of the result array */
    i = 0;
    lChunkCounter = 0;
    while( i < STRLEN(WholeString) ){
				if ( ( i <= STRLEN(WholeString)-STRLEN(Quoter) )
					&& !strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Quoter),STRLEN(Quoter)) ) {
    			i += STRLEN(Quoter);
    			while( ( i <= STRLEN(WholeString)-STRLEN(Quoter) )
    				&& ( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Quoter),STRLEN(Quoter)) ) ) i++;
    			i += STRLEN(Quoter);
				}
	      if ( ( i <= STRLEN(WholeString)-STRLEN(Delimiter) )
    			&&  !strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) ) {
    			while( ( i <= STRLEN(WholeString)-STRLEN(Delimiter) )
    				&& ( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) ) ) i++;
					lChunkCounter ++;
					i += STRLEN(Delimiter);
				}else{
					i++;
				}
		}
	}

  ResultArray = NEWARRAY(0,lChunkCounter);
  if( ResultArray == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);

  Temp = ALLOC(STRLEN(WholeString));

  if( memory_IsUndef(Delimiter) || STRLEN(Delimiter) == 0 ){
    for( i=0 ; i < STRLEN(WholeString) ; i++ ){
      ResultArray->Value.aValue[i] = NEWSTRING(1);
      if( ResultArray->Value.aValue[i] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      *STRINGVALUE(ResultArray->Value.aValue[i]) = STRINGVALUE(WholeString)[i];
      }
    }else{
    /* split the string into the array */
    i = 0;
    iStart = i;
    lChunkCounter = 0;
    iCount = 0;

    while( i < STRLEN(WholeString) ){
			if ( ( i <= STRLEN(WholeString)-STRLEN(Quoter) )
				&& !strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Quoter),STRLEN(Quoter)) ) {
    		i += STRLEN(Quoter);
    		while( ( i <= STRLEN(WholeString)-STRLEN(Quoter) )
    			&& ( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Quoter),STRLEN(Quoter)) ) ) {
       		memcpy(Temp + iCount,STRINGVALUE(WholeString)+i,1);
    			i++;
					iCount ++;
				}
   			i += STRLEN(Quoter);
     		}
			if  ( i <= STRLEN(WholeString)-STRLEN(Delimiter) ) {
    		if ( !strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) ) {
					ResultArray->Value.aValue[lChunkCounter] = NEWSTRING(iCount);
					if( ResultArray->Value.aValue[lChunkCounter] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
					memcpy(STRINGVALUE(ResultArray->Value.aValue[lChunkCounter]),Temp,iCount);
					iCount = 0;
					lChunkCounter ++;
					i += STRLEN(Delimiter);
				}else{
    			memcpy(Temp + iCount,STRINGVALUE(WholeString) + i,1);
    			i++;
    			iCount++;
    		}
			}
		}
		
			ResultArray->Value.aValue[lChunkCounter] = NEWSTRING(iCount);
			if( ResultArray->Value.aValue[lChunkCounter] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
			memcpy(STRINGVALUE(ResultArray->Value.aValue[lChunkCounter]),Temp,iCount);

		}


  /* if this variable had value assigned to it then release that value */
  if( *Array )memory_ReleaseVariable(pEo->pMo,*Array);

	FREE(Temp);
	
  *Array = ResultArray;

#endif
END

/**SPLITA
=section string
=title SPLITA string BY string TO array

Split a string into an array using the second string as delimiter.
If the string has zero length the array becomes undefined.
When the delimiter is zero length string each array element will contain a single
character of the string.

See also R<SPLIT>
*/
COMMAND(SPLITA)
#if NOTIMP_SPLITA
NOTIMPLEMENTED;
#else


  VARIABLE WholeString,Delimiter,ResultArray;
  LEFTVALUE Array;
  unsigned long i,lChunkCounter,iStart;
  long refcount;

  WholeString = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Delimiter = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  /* we get the pointer to the variable that points to the value */
  Array = EVALUATELEFTVALUE_A(PARAMETERNODE);
  ASSERTOKE;
  DEREFERENCE(Array)

  /* if the string to split is empty then the result is undef */
  if( memory_IsUndef(WholeString) || STRLEN(WholeString) == 0L ){
    if( *Array )memory_ReleaseVariable(pEo->pMo,*Array);
    *Array = NULL;
    RETURN;
    }

  if( memory_IsUndef(Delimiter) || STRLEN(Delimiter) == 0 ){
    /* empty delimiter splits the string into characters */
    lChunkCounter = STRLEN(WholeString);
    }else{
    /* calculate the size of the result array */
    if( !strncmp(STRINGVALUE(WholeString),STRINGVALUE(Delimiter),STRLEN(Delimiter)) ){
      /* if the string starts with a delimiter we do not create a starting empty string */
      i = STRLEN(Delimiter);
      }else{
      i = 1;
      }
    lChunkCounter =1;
    while( i < STRLEN(WholeString)-STRLEN(Delimiter) ){
      if( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) )i++;
      else{
        lChunkCounter ++;
        i += STRLEN(Delimiter);
        }
      }
    }

  ResultArray = NEWARRAY(0,lChunkCounter-1);
  if( ResultArray == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);

  if( memory_IsUndef(Delimiter) || STRLEN(Delimiter) == 0 ){
    for( i=0 ; i < STRLEN(WholeString) ; i++ ){
      ResultArray->Value.aValue[i] = NEWSTRING(1);
      if( ResultArray->Value.aValue[i] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      *STRINGVALUE(ResultArray->Value.aValue[i]) = STRINGVALUE(WholeString)[i];
      }
    }else{
    /* split the string into the array */
    if( !strncmp(STRINGVALUE(WholeString),STRINGVALUE(Delimiter),STRLEN(Delimiter)) ){
      /* if the string starts with a delimiter we do not create a starting empty string */
      i = STRLEN(Delimiter);
      }else{
      i = 0;
      }
    iStart = i;
    lChunkCounter = 0;
    while( i <= STRLEN(WholeString)-STRLEN(Delimiter) ){
      if( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) )i++;
      else{
        ResultArray->Value.aValue[lChunkCounter] = NEWSTRING(i-iStart);
        if( ResultArray->Value.aValue[lChunkCounter] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
        memcpy(STRINGVALUE(ResultArray->Value.aValue[lChunkCounter]),(STRINGVALUE(WholeString)+iStart),i-iStart);
        lChunkCounter ++;
        i += STRLEN(Delimiter);
        iStart = i;
        }
      }
    if( iStart < STRLEN(WholeString) ){
      ResultArray->Value.aValue[lChunkCounter] = NEWSTRING(STRLEN(WholeString)-iStart);
      if( ResultArray->Value.aValue[lChunkCounter] == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      memcpy(STRINGVALUE(ResultArray->Value.aValue[lChunkCounter]),(STRINGVALUE(WholeString)+iStart),STRLEN(WholeString)-iStart);
      }
    }


  /* if this variable had value assigned to it then release that value */
  if( *Array )memory_ReleaseVariable(pEo->pMo,*Array);

  *Array = ResultArray;

#endif
END

/**SPLIT
=section string
=title SPLIT string BY string TO var_1,var_2,var_3,...,var_n

Takes the string and splits into the variables using the second string as delimiter.
*/
COMMAND(SPLIT)
#if NOTIMP_SPLIT
NOTIMPLEMENTED;
#else

  NODE nItem;
  VARIABLE WholeString,Delimiter;
  LEFTVALUE LeftValue;
  unsigned long i,iStart;
  long refcount;

  /* Note that we should NOT use _EVALUATEEXPRESSION because the command may use the same variable
     as a target for a sub-string and by that time we still need the original string. Therefore
     we use EVALUATEEXPRESSION that creates a copy of the result.
   */
  WholeString = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Delimiter = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;

  nItem = PARAMETERNODE;

  /* if the string to split is undef or empty then the results are undef */
  if( memory_IsUndef(WholeString) || STRLEN(WholeString) == 0L ){
    while( nItem ){
      LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));
      ASSERTOKE;
      DEREFERENCE(LeftValue)

      if( *LeftValue != NULL )
        memory_ReleaseVariable(pEo->pMo,*LeftValue);
      *LeftValue = NULL;
      nItem = CDR(nItem);
      }
    RETURN;
    }

  if( memory_IsUndef(Delimiter) || STRLEN(Delimiter) == 0 ){
    for( i=0 ; i < STRLEN(WholeString) && nItem ; i++ ){
      LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));
      ASSERTOKE;
      DEREFERENCE(LeftValue);
      if( *LeftValue != NULL )
        memory_ReleaseVariable(pEo->pMo,*LeftValue);
      nItem = CDR(nItem);
      if( nItem ){
        *LeftValue = NEWSTRING(1);
        if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
        *STRINGVALUE(*LeftValue) = STRINGVALUE(WholeString)[i];
        }else{
        /* this is the last variable, it gets the rest of the string */
        *LeftValue = NEWSTRING(STRLEN(WholeString)-i);
        if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
        memcpy(STRINGVALUE(*LeftValue),STRINGVALUE(WholeString)+i,STRLEN(WholeString)-i);
        }
      }
    }else{
    /* split the string into the parameters */
    if( !strncmp(STRINGVALUE(WholeString),STRINGVALUE(Delimiter),STRLEN(Delimiter)) ){
      /* if the string starts with a delimiter we do not create a starting empty string */
      i = STRLEN(Delimiter);
      }else{
      i = 0;
      }
    iStart = i;
    while( i <= STRLEN(WholeString)-STRLEN(Delimiter) && nItem ){
      if( strncmp(STRINGVALUE(WholeString)+i,STRINGVALUE(Delimiter),STRLEN(Delimiter)) )i++;
      else{
        LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));
        ASSERTOKE;
        DEREFERENCE(LeftValue)

        if( *LeftValue != NULL )
          memory_ReleaseVariable(pEo->pMo,*LeftValue);
        nItem = CDR(nItem);
        if( nItem || STRLEN(WholeString)-i == STRLEN(Delimiter) ){
          *LeftValue = NEWSTRING(i-iStart);
          if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
          memcpy(STRINGVALUE(*LeftValue),(STRINGVALUE(WholeString)+iStart),i-iStart);
          i += STRLEN(Delimiter);
          iStart = i;
          }else{
          *LeftValue = NEWSTRING(STRLEN(WholeString)-iStart);
          if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
          memcpy(STRINGVALUE(*LeftValue),(STRINGVALUE(WholeString)+iStart),STRLEN(WholeString)-iStart);
          i += STRLEN(Delimiter);
          iStart = i;
          }
        }
      }
    if( iStart < STRLEN(WholeString) && nItem ){
      LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));
      ASSERTOKE;
      DEREFERENCE(LeftValue);

      if( *LeftValue != NULL )
        memory_ReleaseVariable(pEo->pMo,*LeftValue);
      nItem = CDR(nItem);
      *LeftValue = NEWSTRING(STRLEN(WholeString)-iStart);
      if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      memcpy(STRINGVALUE(*LeftValue),(STRINGVALUE(WholeString)+iStart),STRLEN(WholeString)-iStart);
      }
    }
  /* if there are any variables left then undef all */
  while( nItem ){
    LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));
    ASSERTOKE;
    DEREFERENCE(LeftValue)

    if( *LeftValue != NULL )
      memory_ReleaseVariable(pEo->pMo,*LeftValue);
    *LeftValue = NULL;
    nItem = CDR(nItem);
    }

#endif
END

/**JOIN
=section string
=display JOIN()
=title JOIN(joiner,str1,str2,...)

Join the argument strings using the first argument as a joiner string.
=details
This function can be used to join several strings together.
The first argument of the function is the string used to join the rest 
of the arguments. The rest of the argument are joined together, but also elements on 
an array can be joined together. See the example:

=verbatim
for i=1 to 8
 q[i] = I
next
print join("|",q)
print
print join("/",1,2,3,4,5,6,7,8)
print
print join(" j-s ",q,2,3,4,5,6,7,8)
print
print join("/",1)
print
=noverbatim

will print

=verbatim
1|2|3|4|5|6|7|8
1/2/3/4/5/6/7/8
1 j-s 2 j-s 3 j-s 4 j-s 5 j-s 6 j-s 7 j-s 8
1
=noverbatim

The first join joins the elements of the array. The second join joins the arguments of the 
function. The third example also joins the arguments although the second argument is an array. 
Because there are more arguments each of them is treated as single value and are joined. 
Whenever an array is used in place of a single value, the first element of the array is taken. 
In this example this is 1. The last join is a special one. In this case the join string is not 
used, because there is only one argument after the join string. Because this argument is not 
an array there are no elements of it to join.
*/
COMMAND(JOIN)
#if NOTIMP_JOIN
NOTIMPLEMENTED;
#else

  NODE nItem;
  char *s;
  VARIABLE vJoiner,vStringArray;
  int iFirstLoop;
  struct _JoinItem {
    VARIABLE vThisItem;
    struct _JoinItem *next;
    } *JoinItem,**pJoinItem,*JoinFree;
  unsigned long lResultLength,lItemNumber,i;

  JoinItem = NULL;
  pJoinItem = &JoinItem;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;
  vJoiner = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  if( ! nItem ){/* if there is no second argument. This should not happen, because syntax
                         analysis result a compile time error if there is only one or less arguments. */
    RESULT = NEWMORTALSTRING(0);
    ASSERTNULL(RESULT)
    RETURN;    
    }
  if( ! ( CDR(nItem) ) ){/* if there are no more arguments then
                                       check that this second argument is an array */
    vStringArray = _EVALUATEEXPRESSION_A(CAR(nItem));
    ASSERTOKE;
    iFirstLoop = 1; /* the first element is already evaluated, and is stored in vStringArray 
                       this variable flags the first execution of the loop and helps the program
                       not to evaluate the first string again when join and not joina is performed */
    }else{
    iFirstLoop = 0;       /* there are more than two arguments, therefore we perform join and not joina */
    vStringArray = NULL;  /* we have to set this value to a non-array. undef is non array. but because
                             iFirstLoop is zero this argument will be evaluated because we did not evaluate it
                             now */
    }
  if( vStringArray && TYPE(vStringArray) == VTYPE_ARRAY ){
    lItemNumber = vStringArray->ArrayHighLimit - vStringArray->ArrayLowLimit +1;
    lResultLength = 0;
    for( i=0 ; i< lItemNumber ; i++ ){
      *pJoinItem = ALLOC( sizeof(struct _JoinItem) );
      if( *pJoinItem == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      (*pJoinItem)->vThisItem = CONVERT2STRING(vStringArray->Value.aValue[i]);
      if( (*pJoinItem)->vThisItem != NULL )
        lResultLength += STRLEN((*pJoinItem)->vThisItem);
      nItem = CDR(nItem);
      (*pJoinItem)->next = NULL;
      pJoinItem = &( (*pJoinItem)->next );
      }
    }else{
    lResultLength = 0L;
    lItemNumber = 0L;
    while( nItem ){
      *pJoinItem = ALLOC( sizeof(struct _JoinItem) );
      if( *pJoinItem == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
      if( iFirstLoop ){
        (*pJoinItem)->vThisItem = CONVERT2STRING(vStringArray);
        iFirstLoop = 0;
        }else{
        (*pJoinItem)->vThisItem = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
        ASSERTOKE;
        }
      if( (*pJoinItem)->vThisItem != NULL )
        lResultLength += STRLEN((*pJoinItem)->vThisItem);
      lItemNumber++;
      nItem = CDR(nItem);
      (*pJoinItem)->next = NULL;
      pJoinItem = &( (*pJoinItem)->next );
      }
    }
  if( lItemNumber )
    lResultLength += (lItemNumber-1) * ( vJoiner ? STRLEN(vJoiner) : 0 );

  RESULT = NEWMORTALSTRING(lResultLength);
  ASSERTNULL(RESULT)

  s = STRINGVALUE(RESULT);
  while( JoinItem ){
    if( JoinItem->vThisItem ){
      memcpy(s,STRINGVALUE(JoinItem->vThisItem),STRLEN(JoinItem->vThisItem));
      s += STRLEN(JoinItem->vThisItem);
      }
    /* if there is next element after this and there is separator then append the separator */
    if( JoinItem->next && vJoiner ){
      memcpy(s,STRINGVALUE(vJoiner),STRLEN(vJoiner));
      s += STRLEN(vJoiner);
      }
    /* and now free the allocated list element */
    JoinFree = JoinItem;
    JoinItem = JoinItem->next;
    FREE(JoinFree);
    }

#endif
END

#define INITIALIZE   if( initialize_like(pEo) )ERROR(COMMAND_ERROR_MEMORY_LOW); \
                     pLastResult = (pPatternParam)PARAMPTR(CMD_LIKEOP);

int initialize_like(pExecuteObject pEo){
  pPatternParam pLastResult;

  /* initialize only once */
  if( PARAMPTR(CMD_LIKEOP) )return 0;
  PARAMPTR(CMD_LIKEOP) = ALLOC(sizeof(PatternParam));
  if( PARAMPTR(CMD_LIKEOP) == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pLastResult = (pPatternParam)PARAMPTR(CMD_LIKEOP);

  pLastResult->cArraySize = 0;
  pLastResult->cAArraySize = 0;
  pLastResult->pcbParameterArray = NULL;
  pLastResult->ParameterArray = NULL;
  pLastResult->pszBuffer = NULL;
  pLastResult->cbBufferSize = 0;
  pLastResult->pThisMatchSets = NULL;
  pLastResult->iMatches = 0;
  return 0;
  }

static int allocate_MatchSets(pExecuteObject pEo){
  pPatternParam pLastResult;

  pLastResult = (pPatternParam)PARAMPTR(CMD_LIKEOP);
  if( pLastResult->pThisMatchSets )return 0;
  pLastResult->pThisMatchSets = ALLOC(sizeof(MatchSets));
  if( pLastResult->pThisMatchSets == NULL )return COMMAND_ERROR_MEMORY_LOW;
  match_InitSets(pLastResult->pThisMatchSets);
  return 0;
  }

/**SETJOKER
=title SET JOKER "c" TO "abcdefgh..."
=display SET [NO] JOKER
=section string pattern

Set a joker character to match certain characters when using the R<LIKE> operator. The joker character
T<"c"> can be one of the following characters

=verbatim
*  #  $  @  ?  &  %  !  +  /  |  <  >
=noverbatim

The string after the keyword T<TO> should contain all the characters that the joker
character should match. To have the character to match only itself to be a normal character
say
=verbatim
SET NO JOKER "c"
=noverbatim

See also R<SETWILD>, R<LIKE> (details), R<JOKER>

*/
COMMAND(SETJOKER)
#if NOTIMP_SETJOKER
NOTIMPLEMENTED;
#else

  VARIABLE Op1,Op2;
  pPatternParam pLastResult;
  char JokerCharacter;
  char *p;
  unsigned long pL;

  INITIALIZE;

  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Op2 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;

  if( memory_IsUndef(Op1) || ! match_index(JokerCharacter=*STRINGVALUE(Op1)) )ERROR(COMMAND_ERROR_INVALID_JOKER);

  if( Op2 ){
    p = STRINGVALUE(Op2);
    pL = STRLEN(Op2);
    }else{
    p = "";
    pL = 0;
    }

  allocate_MatchSets(pEo);
  match_ModifySet(pLastResult->pThisMatchSets,JokerCharacter,pL,(unsigned char *)p,MATCH_ADDC|MATCH_SSIJ|MATCH_NULS);

#endif
END

/**SETWILD
=title SET WILD "c" TO "abcdefgh..."
=display SET [NO] WILD
=section string pattern

Set a wild character to match certain characters when using the R<LIKE> operator. The wild character
T<"c"> can be one of the following characters

=verbatim
*  #  $  @  ?  &  %  !  +  /  |  <  >
=noverbatim

The string after the keyword T<TO> should contain all the characters that the wild card
character should match. To have the character to match only itself to be a normal character
say
=verbatim
SET NO WILD "c"
=noverbatim

See also R<SETJOKER>, R<LIKE>  (details), R<JOKER>
*/
COMMAND(SETWILD)
#if NOTIMP_SETWILD
NOTIMPLEMENTED;
#else

  VARIABLE Op1,Op2;
  pPatternParam pLastResult;
  char JokerCharacter;
  char *p;
  unsigned long pL;

  INITIALIZE;


  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  Op2 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;

  if( memory_IsUndef(Op1) || ! match_index(JokerCharacter=*STRINGVALUE(Op1)) )ERROR(COMMAND_ERROR_INVALID_JOKER);

  if( Op2 ){
    p = STRINGVALUE(Op2);
    pL = STRLEN(Op2);
    }else{
    p = "";
    pL = 0;
    }

  allocate_MatchSets(pEo);
  match_ModifySet(pLastResult->pThisMatchSets,JokerCharacter,pL,(unsigned char *)p,MATCH_ADDC|MATCH_SMUJ|MATCH_NULS);

#endif
END

COMMAND(SETNOJO)
#if NOTIMP_SETNOJO
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  pPatternParam pLastResult;
  char JokerCharacter;

  INITIALIZE;

  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;

  if( memory_IsUndef(Op1) || ! match_index(JokerCharacter=*STRINGVALUE(Op1)) )ERROR(COMMAND_ERROR_INVALID_JOKER);

  allocate_MatchSets(pEo);
  match_ModifySet(pLastResult->pThisMatchSets,JokerCharacter,0L,NULL,MATCH_SNOJ);

#endif
END

/**LIKE
=section string pattern
=title string LIKE pattern
Compare a string against a pattern.

=verbatim
      string LIKE pattern
=noverbatim

The pattern may contain joker characters and wild card characters.
=details
Pattern matching in ScriptBasic is similar to the pattern matching that you get used to on 
the UNIX or Windows NT command line. The operator like compares a string to a pattern.

=verbatim
string like pattern
=noverbatim

Both string and pattern are expressions that should evaluate to a string. If the pattern matches 
the string the result of the operator is true, otherwise the result is false.

The pattern may contain normal characters, wild card characters and joker characters. 
The normal characters match themselves. The wild card characters match one or more characters from the 
set they are for. The joker characters match one character from the set they stand for. For example:

=verbatim
Const nl="\n"
print "file.txt" like "*.txt",nl
print "file0.txt" like "*?.txt",nl
print "file.text" like "*.txt",nl
=noverbatim

will print

=verbatim
-1
-1
0
=noverbatim

The wild card character T<*> matches a list of characters of any code. The joker character T<?> matches 
a single character of any code. In the first print statement the T<*> character matches the string file 
and T<.txt> matches itself at the end of the string. In the second example T<*> matches the string file 
and the joker T<?> matches the character T<0>. The wild card character T<*> is the most general wild card 
character because it matches one or more of any character. There are other wild card characters. 
The character T<#> matches one or more digits, T<$> matches one or more alphanumeric characters and
finally T<@> matches one or more alpha characters (letters).

=verbatim
*	all characters
#	0123456789
$	0123456789abcdefghijklmnopqrstxyvwzABCDEFGHIJKLMNOPQRSTXYVWZ
@	abcdefghijklmnopqrstxyvwzABCDEFGHIJKLMNOPQRSTXYVWZ
=noverbatim

A space in the pattern matches one or more white spaces, but the space is not a regular wild card character,
because it behaves a bit different.

Note that wild card character match ONE or more characters and not zero or more as in other systems. Joker 
characters match exactly one character, and there is only one joker character by default, the 
character T<?>, which matches a single character of any code.

We can match a string to a pattern, but that is little use, unless we can tell what 
substring the joker or wildcard characters matched. For the purpose the function joker is available. The argument 
of this function is an integer number, n starting from 1 and the result is the substring that the last 
pattern matching operator found to match the nth joker or wild card character. For example

=verbatim
Const nl="\n"
if "file.txt" like "*.*" then
  print "File=",joker(1)," extension=",joker(2),nl
else
  print "did not match"
endif
=noverbatim

will print

=verbatim
File=file extension=txt
=noverbatim

If the pattern did not match the string or the argument of the function joker is zero or negative, or is larger 
than the serial number of the last joker or wild card character the result is T<undef>.

Note that there is no separate function for the wild card character substrings and one for the joker characters.
The function joker serves all of them counting each from left to right. The function joker does not count, 
nor return the spaces, because programs usually are not interested in the number of the spaces that separate 
the lexical elements matched by the pattern.

Sometimes you want a wild card character or joker character to match only itself. For example you want to 
match the string T<"13*52"> to the pattern two numbers separated by a star. The problem is that the star 
character is a wild card character and therefore T<"#*#"> matches any string that starts and ends with a digit. 
But that may not be a problem. A T<*> character matches one or more characters, and therefore T<"#*#"> will 
indeed match T<"13*52">. The problem is, when we want to use the substrings.

=verbatim
Const nl="\n"
a="13*52" like "#*#"
print joker(1)," ",joker(3),nl
a="13*52" like "#~*#"
print joker(1)," ",joker(2),nl
=noverbatim

will print

=verbatim
1 52
13 52
=noverbatim

The first T<#> character matches one character, the T<*> character matches the substring T<"3*"> and the 
final T<#> matches the number T<52>.


The solution is the pattern escape character. The pattern escape character is the tilde
character: T<~>. Any character following the T<~> character is treated as normal character 
and is matched only by itself. This is true for any normal character, for wild card 
characters; joker characters; for the space and finally for the tilde character itself.
The space character following the tilde character matches exactly one space characters.

Pattern matching is not always as simple as it seems to be from the previous examples.
The pattern T<"*.*"> matches files having extension and T<joker(1)> and T<joker(2)> can 
be used to retrieve the file name and the extension. What about the file T<sciba_source.tar.gz>?
Will it result

=verbatim
File=scriba_source.tar extension=gz
=noverbatim

or

=verbatim
File=scriba_source extension=tar.gz
=noverbatim

The correct result is the second. Wild card characters implemented in ScriptBasic are not greedy.
They eat up only as many characters as they need.

Up to now we were talking about wild card characters and the joker character defining what 
matches what as final rule carved into stone. But these rules are only the default behavior of 
these characters and the program can alter the set of characters that a joker or wild card character matches.

There are 13 characters that can play joker or wild card character role in pattern matching. These are:

=verbatim
*  #  $  @  ?  &  %  !  +  /  |  <  >
=noverbatim

When the program starts only the first five characters have special meaning the others are normal characters. To change the role of a character the program has to execute a set joker or set wild command. The syntax of the commands are:

set joker expression to expression
set wild expression to expression

Both expressions should evaluate to string. The first character of the first string should be the joker or wild card character and the second string should contain all the characters that the joker or wild card character matches. The command set joker alters the behavior of the character to be a joker character matching a single character in the compared string. The command set wild alters the behavior of the character to be a wild card character matching one or more characters in the compared string. For ex-ample if you may want the & character to match all hexadecimal characters the program has to execute:

set wild "&" to "0123456789abcdefABCDEF"

If a character is currently a joker of wild card character you can alter it to be a normal character issuing one of the commands

set no joker expression
set no wild expression

where expression should evaluate to a string and the first character of the string should give the character to alter the behavior of.

The two commands are identical, you may always use one or the other; you can use set no joker for a character being currently wild card character and vice versa. You can execute the command even if the character is currently a normal character in the pattern matching game.

Using the commands now we can see that

*/
COMMAND(LIKEOP)
#if NOTIMP_LIKEOP
NOTIMPLEMENTED;
#else


  NODE nItem;
  VARIABLE Op1,Op2;
  char *s,*p;
  unsigned long sL,pL,i;
  unsigned long cArraySize;
  pPatternParam pLastResult;
  int iError;

  INITIALIZE;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameters */
  nItem = PARAMETERLIST;
  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  Op2 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;

  if( Op1 ){
    s = STRINGVALUE(Op1);
    sL = STRLEN(Op1);
    }else{
    s = "";
    sL = 0;
    }

  if( Op2 ){
    p = STRINGVALUE(Op2);
    pL = STRLEN(Op2);
    }else{
    p = "";
    pL = 0;
    }

  cArraySize = match_count(p,pL);
  if( cArraySize > pLastResult->cArraySize ){
    if( pLastResult->pcbParameterArray )FREE(pLastResult->pcbParameterArray);
    if( pLastResult->ParameterArray)FREE(pLastResult->ParameterArray);
    pLastResult->cArraySize = 0;
    pLastResult->pcbParameterArray = ALLOC(cArraySize*sizeof(unsigned long));
    if( pLastResult->pcbParameterArray == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    pLastResult->ParameterArray    = ALLOC(cArraySize*sizeof(char *));
    if( pLastResult->ParameterArray == NULL ){
      FREE(pLastResult->pcbParameterArray);
      pLastResult->pcbParameterArray = NULL;
      ERROR(COMMAND_ERROR_MEMORY_LOW);
      }

    pLastResult->cArraySize = cArraySize;
    }
  for( i=0 ; i < pLastResult->cArraySize ; i++ ){
    pLastResult->pcbParameterArray[i] = 0;
    pLastResult->ParameterArray[i] = NULL;
    }
  pLastResult->cAArraySize = cArraySize;

  if( pLastResult->cbBufferSize < sL ){
    pLastResult->cbBufferSize = 0;
    if( pLastResult->pszBuffer )FREE(pLastResult->pszBuffer);
    pLastResult->pszBuffer = ALLOC(sL*sizeof(char));
    if( pLastResult->pszBuffer == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    pLastResult->cbBufferSize = sL;
    }

  iError = match_match(p,
                       pL,
                       s,
                       sL,
                       pLastResult->ParameterArray,
                       pLastResult->pcbParameterArray,
                       pLastResult->pszBuffer,
                       pLastResult->cArraySize,
                       pLastResult->cbBufferSize,
                       !(OPTION("compare")&1),
                       pLastResult->pThisMatchSets,
                       &(pLastResult->iMatches));

  if( iErrorCode )ERROR(iErrorCode);

  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = pLastResult->iMatches ? -1 : 0;

#endif
END

/**CHOMP
=section string
=title CHOMP()
=display CHOMP()

Remove the trailing new line from the space. If the last character of the string is not
new line then the original stringis returned. This function is useful to remove the trailing
new line character when reading a line from a file using the command R<LINEINPUT>
*/
COMMAND(CHOMP)
#if NOTIMP_CHOMP
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  long StringLen;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* CONVERT2STRING never modifies the parameter, therefore it is more efficient
     to use _EVALUATEEXPRESSION */
  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }
  StringLen = STRLEN(Op1);

  if( STRINGVALUE(Op1)[StringLen-1] == '\n' )StringLen--;
  RESULT = NEWMORTALSTRING(StringLen);
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),STRINGVALUE(Op1),StringLen);
#endif
END

/**JOKER
=section string pattern
=title JOKER(n)
=display JOKER()

Return the actual match for the n-th joker character from the last executed R<LIKE> operator.
=details

When a T<LIKE> operator is executed ScriptBasic stores the actual strings that
matched the joker and wild card characters from the pattern in an array. Using this
function the programcan access the actual value of the n-th string that was matched
against the n-th joker or wild card character. For example:

=verbatim
Const nl="\n"
if "file.txt" like "*.*" then
  print "File=",joker(1)," extension=",joker(2),nl
else
  print "did not match"
endif
=noverbatim

will print

=verbatim
File=file extension=txt
=noverbatim
*/
COMMAND(JOKER)
#if NOTIMP_JOKER
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  unsigned long index;
  pPatternParam pLastResult;

  INITIALIZE;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op1 = CONVERT2LONG(EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(Op1) || 
      (! pLastResult->iMatches) ||
      (index = LONGVALUE(Op1)) <= 0 ||
      index > pLastResult->cAArraySize   ){
    RESULT = NULL;
    RETURN;
    }

  index--;
  RESULT = NEWMORTALSTRING(pLastResult->pcbParameterArray[index]);
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),pLastResult->ParameterArray[index],pLastResult->pcbParameterArray[index]);

#endif
END

/**OPTION
=section misc
=title OPTION symbol value

Set the integer value of an option. The option can be any string without the double quote. Option names
are case insensitive in ScriptBasic.

This command has no other effect than storing the integer value in the option symbol table. The commands
or extenal modules may access the values and may change their behavior accoring to the actual
values associated with option symbols.

You can retrieve the actual value of an option symbol using the function R<OPTIONF>

*/
COMMAND(OPTION)
#if NOTIMP_OPTION
NOTIMPLEMENTED;
#else

  char *pszOptionName;
  long lOptionValue;
  VARIABLE vOptionValue;

  pszOptionName = pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue;
  NEXTPARAMETER;
  vOptionValue = CONVERT2LONG(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  if( memory_IsUndef(vOptionValue) ){
    options_Reset(pEo,pszOptionName);
    RETURN;
    }
  lOptionValue = LONGVALUE(vOptionValue);
  options_Set(pEo,pszOptionName,lOptionValue);

#endif
END

/**OPTIONF
=section misc
=display OPTION()
=title OPTION("symbol")

Retrieve the actual value of an option symbol as an integer or T<undef> if the option was not set.
Unlike in the command R<OPTION> the argument of this function should be double quoted.
*/
COMMAND(OPTIONF)
#if NOTIMP_OPTIONF
NOTIMPLEMENTED;
#else

  VARIABLE Op1;
  unsigned long *plOptionValue;
  char *buffer;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  Op1 = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(PARAMETERLIST)));
  ASSERTOKE;
  if( memory_IsUndef(Op1) ){
    RESULT = NULL;
    RETURN;
    }

  CONVERT2ZCHAR(Op1,buffer);
  plOptionValue = OPTIONR(buffer);
  FREE(buffer);
  if( plOptionValue ){
    RESULT = NEWMORTALLONG;
    ASSERTNULL(RESULT)
    LONGVALUE(RESULT) = *plOptionValue;
    }else RESULT = NULL;

#endif
END

#define CHUNK_SIZE      1024
#define F_MINUS         1
#define F_PLUS          2
#define F_ZERO          4
#define F_BLANK         8
#define F_SHARP         16
#define FORMAT_SYNTAX_ERROR     iErrorCode = COMMAND_ERROR_ARGUMENT_RANGE; goto error_escape;
#define ASSERT_PARAMETER_COUNT  if (iArg >= cParameters) {iErrorCode = COMMAND_ERROR_FEW_ARGS; goto error_escape;}
#define CHECK_MEM(x)            if (!check_size(&params,(x))){iErrorCode = COMMAND_ERROR_MEMORY_LOW; goto error_escape;}
#define CHECK_OPERATION(x)      if (!x){iErrorCode = COMMAND_ERROR_MEMORY_LOW; goto error_escape;}

typedef struct _formatParams {
    char* buf;
    long bufSize;
    long bufPtr;
    int flags;
    int width;
    int prec;
    char type;
    long vLong;
    double vDouble;
    const char* vString;
    long vSize;
} formatParams, *pFormatParams;

int check_size(pFormatParams params, long len) {
    if (len + params->bufPtr > params->bufSize) {
        char* ptr;
        len += params->bufPtr + CHUNK_SIZE;
        ptr = (char*)realloc(params->buf, len);
        if (!ptr) {
            free(params->buf);
            params->buf = NULL;
            return 0;
        }
        params->bufSize = len;
    }
    return 1;
}

int printInt(pFormatParams params) {
    char buf[100];
    int width;
    int flags;
    int len = params->prec + params->width + 32;
    if (!check_size(params, len))
        return 0;
    width = params->width;
    if (width < 0)
        width = 0;
    flags = params->flags;
    if (params->prec < 0) {
        sprintf(buf, "%%%s%s%s%s%s%dl%c", (flags & F_MINUS) ? "-" : "",
            (flags & F_PLUS) ? "+" : "",
            (flags & F_SHARP) ? "#" : "",
            (flags & F_BLANK) ? " " : "",
            (flags & F_ZERO) ? "0" : "",
            width, params->type);
    }
    else {
        sprintf(buf, "%%%s%s%s%s%s%d.%dl%c", (flags & F_MINUS) ? "-" : "",
            (flags & F_PLUS) ? "+" : "",
            (flags & F_SHARP) ? "#" : "",
            (flags & F_BLANK) ? " " : "",
            (flags & F_ZERO) ? "0" : "",
            width, params->prec, params->type);
    }
    len = sprintf(params->buf + params->bufPtr, buf, params->vLong);
    params->bufPtr += len;
    return 1;
}

int printDouble(pFormatParams params) {
    char buf[100];
    int prec;
    int width;
    int flags;
    int len = params->prec + params->width + 320;
    if (!check_size(params, len))
        return 0;
    width = params->width;
    if (width < 0)
        width = 0;
    prec = params->prec;
    if (prec < 0)
        prec = 6;
    if (prec > 300)
        prec = 300;
    flags = params->flags;
    sprintf(buf, "%%%s%s%s%s%s%d.%d%c", (flags & F_MINUS) ? "-" : "",
        (flags & F_PLUS) ? "+" : "",
        (flags & F_SHARP) ? "#" : "",
        (flags & F_BLANK) ? " " : "",
        (flags & F_ZERO) ? "0" : "",
        width, prec, params->type);
    len = sprintf(params->buf + params->bufPtr, buf, params->vDouble);
    params->bufPtr += len;
    return 1;
}

int printChar(pFormatParams params) {
    long size;
    long pad;
    char padChar;
    if (params->prec < 0)
        size = params->vSize;
    else
        size = (params->vSize > params->prec) ? params->prec : params->vSize;
    pad = (params->width > size) ? params->width - size : 0;
    if (!check_size(params, pad + size))
        return 0;
    padChar = (params->flags & F_ZERO) ? '0' : ' ';
    if (params->flags & F_MINUS) {
        memcpy(params->buf + params->bufPtr, params->vString, size);
        memset(params->buf + params->bufPtr + size, ' ', pad);
    }
    else {
        memset(params->buf + params->bufPtr, padChar, pad);
        memcpy(params->buf + params->bufPtr + pad, params->vString, size);
    }
    params->bufPtr += size + pad;
    return 1;
}

/* The following code was imported from SmallBasic
Hi Paulo

> I'm a developer with http://sourceforge.net/projects/scriptbasic and 
> I'm adding the function FORMAT to it. I've noticed that you've already 
> implemented it in your product but as it's GPL and mine is LGPL I 
> can't
use
> it without your authorization, that I'm asking for.

What is the difference between GPL and LGPL ?
Anyway my purpose is to let the others to use my code, but not for money.

Take anything that you'll need :)

I'll be happy if you just put a 'thanks' somewhere :)

btw, I was saw your project before enough time. I was liked, I can say 
script-basic is the only competitor to small-basic on unix-world (the others

are something like VB)...

Regards
Nicholas

*/

/*
*	INT(x)
*/
static double	fint(double x)
{
    return (x < 0.0) ? -floor(-x) : floor(x);
}

/*
*	FRAC(x)
*/
static double	frac(double x)
{
    return fabs(fabs(x)-fint(fabs(x)));
}

/*
*	SGN(x)
*/
static int		sgn(double x)
{
    return (x < 0.0) ? -1 : 1;
}

/*
*	ZSGN(x)
*/
static int		zsgn(double x)
{
    return (x < 0.0) ? -1 : ((x > 0.0) ? 1 : 0);
}

/*
*	ROUND(x, digits)
*/
static double	fround(double x, int dig)
{
    double	m;
    
    m = floor(pow(10.0, dig));
    if	( x < 0.0 )
        return -floor((-x * m) + .5) / m;
    return floor((x * m) + .5) / m;
}

/*
*	Part of floating point to string (by using integers) algorithm
*	where x any number 2^31 > x >= 0 
*/
static void	fptoa(double x, char *dest)
{
    long	l;
    
    *dest = '\0';
    l = (long) x;
    sprintf(dest, "%ld", l);	/* or l=atol(dest) */
}

/*
*	remove rightest zeroes from the string
*/
static void	rmzeros(char *buf)
{
    char	*p = buf;
    
    p += (strlen(buf) - 1);
    while ( p > buf )	{
        if	( *p != '0' )
            break;
        *p = '\0';
        p --;
    }
}

/*
*	best float to string (lib)
*
*	This is the real float-to-string routine.
*	It used by the routines:
*		bestfta(double x, char *dest)
*		expfta(double x, char *dest)
*/
static void	bestfta_p(double x, char *dest, double minx, double maxx)
{
    double	ipart, fpart, fdif;
    int		sign, i;
    char	*d = dest;
    long	power = 0;
    char	buf[64];
    
    if	( fabs(x) == 0.0 )	{
        strcpy(dest, "0");
        return;
    }
    
    /* find sign */
    sign  = sgn(x);
    if	( sign < 0 )
        *d ++ = '-';
    x = fabs(x);
    
    if	( x >= 1E308 ) {
        *d = '\0';
        strcat(d, "INF");
        return;
    }
    else if	( x <= 1E-307 ) 	{
        *d = '\0';
        strcat(d, "0");
        return;
    }
    
    /* find power */
    if	( x < minx )	{
        for ( i = 37; i >= 0; i -- )	{
            if	( x < nfta_eminus[i] )	{
                x *= nfta_eplus[i];
                power = -((i+1) * 8);
            }
            else
                break;
        }
        
        while ( x < 1.0 && power > -307 )	{
            x *= 10.0;
            power --;
        }
    }
    else if ( x > maxx )	{
        for ( i = 37; i >= 0; i -- )	{
            if	( x > nfta_eplus[i] )	{
                x /= nfta_eplus[i];
                power = ((i+1) * 8);
            }
            else
                break;
        }
        
        while ( x >= 10.0 && power < 308 )	{
            x /= 10.0;
            power ++;
        }
    }
    
    /* format left part */
    ipart = fabs(fint(x));
    fpart = fround(frac(x), FMT_RND) * FMT_xRND;
    if	( fpart >= FMT_xRND )	{	/* rounding bug */
        ipart = ipart + 1.0;
        if	( ipart >= maxx )	{
            ipart = ipart / 10.0;
            power ++;
        }
        fpart = 0.0;
    }
    
    fptoa(ipart, buf);
    strcpy(d, buf);
    d += strlen(buf);
    
    if	( fpart > 0.0 )	{
        /* format right part */
        *d ++ = '.';
        
        fdif = fpart;
        while ( fdif < FMT_xRND2 )	{
            fdif *= 10;
            *d ++ = '0';
        }
        
        fptoa(fpart, buf);
        rmzeros(buf);
        strcpy(d, buf);
        d += strlen(buf);
    }
    
    if	( power )	{
        /* add the power */
        *d ++ = 'E';
        if	( power > 0 )
            *d ++ = '+';
        fptoa(power, buf);
        strcpy(d, buf);
        d += strlen(buf);
    }
    
    /* finish */
    *d = '\0';
}

/*
*	best float to string (user)
*/
static void	bestfta(double x, char *dest)
{
    bestfta_p(x, dest, FMT_xMIN, FMT_xMAX);
}

/*
* 	float to string (user, E mode)
*/
static void	expfta(double x, char *dest)
{
    bestfta_p(x, dest, 10.0, 10.0);
    if	( strchr(dest, 'E') == NULL )
        strcat(dest, "E+0");
}

/*
*	format: map number to format
*
*	dir = direction, 1 = left to right, -1 right to left
*/
static void	fmt_nmap(int dir, char *dest, char *fmt, char *src)
{
    char	*p, *d, *s;
    
    *dest = '\0';
    if	( dir > 0 )	{
        /*	left to right */
        p = fmt;
        d = dest;
        s = src;
        while ( *p )	{
            switch ( *p )	{
            case '#':
            case '^':
                if	( *s )
                    *d ++ = *s ++;
                break;
            case '0':
                if	( *s )
                    *d ++ = *s ++;
                else
                    *d ++ = '0';
                break;
            default:
                *d ++ = *p;
            }
            
            p ++;
        }
        
        *d = '\0';
    }
    else	{
        /*	right to left */
        p = fmt+(strlen(fmt)-1);
        d = dest+(strlen(fmt)-1);
        *(d+1) = '\0';
        s = src+(strlen(src)-1);
        while ( p >= fmt )	{
            switch ( *p )	{
            case '#':
            case '^':
                if	( s >= src )
                    *d -- = *s --;
                else
                    *d -- = ' ';
                break;
            case '0':
                if	( s >= src )
                    *d -- = *s --;
                else
                    *d -- = '0';
                break;
            default:
                if	( *p == ',' )	{
                    if	( s >= src )	{
                        if	( *s == '-' )
                            *d -- = *s --;
                        else
                            *d -- = *p;
                    }
                    else
                        *d -- = ' ';
                }
                else
                    *d -- = *p;
            }
            
            p --;
        }
    }
}

/*
*	format: map number-overflow to format
*/
static void	fmt_omap(char *dest, const char *fmt)
{
    char	*p = (char *) fmt;
    char	*d = dest;
    
    while ( *p )	{
        switch ( *p )	{
        case	'#':
        case	'0':
        case	'^':
            *d ++ = '*';
            break;
        default:
            *d ++ = *p;
        }
        
        p ++;
    }
    *d = '\0';
}

/*
*	format: count digits
*/
static int		fmt_cdig(char *fmt)
{
    char	*p = fmt;
    int		count = 0;
    
    while ( *p )	{
        switch ( *p )	{
        case	'#':
        case	'0':
        case	'^':
            count ++;
            break;
        }
        
        p ++;
    }
    
    return count;
}

/*
*	format: format a number
*
*	symbols:
*		# = digit or space
*		0 = digit or zero
*		^ = exponential digit/format
*		. = decimal point
*		, = thousands
*		- = minus for negative
*		+ = sign of number
*/
static int format_num(char *dest, const char *fmt_cnst, double x)
{
    char	*p, *fmt;
    char	left[64], right[64];
    char	lbuf[64], rbuf[64];
    int		dp = 0, lc = 0, sign = 0;
    int		rsz, sco;
    char c;
    double sng;
    
    /* backup of format */
    fmt = (char*)malloc(strlen(fmt_cnst)+1);
    strcpy(fmt, fmt_cnst);
    
    
    if	( strchr(fmt_cnst, '^') )	{
        /*	E format */
        p = fmt;
        while (*p) {
            if (*p == '^')
                *p= '#';
            ++p;
        }
        sco = strcspn(fmt, "-+");
        if (sco < (int)strcspn(fmt, ".0#"))
            sco = 0;
        else
            sco = 1;
        if (x < 0.0) {
            x = -x;
            sng = -1.0;
        }
        else {
            sng = 1;
            sco = 0;
        }
        lc = fmt_cdig(fmt);
        p = strchr(fmt, '.');
        if (p)
            dp = fmt_cdig(p + 1);
        else
            dp = 0;
        lc -= dp;
        lc -= sco;
        if (lc < 0)
            lc = 0;
        rsz = (int)log10(x);
        x = x / pow(10, rsz);
        x *= pow(10, lc - 1);
        rsz -= lc - 1;
        format_num(dest, fmt, x * sng);
        c = '\0';
        if (strlen(dest)) {
            c = dest[strlen(dest) - 1];
        }
        p = dest + strlen(dest);
        if (c == '-' || c == '+')
            --p;
        else
            c = '\0';
        sprintf(p, "E%+04d%c", rsz, c);

    }
    else	{
        /* check sign */
        if	( strchr(fmt, '-') || strchr(fmt, '+') )	{
            sign = 1;
            if	( x < 0.0 )	{
                sign = -1;
                x = -x;
            }
        }
        /*	normal format */
        
        /* rounding */
        p = strchr(fmt, '.');
        if	( p )	
            x = fround(x, fmt_cdig(p+1));
        else
            x = fround(x, 0);
        
        /* convert */
        bestfta(x, dest);
        if	( strchr(dest, 'E') )	{
            fmt_omap(dest, fmt);
            free(fmt);
            return strlen(dest);
        }
        
        /* left & right parts */
        left[0] = right[0] = '\0';
        p = strchr(dest, '.');
        if	( p )	{
            *p = '\0';
            strcpy(right, p+1);
        }
        strcpy(left, dest);
        
        /* map format */
        rbuf[0] = lbuf[0] = '\0';
        p = strchr(fmt, '.');
        if	( p )	{
            dp = 1;
            *p = '\0';
            fmt_nmap(1, rbuf, p+1, right);
        }
        
        lc = fmt_cdig(fmt);
        if	( lc < (int)strlen(left) )	{
            fmt_omap(dest, fmt_cnst);
            free(fmt);
            return strlen(dest);
        }
        fmt_nmap(-1, lbuf, fmt, left);
        
        strcpy(dest, lbuf);
        if	( dp )	{
            strcat(dest, ".");
            strcat(dest, rbuf);
        }
       /* sign in format */
        if	( sign )	{
            p = strchr(dest, '+');
            if	( p )	
                *p = (sign > 0) ? '+' : '-';
        
            p = strchr(dest, '-');
            if	( p )	
                *p = (sign > 0) ? ' ' : '-';
        }
    }
    
    
    /* cleanup */
    free(fmt);
    return strlen(dest);
}


/**FORMAT
=section string
=title FORMAT()
=display FORMAT()

The function format accepts variable number of arguments. The first argument is a format string and the
rest of the arguments are used to create the result string according to the format string. This way the function
T<format> is like the C function T<sprintf>.

The format string can contain normal characters and control substrings.

The control substring have the form T<%[flags][width][.precision]type>. It follows the general T<sprintf> format
except that type prefixes are not required or allowed and type can only be "dioxXueEfgGsc". The T<*>
for width and precision is supported.

An alternate format BASIC-like for numbers has the form T<%~format~> where T<format> can be:

# Digit or space

0 Digit or zero

^ Stores a number in exponential format.
Unlike QB's USING format this is a place-holder like the #.

. The position of the decimal point.

, Separator.

- Stores minus if the number is negative.

+ Stores the sign of the number.

Acknowledgement: the function T<format> was implemented by Paulo Soares <psoares@consiste.pt>
*/
COMMAND(FORMAT)
#if NOTIMP_FORMAT
NOTIMPLEMENTED;
#else
    unsigned long cParameters;
    unsigned long iArg;
    char* ptr,*p;
    long size;
    formatParams params;
    VARIABLE vFormat,*pvArgs;
    NODE nItem;
    char fmt[128];

    /* this is an operator and not a command, therefore we do not have our own mortal list */
    USE_CALLER_MORTALS;

    /* evaluate the parameter */
    nItem = PARAMETERLIST;
    vFormat = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
    ASSERTOKE;
    
    /* count the number of parameters */
    nItem = CDR(nItem);
    cParameters = 0;
    while( nItem ){
        cParameters ++;
        nItem = CDR(nItem);
    }
    if( cParameters ){
        pvArgs = ALLOC(sizeof(VARIABLE)*cParameters);
        if( pvArgs == NULL )
            ERROR(COMMAND_ERROR_MEMORY_LOW);
    }
    else
        pvArgs = NULL;
    
    /* evaluate the parameters and store the result in the pvArgs array */
    nItem = CDR(PARAMETERLIST);
    iArg = 0;
    while( nItem ){
        pvArgs[iArg] = EVALUATEEXPRESSION(CAR(nItem));
        /* check that the expression was evaluated without error */
        if( iErrorCode ){
            FREE(pvArgs);
            ERROR(iErrorCode);
        }
        nItem = CDR(nItem);
        iArg ++;
    }
    iArg = 0;
    params.buf = (char*)malloc(CHUNK_SIZE);
    params.bufSize = CHUNK_SIZE;
    params.bufPtr = 0;
    ptr = STRINGVALUE(vFormat);
    size = STRLEN(vFormat);
    while (size > 0) {
        char c;
        char* pFound = memchr(ptr, '%', size);
        if (pFound == NULL) {
            CHECK_MEM(size);
            memcpy(params.buf + params.bufPtr, ptr, size);
            params.bufPtr += size;
            break;
        }
        CHECK_MEM(pFound - ptr);
        memcpy(params.buf + params.bufPtr, ptr, pFound - ptr);
        params.bufPtr += pFound - ptr;
        size -= pFound - ptr;
        --size;
        ptr = pFound + 1;
        if (size && *ptr == '~') {
            --size;
            ++ptr;
            p = memchr(ptr, '~', size);
            if (!p || (p - ptr) >= sizeof(fmt)) {
                FORMAT_SYNTAX_ERROR
            }
            memcpy(fmt, ptr, (p - ptr));
            fmt[p - ptr] = '\0';
            ++p;
            size -= p - ptr;
            ptr = p;
            CHECK_MEM(128 + 32);
            ASSERT_PARAMETER_COUNT
            pvArgs[iArg] = CONVERT2DOUBLE(pvArgs[iArg]);
            params.bufPtr += format_num(params.buf + params.bufPtr, fmt, DOUBLEVALUE(pvArgs[iArg]));
            ++iArg;
            continue;
        }
        params.flags = 0;
        params.prec = -1;
        params.width = -1;
        while (size-- > 0) {
            c = *(ptr++);
            switch (c) {
            case ' ':
                params.flags |= F_BLANK;
                continue;
            case '#':
                params.flags |= F_SHARP;
                continue;
            case '-':
                params.flags |= F_MINUS;
                continue;
            case '+':
                params.flags |= F_PLUS;
                continue;
            case '0':
                params.flags |= F_ZERO;
                continue;
            }
            break;
        }
        if (c == '*') {
            ASSERT_PARAMETER_COUNT
            pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
            params.width = LONGVALUE(pvArgs[iArg]);
            if (params.width < 0) {
                params.width = -params.width;
                params.flags |= F_MINUS;
            }
            ++iArg;
            if (size-- > 0)
                c = *(ptr++);
        }
        else if (isdigit(c)) {
            params.width = c - '0';
            while (size-- > 0) {
                c = *(ptr++);
                if (!isdigit(c))
                    break;
                params.width = params.width * 10 + (c - '0');
            }
        }
        if (c == '.') {
            params.prec = 0;
            if (size-- > 0)
                c = *(ptr++);
            if (c == '*') {
                ASSERT_PARAMETER_COUNT
                pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
                params.prec = LONGVALUE(pvArgs[iArg]);
                if (params.prec < 0)
                    params.prec = 0;
                ++iArg;
                if (size-- > 0)
                    c = *(ptr++);
            }
            else if (isdigit(c)) {
                params.prec = c - '0';
                while (size-- > 0) {
                    c = *(ptr++);
                    if (!isdigit(c))
                        break;
                    params.prec = params.prec * 10 + (c - '0');
                }
            }
        }
        if (size < 0) {
            FORMAT_SYNTAX_ERROR
        }
        params.type = c;
        switch (c) {
        case 'd':
        case 'i':
        case 'o':
        case 'X':
        case 'x':
        case 'u':
            ASSERT_PARAMETER_COUNT
            pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
            params.vLong = LONGVALUE(pvArgs[iArg]);
            ++iArg;
            CHECK_OPERATION(printInt(&params))
            break;
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
            ASSERT_PARAMETER_COUNT
            pvArgs[iArg] = CONVERT2DOUBLE(pvArgs[iArg]);
            params.vDouble = DOUBLEVALUE(pvArgs[iArg]);
            ++iArg;
            CHECK_OPERATION(printDouble(&params));
            break;
        case '%':
            CHECK_MEM(1);
            params.buf[params.bufPtr++] = c;
            break;
        case 'c':
            ASSERT_PARAMETER_COUNT
            params.prec = 1;
            params.vSize = 1;
            pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
            c = (char)LONGVALUE(pvArgs[iArg]);
            params.vString = &c;
            ++iArg;
            CHECK_OPERATION(printChar(&params));
            break;
        case 's':
            ASSERT_PARAMETER_COUNT
            pvArgs[iArg] = CONVERT2STRING(pvArgs[iArg]);
            params.vSize = STRLEN(pvArgs[iArg]);
            params.vString = STRINGVALUE(pvArgs[iArg]);
            ++iArg;
            CHECK_OPERATION(printChar(&params));
            break;
        default:
            FORMAT_SYNTAX_ERROR
        }
    }
    FREE(pvArgs);
    RESULT = NEWMORTALSTRING(params.bufPtr);
    if (RESULT == NULL) {
        free(params.buf);
        ERROR(COMMAND_ERROR_MEMORY_LOW);
    }
    memcpy(STRINGVALUE(RESULT), params.buf, params.bufPtr);
    free(params.buf);
    RETURN;
error_escape:
    FREE(pvArgs);
    if (params.buf)
        free(params.buf);
    ERROR(iErrorCode);

#endif
END

/*
This function is used to calculate the stored length of a sting in the
command pack.

The command pack can store string so that the length of the string is
stored on 1, 2 .., 8 bytes. If the string is longer than the length storable
on the number of bytes the string is stored truncated. This function returns
the original length or the truncated length.
*/
static unsigned long TruncatedLength(int lLen, unsigned long iArgStr){
  /* take care of long and truncated strings */
  switch( lLen ){
    case 1: if( iArgStr > 0xFF )
              iArgStr =   0xFF; break;
    case 2: if( iArgStr > 0xFFFF )
              iArgStr =   0xFFFF; break;
    case 3: if( iArgStr > 0xFFFFFF )
              iArgStr =   0xFFFFFF; break;
    case 4: if( iArgStr > 0xFFFFFFFF )
              iArgStr =   0xFFFFFFFF; break;
/*
      NOTE that 32bit architectures will complain about truncation.
*/
#pragma warning (disable:4305)
    case 5: if( iArgStr > (unsigned long)0xFFFFFFFFFF )
              iArgStr =   (unsigned long)0xFFFFFFFFFF; break;
    case 6: if( iArgStr > (unsigned long)0xFFFFFFFFFFFF )
              iArgStr =   (unsigned long)0xFFFFFFFFFFFF; break;
    case 7: if( iArgStr > (unsigned long)0xFFFFFFFFFFFFFF )
              iArgStr =   (unsigned long)0xFFFFFFFFFFFFFF; break;
/* Does anyone know any 128bit architecture with ScriptBasic running on it?
    case 8: if( iArgStr > (unsigned long)0xFFFFFFFFFFFFFFFF )
              iArgStr =   (unsigned long)0xFFFFFFFFFFFFFFFF; break; */
#pragma warning (default:4305)
    }
  return iArgStr;
  }

/* just a piece of code that we use lots of times */
#define GETNPARAM lLen = 0; fLen = 0;\
          while( iStr < STRLEN(vFormat) && isdigit(STRINGVALUE(vFormat)[iStr]) ){\
            lLen = 10*lLen + STRINGVALUE(vFormat)[iStr] - '0';\
            fLen=1;\
            iStr++;\
            }

/**PACK
=section string
=display PACK()
=title pack("format",v1,v2,...,vn)

Pack list of arguments into a binary string.

The format strings can contain the packing control literals.
Each of these characters optionally take the next argument and
convert to the specific binary string format. The result is the
concatenated sum of these strings.

Some control characters do not take argument, but result a constant
string by their own.

=itemize

=item T<SZ> the argument is stored as zero terminated string. If the argument
already contains zchar that is taken as termnator and the rest of the
string is ignored.

=item T<S1> the argument is stored as a string. One byte length and maximum 255
byte strings. If the argument longer than 255 bytes only the first 255 bytes
are used, and the rest is ignored.

=item T<S2> same as T<S1> but with two bytes for the length.

=item T<S3> same as T<S1> but with three bytes for the length.

=item T<S4> same as T<S1> but with four bytes for the length.

=item T<S5..8> the same as T<S1> but with 5..8 bytes for the length.

=item T<Zn> one or more zero characters, does not take argument. T<n> can be
   1,2,3 ... positive numbers

=item T<In> integer number stored on n bytes. Low order byte first. If the number does not
   fit into n bytes the higher bytes are chopped. If the number is negative the
   high overflow bytes are filled with FF.

=item T<C> character (same as T<I1>)

=item T<Un> same as T<In> but for unsigned numbers.

=item T<An> store the argument as string on n bytes. If the argument is longer than
   n bytes only the first n bytes are stored. If the argument is shorter than
   n bytes the higher bytes are filled with space.

=item T<R>  a real number.
=noitemize

See also R<UNPACK>
*/
COMMAND(PACK)
#if NOTIMP_PACK
NOTIMPLEMENTED;
#else
  NODE nItem;
  VARIABLE vFormat,*pvArgs;
  unsigned long cParameters,cbResult;
  unsigned long iArg,iStr,iArgStr,iResult;
  unsigned long lLen,lLenS;
  long lParam;
  unsigned long uParam;
  int fLen;
  char cChar;
  double dParam;
  unsigned char *pszD;

  /* this is a function and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  /* evaluate the parameter */
  nItem = PARAMETERLIST;
  vFormat = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;

  /* count the number of parameters */
  nItem = CDR(nItem);
  cParameters = 0;
  while( nItem ){
    cParameters ++;
    nItem = CDR(nItem);
    }
  if( cParameters ){
    pvArgs = ALLOC(sizeof(VARIABLE)*cParameters);
    if( pvArgs == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    }else pvArgs = NULL;

  /* evaluate the parameters and store the result in the pvArgs array */
  nItem = CDR(PARAMETERLIST);
  iArg = 0;
  while( nItem ){
    pvArgs[iArg] = EVALUATEEXPRESSION(CAR(nItem));
    /* check that the expression was evaluated without error */
    if( iErrorCode ){
      FREE(pvArgs);
      ERROR(iErrorCode);
      }
    nItem = CDR(nItem);
    iArg ++;
    }

  /* calculate the length of the result string */
  iStr = 0;
  iArg = 0;
  cbResult = 0;
  while( iStr < STRLEN(vFormat) ){
    switch( cChar = STRINGVALUE(vFormat)[iStr] ){
      case 'S':
         if( iArg < cParameters )
           pvArgs[iArg] = CONVERT2STRING(pvArgs[iArg]);
         iStr ++;
         switch( iStr < STRLEN(vFormat) ? STRINGVALUE(vFormat)[iStr] : (char)0 ){
           case 'Z':
             iArgStr = 0;
             /* non-existing parameter and undef is zero length string */
             if( iArg < cParameters )
               while( iArgStr < STRLEN(pvArgs[iArg]) ){
                 if( ! STRINGVALUE(pvArgs[iArg])[iArgStr] )break;
                 iArgStr++;
                 }
             iArgStr++; /* count the terminating zero */
             cbResult += iArgStr;
             iArg++; /* step for the next argument */
             iStr ++;
             break;
           case '1': case '2': case '3': case '4':
           case '5': case '6': case '7': case '8':
             iArgStr = TruncatedLength(STRINGVALUE(vFormat)[iStr] - '0',
                                       iArg < cParameters ? STRLEN(pvArgs[iArg]) : 0 );
             cbResult += STRINGVALUE(vFormat)[iStr] - '0';
             if( iArg < cParameters )/* non-existing parameter */
               cbResult += iArgStr;  /* and undef is 
                                            zero length string */
             iStr ++;
             iArg++;
             break;
           default : /* a single S without sub cast */
             cbResult += 2 + STRLEN(pvArgs[iArg]);
             iArg++;
             break;
           }
        break;
      case 'Z': /* fixed length string filled zero */
      case 'A': /* fixed length string             */
      case 'I': /* fixed length signed integer     */
      case 'U': /* fixed length unsigned integer   */
        iStr++;
        GETNPARAM
        /* if there is a length parameter then use it */
        if( fLen )
          cbResult += lLen;
        else /* if there was no length parameter use the default */
          switch( cChar ){
            case 'Z' : cbResult ++; break; /* Z is same as Z1 */
            case 'A' : cbResult += 20; break; /* A is same as A20 */
            case 'U' : /*--------------------------------------------------*/
            case 'I' : cbResult += 8; break; /* 64 bit long is the default */
            }
        /* take care of the argument, convert to the proper format and
           also step the argument index variable iArg */
        switch( cChar ){
          case 'A' :
            if( iArg < cParameters )
              pvArgs[iArg] = CONVERT2STRING(pvArgs[iArg]);
            iArg++;
            break;
          case 'U' :
          case 'I' :
            if( iArg < cParameters )
              pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
            iArg++;
            break;
          }
        break;
      case 'R': /* a double */
        iStr++;
        if( iArg < cParameters )
          pvArgs[iArg] = CONVERT2DOUBLE(pvArgs[iArg]);
        iArg++; /* go for the next argument */
        cbResult += sizeof(double);
        break;

      case 'C': /* a single character */
        iStr++;
        if( iArg < cParameters )
          pvArgs[iArg] = CONVERT2LONG(pvArgs[iArg]);
        iArg++; /* go for the next argument */
        cbResult++;
        break;
      /* any other character is ignored */
      default: iStr++; break;
      }
    }

  /* allocate space for the result string */
  RESULT = NEWMORTALSTRING(cbResult);
  if( RESULT == NULL ){
    FREE(pvArgs);
    ERROR(COMMAND_ERROR_MEMORY_LOW);
    }

  /* create the result string */
  iStr = 0;
  iArg = 0;
  iResult = 0;
#define NEXTCHAR STRINGVALUE(RESULT)[ iResult < cbResult ? iResult++ : cbResult ]
  while( iStr < STRLEN(vFormat) ){
    switch( cChar = STRINGVALUE(vFormat)[iStr] ){
      case 'S':
         iStr ++;
         switch( iStr < STRLEN(vFormat) ? STRINGVALUE(vFormat)[iStr] : (char)0 ){
           case 'Z':
             iArgStr = 0;
             if( iArg < cParameters )
               while( iArgStr < STRLEN(pvArgs[iArg]) ){
                 if( ! STRINGVALUE(pvArgs[iArg])[iArgStr] )break;
                 NEXTCHAR = STRINGVALUE(pvArgs[iArg])[iArgStr];
                 iArgStr++;
                 }
             NEXTCHAR = (char)0;
             iArgStr++; /* count the terminating zero */
             iArg++; /* step for the next argument */
             iStr ++;
             break;
           case '1': case '2': case '3': case '4':
           case '5': case '6': case '7': case '8':
             iArgStr = TruncatedLength((lLen=STRINGVALUE(vFormat)[iStr] - '0'),
                                       iArg < cParameters ? STRLEN(pvArgs[iArg]) : 0 );
             lLenS = iArgStr;
             /* store the length in the result */
             while( lLen-- ){
               NEXTCHAR = (unsigned char)iArgStr & 0xFF;
               iArgStr /= 0x100;
               }
             /* store the characters in the result */
             iArgStr = 0;
             while( iArgStr < lLenS )
               NEXTCHAR = STRINGVALUE(pvArgs[iArg])[iArgStr++];
             iStr ++;
             iArg++;
             break;
           default : /* a single S without sub cast */
             lLen = 2;
             if( iArg < cParameters )
               iArgStr = STRLEN(pvArgs[iArg]);
             else
               iArgStr = 0;
             if( iArgStr > 0xFFFF )iArgStr = 0xFFFF;
             lLenS = iArgStr;
             while( lLen-- ){
               NEXTCHAR = (unsigned char)iArgStr & 0xFF;
               iArgStr /= 0x100;
               }
             while( iArgStr < lLenS )
               NEXTCHAR = STRINGVALUE(pvArgs[iArg])[iArgStr++];
             iStr ++;
             iArg++;
             break;
           }
        break;
      case 'Z': /* fixed length string filled zero */
        iStr++;
        GETNPARAM
        /* if there is a length parameter then use it */
        if( ! fLen )lLen = 1;
        while( lLen-- )
          NEXTCHAR = (char)0;
        break;
      case 'A': /* fixed length string */
        iStr++;
        GETNPARAM
        /* if there is a length parameter then use it */
        if( ! fLen )lLen = 20;
        iArgStr = 0;
        if( iArg < cParameters )
          lLenS = STRLEN(pvArgs[iArg]);
        else
          lLenS = 0;
        while( lLen && iArgStr < lLenS ){
          NEXTCHAR = STRINGVALUE(pvArgs[iArg])[iArgStr++];
          lLen--;
          }
        while( lLen ){
          NEXTCHAR = ' ';
          lLen--;
          }
        iArg++;
        break;
      case 'I': /* fixed length signed integer   */
        iStr++;
        GETNPARAM
        /* if there is a length parameter then use it */
        if( ! fLen )lLen = 8;
        if( iArg < cParameters && pvArgs[iArg] )
          lParam = LONGVALUE(pvArgs[iArg]);
        else
          lParam = 0;
        uParam = (unsigned)lParam;
        while( lLen-- ){
          if( uParam == 0 )
            if( lParam < 0 )
              NEXTCHAR = (unsigned char)0xFF;
            else
              NEXTCHAR = (unsigned char)0x00;
          else
            NEXTCHAR = (unsigned char)( uParam & 0xFF );
          uParam /= 256;
          }
        iArg++;
        break;
      case 'U': /* fixed length unsigned integer */
        iStr++;
        GETNPARAM
        /* if there is a length parameter then use it */
        if( ! fLen )lLen = 8;
        if( iArg < cParameters && pvArgs[iArg])
          uParam = LONGVALUE(pvArgs[iArg]);
        else
          uParam = 0;
        while( lLen-- ){
          NEXTCHAR = (unsigned char)( uParam & 0xFF );
          uParam /= 256;
          }
        iArg++;
        break;
      case 'R':
         iStr++;
         if( iArg < cParameters && pvArgs[iArg])
          dParam = DOUBLEVALUE(pvArgs[iArg]);
        else
          dParam = 0.0;
        lLen = sizeof(double);
        pszD = (unsigned char *)&dParam;
        while( lLen-- ){
          NEXTCHAR = *pszD++;
          }
        iArg++;
        break;
      case 'C': /* a single character */
        iStr++;
        if( iArg < cParameters && pvArgs[iArg])
          uParam = LONGVALUE(pvArgs[iArg]);
        else
          uParam = 0;
        NEXTCHAR = (unsigned char)( uParam & 0xFF );
        iArg++; /* go for the next argument */
        break;
      default: iStr++; break;
      }
    }

#endif
END

#define GETLEFTVALUE if( nItem ){LeftValue = EVALUATELEFTVALUE_A(CAR(nItem));\
                     ASSERTOKE;\
                     DEREFERENCE(LeftValue);\
                     if( *LeftValue != NULL )\
                       memory_ReleaseVariable(pEo->pMo,*LeftValue);\
                     nItem = CDR(nItem);}else LeftValue = NULL;

/**UNPACK
=section string
=display UNPACK
=title UNPACK string BY format TO v1,v2,...,vn

Unpack the binary string T<string> using the format string into the variables. The format string
should have the same format as the format string the in the function R<PACK>.
*/
COMMAND(UNPACK)
#if NOTIMP_UNPACK
NOTIMPLEMENTED;
#else
  NODE nItem;
  VARIABLE vRecord,vFormat;
  LEFTVALUE LeftValue;
  unsigned long lLen,iStr,iRec,lLenS,lMag,i;
  int fLen,iThisChar;
  long refcount;

  vRecord = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;
  vFormat = CONVERT2STRING(EVALUATEEXPRESSION(PARAMETERNODE));
  ASSERTOKE;
  NEXTPARAMETER;

  nItem = PARAMETERNODE;

  if( !memory_IsUndef(vRecord) && !memory_IsUndef(vFormat) ){
    iStr = 0;
    iRec = 0;
    while( iStr < STRLEN(vFormat) ){
      switch( STRINGVALUE(vFormat)[iStr] ){
        case 'R' : /* get a double */
          iStr++;
          GETLEFTVALUE
          if( LeftValue ){
            if( iRec + sizeof(double) <= STRLEN(vRecord) ){
              *LeftValue = NEWDOUBLE;
              if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
              memcpy(&(DOUBLEVALUE(*LeftValue)),STRINGVALUE(vRecord)+iRec,sizeof(double));
              }else *LeftValue = NULL;
            }
          iRec += sizeof(double);
          break;
        case 'A' : /* get n-character string from the record */
          iStr ++;
          GETNPARAM
          if( ! fLen )lLen = 20;
          GETLEFTVALUE
          if( LeftValue ){
            *LeftValue = NEWSTRING(lLen);
            if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
            for( i=0 ; i < lLen ; i++ )
              STRINGVALUE(*LeftValue)[i] = 
                iRec < STRLEN(vRecord) ? STRINGVALUE(vRecord)[iRec++] : (char)0;
            }
          break;
        case 'I' : /* get signed integer from the record */
          iStr++;
          GETNPARAM
          if( ! fLen )lLen = 8;/* as default we store the numbers on 64bit */
          lLenS = 0;
          lMag = 1;
           while( lLen -- ){
             if( iRec >= STRLEN(vRecord) )/* if the record finishes */
               break;
            iThisChar = (unsigned char)STRINGVALUE(vRecord)[iRec++];
            lLenS += lMag * iThisChar;
            lMag *= 0x100;
            }
          GETLEFTVALUE
          if( LeftValue ){
            *LeftValue = NEWLONG;
            if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
            LONGVALUE(*LeftValue) = lLenS;
            }
          break;
        case 'U' : /* get unsigned integer from the record */
          iStr++;
          GETNPARAM
          if( ! fLen )lLen = 8;/* as default we store the numbers on 64bit */
          lLenS = 0;
          lMag = 1;
           while( lLen -- ){
             if( iRec >= STRLEN(vRecord) )/* if the record finishes */
               break;
            iThisChar = (unsigned char)STRINGVALUE(vRecord)[iRec++];
            lLenS += lMag * iThisChar;
            lMag *= 0x100;
            }
          GETLEFTVALUE
          if( LeftValue ){
            *LeftValue = NEWLONG;
            if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
            LONGVALUE(*LeftValue) = lLenS;
            if( LONGVALUE(*LeftValue) < 0 )LONGVALUE(*LeftValue) = LONG_MAX;
            }
          break;
        case 'C' : /* get character code */
          iStr++;
          lLenS = 0;
          if( iRec < STRLEN(vRecord) )
            lLenS = (unsigned char)STRINGVALUE(vRecord)[iRec++];
          GETLEFTVALUE
          if( LeftValue ){
            *LeftValue = NEWLONG;
            if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
            LONGVALUE(*LeftValue) = lLenS;
            if( LONGVALUE(*LeftValue) < 0 )LONGVALUE(*LeftValue) = LONG_MAX;
            }
          break;
        case 'Z' : /* skip one or more zero characters */
          iStr++;
          GETNPARAM
          if( ! fLen )lLen = 1;
          while( lLen-- )iRec++; /* just skip the characters */
          break;
        case 'S' : /* get given number of bytes of string or Z terminated string */
          iStr++;
          switch( iStr < STRLEN(vFormat) ? STRINGVALUE(vFormat)[iStr] : (char)0 ){
            case 'Z': /* get zero terminated string */
              iStr ++;
              lLen = 0;
              while( lLen < STRLEN(vRecord) - iRec && 
                     STRINGVALUE(vRecord)[iRec+lLen] )lLen++;
              GETLEFTVALUE
              if( LeftValue ){
                *LeftValue = NEWSTRING(lLen);
                if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
                for( i=0 ; i < lLen ; i++ )
                  STRINGVALUE(*LeftValue)[i] = 
                    iRec < STRLEN(vRecord) ? STRINGVALUE(vRecord)[iRec++] : (char)0;
                }
              break;
            case '1' : case '2' : case '3' : case '4' :
            case '5' : case '6' : case '7' : case '8' :
              lLen = STRINGVALUE(vFormat)[iStr];
              lLenS = 0;
              lMag = 1;
              while( lLen -- ){
                if( iRec >= STRLEN(vRecord) ){/* if the record finishes before */
                  lLenS = 0;                  /* the length number             */
                  break;
                  }
                lLenS += lMag * STRINGVALUE(vRecord)[iRec++];
                lMag *= 0x100;
                }
              /* if the length indicated by the bytes is longer than the rest
                 of the record */
              if( lLenS > STRLEN(vRecord) - iRec )lLenS = STRLEN(vRecord) - iRec;
              GETLEFTVALUE
              if( LeftValue ){
                *LeftValue = NEWSTRING(lLenS);
                if( *LeftValue == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
                for( i=0 ; i < lLenS ; i++ )
                  STRINGVALUE(*LeftValue)[i] = 
                    iRec < STRLEN(vRecord) ? STRINGVALUE(vRecord)[iRec++] : (char)0;
                }
              break;
            }
          break;
        default: iStr++; break;/* ignore any other character */
        }
      }
    }

  /* make all arguments that were not filled undef */
  while( nItem ){
    GETLEFTVALUE
    if( LeftValue )*LeftValue = NULL;
    nItem = CDR(nItem);
    }
#endif
END

/**CONF
=section string
=display CONF()
=title CONF("conf.key")

This function can be used to retrieve ScriptBasic configuration parameters. This is rarely needed by general programmers.
This is needed only for scripts that maintain the ScriptBasic setup, for example install a new module copying the files
to the appropriate location.

The argument T<"conf.key"> should be the configuration key string. If this key is not on the top level
then the levels should be separated using the dot chatacter, like T<conf("preproc.internal.dbg")> to get the
debugger DLL or SO file.

The return value of the function is the integer, real or string value of the configuration value. If the key is not
defined or if the system manager set the key to be hidden (see later) then the function will raise an error 

T<(0): error &H8:The argument passed to a module function is out of the accepted range.>

Some of the configuration values are not meant to be readable for the BASIC programs for security reasons. A typical
example is the database connection password. The system manager can insert extra "dummy" configuration keys that will
prevent the BASIC program to get the actual value of the configuration key. The extra configuration key has to have
the same name as the key to be hidden with a T<$> sign prepended to it.

For example the MySQL connection named T<test>
has the connection password under the key T<mysql.connections.test.password>. If the key in the compiled configuration
file T<mysql.connections.test.$password> exists then the BASIC function T<conf()> will result error. The value of this
extra key is not taken into account.

The system manager can configure whole configuration branches to be hidden from the BASIC programs. For example the
configuration key T<mysql.connections.$test> defined with any value will prevent access of BASIC programs to any
argument of the connection named T<test>. Similarly the key T<mysql.$connections> will prevent access to any configuration
value of any MySQL connections if defined and finally the key T<$mysql> will stop BASIC programs to discover any MySQL
configuration information if defined.

The current implementation does not examine the actual value of the extra security key. However later implementations
may alter the behaviour of this function based on the value of the key. To remain compatible with later versions it is
recommended that the extra security key is configured to have the value T<1>.
*/
COMMAND(CONF)
#if NOTIMP_CONF
NOTIMPLEMENTED;
#else
  char *pszConf;
  long lConf;
  double dConf;
  int type;
  int iError;
  char *pszKey;
  char *pszSecKey;
  VARIABLE Argument;
  int i,j;

  Argument = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
  ASSERTOKE;
  Argument = CONVERT2STRING(Argument);

  CONVERT2ZCHAR(Argument,pszKey);
  pszSecKey = ALLOC(STRLEN(Argument)+2);
  if( pszSecKey == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);

  /* first check if the whole key is blocked */
  *pszSecKey = '$';
  strcpy(pszSecKey+1,pszKey);
  for( j=1 ; pszSecKey[j] ; j++ )
    if( pszSecKey[j] == '.' ){
      pszSecKey[j] = (char)0;
      break;
      }
  iError = cft_GetEx(pEo->pConfig,pszSecKey,NULL,&pszConf,&lConf,&dConf,&type);
  /* behave like if there was no such key in the configuration (may even happen that actually
     there is not btw) */
  if( iError == COMMAND_ERROR_SUCCESS )ERROR(COMMAND_ERROR_ARGUMENT_RANGE);

  /* check if there is any subkey made hidden */
  for( i=0 ; pszKey[i] ; i++ ){
    if( pszKey[i] == '.' ){
      strcpy(pszSecKey,pszKey);
      pszSecKey[i+1] = '$';
      strcpy(pszSecKey+i+2,pszKey+i+1);
      for( j=i+2 ; pszSecKey[j] ; j++ )
        if( pszSecKey[j] == '.' ){
        pszSecKey[j] = (char)0;
        break;
        }
      iError = cft_GetEx(pEo->pConfig,pszSecKey,NULL,&pszConf,&lConf,&dConf,&type);
      if( iError == COMMAND_ERROR_SUCCESS )ERROR(COMMAND_ERROR_ARGUMENT_RANGE);
      }
    }

  iError = cft_GetEx(pEo->pConfig,pszKey,NULL,&pszConf,&lConf,&dConf,&type);

  FREE(pszKey);

  if( iError || type == CFT_NODE_BRANCH )ERROR(COMMAND_ERROR_ARGUMENT_RANGE);

  switch( type ){
    case CFT_TYPE_STRING :
      RESULT = NEWMORTALSTRING(strlen(pszConf));
      ASSERTNULL(RESULT)
      memcpy(STRINGVALUE(RESULT),pszConf,STRLEN(RESULT));
      return;
    case CFT_TYPE_INTEGER:
      RESULT = NEWMORTALLONG;
      ASSERTNULL(RESULT)
      LONGVALUE(RESULT) = lConf;
      return;
    case CFT_TYPE_REAL   :
      RESULT = NEWMORTALDOUBLE;
      ASSERTNULL(RESULT)
      DOUBLEVALUE(RESULT) = dConf;
      return;
    default : ERROR(COMMAND_ERROR_ARGUMENT_RANGE);
    }

END
#endif

/**BIN
=section planned
=display BIN()

This is a planned function to convert the argument number to binary format.
(aka. format as a binary number containing only 0 and 1 characters and return
this string)
*/
COMMAND(BIN)
#if NOTIMP_BIN
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**CVD
=section planned
=display CVD()

This is a planned function to convert the argument string into a real number.

Converts a passed in string "str$" to a double-precision number. The passed string must be eight (8) bytes or longer. If less than 8 bytes long, an error is generated. If more than 8 bytes long, only the first 8 bytes are used.
*/
COMMAND(CVD)
#if NOTIMP_CVD
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**CVI
=section planned
=display CVI()

This is a planned function to convert the argument string into an integer.

Converts a passed in string "str$" to an integer number. The passed string must be two (2) bytes or longer. If less than 2 bytes long, an error is generated. If more than 2 bytes long, only the first 2 bytes are used.
*/
COMMAND(CVI)
#if NOTIMP_CVI
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**CVL
=section planned
=display CVL()

This is a planned function to convert the argument string into an integer.

Converts a passed in string "str$" to a long-integer number. The passed string must be four (4) bytes or longer. If less than 4 bytes long, an error is generated. If more than 4 bytes long, only the first 4 bytes are used.
*/
COMMAND(CVL)
#if NOTIMP_CVL
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**CVS
=section planned
=display CVS()

This is a planned function to convert the argument string into an integer.

Converts a passed in string "str$" to a single precision number. The passed string must be four (4) bytes or longer. If less than 4 bytes long, an error is generated. If more than 4 bytes long, only the first 4 bytes are used.
*/
COMMAND(CVS)
#if NOTIMP_CVS
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MKD
=section planned
=display MKD()

This is a planned function to convert the argument real number to an 8 byte string.

Converts the double-precision number "n" into an 8-byte string so it can later be retrieved from a random-access file as a numeric value.

*/
COMMAND(MKD)
#if NOTIMP_MKD
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MKI
=section planned
=display MKI()

This is a planned function to convert the argument integer number to an 2 byte string.

Converts the integer number "n" into an 2-byte string so it can later be retrieved from a random-access file as a numeric value.
*/
COMMAND(MKI)
#if NOTIMP_MKI
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MKS
=section planned
=display MKS()

This is a planned function.

Converts the single-precision number "n" into an 4-byte string so it can later be retrieved from a random-access file as a numeric value.
*/
COMMAND(MKS)
#if NOTIMP_MKS
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

/**MKL
=section planned
=display MKL()

This is a planned function.

Converts the long-integer number "n" into an 4-byte string so it can later be retrieved from a random-access file as a numeric value.
*/
COMMAND(MKL)
#if NOTIMP_MKL
NOTIMPLEMENTED;
#else
NOTIMPLEMENTED;
#endif
END

