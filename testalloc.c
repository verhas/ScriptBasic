/* 
FILE:   testalloc.c
HEADER: testalloc.h

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

typedef struct _tAllocUnit {
  long n;                       // the size of the chunk
  long id;                      // unique id of the chunk
  struct _tAllocUnit *next;     // link to the next unit
  struct _tAllocUnit *prev;     // the previous unit
  unsigned char memory[1];      // one or more bytes reserved
  } tAllocUnit, *ptAllocUnit;
// Note that the last member before 'memory' is a pointer which should
// provide sufficient alignment for 'memory' on 32bit and on 64bit systems as well

typedef struct _TAlloc {
  void * (*memory_allocating_function)(size_t);
  void (*memory_releasing_function)(void *);

  ptAllocUnit FirstUnit;
  } TAlloc, *pTAlloc;

*/
#ifdef _DEBUG
#include <stdio.h>
#include <stdlib.h>

/* for offsetof */
#include <stddef.h>
#include <memory.h>
#include "myalloc.h"
#include "testalloc.h"

#define CHECKBYTEVALUE 0xFF

/*POD
=H Memory allocation test module

This module contains two major functions T<testa_Alloc> and T<testa_Free>. These modules keep
track of the allocated memory segments using a similar data structures as the functions
implemented in T<myalloc.c>

The functions in this module use a statically initialized memory segment and ALL memory
allocation is assigned to this segment.

The functions are to test an application so that it does not leak memory. These functions
should NOT be used in a final released version. The functins should be used to track memory
leak.

CUT*/

static pTAlloc pMemorySegment;
static long cbAllocBytes;
static long cbID;

/*POD
=section InitSegment
=H Initialize THE segement

Call this function before calling any other functions once in a process.
/*FUNCTION*/
void testa_InitSegment(
  ){
/*noverbatim
CUT*/
  pMemorySegment = (pTAlloc)malloc(sizeof(TAlloc));
  if( pMemorySegment == NULL )exit(1);
  pMemorySegment->memory_allocating_function = malloc;
  pMemorySegment->memory_releasing_function = free;
  pMemorySegment->FirstUnit = NULL;
  cbAllocBytes = 0L;
  cbID = 0L;
  return;
  }

/*POD
=section Assert0x80
=H Assert that no segment overwrote the 0x80 at the last extra byte

This function can be called from several point of the program to check if
some code overwrote the allocated buffer at least one byte. The test allocation
function allocates one extra byte at the end of the memory block allocated and
fills it with the code 0x80. This extra byte should remain intact at all times
during execution.

The problem is that this is realized after the error. To localize the bug in the code
try to have identical runs. The local variable 'counter' counts each call of the
function and it prints out when the error is catched. Edit the line

=verbatim
  if( counter == 123 ){
=noverbatim

to have the number printed. After that set a debugger breakpoint on the T<printf>
statement on the next line and during the next identical run you can catch the
last time the memory was NOT corrupted. Continue from there step by step.

/*FUNCTION*/
void testa_Assert0x80(
  ){
/*noverbatim
CUT*/
  long k;
  ptAllocUnit pAllU;
  static unsigned long counter=0;

  counter++;
  /* EDIT HERE TO DEBUG */
  if( counter == 123 ){
    printf("");
    }
  if( cbAllocBytes == 0L )return;
  k = 0;
  pAllU = pMemorySegment->FirstUnit;
  while( pAllU ){
    if( pAllU->memory[pAllU->n] != CHECKBYTEVALUE ){
      printf("Segment altered: %d(%d)",pAllU->n,pAllU->id);
      printf("Segment content: %s\n",pAllU->memory);
      printf("Counter to catch is: %d\n",counter-1);
      k = 1;
      }
    pAllU = pAllU->next;
    }
  if( k ){
    printf("\n");

    printf("Press any key to continue...\n");
    getchar();
    exit(1276);
    }
  }

/*POD
=section Alloc
=H Allocate memory from a segment

Use this memory to allocate a memory piece from THE segment.

/*FUNCTION*/
void *testa_Alloc(size_t n
  ){
/*noverbatim

The argument is the size to be allocated.

If the memory allocation fails the function returns T<NULL>.

During debug each allocated memory piece gets an ID. This ID is printed
to the standard error when some error is identified by this module. Edit the
line 

=verbatim
  if( pAllU->id == 26 ){
=noverbatim

to compare the id to the number of the segment that was reported leaking or corrupted and
set the breakpoint on the next line T<printf> during the next identical run. From there
you can trace your code step by step identifying the allocation that was not correct.
CUT*/
  ptAllocUnit pAllU;

  if( n == 0 )
    return NULL;

  testa_Assert0x80();

  cbAllocBytes += n;

  /* allocate one extra byte to detect buffer overflow. The extra one byte is set to 0x80 value and should not
     change when the memory is released */
  pAllU = (ptAllocUnit)pMemorySegment->memory_allocating_function( n + offsetof(struct _tAllocUnit,memory) +1 );

  if( pAllU == NULL )return NULL;

  memset(pAllU,CHECKBYTEVALUE,n + offsetof(struct _tAllocUnit,memory) +1);

  pAllU->n = n;
  pAllU->id = cbID++;
  pAllU->prev = NULL;
  pAllU->next = pMemorySegment->FirstUnit;
  if( pMemorySegment->FirstUnit )pMemorySegment->FirstUnit->prev = pAllU;
  pMemorySegment->FirstUnit = pAllU;

  /* EDIT HERE TO DEBUG */
  if( pAllU->id == 26 ){
    printf("");
    }
  /* return a void* pointer that points to the allocated memory after the header */
  return (void *)( pAllU->memory );
  }

