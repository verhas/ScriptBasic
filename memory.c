/* 
FILE:   memory.c
HEADER: memory.h

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

typedef unsigned char BYTE, *PBYTE;

typedef struct _FixSizeMemoryObject {

  union _fsmoval{
    PBYTE  pValue; // the value of the object
    long   lValue; // the long value of the object
    double dValue; // the double value of the object
    struct _FixSizeMemoryObject **aValue;
    } Value;
  unsigned long Size; // the actual size used (may be different from the size indicated by the type)
  BYTE sType; // index to the array SizeOfType. If this is LARGE_BLOCK_TYPE then
              // Size is the memory allocated for the value
  BYTE vType; // the variable type, VTYPE_/LONG/DOUBLE/STRING/REF/UNDEF/ARRAY
  BYTE State; // state of the variable (mortal, free or assigned)
  // these two pointers help to maintain either the free list or the mortal lists
  // a variable can never be in a free and in a mortal list at the same time
  // in case of reference variable these fields maintain a linked list of
  // variables referencing the same value
  struct _FixSizeMemoryObject *next;     // link to the next object
  union {
    struct _FixSizeMemoryObject *prev;   // link to the previous object
    struct _FixSizeMemoryObject **rprev; // link to the previous variable referencing
    }link;
  long ArrayLowLimit, ArrayHighLimit;    // If this is an array then these fields tell you where to start
                                         // and where to end
  } FixSizeMemoryObject, *pFixSizeMemoryObject, 
    *MortalList, **pMortalList;

// these macros can be used to access the actual values of the objects
#define DOUBLEVALUE(x) ((x)->Value.dValue)
#define LONGVALUE(x)   ((x)->Value.lValue)
#define STRINGVALUE(x) ((char *)((x)->Value.pValue))
#define ARRAYVALUE(x,i)  (((x)->Value.aValue)[(i)-(x)->ArrayLowLimit])
#define ARRAYLOW(x)    ((x)->ArrayLowLimit)
#define ARRAYHIGH(x)   ((x)->ArrayHighLimit)
#define STRLEN(x)      ((x)->Size)

#define MAX_NUMBER_OF_FIX_TYPES 254
#define LARGE_BLOCK_TYPE 255

typedef struct _MemoryObject {
  unsigned long SizeOfType[MAX_NUMBER_OF_FIX_TYPES]; // SizeOfType[i] is the size of the type i
  BYTE iNumberOfFixTypes;    // the number of fix length type memory objects
                             // we can have at most MAX_NUMBER_OF_FIX_TYPES fix size types
                             // LARGE_BLOCK_TYPE means large block which is not stored in free list
  BYTE FirstStringIndex,LastStringIndex;
  pFixSizeMemoryObject MemoryObjectsFreeList[MAX_NUMBER_OF_FIX_TYPES];
  void *(*memory_allocating_function)(size_t);
  void (*memory_releasing_function)(void *);
  void *pMemorySegment; //this pointer is passed to the memory allocating functions
                        //this pointer is initialized in ex_init
  unsigned long maxderef;
  } MemoryObject, *pMemoryObject;

#define TYPESIZE(pMo,X) (pMo->SizeOfType(X))
#define IsMortal(x) ((x)->State == STATE_MORTAL)

// these are fixed and stored inside the FixSizeMemoryObject in the place of the pointer
// these constants should be used for the field sType
#define FIX_TYPE_LONG    0
#define FIX_TYPE_DOUBLE  1
#define FIX_TYPE_CSTRING 2 // constant string variable, pointing to a string that should not be altered
#define MAX_FIX_TYPE     3
#define FIX_TYPE_ALLOC FIX_TYPE_LONG // the one (arbitrary) that is used to list free variables

#define STATE_UNKNOWN  0
#define STATE_FREE     1
#define STATE_MORTAL   2
#define STATE_IMMORTAL 3

// these constants are for vType (which is variable type)
// you can define other variables in case you need other types
#define VTYPE_LONG   0 // long value stored inside the struct
#define VTYPE_DOUBLE 1 // double value stored inside the struct
#define VTYPE_STRING 2 // string pointed by Value.pValue
#define VTYPE_ARRAY  3 // array pointing to other values
#define VTYPE_REF    4 // reference value pointing to another variable
#define VTYPE_UNDEF  5 // this is used, when an undef variable is referenced
*/
#include <stdlib.h>
#include <string.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

#include "errcodes.h"
#include "memory.h"
#include "myalloc.h"

/*POD
=H memory_InitStructure()

Each execution context should have its own memory object responsible for the administration
of the variables and the memory storing the values assigned to variables.

This function initializes such a memory object.

/*FUNCTION*/
int memory_InitStructure(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
/*  int i;

  for( i=0 ; i < MAX_NUMBER_OF_FIX_TYPES ; i++ ){
    pMo->SizeOfType[i] = 0;
    pMo->MemoryObjectsFreeList[i] = NULL;
    }
*/
  memset(pMo->SizeOfType,0,sizeof(long)*MAX_NUMBER_OF_FIX_TYPES);
  memset(pMo->MemoryObjectsFreeList,0,sizeof(void *)*MAX_NUMBER_OF_FIX_TYPES);
  pMo->iNumberOfFixTypes = MAX_FIX_TYPE; /* long, double and constant string are implicit types that 
                                            are stored inside the FixSizeMemoryObject */
  pMo->pMemorySegment = alloc_InitSegment(pMo->memory_allocating_function,pMo->memory_releasing_function);
  if( pMo->pMemorySegment == NULL )return MEM_ERROR_MEMORY_LOW;
  return MEM_ERROR_SUCCESS;
  }

