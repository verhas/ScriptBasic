/* 
FILE:   myalloc.c
HEADER: myalloc.h

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


/* Define it 0 if you do not want to compile a multi threaded version */
#ifndef MTHREAD
#define MTHREAD 0
#endif

#include <stdlib.h>
#include <stdio.h>

/* for offsetof */
#include <stddef.h>
#ifdef _DEBUG
#include <memory.h>
#endif

#include "myalloc.h"

static unsigned long MaxNetSize,
                     MinNetSize,
                     MaxBruSize,
                     MinBruSize,
                     ActNetSize,
                     ActBruSize;
#if MTHREAD
#include "thread.h"

#define LOCK_SEGMENT    thread_LockMutex(&(pMemorySegment->mxSegment));
#define UNLOCK_SEGMENT  thread_UnlockMutex(&(pMemorySegment->mxSegment));

static unsigned char fLockMalloc=0;
static MUTEX mxMalloc;
#define LOCK_MALLOC   if( fLockMalloc )thread_LockMutex(&mxMalloc);
#define UNLOCK_MALLOC if( fLockMalloc )thread_UnlockMutex(&mxMalloc);

static MUTEX mxStat;
#define LOCK_STAT   thread_LockMutex(&mxStat);
#define UNLOCK_STAT thread_UnlockMutex(&mxStat);

#else

#define LOCK_SEGMENT
#define UNLOCK_SEGMENT
#define LOCK_MALLOC
#define UNLOCK_MALLOC
#define LOCK_STAT
#define UNLOCK_STAT
#endif

/* lock the statistics, the modify the global values */
#define STAT_ADD(NET,BRU) LOCK_STAT \
                          ActNetSize += (NET); \
                          if( ActNetSize > MaxNetSize )MaxNetSize = ActNetSize; \
                          if( ActNetSize < MinNetSize )MinNetSize = ActNetSize; \
                          ActBruSize += (BRU); \
                          if( ActBruSize > MaxBruSize )MaxBruSize = ActBruSize; \
                          if( ActBruSize < MinBruSize )MinBruSize = ActBruSize; \
                          UNLOCK_STAT

#define STAT_SUB(NET,BRU) LOCK_STAT \
                          ActNetSize -= (NET); \
                          if( ActNetSize > MaxNetSize )MaxNetSize = ActNetSize; \
                          if( ActNetSize < MinNetSize )MinNetSize = ActNetSize; \
                          ActBruSize -= (BRU); \
                          if( ActBruSize > MaxBruSize )MaxBruSize = ActBruSize; \
                          if( ActBruSize < MinBruSize )MinBruSize = ActBruSize; \
                          UNLOCK_STAT

typedef struct _AllocUnit {
  unsigned long      Size;          /* size of the chunk */
  struct _AllocUnit *next;     /* link to the next unit */
  struct _AllocUnit *prev;     /* the previous unit */
  unsigned char      memory[1];     /* one or more bytes reserved */
  } AllocUnit, *pAllocUnit;
/* Note that the last member before 'memory' is a pointer which should
   provide sufficient alignment for 'memory' on 32bit and on 64bit systems as well */

typedef struct _MyAlloc {
#if MTHREAD
  MUTEX mxSegment;
#endif
  void * (*memory_allocating_function)(size_t);
  void (*memory_releasing_function)(void *);

  unsigned long MaxSize,CurrentSize,
                MaxNetSize,MinNetSize;

  pAllocUnit FirstUnit;
  } MyAlloc, *pMyAlloc;


/*POD
=H Multi-thread use of this module

You can use this module in multi threaded environment. In this case the module depend on the module T<thread.c>
which contains the thread and mutex interface functions that call the operating system thread and mutex functions
on UNIX and on Windows NT.

In single thread environment there is no need to use the locking mechanism. To get a single-thread version either you can
edit this file (T<myalloc.c>) or compile is using the option T<-DMTHREAD=0> The default compilation is multi threaded.

Multi thread implementation has two levels. One is that the subroutines implemented in this module call
the appropriate locking functions to ensure that no two concurrent threads access and modify the same data at a time
and thus assure that the data of the module is correct. The other level is that you can tell the module that the
underlying memory allocation and deallocation modules are mot thread safe. There are global variables
implementing global mutexes that are locked and unlocked if you use the module that way. This can be useful in some
environment where T<malloc> and T<free> are not thread safe.

Note that this should not be the case if you call T<malloc> and T<free> or you linked the wrong versio of libc.
However you may use a non-thread safe debug layer for example the one that ScriptBasic uses.

CUT*/

