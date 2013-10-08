/*
FILE:   match.c
HEADER: match.h

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

TO_HEADER:

#define JOKERNR 13

#define NOJOKER     0
#define SINGLEJOKER 1
#define MULTIJOKER  3

typedef struct _MatchSets {
  unsigned char SetType[JOKERNR];
  unsigned char set[JOKERNR][32];
  } MatchSets, *pMatchSets;

*/
#include <ctype.h>
#include <string.h>
#include "match.h"
#include "errcodes.h"

/*POD
@c Simple Pattern Matching

=abstract
A simple, non-regular expression pattern matching module mainly to perform
file name pattern matching, like T<*.txt> or T<file0?.bin> and alikes.
=end

This is a simple and fast pattern matching algorithm.
This can be used when the matching does not require
regular expression complexity and the processign on 
the other hand should be fast.

There are two major tasks implemented here. One is to match a string against a pattern.
The second is to create a replacement string. When a pattern is matched by a string an
array of string values are created. Each contains a substring that matches a joker character.
Combining this array and a format string a replacement string can be created.

For example:

=verbatim

String = "mortal  combat"
Pattern = "mo?tal co*"

=noverbatim

the joker characters are the ?, the space (matching one or more space) and the * character.
They are matched by T<r>, two spaces and T<mbat>. If we use the format string

=verbatim
Format string = "$1u$2"
=noverbatim

we get the result string T<rumbat>. The format string can contain T<$n> placeholders
where T<n> starts with 1 and is replaced by the actual value of the n-th joker character.


CUT*/

#define ESCAPE_CHARACTER '~'

/* the characters that can be joker character */
static char JokerCharacter[] = "*#$@?&%!+/|<>";