/*POD
=section Free
=H Release memory

You should call this function whenever you want to release a single piece of memory
allocated from THE segment.

/*FUNCTION*/
void testa_Free(void *pMem
  ){
/*noverbatim
CUT*/
  ptAllocUnit pAllU;

  if( pMemorySegment == NULL )return;
  pAllU = (ptAllocUnit)( ((unsigned char *)pMem) - offsetof(struct _tAllocUnit,memory) ); 

  /* check here that the extra 0x80 byte remained intact */
  if( pAllU->memory[pAllU->n] != CHECKBYTEVALUE ){
    printf("Segment altered: %d(%d)",pAllU->n,pAllU->id);
    printf("Segment content: %s\n",pAllU->memory);
    printf("Press any key to continue...\n");
    getchar();
    exit(1276);
    }

  if( pAllU->next )
    pAllU->next->prev = pAllU->prev;
  if( pAllU->prev )
    pAllU->prev->next = pAllU->next;
  else
    pMemorySegment->FirstUnit = pAllU->next;
  cbAllocBytes -= pAllU->n;

  pMemorySegment->memory_releasing_function(pAllU);
  }


/*POD
=section Check
=H Check memory

This function does nothing but checks that a memory segment is valid,
in other words it checks that the memory segment was allocated by the
program and the pointer is valid.

/*FUNCTION*/
void testa_Check(void *pMem
  ){
/*noverbatim
CUT*/
  ptAllocUnit pAllU,pAllUi;

  if( pMemorySegment == NULL ){
    printf("pMem %p can not be a valid segment as there are not segments\n",pMem);
    exit(1277);
    }
  pAllU = (ptAllocUnit)( ((unsigned char *)pMem) - offsetof(struct _tAllocUnit,memory) ); 

  pAllUi = pMemorySegment->FirstUnit;
  while( pAllUi ){
    if( pAllUi == pAllU ){
      if( pAllU->memory[pAllU->n] != CHECKBYTEVALUE ){
        printf("Segment altered: %d(%d)",pAllU->n,pAllU->id);
        printf("Segment content: %s\n",pAllU->memory);
        exit(1278);
        }
      return;
      }
    pAllUi = pAllUi->next;
    }

  printf("The segment %p is not on the list\n",pMem);
  exit(1279);
  }

/*POD
=section AssertLeak
=H Assert that there is no memory leak

Returns if all memory was relased. Otherwise it prints the information of non-released
memory chunks, and then exits with code 1276.

Then you can debug the application based on the reported memory chunk size and id to hunt
where you allocate memory and not release.
/*FUNCTION*/
void testa_AssertLeak(
  ){
/*noverbatim
CUT*/
  long k;
  ptAllocUnit pAllU;

  if( cbAllocBytes == 0L )return;

  printf("There is a memory leak of %ld bytes\n",cbAllocBytes);
  k = 0;
  pAllU = pMemorySegment->FirstUnit;
  while( pAllU ){
    pAllU = pAllU->next;
    k++;
    }
  printf("This memory is lost in %d chunks\n",k);
  pAllU = pMemorySegment->FirstUnit;
  while( pAllU ){
    printf("%d(%d)",pAllU->n,pAllU->id);
    pAllU = pAllU->next;
    if( pAllU )printf(",");
    }
  printf("\n");

  printf("Press any key to continue...\n");
  getchar();
  exit(1276);
  }

/*POD
=section ReportLeak
=H Reports memory leak


/*FUNCTION*/
unsigned long testa_ReportLeak(
  ){
/*noverbatim
CUT*/
  long k;
  ptAllocUnit pAllU;

  if( cbAllocBytes == 0L ){
    printf("No memory leak\n");
    return 0;
    }

  printf("There is a memory leak of %ld bytes\n",cbAllocBytes);
  k = 0;
  pAllU = pMemorySegment->FirstUnit;
  while( pAllU ){
    pAllU = pAllU->next;
    k++;
    }
  printf("This memory is lost in %d chunks\n",k);
  pAllU = pMemorySegment->FirstUnit;
/*  while( pAllU ){
    printf("%d(%d)",pAllU->n,pAllU->id);
    pAllU = pAllU->next;
    if( pAllU )printf(",");
    }*/
  printf("\n");
  return cbAllocBytes;
  }
#endif
