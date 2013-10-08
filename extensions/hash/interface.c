/* FILE:hash.c

This file implements hash functionality for the ScriptBasic interpreter as
an external module. The functions implemented in this file are:

NewHash to create a new hash
ReleaseHash to release a hash
StartHash to reset hash iteration to the first key
EndHash to set hash iteration to the last key
NextHashKey to get the next hash key in iteration
PreviousHashKey to get the previous key in iteration
GetHashValue to get the value for the given key
SetHashValue to set the value for the given key
DeleteHashKey to delete a hash key entirely from the hash

NTLIBS:
UXLIBS:
DWLIBS:
MCLIBS:

*/
#include <stdio.h>
#define PRIME 211
#include "../../basext.h"

typedef struct _hashe {
  VARIABLE Key,Value;
  struct _hashe *small_son, *big_son;
  struct _hashe *next,*prev;
  } tHashE, *ptHashE;

typedef struct _hash {
  ptHashE Table[PRIME];
  ptHashE FirstElement,
          LastElement,
          ThisElement;
  } tHash,*ptHash;
typedef struct _myOBJECT {
  void *HandleArray;
  } myOBJECT, *pmyOBJECT;

#define HASH_ERROR_INVALID_HASH_HANDLE 0x00080001
#define HASH_ERROR_NO_CURRENT_ELEMENT  0x00080002
#define HASH_ERROR_INVALID_VALUE       0x00080003
#define HASH_ERROR_INTERNAL001         0x00080004
#define HASH_ERROR_INTERNAL002         0x00080005

/*

varcmp compares two variables. The purpose of this function is to give
some ordering when more than one keys have the same hash value and a binary
tree has to be ordered. The function arbitrarily say that

undef equals to undef only and is smaller than anything else
double is smaller than a long (even 3.2 is smaller than 0).

strings, longs and doubles are compared using their value and
the natural comparision functions, operators.

*/
static int varcmp(VARIABLE a, VARIABLE b){
  long minlen;
  int k;

  /* undef equals undef */
  if( a == NULL && b == NULL )return 0;
  /* undef is smaller than anything */
  if( a == NULL )return -1;
  if( b == NULL )return +1;

  /* compare strings */
  if( TYPE(a) == VTYPE_STRING && TYPE(b) == VTYPE_STRING ){
    minlen = STRLEN(a) < STRLEN(b) ? STRLEN(a) : STRLEN(b);
    k = memcmp(STRINGVALUE(a),STRINGVALUE(b),minlen);
    if( k == 0 && STRLEN(a) < STRLEN(b) )k = -1;
    if( k == 0 && STRLEN(a) > STRLEN(b) )k = +1;
    return k;
    }
  /* compare longs */
  if( TYPE(a) == VTYPE_LONG && TYPE(b) == VTYPE_LONG ){
    if( LONGVALUE(a) == LONGVALUE(b) )return 0;
    return LONGVALUE(a) < LONGVALUE(b) ? -1 : +1;
    }
  /* compare doubles */
  if( TYPE(a) == VTYPE_DOUBLE && TYPE(b) == VTYPE_DOUBLE ){
    if( DOUBLEVALUE(a) == DOUBLEVALUE(b) )return 0;
    return DOUBLEVALUE(a) < DOUBLEVALUE(b) ? -1 : +1;
    }

  /*
          GO ON AND COMPARE DIFFERENT TYPES HERE
   */

  /* string is smaller than any other type */
  if( TYPE(a) == VTYPE_STRING ) return -1;
  if( TYPE(b) == VTYPE_STRING ) return +1;

  /* long is smaller than double */
  if( TYPE(a) == VTYPE_LONG ) return -1;
  if( TYPE(b) == VTYPE_LONG ) return +1;

  /* we may end up here if both variables are arrays that they should not be. */
  return 0;
  }

