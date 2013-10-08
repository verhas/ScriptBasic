/* 
FILE:   rpool.c
HEADER: rpool.h

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

#include "../basext.h"
#include "../thread.h"

typedef struct _rpm_resource_t {
  void *handle; // handle to the resouce
  unsigned long lBurn; // the current burn level
  unsigned long lCreateTime; // when the resource was created
  unsigned long lUseCounter; // how many thread has used the resource
  struct _rpm_resource_t *flink,*blink; // link for the linked list
  struct _rpm_pool_t *pool; // the pool the resource belongs to
  } rpm_resource_t;

typedef struct _rpm_pool_t {
  pSupportTable pSt;
  unsigned long lMaxBurn; // limit for resource burning
  unsigned long lMaxTime; // limit for resource age
  unsigned long lMaxUse;  // limit for number of use

  void *pool;             // pool pointer passed to resource handling functions

  void *(*fpOpen)(void *);         // func pointer to function that opens the resource
  void (*fpClose)(void *, void *); // func pointer to function that closes the resource

  void *pMemorySegment; // memory segment pointer used by alloc_*

  unsigned long (*timefun)(void *);// pointer to the time function

  unsigned long nFree;    // resources in the list
  unsigned long nUsed;    // the number of resources used currently
  unsigned long nOpening; // the number of resources the RPM is currently opening

  unsigned long lMinFree; // the minimum number of free resources in the pool
  unsigned long lMaxFree;  // the maximum number of free resources in the pool

  unsigned long lWaitSleep; // the time the ager thread should sleep in seconds

  rpm_resource_t *first_resource;  // pointer to the list of resources
  rpm_resource_t *first_used;      // pointer to the list of currently used resources
  MUTEX mxPool;                    // mutex to lock the whole pool
  MUTEX mxRun;                     // mutex that the worker thread waits for

  } rpm_pool_t;

*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "rpool.h"

#define rpmALLOC(X) RPool->pSt->Alloc((X),RPool->pMemorySegment)
#define rpmFREE(X)  RPool->pSt->Free((X),RPool->pMemorySegment)
/*POD

This file implements resource pool handling routines. This helps multi-thread programs
to maintain a pool of resources. If it makes you problem to think about a resource as
an abstract entity then replace in your mind to database connection. This is especially
true because the very first version of this program is developed to help multi-thread
variation of ScriptBasic to efficiently handle DB connections to MySQL, PostgreSQL and
ORACLE. (Well, at the time we only have MySQL, but we plan the future.)

In an abstract way a resource is something that has to be opened, used and closed. Opening
a resource give a handle to a resource in the form of a pointer. This pointer is used to
reference this resource whenever it is used or is closed.

The resource pool (RP) contains resources. The resource pool manager (RPM) manages the pool.

The RPM has opened resources. Whenever a thread needs a resource it retrieves one from
the RP calling RPM function. When the thread does not need the resource anymore if passes 
it back to the RPM.

Between getting the resource and passing it back the thread uses the resource. We say
that the thread burns the resource. When a resource is used a lot it is burnt. The RPM
keeps track of the burn level of the resources and when a
resource is burnt out it is closed it and removed from the pool.

To keep track the burn level of the resource the thread using it can call the 
RPM burning function telling that the actual resource was used.

Assume the following example:

The resource is a database connection. There is a server process in the database for each connection.
The server is bugous and looses memory. You experience that a database connection used a lot of times
causes huge memory eating processes on the DB server. Thus you decide that the DB connection should not
be used more than a 100 queries.

The RPM also counts the number of times a resource was passed to a thread.
If this number reaches a limit the RPM removes the resource from the pool.

There is a third way of aging a resource. The RPM keeps track of the absolute age of the resource.
If this age reaches a limit the resource is closed.

A resource is never closed while used by a thread.

If there is any module that uses this module the module T<myalloc.c> has to be compiled in multi-threaded
mode defining T<-DMTHREAD=1> when compiling.

CUT*/