#define F 0xFF
#define E 0xFE
static MatchSets DefaultMatchSets = {
/*  *           #           $           @           ? */
    { MULTIJOKER, MULTIJOKER, MULTIJOKER, MULTIJOKER, SINGLEJOKER, 
/*  &        %        !        +        /        |        <        > */
      NOJOKER, NOJOKER, NOJOKER, NOJOKER, NOJOKER, NOJOKER, NOJOKER, NOJOKER },
     /*                            1  1  1  1  1  1  1  1  1  1  2  2  2  2  2  2  2  2  2  2  3  3  3 */
    {/* 1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1  2 */

/*      0  8  16 24 32 40 48 56 64 72 80 88 96 104
*/
/***/ { F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F} ,
/*#*/ { 0, 0, 0, 0, 0, 0, F, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*$*/ { 0, 0, 0, 0, 0, 0, F, 3, E, F, F, 7, E, F, F, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*@*/ { 0, 0, 0, 0, 0, 0, 0, 0, E, F, F, 7, E, F, F, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*?*/ { F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F} ,
/*&*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*%*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*!*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*+*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*/*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*|*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*<*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
/*>*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    }
  };
#undef F
#undef E

/*POD
=H match_index
@c Return the joker index of the character

There are a few characters that can be used as joker character. These are

=verbatim
*#$@?&%!+/|<>
=noverbatim

T<match_index> returns the serial number of the character.
/*FUNCTION*/
unsigned long match_index(char ch
){
/*noverbatim
CUT*/
  int i;
  for( i = 0 ; JokerCharacter[i] ; i++ )
    if( JokerCharacter[i] == ch )return i+1;

  return 0;
  }

static unsigned char *MultiJokerSet(pMatchSets pMS, char ch){
  int i;

  if( ! (i = match_index(ch)) )return NULL;
  i--;
  if( pMS->SetType[i] == MULTIJOKER )return pMS->set[i];
  return NULL;
  }

static unsigned char *SingleJokerSet(pMatchSets pMS, char ch){
  int i;

  if( !(i = match_index(ch)) )return NULL;
  i--;
  if( pMS->SetType[i] == SINGLEJOKER )return pMS->set[i];
  return NULL;
  }

static int JokerMatch(unsigned char *set, unsigned char ch){
  int index,offset;

  index = ch >> 3;
  offset = ch & 7;
  index = set[index];
  index >>= offset;
  return index;
  }
/*POD
=H InitSets
@c Initialize a set collection

Call this function to initialize a set collection. The argument should point to
a T<MatchSets> structure and the function fills in the default values.

/*FUNCTION*/
void match_InitSets(pMatchSets pMS
  ){
/*noverbatim
CUT*/
  memcpy((void *)pMS,(void *)&DefaultMatchSets,sizeof(DefaultMatchSets));
  }

/*POD
=H ModifySet
@c Modify a joker set

This function can be used to modify a joker set. The first argument T<pMS> points to the
joker set collection. The second argument T<JokerCharacter> specifies the joker character
for which the set has to be modified.

The argument T<nChars> and T<pch> give the characters that are to be modified in the set.
T<nChars> is the number of characters in the character array pointed by T<pch>.

The last argument T<fAction> specifies what to do. The following constants can be used in
logical OR.

=verbatim
TO_HEADER:

#define MATCH_ADDC 0x0001 //add characters to the set
#define MATCH_REMC 0x0002 //remove characters from the set
#define MATCH_INVC 0x0004 //invert the character
#define MATCH_SNOJ 0x0008 //set becomes no-joker
#define MATCH_SSIJ 0x0010 //set becomes single joker
#define MATCH_SMUJ 0x0020 //set becomes multiple joker
#define MATCH_NULS 0x0040 //nullify the set
#define MATCH_FULS 0x0080 //fullify the set

*/
/*noverbatim

The function first checks if it has to modify the state of the joker character. If
any of the bits T<MATCH_SNOJ>, T<MATCH_SSIJ> or T<MATCH_SMUJ> is set in the field
T<fAction> the type of the set is modified.

If more than one bit of these is set then result is undefined. Current implementation
checks these bits in a specific order, but later versions may change.

If the bit T<MATCH_NULS> is set all the characters are removed from the set. If
the bit T<MATCH_FULS> is set all characters are put into the set.

If more than one bit of these is set then result is undefined. Current implementation
checks these bits in a specific order, but later versions may change.

T<MATCH_NULS> or T<MATCH_FULS> can be used in a single call to initialize the set before
adding or removing the specific characters.

The bits T<MATCH_ADDC>, T<MATCH_REMC> and T<MATCH_INVC> can be used to add characters to the set,
remove characters from the set or to invert character membership. The characters are taken
from the character array pointed by the function argument T<pch>.

If more than one bit of these is set then result is undefined. Current implementation
checks these bits in a specific order, but later versions may change.

If none of these bits is set the value of the pointer T<pch> is ignored.

It is no problem if a character is already in the set and is added or if it is not member of the set
and is removed. Although it has no practical importance the array pointed by T<pch> may contain a
character many times.

/*FUNCTION*/
void match_ModifySet(pMatchSets pMS,
                     char JokerCharacter,
                     int nChars,
                     unsigned char *pch,
                     int fAction
  ){
/*noverbatim
CUT*/
  int i,index,offset;
  unsigned char *set,mask;

  i = match_index(JokerCharacter);
  if( i == 0 )return;
  i--;
  if( fAction & MATCH_SNOJ )pMS->SetType[i] = NOJOKER;
  if( fAction & MATCH_SSIJ )pMS->SetType[i] = SINGLEJOKER;
  if( fAction & MATCH_SMUJ )pMS->SetType[i] = MULTIJOKER;

  set = pMS->set[i];
  if( fAction & MATCH_NULS )
    for( i=0 ; i<32 ; i++ )set[i] = 0;
  if( fAction & MATCH_FULS )
    for( i=0 ; i<32 ; i++ )set[i] = 0xFF;

  if( fAction & (MATCH_ADDC|MATCH_REMC) )
    while( nChars-- ){
      mask = 1;
      index = (*pch) >> 3;
      offset = (*pch) & 7;
      mask <<= offset;
      if( fAction & MATCH_ADDC )set[index] |= mask;
      if( fAction & MATCH_INVC )set[index] ^= mask;
      if( fAction & MATCH_REMC ){
        mask = ~mask;
        set[index] &= mask;
        }
      pch++;
      }
  return;
  }

/*POD
=H match
@c Match pattern to string
FUNCTION:

T<match> checks if pszString matches the pattern pszPattern.
pszPattern is a string containing joker characters. These are:

=verbatim
 * matches one or more any character
 # matches one or more digit
 $ matches one or more alphanumeric character
 @ matches one or more alpha character
   (space) matches one or more spaces
 ? matches a single character
=noverbatim

 T<~x> matches T<x> even if T<x> is pattern matching character or tilde

 T<x> matches character T<x> unless it is a joker character

RETURN VALUE:

The function returns zero if no error occures and returns an error code
in case some of the memory buffer does not have enough space. (Either
pszBuffer or ParameterArray)

PARAMETERS:

T<pszPattern> IN
the pattern to match

--

T<cbPattern> IN
the number of characters in the pattern

--

T<pszString> IN 
the string which is compared to the pattern

--

T<cbString> IN
the number of characters in the string

--

T<ParameterArray> OUT
is an uninitialized character pointer array. Upon return
T<ParameterArray[i]> points the string that matches the
T<i>-th joker character.

--

T<pcbParameterArray> OUT
is an uninititalized T<unsigned long> array. Upon return
T<pcbParameterArray[i]> contains the length of the
output parameter T<ParameterArray[i]>.

--

T<pszBuffer> OUT
should point to a buffer. The size of the buffer
should be specified by cbBufferSize. A size equal

=verbatim
             cbString
=noverbatim
is a safe size. The actual strings matching the joker characters
will get into this buffer zero terminated one after the other:

--

T<cArraySize> IN
number of elements in the array T<ParameterArray>

--

T<cbBufferSize> IN
size of the buffer pointed by pszBuffer

--

T<fCase> IN
pattern matching is performed case sensitive if this value if TRUE.

--

T<iResult> OUT
TRUE if T<pszString> matches the pattern T<pszPattern>.
FALSE otherwise.

NOTE:

T<pszPattern> and T<pszString> are NOT changed.

If the function returns non-zero (error code) none of the output
variables can be reliably used.
/*FUNCTION*/
int match_match(char *pszPattern,
                unsigned long cbPattern,
                char *pszString,
                unsigned long cbString,
                char **ParameterArray,
                unsigned long *pcbParameterArray,
                char *pszBuffer,
                int cArraySize,
                int cbBufferSize,
                int fCase,
                pMatchSets pThisMatchSets,
                int *iResult
  ){
/*noverbatim
CUT*/
  char cA,cB;
  int ErrorCode;
  unsigned char *set;

  if( pThisMatchSets == NULL )pThisMatchSets = &DefaultMatchSets;

  *iResult = 0;/* note that iResult is set to 1 directly before success return */
  while( 1 ){

    if( ! cbString ){
      *iResult = ! cbPattern;
      return MATCH_ERROR_SUCCESS;
      }

    if( set=MultiJokerSet(pThisMatchSets,*pszPattern) ){
      if( ! cbString )return MATCH_ERROR_SUCCESS; /* nothing to match by the joker character */
      if( cArraySize == 0 )return MATCH_ERROR_ARRAY_SHORT;
      if( cbBufferSize < 1 )return MATCH_ERROR_BUFFER_SHORT; /* we have at least one character */
      /* check that the first character matches the joker */
      cA = *pszString;
      if( !fCase && islower(cA) )cA = toupper(cA);
      if( ! JokerMatch(set,cA) )return MATCH_ERROR_SUCCESS;

      /* handle the first character that matched the joker */
      *ParameterArray = pszBuffer;
      *pszBuffer++ = *pszString++;
      cbString--;
      cbBufferSize--;
      *pcbParameterArray = 1; /* the first character is put into the ParameterArray */
      //if( ! cbBufferSize )return STRING_MATCH_BUFFER_SHORT;
      while( (ErrorCode=match_match(pszPattern+1,
                                    cbPattern-1,
                                    pszString,
                                    cbString,
                                    ParameterArray+1,
                                    pcbParameterArray+1,
                                    pszBuffer,             /* while there is no error and does not match */
                                    cArraySize-1,
                                    cbBufferSize,
                                    fCase,
                                    pThisMatchSets,
                                    iResult)
             ) == MATCH_ERROR_SUCCESS
                 &&
             !*iResult ){
        if( ! cbString )return MATCH_ERROR_SUCCESS;
        cA = *pszString;
        if( !fCase && islower(cA) )cA = toupper(cA);
        if( ! JokerMatch(set,cA) )return MATCH_ERROR_SUCCESS;
        *pszBuffer++ = *pszString++;
        cbBufferSize --;
        cbString--;
        (*pcbParameterArray)++;
        if( ! cbBufferSize )return MATCH_ERROR_BUFFER_SHORT;
        }
      *iResult = 1;
      return MATCH_ERROR_SUCCESS;
      }

    /* ~x is just x no matter what x is */
    if( *pszPattern == ESCAPE_CHARACTER ){
      pszPattern ++;
      cbPattern--;
      if( ! cbPattern )return MATCH_ERROR_SYNTAX_ERROR;

      if( ! fCase ){
        cA = *pszPattern;
        cB = *pszString;
        if( islower(cA) )cA = toupper(cA);
        if( islower(cB) )cB = toupper(cB);
        if( cA != cB )return MATCH_ERROR_SUCCESS;
        }else{
        if( *pszPattern != *pszString )return MATCH_ERROR_SUCCESS;
        }
      }else{

      if( set=SingleJokerSet(pThisMatchSets,*pszPattern) ){
        cA = *pszString;
        if( !fCase && islower(cA) )cA = toupper(cA);
        if( ! JokerMatch(set,cA) )return MATCH_ERROR_SUCCESS;
        *ParameterArray++ = pszBuffer;
        *pszBuffer++ = *pszString;
        cbBufferSize--;
        *pcbParameterArray++ = 1;
        }
      else if( *pszPattern == ' ' ){
        if( ! isspace(*pszString) )return MATCH_ERROR_SUCCESS;
        while( cbString && isspace( *pszString ) ){
          pszString ++;
          cbString --;
          }
        pszString--;
        cbString++;
        }else{
        /* Normal character. */
        if( ! fCase ){
          cA = *pszPattern;
          cB = *pszString;
          if( islower(cA) )cA = toupper(cA);
          if( islower(cB) )cB = toupper(cB);
          if( cA != cB )return MATCH_ERROR_SUCCESS;
          }else{
          if( *pszPattern != *pszString )return MATCH_ERROR_SUCCESS;
          }
        }
      }
    pszPattern++; pszString++;
    cbPattern--; cbString--;
    }/* end of 'while(1)' */
  }

/*POD
=H count
@c Count the joker characters in a pattern

This function counts the number of jokers in the string and returns it.
This function should be used to calculate the safe length of the pszBuffer
given as a parameter to match.
/*FUNCTION*/
int match_count(char *pszPattern,
                unsigned long cbPattern
  ){
/*noverbatim
CUT*/
  int iCounter = 0;

  while( cbPattern ){
    if( match_index(*pszPattern) )iCounter++;
    if( *pszPattern == ESCAPE_CHARACTER ){
      pszPattern++;
      cbPattern--;
      if( ! cbPattern )return iCounter;
      }
    pszPattern++;
    cbPattern--;
    }
  return iCounter;
  }

/*POD
=H parameter
@c Fill parameters into format string

This function takes a format string and a string array and
copies the format string replacing T<$0>, T<$1> ... T<$n> values with
the appropriate string values given in the array pointed by
ParameterArray.

RETURN VALUE:

The function returns zero if no error occures and returns an error code
in case some of the memory buffer does not have enough space or invalid
parameter is referenced.

PARAMETERS:
T<pszFormat> IN
The format string containing the $i placeholders.

--

T<cbFormat> IN
The number of characters in the format string

--

T<ParameterArray> IN
string array so that T<ParameterArray[i]> is to be inserted in place of the T<$i> placeholders

--

T<pcbParameterArray> IN
array of T<unsigned long> values. T<pcbParameterArray[i]> gives the length of the i-th string parameter.

--

T<pszBuffer> OUT
buffer to put the result

--

T<cArraySize> IN
Number of parameters given in the ParameterArray

--

T<pcbBufferSize> IN/OUT
Available bytes in buffer pointed by T<pszBuffer>. Upon return it contains the number of characters
that were placed in the buffer.

--

NOTE:

If the function returns non-zero (error code) none of the output
variables can be reliably used.
/*FUNCTION*/
int match_parameter(char *pszFormat,
                    unsigned long cbFormat,
                    char **ParameterArray,
                    unsigned long *pcbParameterArray,
                    char *pszBuffer,
                    int cArraySize,
                    unsigned long *pcbBufferSize
  ){
/*noverbatim
CUT*/
  int index;
  char *s;
  unsigned long ulParamSize,ulResultSize;

  ulResultSize = 0;
  while( cbFormat ){
    if( *pszFormat == '$' ){

      /***            CONVERT $$ to $            ***/
      /***                                       ***/
      pszFormat++;
      cbFormat --;
      if( cbFormat && *pszFormat == '$' ){/* $$ is converted to single $ */
        *pszBuffer++ = *pszFormat++;
        cbFormat--;
        (*pcbBufferSize)--;
        ulResultSize;
        if( *pcbBufferSize < 1 )return MATCH_ERROR_BUFFER_SHORT;

        }else{

      /***            CONVERT $n to parameter    ***/
      /***                                       ***/
        if( cbFormat && isdigit( *pszFormat ) ){
          index = 0;
          while( cbFormat && isdigit( *pszFormat ) ){
            index = 10*index + *pszFormat - '0';
            pszFormat++;
            cbFormat--;
            }
/*          if( index == 0 )return MATCH_ERROR_INDEX_OUT_OF_RANGE;
            index --; /* convert $i to zero based */
          if( index >= cArraySize )return MATCH_ERROR_INDEX_OUT_OF_RANGE;
          s = ParameterArray[index];
          ulParamSize = pcbParameterArray[index];
          while( ulParamSize ){
            if( *pcbBufferSize < 1 )return MATCH_ERROR_BUFFER_SHORT;
            *pszBuffer++ = *s++;
            (*pcbBufferSize)--;
            ulResultSize++;
            ulParamSize--;
            }
          }else{

      /***            $ w/o number is just a $   ***/
      /***                                       ***/
          *pszBuffer++ = '$';
          (*pcbBufferSize)--;
          ulResultSize++;
          if( *pcbBufferSize < 1 )return MATCH_ERROR_BUFFER_SHORT;
          }
        }

      }else{

      /***            copy character             ***/
      /***                                       ***/
      if( *pcbBufferSize < 1 )return MATCH_ERROR_BUFFER_SHORT;
      *pszBuffer++ = *pszFormat++;
      (*pcbBufferSize)--;
      ulResultSize++;
      cbFormat--;
      }
    }
  *pcbBufferSize = ulResultSize;
  return MATCH_ERROR_SUCCESS;
  }

/*POD
=H size
@c Calculate the neccessary buffer size

Calculate the size of the output. The IN/OUT parameter T<cbBufferSize>
is increased by the number of needed characters.

The return value is zero if no error occured or the error code.

NOTE: cbBuffer size should be initialized to 0 if you want to get the
size of the buffer needed.
/*FUNCTION*/
int match_size(char *pszFormat,
               unsigned long cbFormat,
               unsigned long *pcbParameterArray,
               int cArraySize,
               int *cbBufferSize
  ){
/*noverbatim
CUT*/
  int index;

  while( cbFormat ){
    if( *pszFormat == '$' ){
      pszFormat++;
      cbFormat--;
      if( cbFormat && *pszFormat == '$' ){/* $$ is converted to single $ */
        pszFormat++;
        cbFormat--;
        (*cbBufferSize)++;

        }else{

      /***            CONVERT $n to parameter    ***/
      /***                                       ***/
        if( cbFormat && isdigit( *pszFormat ) ){
          index = 0;
          while( cbFormat && isdigit( *pszFormat ) ){
            index = 10*index + *pszFormat - '0';
            pszFormat++;
            cbFormat--;
            }
/*          if( index == 0 )return MATCH_ERROR_INDEX_OUT_OF_RANGE;
          index--;*/
          if( index >= cArraySize )return MATCH_ERROR_INDEX_OUT_OF_RANGE;
          (*cbBufferSize) += pcbParameterArray[index];
          }else{

      /***            $ w/o number is just a $   ***/
      /***                                       ***/
          cbBufferSize ++;
          }
        }

      }else{

      /***            copy character             ***/
      /***                                       ***/
      pszFormat++;
      cbFormat--;
      (*cbBufferSize)++;
      }
    }
  return MATCH_ERROR_SUCCESS;
  }