/*POD
=H memory_RegisterType()

This function should be used to register a variable type. The return value
is the serial number of the type that should later be used to reference the type.

/*FUNCTION*/
int memory_RegisterType(pMemoryObject pMo,
                        unsigned long SizeOfThisType
  ){
/*noverbatim
The argument of the function is the size of the type in terms of bytes. Usually this
is calculated using the C structure T<sizeof>.

If the type can not be registered -1 is returned.
CUT*/
  if( pMo->iNumberOfFixTypes >= MAX_NUMBER_OF_FIX_TYPES )return -1;
  pMo->SizeOfType[pMo->iNumberOfFixTypes++] = SizeOfThisType;
  return pMo->iNumberOfFixTypes-1;
  }


/*POD
=H memory_RegisterTypes()

This function should be used to initialize the usual T<FixSizeMemoryObject> types. This
sets some usual string sizes, but the caller may not call this function and set different
size objects.

/*FUNCTION*/
void memory_RegisterTypes(pMemoryObject pMo
  ){
/*noverbatim
This function registers the different string sizes. In the current implementation a string has
at least 32 characters. If this is longer that that (including the terminating zchar) then
a 64 byte fix size object is allocated. If this is small enough then a 128 byte fix size memory
object is allocated and so on up to 1024 bytes. If a string is longer that that then a LARGE_OBJECT_TYPE
is allocated.

The reason to register these types is that this memory management module keeps a list for these
memory pieces and when a new short string is needed it may be available already without calling
T<malloc>. On the other hand when a T<LARGE_OBJECT_TYPE> value is released it is always passed back
to the operating system calling T<free>.

CUT*/

  pMo->FirstStringIndex = memory_RegisterType(pMo,32);
                          memory_RegisterType(pMo,64);
                          memory_RegisterType(pMo,128);
                          memory_RegisterType(pMo,256);
                          memory_RegisterType(pMo,512);
  pMo->LastStringIndex = memory_RegisterType(pMo,1024);
  }

#ifdef _DEBUG
/*POD
=H memory_DebugDump()

This is a debugging function that dumps several variable data to the standard output.
The actual behavior of the function may change according to the actual debug needs.

/*FUNCTION*/
void memory_DebugDump(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject p;
  int i;
  unsigned long count;

  for( i = pMo->FirstStringIndex ; i <= pMo->LastStringIndex ; i++ ){
    p = pMo->MemoryObjectsFreeList[i];
    count = 0;
    while( p ){
      p = p->next;
      count++;
      }
    printf("list %d. length=%d\n",i,count);
    }

  }
#endif

/*POD
=H memory_NewVariable()

This function should be used whenever a new variable is to be allocated.
The function returns a pointer to a T<FixSizeMemoryObject> structure that
holds the variable information and pointer to the memory that stores the
actual value for the memory.

If there is not engough memory or the calling is illegal the returned value is T<NULL>
/*FUNCTION*/
pFixSizeMemoryObject memory_NewVariable(pMemoryObject pMo,
                                        int type,
                                        unsigned long LargeBlockSize
  ){
/*noverbatim

The second argument gives the type of the memory object to be allocated. If this
value is T<LARGE_BLOCK_TYPE> then the third argument is used to determine the size of the
memory to be allocated. If the type if NOT T<LARGE_BLOCK_TYPE> then this argument is
ignored and the proper size is allocated.

If the type has memory that was earlier allocated and released it is stored in a free list
and is reused.

CUT*/
  pFixSizeMemoryObject p;
  int atype; /* from which we allocate */

  if( type == LARGE_BLOCK_TYPE ){
    p = alloc_Alloc(sizeof(FixSizeMemoryObject),pMo->pMemorySegment);
    if( p == NULL )return NULL;
    p->sType = type;
    p->State = STATE_UNKNOWN;
    p->next = NULL;
    p->link.prev = NULL;
    p->ArrayHighLimit = 0;
    p->ArrayLowLimit  = 1;
    p->Size = LargeBlockSize;
    p->Value.pValue = alloc_Alloc(LargeBlockSize,pMo->pMemorySegment);
    if( p->Value.pValue == NULL ){
      alloc_Free(p,pMo->pMemorySegment);
      return NULL;
      }
    return p;
    }

  if( type >= MAX_NUMBER_OF_FIX_TYPES )return NULL;

  /* if the type is so that it does not use extra memory to store the value, like
     long, double or constant string then use only one list. No matter which, but
     only one. */
  if( type < MAX_FIX_TYPE )
    atype = FIX_TYPE_ALLOC;
  else
    atype = type;

  /* Check if there is any free variable on the free list of the type. */
  if( pMo->MemoryObjectsFreeList[atype] == NULL ){
    p = alloc_Alloc(sizeof(FixSizeMemoryObject),pMo->pMemorySegment);
    if( p == NULL )return NULL;
    p->sType = type;
    p->State = STATE_UNKNOWN;
    p->next = NULL;
    p->link.prev = NULL;
    /* if size is null then the variable is stored inside the FixSizeMemoryObject */
    if( pMo->SizeOfType[type] != 0 ){
      p->Value.pValue = alloc_Alloc(pMo->SizeOfType[type],pMo->pMemorySegment);
      if( p->Value.pValue == NULL ){
        alloc_Free(p,pMo->pMemorySegment);
        return NULL;
        }
      }else{
      p->Value.pValue = NULL;
      }
    return p;
    }

  p = pMo->MemoryObjectsFreeList[atype];
  pMo->MemoryObjectsFreeList[atype] = pMo->MemoryObjectsFreeList[atype]->next;
  if( pMo->MemoryObjectsFreeList[atype] )
    pMo->MemoryObjectsFreeList[atype]->link.prev = NULL;

  p->sType = type;
  p->next = NULL;
  p->link.prev = NULL;
  p->ArrayHighLimit = 0;
  p->ArrayLowLimit  = 1;
  p->State = STATE_UNKNOWN;
  return p;
  }