/*POD
=H rpm_open

This function is called by the R<rpm_thread> function to open a new resource. This function is
started as a separate thread. This allocates memory to store a new resource, calls the resource
opening function pointed by the function pointer T<fpOpen> and intializes the resource. This
means setting the burn level, use level to zero and the resource create time to the current time.

When the resource is ready the function locks the resource pool and links the resource into the
free list. While linking the resource into the free list the function decrements the field T<nOpening>
and increments the field T<nFree> as the resource is not in the opening state anymore but free and available for
use.

=verbatim
*/
static void rpm_open(void *p){
/*noverbatim
CUT*/
  pSupportTable pSt;
  rpm_pool_t *RPool;
  rpm_resource_t *pR;

  RPool = p;
  /* copy it to use the besXXX macros safely */
  pSt = RPool->pSt;

  pR = rpmALLOC(sizeof(rpm_resource_t));
  /* if there is no memory enough then */
  if( pR == NULL ){
    besLockMutex( &(RPool->mxPool) );
    RPool->nOpening--;
    besUnlockMutex( &(RPool->mxPool) );
    return;
    }

  /* Open the resource */
  pR->handle = RPool->fpOpen(RPool->pool);
  if( pR->handle == NULL ){
    besLockMutex( &(RPool->mxPool) );
    RPool->nOpening--;
    besUnlockMutex( &(RPool->mxPool) );
    return;
    }

  pR->lBurn = 0;       /* it is not burnt so far */
  pR->lCreateTime = RPool->timefun(RPool->pool); /* get the creation time for aging */
  pR->lUseCounter = 0; /* it is not used so far */
  pR->pool = RPool;    /* this pointer is needed when closing the resource */

  /* lock the pool for the time we insert the opened resource into the list */
  besLockMutex( &(RPool->mxPool) );
  RPool->nOpening--;  /* it is not opening */
  RPool->nFree++;     /* rather it is opened and free */

  /* link the resource into the freelist */
  pR->blink = NULL;   /* no previous, this is the first */
  pR->flink = RPool->first_resource; 
  if( RPool->first_resource )/* the one that was the first so far becomes the second and points to this one */
    RPool->first_resource->blink = pR;
  RPool->first_resource = pR;

  /* we are done the resource can be used, the pool is released */
  besUnlockMutex( &(RPool->mxPool) );
  return;
  }

/*POD
=H rpm_close

This function closes a resource. This function is started in separate thread thus no other
thread waits for it to close the resource in case the closing takes too long time. The function
R<rpm_close_excess> call this function in its own thread but that function is also started as
a separate thread.

When this function is called the resource is already unlinked from the free list and no-one can
access it.

The argument is a pointer pointing to the resource record.

The resource record has a pointer to the resource pool, but does not access the pool more than
acessing the memory segment pointer and close function.

=verbatim
*/
void rpm_close(void *p){
/*noverbatim
CUT*/
  rpm_resource_t *pR;
  rpm_pool_t *RPool;

  pR = p;
  RPool = pR->pool;
  /* Note that RPool->pool is a pointer that points to a structure that we never touch. That is
     the responsibility of the caller what it stores there and how uses this pointer. */
  RPool->fpClose(RPool->pool,pR->handle);
  rpmFREE(pR);
  }

/*POD
=H rpm_close_excess

This function is called to close the excess resources when there are too many free resources.
This function should close no more than exactly one resource.

The argument is a pointer to the resource pool.

=verbatim
*/
void rpm_close_excess(void *p){
/*noverbatim
CUT*/
  rpm_pool_t *RPool;
  pSupportTable pSt;
  rpm_resource_t *pR;

  RPool = p;
  pSt = RPool->pSt;
  besLockMutex( &(RPool->mxPool) );
  /* it may happen that usage grew in the meantime and thus there is no need to close resource anymore */
  if( RPool->nFree <= RPool->lMaxFree ){
    besUnlockMutex( &(RPool->mxPool) );
    return;
    }

  /* select a free resource to close Later version may be more complex. Now we select the first. */
  pR = RPool->first_resource;
  /* ulink the resource */
  if( pR->flink )pR->flink->blink = pR->blink;
  if( pR->blink )pR->blink->flink = pR->flink;
  pR->blink = pR->flink = NULL;
  besUnlockMutex( &(RPool->mxPool) );
  /* close the resource in syncronous mode */
  rpm_close( (void *)pR);
  }