#define MASK  0xf0000000l
static int hashpjw(char *s, long len){
  unsigned long h = 0, g;
  for ( ; len ; s++,len-- ) {
    h = (h << 4) + (*s);
    if (g = h&MASK) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % PRIME;
}

static int varhashpjw(VARIABLE a){

  if( a == NULL )return 0;
  switch( TYPE(a) ){
    case VTYPE_STRING: return hashpjw(STRINGVALUE(a),STRLEN(a));
    case VTYPE_LONG:   return hashpjw((char *)&(LONGVALUE(a)),sizeof(long));
    case VTYPE_DOUBLE: return hashpjw((char *)&(DOUBLEVALUE(a)),sizeof(double));
    default: return 0;
    }
  }

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START
  pmyOBJECT p;

  besMODULEPOINTER = besALLOC(sizeof(myOBJECT));
  if( besMODULEPOINTER == NULL )return COMMAND_ERROR_MEMORY_LOW;
  p = (pmyOBJECT)besMODULEPOINTER;
  p->HandleArray = NULL;

besEND

besSUB_FINISH
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;
  if( p != NULL ){
    besHandleDestroyHandleArray(p->HandleArray);
    }
besEND

/*
  Create a new hash and return the handle of it.
*/
besFUNCTION(newh)
  ptHash r;
  int i;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;
  r = besALLOC(sizeof(tHash));
  if( r == NULL )return COMMAND_ERROR_MEMORY_LOW;
  r->FirstElement = NULL;
  r->LastElement = NULL;
  r->ThisElement = NULL;
  for( i = 0 ; i < PRIME ; i++ )r->Table[i] = NULL;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = besHandleGetHandle(p->HandleArray,r);
  return COMMAND_ERROR_SUCCESS;
besEND

#define GET_HASH_HANDLE \
  Argument = besARGUMENT(1);\
  besDEREFERENCE(Argument);\
  Argument = besCONVERT2LONG(Argument);\
  pH = besHandleGetPointer(p->HandleArray,LONGVALUE(Argument));

besFUNCTION(sethv)
  VARIABLE Argument,vKey,vValue;
  int k;
  ptHash pH;
  ptHashE pE,*ppE;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  vKey = Argument;

  /* get the value, possibly a valid undef (aka NULL) */
  Argument = besARGUMENT(3);
  if( Argument ){
    besDEREFERENCE(Argument);
    /* make a local non-mortal copy of the argument */
    switch( Argument->vType ){
      case VTYPE_STRING:
        vValue = besNEWSTRING(STRLEN(Argument));
        if( vValue == NULL )return COMMAND_ERROR_MEMORY_LOW;
        memcpy(STRINGVALUE(vValue),STRINGVALUE(Argument),STRLEN(Argument));
        break;
      case VTYPE_LONG:
        vValue = besNEWLONG;
        if( vValue == NULL )return COMMAND_ERROR_MEMORY_LOW;
        LONGVALUE(vValue) = LONGVALUE(Argument);
        break;
      case VTYPE_DOUBLE:
        vValue = besNEWDOUBLE;
        if( vValue == NULL )return COMMAND_ERROR_MEMORY_LOW;
        DOUBLEVALUE(vValue) = DOUBLEVALUE(Argument);
        break;
      default: return HASH_ERROR_INVALID_VALUE;
      }
    }else vValue = NULL; /*this is a "copy" of undef*/

  /* try to find the key in the hash table */
  ppE = pH->Table+varhashpjw(vKey);
  while( *ppE ){
    k = varcmp(vKey,(*ppE)->Key);
    if( k == 0 )break;/* we have found the key */
    ppE = k < 0 ? &( (*ppE)->small_son ) : &( (*ppE)->big_son );
    }

  if( *ppE ){
    /* if the key exists, release the old value and insert the new */
    if( (*ppE)->Value )besRELEASE( (*ppE)->Value );
    (*ppE)->Value = vValue;
    return COMMAND_ERROR_SUCCESS;
    }

  /* 
         here we have to insert a new element
   */

  /* make a local non-mortal copy of the key value */
  Argument = vKey;
  if( Argument ){
    besDEREFERENCE(Argument);
    /* make a local non-mortal copy of the argument */
    switch( Argument->vType ){
      case VTYPE_STRING:
        vKey = besNEWSTRING(STRLEN(Argument));
        if( vKey == NULL )return COMMAND_ERROR_MEMORY_LOW;
        memcpy(STRINGVALUE(vKey),STRINGVALUE(Argument),STRLEN(Argument));
        break;
      case VTYPE_LONG:
        vKey = besNEWLONG;
        if( vKey == NULL )return COMMAND_ERROR_MEMORY_LOW;
        LONGVALUE(vKey) = LONGVALUE(Argument);
        break;
      case VTYPE_DOUBLE:
        vKey = besNEWDOUBLE;
        if( vKey == NULL )return COMMAND_ERROR_MEMORY_LOW;
        DOUBLEVALUE(vKey) = DOUBLEVALUE(Argument);
        break;
      default: return HASH_ERROR_INVALID_VALUE;
      }
    }else vKey = NULL; /*this is a "copy" of undef*/

  *ppE = pE = besALLOC(sizeof(tHashE));
  if( pE == NULL )return COMMAND_ERROR_MEMORY_LOW;
  pE->Key = vKey;
  pE->Value = vValue;
  pE->small_son = NULL;
  pE->big_son = NULL;
  pE->next = NULL;
  if( pH->LastElement ){
    pH->LastElement->next = pE;
    }else
    pH->LastElement = pE;

  if( pH->FirstElement == NULL ){
    pH->FirstElement = pE;
    pH->ThisElement = pE;
    pE->prev = NULL;
    }else
    pE->prev = pH->LastElement;

  pH->LastElement = pE;

besEND

besFUNCTION(gethv)
  VARIABLE Argument,vKey;
  int k;
  ptHash pH;
  ptHashE *ppE;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  vKey = Argument;

  /* try to find the key in the hash table */
  ppE = pH->Table+varhashpjw(vKey);
  while( *ppE ){
    k = varcmp(vKey,(*ppE)->Key);
    if( k == 0 )break;/* we have found the key */
    ppE = k < 0 ? &( (*ppE)->small_son ) : &( (*ppE)->big_son );
    }

  pH->ThisElement = *ppE;

  /* if the key does not exists, we return undef */
  if( *ppE == NULL )return COMMAND_ERROR_SUCCESS;

  /* return a reference value to the value */
  besRETURNVALUE  = besNEWMORTALREF;
  if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;
  besRETURNVALUE->Value.aValue = &((*ppE)->Value);

besEND

besFUNCTION(ivhv)
  VARIABLE Argument,vKey;
  int k;
  ptHash pH;
  ptHashE *ppE;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  if( besARGNR == 1 ){/*no key, see the iteration pointer */
  besALLOC_RETURN_LONG;
  /* if the key does not exists, we return false */
    if( pH->ThisElement == NULL ){
      LONGVALUE(besRETURNVALUE) = 0L;
      }else{
      LONGVALUE(besRETURNVALUE) = 1L;
      }
    return COMMAND_ERROR_SUCCESS;
    }

  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  vKey = Argument;

  /* try to find the key in the hash table */
  ppE = pH->Table+varhashpjw(vKey);
  while( *ppE ){
    k = varcmp(vKey,(*ppE)->Key);
    if( k == 0 )break;/* we have found the key */
    ppE = k < 0 ? &( (*ppE)->small_son ) : &( (*ppE)->big_son );
    }

  pH->ThisElement = *ppE;

  besALLOC_RETURN_LONG;
  /* if the key does not exists, we return false */
  if( *ppE == NULL ){
    LONGVALUE(besRETURNVALUE) = 0L;
    }else{
    LONGVALUE(besRETURNVALUE) = 1L;
    }
  return COMMAND_ERROR_SUCCESS;

besEND

besFUNCTION(delhk)
  VARIABLE Argument,vKey;
  unsigned long minlen;
  int k;
  ptHash pH;
  ptHashE pE,*ppE;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument != NULL )
    vKey = besCONVERT2STRING(Argument);
  else
    vKey = NULL;

  /* try to find the key in the hash table */
  ppE = pH->Table+varhashpjw(vKey);
  while( *ppE ){
    minlen = vKey ? STRLEN(vKey) : 0;
    if( minlen > STRLEN( (*ppE)->Key ) )minlen = STRLEN( (*ppE)->Key );
    k = varcmp(vKey,(*ppE)->Key);
    if( k == 0 )break;/* we have found the key */
    ppE = k < 0 ? &( (*ppE)->small_son ) : &( (*ppE)->big_son );
    }

  /* if the key does not exist the it is no problem trying to delete it */
  if( *ppE == NULL )return COMMAND_ERROR_SUCCESS;

  /* if the key exists, release the old value and insert the new */
  if( (*ppE)->Value )besRELEASE( (*ppE)->Value );
  if( (*ppE)->Key   )besRELEASE( (*ppE)->Key   );

  /* unlink from the linked list */
  if( (*ppE)->next )(*ppE)->next->prev = (*ppE)->prev;
  if( (*ppE)->prev )(*ppE)->prev->next = (*ppE)->next;

  if( (*ppE)->prev == NULL )
    /* this was the first element */
    pH->FirstElement = (*ppE)->next;

  if( (*ppE)->next == NULL )
    /* this was the last element */
    pH->LastElement = (*ppE)->prev;

  pH->ThisElement = NULL; /* to be safe, we may have just destroyed the actual element,
                             and I hate comparing pointers*/

  pE = *ppE;

  (*ppE) = pE->big_son; /* we hook big son on the place of the curently deleted record */

  if( pE->small_son ){/* if there is small son */
    vKey = pE->small_son->Key;
    while( *ppE ){
      minlen = vKey ? STRLEN(vKey) : 0;
      if( minlen > STRLEN( (*ppE)->Key ) )minlen = STRLEN( (*ppE)->Key );
      k = varcmp(vKey,(*ppE)->Key);
      if( k == 0 )break;/* we have found the key */
      ppE = k < 0 ? &( (*ppE)->small_son ) : &( (*ppE)->big_son );
      }
   if( *ppE ){
     /* If *ppE is not NULL it means that we have found the key. But the key belongs to
        a node of the hash that was already inserted and currently off hooked. We just
        could not find the one that holds this key therefore getting here means that the
        key is duplicated. On the other hand keys can not be duplicated, so this means
        an internal error. I hope that this is dead code and I wish to have a C compiler
        that optimizes to the level recognising it.
     */
     return HASH_ERROR_INTERNAL001;
     }
   *ppE = pE->small_son; /* hook small son pon its new place */
   }
  besFREE(pE);/* finally release the node */