/*POD
=H memory_ReleaseVariable()

This function should be used to release a memory object.

/*FUNCTION*/
int memory_ReleaseVariable(pMemoryObject pMo,
                           pFixSizeMemoryObject p
  ){
/*noverbatim
CUT*/
  long i;
  int atype; /* from which we allocate */
  pFixSizeMemoryObject *pRefHead,*pRef,pSwap;
#ifdef _DEBUG
pFixSizeMemoryObject q;
static int dbgc=0;
#endif
  /* ease the life of caller if it does not check that the value is undef. */
  if( p == NULL )return 0;

#ifdef _DEBUG
  if( p->sType != LARGE_BLOCK_TYPE ){
    q = pMo->MemoryObjectsFreeList[p->sType];
    while( q ){
      if( q == p ){
        printf("A released memory location is rereleased.\n");
        exit(666);
        }
      q = q->next;
      }
   }
#endif

  /* if there is any variable referring to this value then we need this value
     we will swap the two variables. */
  if( p->State == STATE_IMMORTAL && p->link.rprev && p->vType != VTYPE_REF){
    /* change the two variables so that the variable being released will point to
       the REF structure and the first in the list referring to this variable will point
       to the actual value. */
    pRefHead = p->link.rprev;
    /* save this pointer, we will need it after the swap */
    pRef = (*pRefHead)->link.rprev;
    /* perform the swapping and the REF variable will be released (but w/o unlinking!!!!)*/
    pSwap = p;
    p = *pRefHead;
    *pRefHead = pSwap;
    /* The value should start the link correctly, instead of pointing to itself. Before the swap
       the pointer (pRefHead)->link.rprev pointed to the second element (or NULL). This pointer is
       in p->link.rprev after the swap. This has to be placed "back" to the structure pointed by
       *pRefHead */
    (*pRefHead)->link.rprev = pRef;
    /* this is not a reference anymore thus there is no next anymore */
    (*pRefHead)->next = NULL;
    if( pRef )(*pRef)->next = *pRefHead;
    /* alter all other references so that they reference the new head */
    while( pRef ){
      (*pRef)->Value.aValue = pRefHead;
      pRef = (*pRef)->link.rprev;
      }
    }else{/* if a reference variable is deleted then we have to unlink it
    but this is an "else" branch and we do not perform this unlinking when
    releasing a reference variable that was released because it was swapped
    with a non-ref variable Such a ref variable is released the normal way
    just as if it was a long. */
    if( p->State == STATE_IMMORTAL && p->vType == VTYPE_REF ){
      if( p->next )
        p->next->link.rprev = p->link.rprev;
      if( p->link.rprev )
        (*(p->link.rprev))->next = p->next;
      p->link.rprev = NULL;
      p->next = NULL;
      }
    }
  /* if the variable is an array then all the elements should be released */
  if( p->vType == VTYPE_ARRAY ){
    for( i = p->ArrayLowLimit ; i<= p->ArrayHighLimit ; i++ )
      if( p->Value.aValue[i - p->ArrayLowLimit] )
        memory_ReleaseVariable(pMo,p->Value.aValue[i - p->ArrayLowLimit]);
    }

  /* if it is a large block, then release it */
  if( p->sType == LARGE_BLOCK_TYPE ){
    if( p->Value.pValue )
      alloc_Free(p->Value.pValue,pMo->pMemorySegment);
    alloc_Free(p,pMo->pMemorySegment);
    return 0;
    }

  /* if the type is so that it does not use extra memory to store the value, like
     long, double or constant string then use only one list. No matter which, but
     only one. */
  if( p->sType < MAX_FIX_TYPE )
    atype = FIX_TYPE_ALLOC;
  else
    atype = p->sType;

  /* link the item into the free list */
  p->next = pMo->MemoryObjectsFreeList[atype];
  p->link.prev = NULL;
  if( p->next )
    p->next->link.prev = p;
  p->State = STATE_FREE;
  pMo->MemoryObjectsFreeList[atype] = p;
  return 0;
  }

/*POD
=H memory_NewString()

This function should be used to allocate string variable.
/*FUNCTION*/
pFixSizeMemoryObject memory_NewString(pMemoryObject pMo,
                                      unsigned long StringSize
  ){
/*noverbatim

The second argument specifies the length of th required string including.

The function checks the desired length and if this is small then is allocates a fix size
object. If this is too large then it allocates a T<LARGE_BLOCK_TYPE>

CUT*/
  BYTE i;
  pFixSizeMemoryObject p;

  for( i=pMo->FirstStringIndex ; i<= pMo->LastStringIndex ; i++ ){
    if( StringSize <= pMo->SizeOfType[i] ){
      p = memory_NewVariable(pMo,i,0);
      if( p == NULL )return NULL;
      p->vType = VTYPE_STRING;
      p->Size = StringSize;
      return p;
      }
    }
  p = memory_NewVariable(pMo,(int)LARGE_BLOCK_TYPE,StringSize);
  if( p == NULL )return NULL;
  p->vType = VTYPE_STRING;
  return p;
  }