/*POD
=H rpm_thread

This function implements the worker thread for the resource pool. For each resource pool there is
a worker thread that continously runs and maintains the pool.

=verbatim
*/
static void rpm_thread(void *p){
/*noverbatim
CUT*/
  pSupportTable pSt;
  rpm_pool_t *RPool;
  rpm_resource_t *pR,*pRn;
  THREADHANDLE T;
  unsigned long nClosing;
  unsigned long lTimeNow;
  int fActive;

  RPool = p;
  /* copy it to use the besXXX macros safely */
  pSt = RPool->pSt;
  while( 1 ){
    /* this mutex is locked so long as long there is no need to run this */
    besLockMutex( &(RPool->mxRun) );

    do{
      fActive = 0;
      besLockMutex( &(RPool->mxPool) );

      /* if there are not enough free resources */
      while( RPool->nFree + RPool->nOpening < RPool->lMinFree ){
        fActive = 1;
        besCreateThread(&T,rpm_open,RPool);
        RPool->nOpening++;
        besUnlockMutex( &(RPool->mxPool) );
        /* here is a small chanche for other threads to get hold of the resource pool */
        besLockMutex( &(RPool->mxPool) );
        }

      /* if there are too many free resources */ 
      if( RPool->nFree > RPool->lMaxFree ){
        fActive = 1;
        nClosing = RPool->nFree -RPool->lMaxFree;
        /* start nClose threads to close free resources */
        while( nClosing ){
          besCreateThread(&T,rpm_close_excess,RPool);
          nClosing ++;;
          }
        besUnlockMutex( &(RPool->mxPool) );
        /* here is a small chanche for other threads to get hold of the resource pool */
        besLockMutex( &(RPool->mxPool) );
        }

      /* check if there are aged resources */
      if( RPool->lMaxTime ){/* if we age resources at all */
        pR = RPool->first_resource;
        lTimeNow = RPool->timefun(RPool->pool);
        /* go through all the free list */
        while( pR ){
          pRn = pR->flink;
          /* if the actual resource is aged */
          if( pR->lCreateTime + RPool->lMaxTime > lTimeNow ){
            /* ulink the resource */
            if( pR->flink )pR->flink->blink = pR->blink;
            if( pR->blink )pR->blink->flink = pR->flink;
            pR->blink = pR->flink = NULL;
            /* close the resource */
            besCreateThread(&T,rpm_close,pR);
            }
          pR = pRn;
          }
        }
      besUnlockMutex( &(RPool->mxPool) );
      }while(fActive);
    }  

  }

/*POD
=H rpm_ager

This function is started as a separate thread during resource pool initialization only
if resource aging is active. In other words if there is T<lMaxTime> is not zero for a
resource pool then this function as a thread is started. This thread runs infinitely.

The function periodically checks if there is any resource aged in the free list. If there is
then it triggers the RPM worker thread that will close these ages items.

=verbatim
*/
static void rpm_ager(void *p){
/*noverbatim
CUT*/
  rpm_pool_t *RPool;
  rpm_resource_t *pR;
  pSupportTable pSt;
  unsigned long lTimeNow;

  RPool = p;
  pSt = RPool->pSt;
  lTimeNow = RPool->timefun(RPool->pool);
  while( 1 ){
    besLockMutex( &(RPool->mxPool) );
    for( pR = RPool->first_resource ; pR ; pR = pR->flink ){
      /* if we find a resource that is already over age */
      if( pR->lCreateTime+RPool->lMaxTime < lTimeNow ){
          besUnlockMutex( &(RPool->mxRun) );/* trigger the RPM thread */
          break;
          }
        }
    besUnlockMutex( &(RPool->mxPool) );
    besSLEEP(RPool->lWaitSleep);
    }
  }