besEND

besFUNCTION(starth)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  pH->ThisElement = pH->FirstElement;
besEND

besFUNCTION(endh)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  pH->ThisElement = pH->LastElement;
besEND

besFUNCTION(nexthk)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  if( pH->ThisElement == NULL )return HASH_ERROR_NO_CURRENT_ELEMENT;
  pH->ThisElement = pH->ThisElement->next;
  if( pH->ThisElement == NULL || pH->ThisElement->Key == NULL )return COMMAND_ERROR_SUCCESS;
  switch( pH->ThisElement->Key->vType ){
    case VTYPE_STRING:
      besALLOC_RETURN_STRING(STRLEN(pH->ThisElement->Key));
      memcpy(STRINGVALUE(besRETURNVALUE),STRINGVALUE(pH->ThisElement->Key),STRLEN(pH->ThisElement->Key));
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_LONG:
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = LONGVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_DOUBLE:
      besALLOC_RETURN_DOUBLE;
      DOUBLEVALUE(besRETURNVALUE) = DOUBLEVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    default: return HASH_ERROR_INTERNAL002;
    }
besEND

besFUNCTION(pervhk)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  if( pH->ThisElement == NULL )return HASH_ERROR_NO_CURRENT_ELEMENT;
  pH->ThisElement = pH->ThisElement->prev;
  if( pH->ThisElement == NULL || pH->ThisElement->Key == NULL )return COMMAND_ERROR_SUCCESS;
  switch( pH->ThisElement->Key->vType ){
    case VTYPE_STRING:
      besALLOC_RETURN_STRING(STRLEN(pH->ThisElement->Key));
      memcpy(STRINGVALUE(besRETURNVALUE),STRINGVALUE(pH->ThisElement->Key),STRLEN(pH->ThisElement->Key));
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_LONG:
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = LONGVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_DOUBLE:
      besALLOC_RETURN_DOUBLE;
      DOUBLEVALUE(besRETURNVALUE) = DOUBLEVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    default: return HASH_ERROR_INTERNAL002;
    }
