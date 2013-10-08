/* 
FILE:   thread.c
HEADER: thread.h

TO_HEADER:

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <direct.h>
#else
#include <sys/file.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#endif

#ifdef WIN32
typedef HANDLE THREADHANDLE,*PTHREADHANDLE;
typedef HANDLE MUTEX,*PMUTEX;
#else
typedef pthread_t THREADHANDLE,*PTHREADHANDLE;
typedef pthread_mutex_t MUTEX,*PMUTEX;
#endif

typedef struct _SHAREDLOCK {
  MUTEX mxWrite;
  MUTEX mxRead;
  MUTEX mxCounter;
  int dwReaders; 
  } SHAREDLOCK, *PSHAREDLOCK;


*/

#include "thread.h"

/*POD
@c thread handling routines

This file implements global thread handling functions. If the programmer uses these functions instead
of the operating system provided functions the result will be Windows NT I<and> UNIX portable program.
These routines handling thread and mutex locking functions had been extensively tested in commercial
projects.

CUT*/

/*POD
=H thread_CreateThread
@c Create a new thread

This is a simplified implementation of the create thread interface.

The function creates a new B<detached> thread.
If the thread can not be created for some reason the return value is the error
code returned by the system call T<pthread_start> on UNIX or T<GetLastError> on NT.

If the thread was started the return value is 0.

/*FUNCTION*/
int thread_CreateThread(PTHREADHANDLE pThread,
                      void *pStartFunction,
                      void *pThreadParameter
  ){
/*noverbatim
The arguments
=itemize
=item T<pThread> is a thread handle. This should be a pointer to a variable of type T<THREADHANDLE>. This
argument is set to hold the thread handle returned by T<CreateThread> on NT or the pointer
returned as first argument of T<pthread_create> under UNIX. This argument is not used further in this
module but can be used if calling system dependant functions.
=item T<pStartFunction> should be a pointer pointing to the start function where the thread should start. This
is usually just the name of the function to start in the separate thread.
=item T<pThreadParameter> is the pointer passed as argument to the start function.
=noitemize

CUT*/
#ifdef WIN32
  DWORD TID;

  *pThread = CreateThread(NULL, /* default security attribute */
                         0,    /* default initial stack size */
                         (LPTHREAD_START_ROUTINE)pStartFunction,
                         (LPVOID) pThreadParameter,
                         0,    /* should start immediately */ 
                         &TID);
  if( pThread != NULL ){
    CloseHandle(*pThread);
    return 0;
    }
  return GetLastError();
#else
 int i;
 i = pthread_create(pThread, NULL , pStartFunction, pThreadParameter);
 if( i == 0 )
   pthread_detach(*pThread);
 return i;
#endif
  }

/*POD
=H thread_ExitThread
@c Exit from a thread

Exit from a thread created by R<CreateThread>. The implementation is simple
and does not allow any return value from the thread.

/*FUNCTION*/
void thread_ExitThread(
  ){
/*noverbatim
CUT*/
#ifdef WIN32
 ExitThread(0);
#else
 pthread_exit(NULL);
#endif
  }

/*POD
=H thread_InitMutex
@c Initialize a mutex object

This function initializes a T<MUTEX> variable. A T<MUTEX> variable can be used for exclusive access.
If a mutex is locked another lock on that mutex will wait until the first lock is removed. If there are
several threads waiting for a mutex to be released a random thread will get the lock when 
the actually locking thread releases the mutex. In other words
if there are several threads waiting for a mutex there is no guaranteed order of the threads getting the
mutex lock.

Before the first use of a T<MUTEX> variable it has to be initialized calling this function.

/*FUNCTION*/
void thread_InitMutex(PMUTEX pMutex
  ){
/*noverbatim
Arguments:
=itemize
=item T<pMutex> should point to a mutex variable of the type T<MUTEX>
=noitemize
CUT*/
#ifdef WIN32
 *pMutex = CreateSemaphore(NULL,1,1,NULL);
#else
 pthread_mutex_init(pMutex,NULL);
#endif
  }

/*POD
=H thread_FinishMutex
@c Delete a mutex object

When a mutex is not used anymore by a program it has to be released to free the system resources
allocated to handle the mutex.

/*FUNCTION*/
void thread_FinishMutex(PMUTEX pMutex
  ){
/*noverbatim
Arguments:
=itemize
=item T<pMutex> should point to an initialized mutex variable of the type T<MUTEX>
=noitemize
CUT*/
#ifdef WIN32
 CloseHandle(*pMutex);
#else
 pthread_mutex_destroy(pMutex);
#endif
  }

/*POD
=H thread_LockMutex
@c Lock a mutex object

Calling this function locks the mutex pointed by the argument. If the mutex is currently locked the
calling thread will wait until the mutex becomes available.

/*FUNCTION*/
void thread_LockMutex(PMUTEX pMutex
  ){
/*noverbatim
Arguments:
=itemize
=item T<pMutex> should point to an initialized mutex variable of the type T<MUTEX>
=noitemize
CUT*/
#ifdef WIN32
 WaitForSingleObject(*pMutex,INFINITE);
#else
 pthread_mutex_lock(pMutex);
#endif
  }