/*POD
=H memory_NewCString()

This function should be used to allocate variable to store a constant string.
/*FUNCTION*/
pFixSizeMemoryObject memory_NewCString(pMemoryObject pMo,
                                       unsigned long StringSize
  ){
/*noverbatim

The second argument specifies the length of the required string.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewVariable(pMo,FIX_TYPE_CSTRING,0);
  if( p == NULL )return NULL;
  p->Size = StringSize;
  p->vType = VTYPE_STRING;
  return p;
  }

/*POD
=H memory_SetRef()

Set the variable T<ppVar> to reference the variable T<ppVal>.

/*FUNCTION*/
int memory_SetRef(pMemoryObject pMo,
                   pFixSizeMemoryObject *ppVar,
                   pFixSizeMemoryObject *ppVal
  ){
/*noverbatim

CUT*/
  unsigned long refcount;
  pFixSizeMemoryObject *ppVAL;
  refcount = pMo->maxderef;

  /* if the value to reference points to a variable that is a reference itself then
     we dereference it so that the reference always points to the final variable
     holding a normal (non-reference) value */
  while( ppVal && *ppVal && (*ppVal)->vType == VTYPE_REF && refcount ){
    ppVal = (*ppVal)->Value.aValue ;
    refcount --;
    }

  if( *ppVal && (*ppVal)->vType == VTYPE_REF )return COMMAND_ERROR_CIRCULAR;

  /* If the variable to be referenced is undef presented as NULL pointer then
     we have to convert it to "stored" undef to have a structure that has space
     for the rprev pointer that points backward.
  */
  if( *ppVal == NULL ){
    *ppVal = memory_NewUndef(pMo);
    if( *ppVal == NULL )return COMMAND_ERROR_MEMORY_LOW;
    }

  ppVAL = ppVal;
  /* now hook the new reference variable to the end of the list of the referencing variables */
  refcount = pMo->maxderef;
  while( (*ppVal)->link.rprev ){
    ppVal = (*ppVal)->link.rprev;
    if( ! refcount-- )return COMMAND_ERROR_CIRCULAR;
    }
  (*ppVal)->link.rprev = ppVar;
  (*ppVar)->next = *ppVal;

  /* now set all variables that were referencing ppVar to reference ppVal */
  refcount = pMo->maxderef;
  while( (*ppVar)->link.rprev ){
    (*ppVar)->Value.aValue = ppVAL;
    ppVar = (*ppVar)->link.rprev;
    if( ! refcount-- )return COMMAND_ERROR_CIRCULAR;
    }
  /* Set the reference even for the last one that has no predeccessor. */
  (*ppVar)->Value.aValue = ppVAL;

  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H memory_NewRef()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewRef(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject p;
  p = memory_NewVariable(pMo,(BYTE)FIX_TYPE_LONG,0);
  if( p == NULL )return NULL;
  p->vType = VTYPE_REF;
  p->State = STATE_IMMORTAL;
  p->link.prev = p->next = NULL;
  p->Value.aValue = NULL;
  return p;
  }

/*POD
=H memory_IsUndef()

This function returns if the examined variable is T<undef>. Since a
variable containing T<undef> but having other variables referencing this
variable is NOT stored as T<NULL> examining the variable agains T<NULL> is
not enough anymore since reference variables were introduced.

/*FUNCTION*/
int memory_IsUndef(pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  if( pVar == NULL )return 1;
  if( pVar->vType == VTYPE_UNDEF )return 1;
  return 0;
  }

/*POD
=H memory_Type()

This function returns the type of the variable. In case the program
does not want to check the T<NULL> undef, but wants to get T<VTYPE_UNDEF>
even if the variable is real T<undef> being T<NULL> calling this function
is safe. Use this function instead of the macro T<TYPE> defined in T<command.h>
is there is doubt.

/*FUNCTION*/
int memory_Type(pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  if( pVar == NULL )return VTYPE_UNDEF;
  return pVar->vType;
  }

/*POD
=H memory_SelfOrRealUndef()

/*FUNCTION*/
pFixSizeMemoryObject memory_SelfOrRealUndef(pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  if( pVar == NULL )return NULL;
  if( pVar->vType == VTYPE_UNDEF )return NULL;
  return pVar;
  }

/*POD
=H memory_NewUndef()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewUndef(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject p;
  p = memory_NewVariable(pMo,(BYTE)FIX_TYPE_LONG,0);
  if( p == NULL )return NULL;
  p->vType = VTYPE_UNDEF;
  p->State = STATE_IMMORTAL;
  p->link.prev = p->next = NULL;
  p->Value.aValue = NULL;
  return p;
  }

/*POD
=H memory_ReplaceVariable()

/*FUNCTION*/
int memory_ReplaceVariable(pMemoryObject pMo,
                           pFixSizeMemoryObject *Lval,
                           pFixSizeMemoryObject NewValue,
                           pMortalList pMortal,
                           int iDupFlag
  ){
/*noverbatim
CUT*/
  int iErrorCode;

  iErrorCode = 0;
  /* This is quite common that a variable is used to hold the same type of value. Changing the value
     only and not allocating a new variable and releasing the old speeds up execution a bit. I could
     see some speed improvement when running the program mandel1.bas */
  if( memory_IsUndef(*Lval) && memory_IsUndef(NewValue) )return COMMAND_ERROR_SUCCESS;

  if( *Lval && NewValue ){
    if( NewValue->vType == VTYPE_LONG && (*Lval)->vType == VTYPE_LONG ){
      LONGVALUE(*Lval) = LONGVALUE(NewValue);
      return COMMAND_ERROR_SUCCESS;
      }

    if( NewValue->vType == VTYPE_DOUBLE && (*Lval)->vType == VTYPE_DOUBLE ){
      DOUBLEVALUE(*Lval) = DOUBLEVALUE(NewValue);
      return COMMAND_ERROR_SUCCESS;
      }
    }

  /* after the references have been looked after we duplicate the result to have an own copy of the value. */
  if( NewValue && iDupFlag ){
    NewValue = memory_DupMortalize(pMo,NewValue,pMortal,&iErrorCode);
    if( iErrorCode )return iErrorCode;
    }


  /* if the result of the expression is not undef then immortalize */
  if( NewValue )
    /* we immortalize the new variable if it is a variable and not NULL meaning undef */
    memory_Immortalize(NewValue,pMortal);

  /* if there are other variables that reference this variable and the value is undef then we need a
      stored undef */
  if( *Lval && (*Lval)->link.rprev && NewValue == NULL )NewValue = memory_NewUndef(pMo);

  /* The new value will hold the same reference information as the former value.
     The former on the other hand is going to be unhooked. */
  if( *Lval && (*Lval)->link.rprev ){
    NewValue->link.rprev = (*Lval)->link.rprev;
    if( NewValue->link.rprev )(*(NewValue->link.rprev))->next = NewValue;
    (*Lval)->link.rprev = NULL;
    NewValue->next = (*Lval)->next;
    (*Lval)->next = NULL;
    }
  /* if this variable had value assigned to it then release that value */
  if( *Lval )memory_ReleaseVariable(pMo,*Lval);

  /* and finally assign the code to the variable */
  *Lval = NewValue;

  return COMMAND_ERROR_SUCCESS;
  }

/*POD
=H memory_NewLong()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewLong(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject p;
  p = memory_NewVariable(pMo,(BYTE)FIX_TYPE_LONG,0);
  if( p == NULL )return NULL;
  p->vType = VTYPE_LONG;
  p->State = STATE_IMMORTAL;
  p->link.prev = p->next = NULL;
  return p;
  }

/*POD
=H memory_NewDouble()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewDouble(pMemoryObject pMo
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject p;
  p = memory_NewVariable(pMo,FIX_TYPE_DOUBLE,0);
  if( p == NULL )return NULL;
  p->vType = VTYPE_DOUBLE;
  p->link.prev = p->next = NULL;
  p->State = STATE_IMMORTAL;
  return p;
  }

/*POD
=H memory_CopyArray

/*FUNCTION*/
pFixSizeMemoryObject memory_CopyArray(pMemoryObject pMo,
                                      pFixSizeMemoryObject p
  ){
/*noverbatim
CUT*/
  long aLow,aHigh;
  pFixSizeMemoryObject result;
  long i;

  if( p == NULL )return NULL;
  if( p->vType != VTYPE_ARRAY )return NULL;
  aLow = p->ArrayLowLimit;
  aHigh = p->ArrayHighLimit;
  result = memory_NewArray(pMo,aLow,aHigh);
  if( result == NULL )return NULL;
  for( i=0 ; i <= aHigh-aLow ; i++ ){
    if( memory_IsUndef(p->Value.aValue[i]) )continue;
    switch(p->Value.aValue[i]->vType ){
      case VTYPE_ARRAY :
        result->Value.aValue[i] = memory_CopyArray(pMo,p->Value.aValue[i]);
        if( result->Value.aValue[i] == NULL )return NULL;
        continue;
      case VTYPE_LONG:
        result->Value.aValue[i] = memory_NewLong(pMo);
        if( result->Value.aValue[i] == NULL )return NULL;
        result->Value.aValue[i]->Value.lValue = p->Value.aValue[i]->Value.lValue;
        continue;
      case VTYPE_DOUBLE:
        result->Value.aValue[i] = memory_NewDouble(pMo);
        if( result->Value.aValue[i] == NULL )return NULL;
        result->Value.aValue[i]->Value.dValue = p->Value.aValue[i]->Value.dValue;
        continue;
      case VTYPE_STRING:
        result->Value.aValue[i] = memory_NewString(pMo,STRLEN(p->Value.aValue[i]));
        if( result->Value.aValue[i] == NULL )return NULL;
        memcpy(result->Value.aValue[i]->Value.pValue,
                    p->Value.aValue[i]->Value.pValue,STRLEN(p->Value.aValue[i]));
        continue;
      case VTYPE_REF:
        result->Value.aValue[i] = memory_NewRef(pMo);
        memory_SetRef(pMo,&(result->Value.aValue[i]),&(p->Value.aValue[i]));
        continue;
      }
    }
  return result;
  }

/*POD
=H memory_NewArray()

This function should be used whenever a new array is to be allocated.

/*FUNCTION*/
pFixSizeMemoryObject memory_NewArray(pMemoryObject pMo,
                                     long LowIndex,
                                     long HighIndex
  ){
/*noverbatim
The index variables define the indices that are to be used when accessing an
array element. The index values are inclusive.
CUT*/
  pFixSizeMemoryObject p;
  long i;

  p = memory_NewVariable(pMo,LARGE_BLOCK_TYPE, (HighIndex-LowIndex+1)*sizeof(void *));
  if( p == NULL )return NULL;
  p->vType = VTYPE_ARRAY;
  p->ArrayHighLimit = HighIndex;
  p->ArrayLowLimit = LowIndex;
  /* initialize the array */
  for( i=LowIndex ; i <= HighIndex ; i++ )
    p->Value.aValue[i - LowIndex] = NULL;
  p->link.prev = p->next = NULL;
  p->State = STATE_IMMORTAL;
  return p;
  }

/*POD
=H memory_ReDimArray()

This function should be used when an array needs redimensioning.
If the redimensioning is succesful the function returns the pointer
to the argument T<p>. If memory allocation is needed and the memory
allocation fails the function returns T<NULL>. In this case the 
original array is not changed.

If the redimensioned array is smaller that the original no memory allocation
takes place, only the array elements (pointers) are moved.

/*FUNCTION*/
pFixSizeMemoryObject memory_ReDimArray(pMemoryObject pMo,
                                       pFixSizeMemoryObject p,
                                       long LowIndex,
                                       long HighIndex
  ){
/*noverbatim
CUT*/
  unsigned long NewSize;
  long i;
  pFixSizeMemoryObject *pValue;

  NewSize = (HighIndex-LowIndex+1)*sizeof(void*);
  if( NewSize > p->Size ){
    pValue = alloc_Alloc(NewSize,pMo->pMemorySegment);
    if( pValue == NULL )return NULL;

    for( i = LowIndex ; i<= HighIndex ; i++ )
      if( i < p->ArrayLowLimit || i > p->ArrayHighLimit )
        pValue[i-LowIndex] = NULL;
      else{
        pValue[i-LowIndex] = p->Value.aValue[i-p->ArrayLowLimit];
        /* If this is a reference value and there is a next (should be) than that points
           to this variable in the field link.rprev. Because we allocated a new variable
           as an array element we have to alter that pointer to point to the new variable. */
        if( p->Value.aValue[i-p->ArrayLowLimit] && 
            p->Value.aValue[i-p->ArrayLowLimit]->vType == VTYPE_REF &&
            (p->Value.aValue[i-p->ArrayLowLimit])->next )
          (p->Value.aValue[i-p->ArrayLowLimit])->next->link.rprev = pValue+i-LowIndex;
        }

    alloc_Free(p->Value.pValue,pMo->pMemorySegment);
    p->Value.aValue = pValue;
    p->ArrayHighLimit = HighIndex;
    p->ArrayLowLimit = LowIndex;
    return p;
    }

  pValue = p->Value.aValue;
  if( LowIndex < p->ArrayLowLimit )
    for( i = HighIndex ; i>= LowIndex ; i-- )
      if( i < p->ArrayLowLimit || i > p->ArrayHighLimit )
        pValue[i-LowIndex] = NULL;
      else{
        pValue[i-LowIndex] = pValue[i-p->ArrayLowLimit];
        /* see comments above */
        if( p->Value.aValue[i-p->ArrayLowLimit]->vType == VTYPE_REF &&
            (p->Value.aValue[i-p->ArrayLowLimit])->next )
          (p->Value.aValue[i-p->ArrayLowLimit])->next->link.rprev = pValue+i-LowIndex;
        }
  else
    for( i = LowIndex ; i<= HighIndex ; i++ )
      if( i < p->ArrayLowLimit || i > p->ArrayHighLimit )
        pValue[i-LowIndex] = NULL;
      else{
        pValue[i-LowIndex] = pValue[i-p->ArrayLowLimit];
        /* see comments above */
        if( p->Value.aValue[i-p->ArrayLowLimit]->vType == VTYPE_REF &&
            (p->Value.aValue[i-p->ArrayLowLimit])->next )
          (p->Value.aValue[i-p->ArrayLowLimit])->next->link.rprev = pValue+i-LowIndex;
        }
  p->ArrayHighLimit = HighIndex;
  p->ArrayLowLimit = LowIndex;
  return p;
  }

/*POD
=H memory_CheckArrayIndex()

This function should be called before accessing a certain element of an array.
The function checks that the index is within the index limitsof the array
and in case the index is outside the index limits of the array it redimensionate the
array.

The function returns the pointer passed as parameter T<p> or NULL in case there is a
memory allocation error.
/*FUNCTION*/
pFixSizeMemoryObject memory_CheckArrayIndex(pMemoryObject pMo,
                                            pFixSizeMemoryObject p,
                                            long Index
  ){
/*noverbatim
CUT*/

  if( p->ArrayHighLimit < Index )
    return memory_ReDimArray(pMo,p,p->ArrayLowLimit,Index);
  if( p->ArrayLowLimit > Index )
    return memory_ReDimArray(pMo,p,Index,p->ArrayHighLimit);
  return p;
  }


/*POD
=H memory_Mortalize()

This function should be used when a variable is to be put in a mortal list.

/*FUNCTION*/
void memory_Mortalize(pFixSizeMemoryObject p,
                      pMortalList pMortal
  ){
/*noverbatim
Note that care should be taken to be sure that the variable is NOT on a mortal
list. If the variable is already on a mortal list calling this function will
break the original list and therefore may loose the variables that follow this one.
CUT*/

  p->next = *pMortal;
  if( p->next )
    p->next->link.prev = p;
  p->link.prev = NULL;
  p->State = STATE_MORTAL;
  *pMortal = p;
  return;
  }

/*POD
=H memory_Immortalize()

Use this function to immortalize a variable. This can be used when the result of an expression
evaluation gets into a mortal variable and instead of copiing the value from the mortal variable to
an immortal variable the caller can immortalize the variable. However it should know which mortal list
the variable is on.

/*FUNCTION*/
void memory_Immortalize(pFixSizeMemoryObject p,
                        pMortalList pMortal
  ){
/*noverbatim
CUT*/
  if( ! p )return;/* the code tries to immortalize undef at several location, therefore it is better
                     to handle this situation here. */
  if( p->State == STATE_IMMORTAL )return;
  if( p->link.prev )
    p->link.prev->next = p->next;
  else
    *pMortal = p->next;

  if( p->next )
    p->next->link.prev = p->link.prev;

  p->link.prev = NULL;
  p->next = NULL;
  p->State = STATE_IMMORTAL;
  return;
  }

/*POD
=H memory_NewMortal()

When an expression is evaluated mortal variables are needed to store
the intermediate results. These variables are called mortal variables.
Such a variable is is allocated using this function and specifying a
variable of type T<MortalList> to assign the mortal to the list of
mortal variables.

When the expression is evaluated all mortal variables are to be released
and they are calling the function T<memory_ReleaseMortals> (see R<memory_ReleaseMortals()>).
/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortal(pMemoryObject pMo,
                                      BYTE type,
                                      unsigned long LargeBlockSize,
                                      pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewVariable(pMo,type,LargeBlockSize);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }


/*POD
=H memory_DupImmortal()

This function creates a new mortal and copies the argument T<pVar> into this
new mortal.

/*FUNCTION*/
pFixSizeMemoryObject memory_DupImmortal(pMemoryObject pMo,
                                        pFixSizeMemoryObject pVar,
                                        int *piErrorCode
  ){
/*noverbatim
CUT*/
  pFixSizeMemoryObject mypVar ;
  int i;

  *piErrorCode = MEM_ERROR_SUCCESS;
  if( pVar == NULL )return NULL;

  mypVar = memory_NewVariable(pMo,pVar->sType,pVar->Size);
  if( mypVar == NULL ){
    *piErrorCode = MEM_ERROR_MEMORY_LOW;
    return NULL;
    }

  mypVar->vType = pVar->vType;
  mypVar->Size  = pVar->Size;
  if( mypVar->vType == VTYPE_ARRAY ){
    mypVar->ArrayHighLimit = pVar->ArrayHighLimit;
    mypVar->ArrayLowLimit = pVar->ArrayLowLimit;
    for( i = 0 ; i <= mypVar->ArrayHighLimit - mypVar-> ArrayLowLimit ; i++ ){
      mypVar->Value.aValue[i] = memory_DupImmortal(pMo,pVar->Value.aValue[i],piErrorCode);
      if( *piErrorCode )return NULL;
      }
    }else{
    if( pVar->sType == LARGE_BLOCK_TYPE ){
      if( pVar->Size )
        memcpy(mypVar->Value.pValue, pVar->Value.pValue, mypVar->Size);
      }else{
      if( pVar->vType == VTYPE_STRING && pVar->sType != FIX_TYPE_CSTRING ){
        if( pVar->Size )
          memcpy(mypVar->Value.pValue,pVar->Value.pValue,mypVar->Size);
        }else{
        mypVar->Value = pVar->Value;
        }
      }
    }
  return mypVar;
  }

/*POD
=H memory_DupVar()

This function creates a new mortal and copies the argument T<pVar> into this
new mortal.

/*FUNCTION*/
pFixSizeMemoryObject memory_DupVar(pMemoryObject pMo,
                                   pFixSizeMemoryObject pVar,
                                   pMortalList pMyMortal,
                                   int *piErrorCode
  ){
/*noverbatim
This function is vital, when used in operations that convert the
values to T<long> or T<double>. Expression evaluation may return an immortal
value, when the expression is a simple variable access. Conversion of the
result would modify the value of the variable itself. Therefore functions and
operators call this function to duplicate the result to be sure that the value
they convert is mortal and to be sure they do not change the value of a variable
when they are not supposed to.

Note that you can duplicate T<long>, T<double> and T<string> values, but you can not
duplicate arrays! The string value is duplicated and the characters are copied to
the new location. This is perfect. However if you do the same with an array the array
pointers will point to the same variables, which are not going to be duplicated. This
result multiple reference to a single value. This situation is currently not supported
by this system as we do not have either garbage collection or any other solution to support
such memory structures.
CUT*/
  pFixSizeMemoryObject mypVar ;

  *piErrorCode = MEM_ERROR_SUCCESS;
  if( pVar == NULL )return NULL;

  if( pVar->vType == VTYPE_ARRAY ){
    *piErrorCode = MEM_ERROR_INTERNAL001;
    return NULL;
    }

  mypVar = memory_NewMortal(pMo,pVar->sType,pVar->Size,pMyMortal);
  if( mypVar == NULL ){
    *piErrorCode = MEM_ERROR_MEMORY_LOW;
    return NULL;
    }

  mypVar->vType = pVar->vType;
  mypVar->Size  = pVar->Size;
  if( pVar->sType == LARGE_BLOCK_TYPE ){
    if( pVar->Size )
      memcpy(mypVar->Value.pValue, pVar->Value.pValue, mypVar->Size);
    }else{
    if( pVar->vType == VTYPE_STRING && pVar->sType != FIX_TYPE_CSTRING ){
      if( pVar->Size )
        memcpy(mypVar->Value.pValue,pVar->Value.pValue,mypVar->Size);
      }else{
      memcpy(&(mypVar->Value),&(pVar->Value),sizeof(union _fsmoval));
      }
    }
  return mypVar;
  }

/*POD
=H memory_DupMortalize()

This function creates a new mortal and copies the argument T<pVar> into this
new mortal only if the value is immortal. If the value is mortal the it returns
the original value.

/*FUNCTION*/
pFixSizeMemoryObject memory_DupMortalize(pMemoryObject pMo,
                                         pFixSizeMemoryObject pVar,
                                         pMortalList pMyMortal,
                                         int *piErrorCode
  ){
/*noverbatim
CUT*/

  if( *piErrorCode )return NULL;/* if it was called after an erroneous evaluate then do not reset the error code. */
  if( pVar && IsMortal(pVar) )return pVar;
  return memory_DupVar(pMo,pVar,pMyMortal,piErrorCode);
  }

/*POD
=H memory_ReleaseMortals()

This function should be used to release the mortal variables.

When an expression is evaluated mortal variables are needed to store
the intermediate results. These variables are called mortal variables.
Such a variable is is allocated using this function and specifying a
variable of type T<MortalList> to assign the mortal to the list of
mortal variables.

/*FUNCTION*/
void memory_ReleaseMortals(pMemoryObject pMo,
                           pMortalList pMortal
  ){
/*noverbatim
CUT*/
  MortalList p;

  if( pMortal == NULL )return; /* this is to help lasy callers */
  while( p=*pMortal ){
    *pMortal = p->next;
    memory_ReleaseVariable(pMo,p);
    }
  *pMortal = NULL;
  }

#ifdef _DEBUG
/*POD
=H memory_DebugDumpVariable()

This function is used for debugging purposes. (To debug ScriptBasic and not
to debug a BASIC program using ScriptBasic. T<:-o> )

The function prints the content of a variable to the standard output.

/*FUNCTION*/
void memory_DebugDumpVariable(pMemoryObject pMo,
                              pFixSizeMemoryObject pVar
  ){
/*noverbatim
CUT*/
  long i;

  if( pVar == NULL ){
    printf("real undef");
    return;
    }

  switch( pVar->vType ){
    case VTYPE_UNDEF:
      printf("ref undef");
      return;

    case VTYPE_STRING:
      printf("string(%d)[",STRLEN(pVar));
      for( i=0 ;  ((unsigned long)i) < STRLEN(pVar) ; i++ ){
        printf("%c",STRINGVALUE(pVar)[i]);
        }
      printf("]");
      return;

    case VTYPE_LONG:
      printf("long[%d]",LONGVALUE(pVar));
      return;

    case VTYPE_DOUBLE:
      printf("long[%f]",DOUBLEVALUE(pVar));
      return;

    case VTYPE_ARRAY:
      printf("ARRAY(%d,%d)\n[",pVar->ArrayLowLimit, pVar->ArrayHighLimit);
      for( i=pVar->ArrayLowLimit ; i <= pVar->ArrayHighLimit  ; i++ ){
        memory_DebugDumpVariable(pMo,pVar->Value.aValue[i-pVar->ArrayLowLimit]);
        }
      printf(" ]");
      return;

    case VTYPE_REF:
      printf("->");
      memory_DebugDumpVariable(pMo,*(pVar->Value.aValue));
      return;
    }

  }

/*POD
=H memory_DebugDumpMortals()

This function is used for debugging purposes. (To debug ScriptBasic and not
to debug a BASIC program using ScriptBasic. T<:-o> )

The function prints the content of the mortal list to the standard output.

/*FUNCTION*/
void memory_DebugDumpMortals(pMemoryObject pMo,
                             pMortalList pMortal
  ){
/*noverbatim
CUT*/
  MortalList p,q;
  long i;

  if( pMortal == NULL ){
    printf("This mortal list is empty\n");
    return; /* this is to help lasy callers */
    }

  printf("Mortal list %p:\n",pMortal);
  i = 1;
  q = *pMortal;
  while( p = q ){
    q = p->next;
    printf("%d. ",i++);
    memory_DebugDumpVariable(pMo,p);
    printf("\n");
    }
  }
#endif

/*POD
=H memory_NewMortalString()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalString(pMemoryObject pMo,
                                            unsigned long StringSize,
                                            pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewString(pMo,StringSize);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }

/*POD
=H memory_NewMortalCString()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalCString(pMemoryObject pMo,
                                             unsigned long StringSize,
                                             pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewCString(pMo,StringSize);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }

/*POD
=H memory_NewMortalLong()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalLong(pMemoryObject pMo,
                                            pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewLong(pMo);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }

/*POD
=H memory_NewMortalRef()

This function was never used. It was presented in the code to allow external modules to
create mortal reference variables. However later I found that the variable structure design does
not allow mortal reference variables and thus this function is nonsense.

Not to change the module interface defintion the function still exists but returns NULL, like if
memory were exhausted.

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalRef(pMemoryObject pMo,
                                         pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/

  /* just in case somebody uses this function to get immortal refs */
  if( pMortal == NULL )return memory_NewRef(pMo);
  return NULL;
#if 0
  /* see the documentation above why this is commented out */
  pFixSizeMemoryObject p;
  p = memory_NewRef(pMo);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
#endif
  }


/*POD
=H memory_NewMortalDouble()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalDouble(pMemoryObject pMo,
                                            pMortalList pMortal
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewDouble(pMo);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }


/*POD
=H memory_NewMortalArray()

/*FUNCTION*/
pFixSizeMemoryObject memory_NewMortalArray(pMemoryObject pMo,
                                           pMortalList pMortal,
                                           long IndexLow,
                                           long IndexHigh
  ){
/*noverbatim
If the parameter T<pMortal> is T<NULL> the generated variable is not mortal.
CUT*/
  pFixSizeMemoryObject p;

  p = memory_NewArray(pMo,IndexLow,IndexHigh);
  if( p == NULL )return NULL;
  if( pMortal )
    memory_Mortalize(p,pMortal);
  return p;
  }