/*POD
=H alloc_InitSegment()

Call this function to get a new segment. You should specify the functions that the segement should use to
get memory from the operating system, and the function the segment should use to release the memory to the
operating system. These functions should be like T<malloc> and T<free>.

If the second argument is T<NULL> then the function will treat the first argument as an already
allocated and initialized memory segment and the memory allocation and freeing functions will be
inherited from that segment.

/*FUNCTION*/
void *alloc_InitSegment(void * (*maf)(size_t), /* a 'malloc' and a 'free' like functions */
                        void   (*mrf)(void *)
  ){
/*noverbatim

The return value is a T<void*> pointer which identifies the segment and should be passed to the other functions
as segment argument.

The first argument is the T<malloc> like function and the second if the T<free> like function.
CUT*/
  pMyAlloc pMemorySegment;

  /* If the second argument is T<NULL> then the first argument is a valid and initialized memory
     segment and in this case the primitive allocation functions are inherited */
  if( mrf == NULL ){
    pMemorySegment = (pMyAlloc)maf;
    maf = pMemorySegment->memory_allocating_function;
    mrf = pMemorySegment->memory_releasing_function;
    }

#if MTHREAD
  if( fLockMalloc )thread_LockMutex(&mxMalloc);
#endif

  pMemorySegment = (pMyAlloc)maf(sizeof(MyAlloc));
  STAT_ADD( 0 , sizeof(MyAlloc) )

#if MTHREAD
  if( fLockMalloc )thread_UnlockMutex(&mxMalloc);
#endif

  if( pMemorySegment == NULL )return NULL;
  pMemorySegment->memory_allocating_function = maf;
  pMemorySegment->memory_releasing_function = mrf;
  pMemorySegment->FirstUnit = NULL;
  pMemorySegment->CurrentSize = 0; /* the segment is initialized, no memory is allocated */
  pMemorySegment->MaxSize = 0; /* there is no limit by default */
  pMemorySegment->MaxNetSize = 0;
  pMemorySegment->MinNetSize = 0;
#if MTHREAD
  thread_InitMutex(&(pMemorySegment->mxSegment));
#endif
  return (void *)pMemorySegment;
  }

/*POD
=H alloc_GlobalUseGlobalMutex()

Some installation use memory allocation function that are not thread safe. On some
UNIX installations T<malloc> is not thread safe. To tell the module that all the allocation
function primitives are not thread safe call this function before initializing any segment.

/*FUNCTION*/
void alloc_GlobalUseGlobalMutex(
  ){
/*noverbatim
CUT*/
#if MTHREAD
  fLockMalloc = 1;
  thread_InitMutex(&mxMalloc);
#else
  exit(63);
#endif
  }