/*POD
=H rpm_NewPool

This function creates and initializes a new resource pool.

/*FUNCTION*/
void *rpm_NewPool(
  pSupportTable pSt,
  unsigned long lMaxBurn,
  unsigned long lMaxTime,
  unsigned long lMaxUse,

  unsigned long lMinFree,
  unsigned long lMaxFree,

  unsigned long lWaitSleep,

  void *pool,

  void *(*fpOpen)(void *),
  void (*fpClose)(void *, void *),

  void *(*myalloc)(size_t),
  void (*myfree)(void *),
  unsigned long (*timefun)(void *)
  ){
/*noverbatim
The arguments:

=itemize
=item T<pSt> should point to the ScriptBasic support function table. This is needed to call the functions available
in the ScriptBasic core code and callable by the extensions.

=item T<lMaxBurn> the maximum number of burn value that a resource can get. When a resource is used the thread
using it may call the burning function to increase the burn level of the resource. If the burn level
gets higher than this limit the resource is not used anymore and is closed by the RPM when the thread
passes it back to the RPM. If this value is zero then burn values are not calculated.

=item T<lMaxTime> the maximum number of seconds (or other time base) that a resource can be used. When a resource
gets older than this value it is not given to a thread anymore but closed by the RPM. If this value is zero any
age of resource is used. The time is calculated calling the function T<timefun> which may not return the actual
time but any value that is appropriate to age resources.

=item T<lMaxUse> the maximum number of times a resource is passed to a thread. When a resource is passed to a thread
an internal counter is increased. When the counter reaches this limit the resource is not used any more but
closed. If this limit is zero a reasource can be passed to threads any times.

=item T<lMinFree> this argument gives the number of free resources that the RPM tries to keep available. When the number
of resources gets lower than this the RPM starts to allocate more free resources smaking them available by the
time when a thread asks for it. This value should be positive.

=item T<lMaxFree> this argument gives the number of free resources that the RPM keeps at most. If the number of free
resources gets higher than this number the RPM starts to close some resources. This value should be no smaller
than T<lMinFree>.

=item T<lWaitSleep> should specify the time in seconds to sleep when a thread waits for something. This is the
interval the R<rpm_ager> sleeps between checking that there are ages resources and this is the interval the resource
allocator sleeps waiting for free resource when there is no available free resource.

=item T<pool> is a pointer to a resource type handle. This pointer is not used by the RPM but is passed to the
T<pfOpen()>, T<fpClose()> and T<timefun> functions.

=item T<fpOpen> pointer to a function that opens a resource. This function should accept a T<void *> pointer. Ther
RPM will pass the T<pool> to this pointer. The function should return the handle T<void *> pointer to the resource
or T<NULL> if the resource is not openable.

=item T<fpClose> pointer to a function that closes a resource. This function shoudl accept two T<void *> pointer arguments.
The first one is the T<pool> pointer, the second is the pointer that was passed back by the function T<fpOpen>.

=item T<myalloc> function to a T<malloc> like function. If this argument is T<NULL> then T<malloc> is used.

=item T<myfree> function to a T<free> like function. If this argument is T<NULL> then T<free> is used.

=item T<timefun> function to a function that returns the actual time in terms of resource age. 
This can be the number of seconds since the epoch, like returned by the system function T<time()> or
some other increasing value that correspnds with resource age. The function should accept a T<void *> pointer
as argument. The RPM passes the pointer T<pool> in this argument. If this argument is T<NULL> then the
system function T<time()> is used.

=noitemize

Return value is a pointer to the new pool or T<NULL> if there is memory allocation error or if the arguments
are erroneous.

CUT*/
  void *pMemorySegment;
  rpm_pool_t *RPool;
  THREADHANDLE T;

  if( lMinFree == 0       ||
      lMaxFree < lMinFree ||
      fpOpen   == NULL    ||
      fpClose  == NULL       )return NULL;

  if( myalloc == NULL )myalloc = malloc;
  if( myfree == NULL )myfree = free;
  if( timefun == NULL )timefun = (void *)time;

  pMemorySegment = besINIT_SEGMENT(myalloc,myfree);
  if( pMemorySegment == NULL )return NULL;

  RPool = pSt->Alloc(sizeof(rpm_pool_t),pMemorySegment);
  if( RPool == NULL )return NULL;
  RPool->pMemorySegment = pMemorySegment;
  RPool->pSt = pSt;

  RPool->lMaxBurn = lMaxBurn;
  RPool->lMaxTime = lMaxTime;
  RPool->lMaxUse  = lMaxUse;

  RPool->lMinFree = lMinFree;
  RPool->lMaxFree = lMaxFree;

  RPool->pool     = pool;

  RPool->fpOpen   = fpOpen;
  RPool->fpClose  = fpClose;

  RPool->timefun  = timefun;

  RPool->nFree    = 0;
  RPool->nUsed    = 0;
  RPool->nOpening = 0;

  RPool->first_resource = NULL;
  RPool->first_used     = NULL;

  RPool->lWaitSleep = lWaitSleep;
  if( RPool->lMaxTime )
    besCreateThread(&T,rpm_ager,RPool);

  besInitMutex(&(RPool->mxPool));
  besInitMutex(&(RPool->mxRun));
  besCreateThread(&T,rpm_thread,RPool);

  return RPool;
  }

