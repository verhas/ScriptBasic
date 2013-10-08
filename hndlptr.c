/*
FILE: hndlptr.c
HEADER: hndlptr.h

TO_HEADER:

*/

/*POD
@c Handling handle pointer conversion

The functions in this file help the various ScriptBasic extension
modules to avoid crashing the system even if the BASIC programs use 
the values passed by the module in a bad way. 

For example a database handling module opens a database and allocates 
a structure describing the connection. The usual way to identify the structure 
is to return a BASIC string variable to the BASIC code that byte by byte holds 
the value of the pointer. This works on any machine having 32bit or 64bit pointers 
because strings can be arbitrary length in ScriptBasic. 

When another external module function need access to the structure it needs a 
pointer to it. This is easily done by passing the string variable to the module. 
The module converts the string variable back byte by byte to a pointer and all is fine.

Is it?

The issue is that the BASIC program may alter the pointer and pass a string containg garbage
back to the module. The module has no way to check the correctness tries to use it
and crashes the whole interpreter. (Even the other interpreters running in the same process
in different threads.)

=bold
ScriptBasic external modules should never ever pass pointers in strings back to the BASIC code.
=nobold

(Even that some of the modules written by the ScriptBasic developers followed this method formerly.)

The better solution is to store these module pointers in arrays and pass the index of the pointer
in the array to the basic application. This way the BASIC program will get INTEGER values instead
of STRING and will not be able to alter the pointer value and crash the program.

To store the pointer and get the index (we call it a handle) these functions can be used.

Whenever a pointer needs a handle the module has to call T<GetHandle>. This function stores the
pointer and returns the handle to it. When the BASIC program passes the handle back to the module
and the module needs the pointer associated with the handle it has to call T<GetPointer>.

When a pointer is not needed anymore the handle should be freed calling T<FreeHandle>.

This implementation uses arrays to hold the pointers. The handles are the indexes to the array.
The index 0 is never used. Handle value zero is returned as an invalid handle value whenever
some error occures, like out of memory condition.

CUT*/
#include <stdlib.h>

#include "myalloc.h"
#include "thread.h"

#define ARRAY_INCREMENT 100

typedef struct _HandleArray {
  unsigned long n;
  MUTEX mx;
  void **pointer;
  }HandleArray,*pHandleArray;

/*POD
=H handle_GetHandle
@c GetHandle

Having a pointer allocate a handle. This function stores the
pointer and returns the handle.

The handle is a small positive integer.

If any error is happened (aka out of memory) zero is returned.
/*FUNCTION*/
unsigned long handle_GetHandle(void **pHandle,
                               void *pMEM,
                               void *pointer
  ){
/*noverbatim

=itemize
=item The first argument T<pHandle> is a pointer to the handle array.
=item The second argument T<pMEM> is the memory segment that is to be used to allocate
memory.
=item The last argument T<pointer> is the pointer to store.
=noitemize

Note that T<NULL> pointer can not be stored in the array.

The pointer to the handle array T<pHandle> should be initialized to NULL
before the first call to T<handle_GetHandle>. For example:
=verbatim
   void *Handle = NULL;
     ....
   if( !handle_GetHandle(&Handle,pMEM,pointer) )return ERROR_CODE;
=noverbatim
CUT*/
  pHandleArray q;
  unsigned long i;
  void **z;

  if( pointer == NULL )return 0;
  if( *pHandle == NULL ){
    *pHandle = alloc_Alloc(sizeof(HandleArray),pMEM);
    if( *pHandle == NULL )return 0;
    q = *pHandle;
    thread_InitMutex( &(q->mx) );
    q->n = 0;
    q->pointer = NULL;
    }
  q = (pHandleArray)*pHandle;
  thread_LockMutex( &(q->mx) );
  /* search for a free handle */
  for( i=1 ; i < q->n ; i++ )
    if( q->pointer[i] == NULL )break;
  /* we need to allocate more space */
  if( i >= q->n ){
    z = alloc_Alloc((q->n+ARRAY_INCREMENT)*sizeof(void*),pMEM);
    if( z == NULL )return 0;
    memset(z,0,(q->n+ARRAY_INCREMENT)*sizeof(void*));
    memcpy(z,q->pointer,q->n*sizeof(void *));
    alloc_Free(q->pointer,pMEM);
    q->pointer = z;
    q->n += ARRAY_INCREMENT;
    }
  q->pointer[i] = pointer;
  thread_UnlockMutex( &(q->mx) );
  return i;
  }

/*POD
=H handle_GetPointer
@c GetPointer

This function is the opposite of R<GetHandle>. If a pointer was
stored in the handle array this function can be used to retrieve the
pointer knowing the handle.

/*FUNCTION*/
void *handle_GetPointer(void **pHandle,
                        unsigned long handle
  ){
/*noverbatim
=itemize
=item The first argument T<pHandle> is the pointer to the handle array.
=ite, The second argument T<handle> is the handle of the pointer.
=noitemize

If there was not pointer registered with that handle the return value of the
function is T<NULL>.
CUT*/
  pHandleArray q;
  void *z;

  q = *pHandle;
  /* if there is no registered handle or the handle is
     out of range of registered handles. */
  if( q == NULL || handle < 1 || handle >= q->n )return NULL;
  thread_LockMutex( &(q->mx) );
  z = q->pointer[handle];
  thread_UnlockMutex( &(q->mx) );
  return z;
  }

/*POD
=H handle_FreeHandle
@c FreeHandle

Use this function when a pointer is no longer valid. Calling
this function releases the T<handle> for further pointers.

/*FUNCTION*/
void handle_FreeHandle(void **pHandle,
                       unsigned long handle
  ){
/*noverbatim
CUT*/
  pHandleArray q;

  q = *pHandle;
  /* if there is no registered handle or the handle is
     out of range of registered handles. */
  if( q == NULL || handle < 1 || handle >= q->n )return;
  thread_LockMutex( &(q->mx) );
  q->pointer[handle] = NULL;
  thread_UnlockMutex( &(q->mx) );
  return;
  }

/*POD
=H handle_DestroyHandleArray
@c DestroyHandleArray

Call this function to release the handle array after all handles are
freed and there is no need for the handle heap.

Use the same memory head T<pMEM> that was used in R<GetHandle>.

/*FUNCTION*/
void handle_DestroyHandleArray(void **pHandle,
                               void *pMEM
  ){
/*noverbatim
CUT*/
  pHandleArray q;

  q = *pHandle;
  if( q == NULL  )return;

  thread_FinishMutex( &(q->mx) );
  alloc_Free(q,pMEM);
  }