/*POD
=H alloc_SegmentLimit()

You can call this function to set a segment limit. Each segment keeps track of the actual memory
allocated to the segment. When a new piece of memory allocated in a segment the calculated segment
size is increased by the size of the memory chunk. When a piece of memory is release the calculated
size of the segment is decreased.

Whenever a segment approaches its limit the next allocation function requesting memory
that would exceed the limit returns T<NULL> and does not allocate memory.

The value of the limit is the number of bytes allowed for the segment. This is the requested number of
bytes without the segment management overhead.

Setting the limit to zero means no limit except the limits of the underlying memory allocation layers,
usually T<malloc>.

You can dynamically set the limit during handling the memory at any time except that you should not
set the limit to zero unless the segment is empty and you should not set the limit to a positive value
when the actual limit is zero (no limit) and the segment is not empty. This restriction is artificial
in this release but is needed to be followed to be compatible with planned future developments.

This function sets the limit for the segment pointed by T<p> and returns the old value of the segment.
/*FUNCTION*/
long alloc_SegmentLimit(void *p,
                        unsigned long NewMaxSize
  ){
/*noverbatim
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;
  unsigned long lOldValue;

  LOCK_SEGMENT
  lOldValue = pMemorySegment->MaxSize;
  pMemorySegment->MaxSize = NewMaxSize;
  UNLOCK_SEGMENT
  return lOldValue;
  }
/*POD
=H alloc_FreeSegment()

Use this function to release all the memory that was allocated to the segment T<p>.
Note that after calling this function the segment is still usable, only the memory
that it handled was released. If you do not need the segment anymore call the function
R<alloc_FinishSegment()> that calls this function and then releases the memory allocated to store
the segment information.

Sloppy programmers may pass T<NULL> as argument, it just returns.
/*FUNCTION*/
void alloc_FreeSegment(void *p
  ){
/*noverbatim
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;
  void   (*mrf)(void *);
  pAllocUnit pAllU = pMemorySegment->FirstUnit;
  pAllocUnit pNext;

  if( !p )return;
  mrf = pMemorySegment->memory_releasing_function;
  LOCK_SEGMENT
  while( pAllU ){
    pNext = pAllU->next;
    LOCK_MALLOC
    STAT_SUB( pAllU->Size , pAllU->Size+sizeof(AllocUnit)-1 )
#ifdef _DEBUG
//    memset(pAllU,0x80,pAllU->Size+sizeof(AllocUnit)-1);
#endif
    mrf(pAllU);
    UNLOCK_MALLOC
    pAllU = pNext;
    }
  pMemorySegment->FirstUnit = NULL;
  pMemorySegment->CurrentSize = 0; /* the segment is initialized, no memory is allocated */
  pMemorySegment->MinNetSize = 0; /* MinNetSiez can not be smaller, and MaxNetSize can only be larger. */
  UNLOCK_SEGMENT
  }

/*POD
=H alloc_FinishSegment()

Use this function to release all the memory that was allocated to the segment T<p>.
This function also releases the memory of the segment head and therefore the
segment pointed by T<p> is not usable anymore.

/*FUNCTION*/
void alloc_FinishSegment(void *p
  ){
/*noverbatim
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;
  void   (*mrf)(void *);

  if( !p )return;
  mrf = pMemorySegment->memory_releasing_function;
  alloc_FreeSegment(p);
#if MTHREAD
  thread_FinishMutex(&(pMemorySegment->mxSegment));
#endif
  /* here we can not lock the segment because the segment including the mutex is deleted */
  LOCK_MALLOC
  mrf(pMemorySegment);
  UNLOCK_MALLOC
  STAT_SUB( 0 , sizeof(pMyAlloc) );
  }

/*POD
=H alloc_Alloc()

Use this function to allocate a memory piece from a segment.

/*FUNCTION*/
void *alloc_Alloc(size_t n,
                  void *p
  ){
/*noverbatim

The first argument is the size to be allocated. The second argument is the
segment which should be used for the allocation.

If the memory allocation fails the function returns T<NULL>.
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;
  pAllocUnit pAllU;

  if( n == 0 )
    return NULL;
#ifdef _DEBUG
  if( pMemorySegment == NULL ){
    exit(666);
    }
#endif
  /* Check if there is any limit for this segment and if we have reached the limit. */
  if( pMemorySegment->MaxSize && pMemorySegment->MaxSize < pMemorySegment->CurrentSize + n )
    return NULL;

  LOCK_SEGMENT

  LOCK_MALLOC

/* When we debug the buffer over runs should be detected by all means. When in release mode
   the bugs if any should be handled more generous. Do not kill the program if I still forgot
   to correct some code where the string terminating zero was neglected and no buffer allocated
   for it. Anyway: older code allocated extra memory because of gaps in the structure.
*/
#ifdef _DEBUG
#define EXTRA_BYTE (0)
#else
#define EXTRA_BYTE (1)
#endif

  pAllU = (pAllocUnit)pMemorySegment->memory_allocating_function( n + offsetof(struct _AllocUnit,memory) + EXTRA_BYTE);
  UNLOCK_MALLOC

  if( pAllU == NULL ){
    UNLOCK_SEGMENT
    return NULL;
    }

  pAllU->prev = NULL;
  pAllU->next = pMemorySegment->FirstUnit;
  pAllU->Size = n;

  pMemorySegment->CurrentSize += n;
  if( pMemorySegment->CurrentSize > pMemorySegment->MaxNetSize )
    pMemorySegment->MaxNetSize = pMemorySegment->CurrentSize;

  if( pMemorySegment->FirstUnit )pMemorySegment->FirstUnit->prev = pAllU;
  pMemorySegment->FirstUnit = pAllU;
  /* return a void* pointer that points to the allocated memory after the header */
  UNLOCK_SEGMENT

  STAT_ADD( n , n-1 + sizeof(AllocUnit) )
  return (void *)( pAllU->memory );
  }

/*POD
=H alloc_Free()

You should call this function whenever you want to release a single piece of memory
allocated from a segment. Note that you also have to pass the segment pointer as the
second argument, because the segment head pointed by this T<void> pointer contains the
memory releasing function pointer.

Sloppy programmers may try to release T<NULL> pointer without harm.
/*FUNCTION*/
void alloc_Free(void *pMem, void *p
  ){
/*noverbatim
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;
  pAllocUnit pAllU,pAllUi;

  if( pMemorySegment == NULL )return;
  if( pMem == NULL )return;
  LOCK_SEGMENT
  pAllU = (pAllocUnit)( ((unsigned char *)pMem) - offsetof(struct _AllocUnit,memory) ); 
#ifdef _DEBUG
  pAllUi = pMemorySegment->FirstUnit;
  while( pAllUi ){
    if( pAllUi == pAllU )break;
    pAllUi = pAllUi->next;
    }
  if( pAllUi == NULL ){
    printf("Memory segment was released not belonging to the segment.\n");
    exit(1);
    }
#endif
  pMemorySegment->CurrentSize -= pAllU->Size;
  if( pMemorySegment->CurrentSize < pMemorySegment->MinNetSize )
    pMemorySegment->MinNetSize = pMemorySegment->CurrentSize;

  if( pAllU->next )
    pAllU->next->prev = pAllU->prev;
  if( pAllU->prev )
    pAllU->prev->next = pAllU->next;
  else
    pMemorySegment->FirstUnit = pAllU->next;

  STAT_SUB( pAllU->Size , pAllU->Size + sizeof(AllocUnit) -1 )

#ifdef _DEBUG
//    memset(pAllU,0x80,pAllU->Size+sizeof(AllocUnit)-1);
#endif
  LOCK_MALLOC
  pMemorySegment->memory_releasing_function(pAllU);
  UNLOCK_MALLOC

  UNLOCK_SEGMENT
  }

/*POD
=H alloc_Merge()

Call this function in case you want to merge a segment into another. This can be the
case when your program builds up a memory structure in several steps.

This function merges the segment T<p2> into T<p1>. This means that the segment T<p1> will
contain all the memory pieces that belonged to T<p2> before and T<p2> will not contain any
allocated memory. However the segment T<p2> is still valid and can be used to allocated memory
from. If you also want to finish the segment T<p2> call the function R<alloc_MergeAndFinish()>.

/*FUNCTION*/
void alloc_Merge(void *p1, void *p2
  ){
/*noverbatim

Note that the two segments SHOULD use the same, or at least compatible system memory handling functions!
You better use the same functions for both segments.

Example:

ScriptBasic builds up a sophisticated memory structure during syntactical analysis. This memory structure
contains the internal code generated from the program lines of the basic program. When ScriptBasic analyses
a line it tries several syntax descriptions. It checks each syntax defintion against the tokens of the line
until it finds one that fits. These checks need to build up memory structure. However if the check fails and
ScriptBasic should go for the next syntac definition line to check the memory allocated during the failed
checking should be released. Therefore these memory pieces are allocated from a segment that the program
calls T<pMyMemorySegment>. If the syntax check fails this segment if freed. If the syntax check succedes this
segment is merged into another segement that contains the memory structures allocated from the previous basic program
lines.
CUT*/
  pMyAlloc p1MemorySegment = (pMyAlloc)p1;
  pMyAlloc p2MemorySegment = (pMyAlloc)p2;
  pAllocUnit *p,q;

#if MTHREAD
  thread_LockMutex(&(p1MemorySegment->mxSegment));
  thread_LockMutex(&(p2MemorySegment->mxSegment));
#endif
  /* go to the end of the segment #1 */
  p = &(p1MemorySegment->FirstUnit);
  q = p1MemorySegment->FirstUnit;
  while( *p )p = &((q=*p)->next);
  /* link the first unit of segment #2 */
  *p = p2MemorySegment->FirstUnit;
  if( *p )(*p)->prev = q;

  /* hook off the allocated units from segment #2 */
  p2MemorySegment->FirstUnit = NULL;
  p1MemorySegment->CurrentSize += p2MemorySegment->CurrentSize;
#if MTHREAD
  thread_UnlockMutex(&(p2MemorySegment->mxSegment));
  thread_UnlockMutex(&(p1MemorySegment->mxSegment));
#endif
  }

/*POD
=H alloc_MergeAndFinish()
Use this function in case you not only want to merge a segment into another but you also
want to finish the segment that was merged into the other.


See also R<alloc_Merge()>
/*FUNCTION*/
void alloc_MergeAndFinish(void *p1, void *p2
  ){
/*noverbatim
CUT*/
  /* we should not lock either of the segments because the underlying functions do the locking */
  alloc_Merge(p1,p2);
  /* and clean up segment #2 */
  alloc_FinishSegment( p2 );
  }

/*POD
=H alloc_InitStat()

This function initializes the global statistical variables. These variables
can be used in a program to measure the memory usage.

This function should be called before any other memory handling function.

/*FUNCTION*/
void alloc_InitStat(
  ){
/*noverbatim
CUT*/

#if MTHREAD
  thread_InitMutex(&mxStat);
#endif

  MaxNetSize = 0;
  MinNetSize = 0;
  MaxBruSize = 0;
  MinBruSize = 0;
  ActNetSize = 0;
  ActBruSize = 0;
  }

/*POD
=H alloc_GlobalGetStat()

From period to period the code using this memory management layer may need to know
how much memory the program is using.

Calling this function from time to time you can get the minimum and maximum memory
that the program used via this layer since the last call to this function or since
program start in case of the first call.

/*FUNCTION*/
void alloc_GlobalGetStat(unsigned long *pNetMax,
                         unsigned long *pNetMin,
                         unsigned long *pBruMax,
                         unsigned long *pBruMin,
                         unsigned long *pNetSize,
                         unsigned long *pBruSize
  ){
/*noverbatim
CUT*/

  LOCK_STAT

  if( pNetMax )*pNetMax = MaxNetSize;
  MaxNetSize = ActNetSize;
  if( pNetMin )*pNetMin = MinNetSize;
  MinNetSize = ActNetSize;
  if( pBruMax )*pBruMax = MaxBruSize;
  MaxBruSize = ActBruSize;
  if( pBruMin )*pBruMin = MinBruSize;
  MinBruSize = ActBruSize;

  if( pNetSize )*pNetSize = ActNetSize;
  if( pBruSize )*pBruSize = ActBruSize;

  UNLOCK_STAT
  }

/*POD
=H alloc_GetStat()

From period to period the code using this memory management layer may need to know
how much memory the program is using.

Calling this function from time to time you can get the minimum and maximum memory
that the program used via this layer since the last call to this function or since
program start in case of the first call.

/*FUNCTION*/
void alloc_GetStat(void *p,
                   unsigned long *pMax,
                   unsigned long *pMin,
                   unsigned long *pActSize
  ){
/*noverbatim
CUT*/
  pMyAlloc pMemorySegment = (pMyAlloc)p;

  LOCK_SEGMENT
  if( pMax )*pMax = pMemorySegment->MaxNetSize;
  pMemorySegment->MaxNetSize = pMemorySegment->CurrentSize;
  if( pMin )*pMin = pMemorySegment->MinNetSize;
  pMemorySegment->MinNetSize = pMemorySegment->CurrentSize;

  if( pActSize )*pActSize = pMemorySegment->CurrentSize;
  UNLOCK_SEGMENT

  }