besEND

besFUNCTION(thishk)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  if( pH->ThisElement == NULL )return HASH_ERROR_NO_CURRENT_ELEMENT;
  if( pH->ThisElement->Key == NULL )return COMMAND_ERROR_SUCCESS;
  switch( pH->ThisElement->Key->vType ){
    case VTYPE_STRING:
      besALLOC_RETURN_STRING(STRLEN(pH->ThisElement->Key));
      memcpy(STRINGVALUE(besRETURNVALUE),STRINGVALUE(pH->ThisElement->Key),STRLEN(pH->ThisElement->Key));
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_LONG:
      besALLOC_RETURN_LONG;
      LONGVALUE(besRETURNVALUE) = LONGVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    case VTYPE_DOUBLE:
      besALLOC_RETURN_DOUBLE;
      DOUBLEVALUE(besRETURNVALUE) = DOUBLEVALUE(pH->ThisElement->Key);
      return COMMAND_ERROR_SUCCESS;
    default: return HASH_ERROR_INTERNAL002;
    }
besEND

besFUNCTION(thishv)
  VARIABLE Argument;
  ptHash pH;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  if( pH->ThisElement == NULL )return HASH_ERROR_NO_CURRENT_ELEMENT;
  /* return a reference value to the value */
  besRETURNVALUE  = besNEWMORTALREF;
  if( besRETURNVALUE  == NULL )return COMMAND_ERROR_MEMORY_LOW;
  besRETURNVALUE->Value.aValue = &(pH->ThisElement->Value);
besEND

besFUNCTION(relh)
  VARIABLE Argument;
  ptHash pH;
  LEFTVALUE Lval;
  unsigned long __refcount_;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  GET_HASH_HANDLE

  while( pH->FirstElement ){
    pH->ThisElement = pH->FirstElement;
    pH->FirstElement = pH->FirstElement->next;
    besRELEASE(pH->ThisElement->Key);
    besRELEASE(pH->ThisElement->Value);
    besFREE(pH->ThisElement);
    }
  besFREE(pH);

  /* if the argument is a variable, and I hope it is, then undef it
     so that it will not later be mistakenly used */
  Argument = besARGUMENT(1);
  besLEFTVALUE(Argument,Lval);
  if( Lval ){
    besRELEASE(*Lval);
    *Lval = NULL;
    }
besEND