/*POD
=H rpm_GetResource

Get a resource from a pool. Arguments:

=itemize
=item T<RPool> is the resource pool to get the resource from.
=item T<lMaxWait> is the maximum number of seconds the function waits to have a free resource.
If this argument is zero the function waits infinitely until there is a free resource.
=noitemize

/*FUNCTION*/
void *rpm_GetResource(
  rpm_pool_t *RPool,
  unsigned long lMaxWait
  ){
/*noverbatim
If there is a free resource available the function unlinks it from the used list and links it into the used list.
In this case the function returns the resource pointer.

If there is no available resource the function sleeps T<RPool->lWaitSleep> seconds and tries again. If the repetitive
sleep time exceeds the argument T<lMaxWait> the function returns T<NULL>.

Note that this is a simple v1.0 implementation that does not guarantee first arrived first served.

CUT*/
  pSupportTable pSt;
  rpm_resource_t *pR;
  unsigned long lSlept;

  pSt = RPool->pSt;
  lSlept = 0;
  while( 1 ){
    besLockMutex( &(RPool->mxPool) );
    /* if there is free resource to be used */
    if( RPool->first_resource ){
      /* get the resource */
      pR = RPool->first_resource;

      /* unlink the resource from the free list */
      RPool->first_resource = RPool->first_resource->flink;
      if( RPool->first_resource )RPool->first_resource->blink = NULL;

      /* link the resource to the used list */
      pR->flink = RPool->first_used;
      pR->blink = NULL;
      if( pR->flink )pR->flink->blink = pR;
      RPool->first_used = pR;
      besUnlockMutex( &(RPool->mxPool) );
      return (void *)pR;
      }
    besUnlockMutex( &(RPool->mxPool) );
    /* if there was no available resource */
    if( lMaxWait && lSlept + RPool->lWaitSleep > lMaxWait )return NULL;
    besSLEEP(RPool->lWaitSleep);
    }
  }

/*POD
=H rpm_PutResource

Get a resource from a pool. Arguments:

=itemize
=item T<RPool> is the resource pool to put the resource back.
=item T<p> is the resource
=noitemize

/*FUNCTION*/
void rpm_PutResource(
  rpm_pool_t *RPool,
  void *p
  ){
/*noverbatim
This function should be used to put a resource back into the pool. When a thread does not need the
resource anymore it puts it back to the resource pool handling the resource to the resource pool manager
calling this function.
CUT*/
  pSupportTable pSt;
  rpm_resource_t *pR;

  pR = p;
  pSt = RPool->pSt;
  pR->lUseCounter ++;
  if( (RPool->lMaxUse && pR->lUseCounter > RPool->lMaxUse) ||
      (RPool->lMaxBurn && pR->lBurn > RPool->lMaxBurn)
     ){
    /* The resource is not put back to the free list, rather closed. */
    rpm_close( (void *)pR);
    return;
    }
  /* lock the pool to handle the used and the free list */
  besLockMutex( &(RPool->mxPool) );

  /* unlink the resource from the used list */
  if( pR->blink )
    pR->blink->flink = pR->flink;
  else
    RPool->first_used = pR->flink;
  if( pR->flink )
    pR->flink->blink = pR->blink;

  /* link the resource into the free list */
  pR->flink = RPool->first_resource;
  pR->blink = NULL;
  if( pR->flink )pR->flink->blink = pR;
  RPool->first_resource = pR;

  /* release the pool */
  besUnlockMutex( &(RPool->mxPool) );
  }