/*POD
=H thread_UnlockMutex
@c Unlock a mutex object

Calling this function unlocks the mutex pointed by the argument. Calling this function on a mutex
currently not locked is a programming error and results undefined result. Different operating system
may repond different.

/*FUNCTION*/
void thread_UnlockMutex(PMUTEX pMutex
  ){
/*noverbatim
Arguments:
=itemize
=item T<pMutex> should point to an initialized mutex variable of the type T<MUTEX>
=noitemize
CUT*/
#ifdef WIN32
  ReleaseSemaphore(*pMutex,1,NULL);
#else
  pthread_mutex_unlock(pMutex);
#endif
  }


/*POD
=H thread_shlckstry
@c Shared locks

The following functions implement shared locking. These functions do not call system
dependant functions. These are built on the top of the MUTEX locking functions.

A shareable lock can be B<READ> locked and B<WRITE> locked. When a shareable lock is READ locked
another thread can also read lock the lock.

On the other hand a write lock is exclusive. A write lock can appear when there is no read lock on
a shareable lock and not write lock either.

@cr

The story to understand the workings:

Imagine a reading room with several books. You can get into the room through a small 
entrance room, which is dark. To get in you have to switch on the light. The reading room 
has a light and a switch as well. You are not expected to read in the dark. The reading 
room is very large with several shelves that easily hide the absent minded readers and 
therefore the readers can not easily decide upon leaving if they are the last or not. This 
actually led locking up late readers in the dark or the opposite: lights running all the night.

To avoid this situation the library placed a box in the entrance room where each reader 
entering the room have to place his reader Id card. When they leave they remove the 
card. The first reader coming switches the light on, and the last one switches the light off. 
Coming first and leaving last is easily determined looking at the box after dropping the 
card or after taking the card out. If there is a single card after dropping the reader card 
into you are the first coming and if there is no card in it you took your one then you are 
the last.

To avoid quarreling and to save up energy the readers must switch on the light of the 
entrance room when they come into and should switch it off when they leave. However 
they have to do it only when they go into the reading room, but not when leaving. When 
someone wants to switch a light on, but the light is already on he or she should wait until 
the light is switched off. (Yes, this is a MUTEX.)

When the librarian comes to maintain ensures that no one is inside, switches the light of 
the entrance room on, and then switches the reading room light on. If someone is still 
there he cannot switch the light on as it is already switched on. He waits until the light is 
switched off then he switches it on. When he has switched the light of the reading room on 
he switches the light of the entrance room off and does his job in the reading room. Upon 
leaving he switches off the light of the reading room.

Readers can easily enter through the narrow entrance room one after the other. They can 
also easily leave. When the librarian comes he can not enter until all readers leave the 
reading room. Before getting into the entrance room he has equal chance as any of the 
readers. 

CUT*/

/*POD
=H thread_InitLock
@c Initialize a shareable lock

/*FUNCTION*/
void shared_InitLock(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  thread_InitMutex( &(p->mxWrite) );
  thread_InitMutex( &(p->mxRead) );
  thread_InitMutex( &(p->mxCounter) );
  p->dwReaders = 0;
  }

/*POD
=H thread_FinishLock
@c Finish a shareable lock

/*FUNCTION*/
void shared_FinishLock(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  thread_FinishMutex( &(p->mxWrite) );
  thread_FinishMutex( &(p->mxRead) );
  thread_FinishMutex( &(p->mxCounter) );
  }

/*POD
=H thread_LockRead
@c Lock a shareable lock for shared (read) lock

/*FUNCTION*/
void shared_LockRead(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  int iReaders;
  thread_LockMutex( &(p->mxWrite) );
  thread_LockMutex( &(p->mxCounter) );
  p->dwReaders ++;
  iReaders = p->dwReaders;
  if( iReaders == 1 )
    thread_LockMutex( &(p->mxRead) );
  thread_UnlockMutex( &(p->mxCounter) );
  thread_UnlockMutex( &(p->mxWrite) );
  }

/*POD
=H thread_LockWrite
@c Lock a shareable lock for exclusive locking

/*FUNCTION*/
void shared_LockWrite(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  thread_LockMutex( &(p->mxWrite) );
  thread_LockMutex( &(p->mxRead) );
  thread_UnlockMutex( &(p->mxWrite) );
  }

/*POD
=H thread_UnlockRead
@c Unlock a sharebale lock that was locked shared

/*FUNCTION*/
void shared_UnlockRead(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  int iReaders;
  thread_LockMutex( &(p->mxCounter) );
  p->dwReaders --;
  iReaders = p->dwReaders;
  if( ! iReaders )
    thread_UnlockMutex( &(p->mxRead) );
  thread_UnlockMutex( &(p->mxCounter) );
  }

/*POD
=H thread_UnlockWrite
@c Unlock a sharebale lock that was locked exclusive

/*FUNCTION*/
void shared_UnlockWrite(PSHAREDLOCK p
  ){
/*noverbatim
CUT*/
  thread_UnlockMutex( &(p->mxRead) );
  }

