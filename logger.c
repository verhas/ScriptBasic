/*
FILE: logger.c
HEADER: logger.h

TO_HEADER:

typedef struct _tLogItem {
  char *pszMessage;
  long Time;
  struct _tLogItem *p,*n;
  } tLogItem, *ptLogItem;

#define FNMAX 256
typedef struct _tLogger {
  char *pszFileName;
  char szFileName[FNMAX];
  FILE *fp;
  long LastTime; // the hour of the log was written last time (hour is timestamp/60/60)
  long TimeSpan;
  void *(*memory_allocating_function)(size_t, void *);
  void (*memory_releasing_function)(void *, void *);
  void *pMemorySegment;
  long MaxItemLen; // the maximal length of a log line
  ptLogItem QueueStart,QueueEnd;
  MUTEX mxChain;
  MUTEX mxRun;
  MUTEX mxState;
  int type; // 0 normal or 3 synchronous logger
  int state; // 0 normal, 1 currently shutting down, 2 finished, closed, dead
             // or 3 that means this is a synchronous logger
  } tLogger,*ptLogger;

#define LOGSTATE_NORMAL      0
#define LOGSTATE_SHUTTING    1
#define LOGSTATE_DEAD        2
#define LOGSTATE_SYNCHRONOUS 3

#define LOGTYPE_SYNCHRONOUS 0
#define LOGTYPE_NORMAL      1
*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "filesys.h"
#include "thread.h"
#include "logger.h"
#include "mygmtime.h"

/*POD
This module can be used to log events. The module implements two type of logs.

=itemize
=item synchronous logs
=item asynchronous logs
=noitemize

B<Synchronous> logs are just the normal plain logging technic writing messages to a log file. This
is low performance, because the caller has to wait until the logging is performed and written to a file.
On the other hand this is a safe logging.

Asynchronous logging is a fast performance logging method. In this case the caller passes
the log item to the logger. The logger puts the item on a queue and sends it to the log file
in another thread when disk I/O bandwith permits. This is high performance, because the caller
does not need to wait for the log item written to the disk. On the other hand this logging is not
safe because the caller can not be sure that the log was written to the disk.

The program using this module should use asynchronous logging for high volume logs and synchronous
logging for low volume logging. For example a panic log that reports configuration error has to
be written synchronously.

Using this module you can initialize a log specifying the file where to write the log, send logs and
you can tell the log to shut down. When shutting down all waiting logs are written to the file and no more
log items are accepted. When all logs are written the logging thread terminates.

CUT*/

#define ALLOC(X) (pLOG->memory_allocating_function((X),pLOG->pMemorySegment))
#define FREE(X)  (pLOG->memory_releasing_function((X),pLOG->pMemorySegment))

static void * _mya(size_t x,void *y){
  return malloc(x);
  }
static void _myf(void *x, void *y){
  free(x);
  }

/*
This is the logging thread started for each log file. This
thread waits on the mutex that signals that there is some
item to be written and then writes the log into the text file
specified by the file name.
*/
static void log_thread(void *q){
  ptLogger pLOG;
  ptLogItem pLI;
  struct tm gm_time;

  /* The actual log structure is passed as the thread argument. */
  pLOG = q;
  while( 1 ){
    /* This mutex is locked by default and is unlocked whenever there is something to send to file. */
    thread_LockMutex(&(pLOG->mxRun));
    while( 1 ){/* write all queue items to the file and break loop in the middle*/
      thread_LockMutex(&(pLOG->mxChain));
      if( pLOG->QueueStart == NULL ){/* if there are no more elements in the queue then we stop the writing loop */
        if( pLOG->fp )fflush(pLOG->fp);
        thread_UnlockMutex(&(pLOG->mxChain));
        if( log_state(pLOG) == LOGSTATE_SHUTTING ){/* if we are shutting down the log */
          thread_LockMutex(&(pLOG->mxState));
          pLOG->state = LOGSTATE_DEAD; /* signal that we are dead */
          thread_UnlockMutex(&(pLOG->mxState));
          if( pLOG->fp ){
            fclose(pLOG->fp);
            pLOG->fp = NULL;
            }
          return; /* finish the worker thread */
          }
        break;
        }
      /* if the queue is not empty */
      pLI = pLOG->QueueStart; /* get the first element of the queue and */
      pLOG->QueueStart = pLOG->QueueStart->n;/* step the queue head ahead one step */
      /* if the queue became empty then the QueueEnd pointer has also to be null */
      if( pLOG->QueueStart == NULL )pLOG->QueueEnd = NULL;
      thread_UnlockMutex(&(pLOG->mxChain));/* and release the chain so other 
                                              threads can add new elements to it 
                                              in the meantime */
      mygmtime(&(pLI->Time),&gm_time);
      /* if there is no file opened or we stepped into a different time interval than the previous log was */
      if( (!pLOG->fp) || (pLOG->TimeSpan && (pLOG->LastTime != pLI->Time / pLOG->TimeSpan)) ){
        if( pLOG->fp ){
          fclose(pLOG->fp);/* close the file if it was opened */
          pLOG->fp = NULL;
          }
        sprintf(pLOG->szFileName,pLOG->pszFileName,gm_time.tm_year+1900,gm_time.tm_mon,gm_time.tm_mday,gm_time.tm_hour);
        pLOG->fp = fopen(pLOG->szFileName,"a");
        /* store the time interval value of the current log */
        if( pLOG->TimeSpan )
          pLOG->LastTime = pLI->Time / pLOG->TimeSpan;
        }
      if( pLOG->fp )
        fprintf(pLOG->fp,"%4d.%02d.%02d %02d:%02d:%02d %s\n",gm_time.tm_year+1900,
                                                             gm_time.tm_mon+1,
                                                             gm_time.tm_mday,
                                                             gm_time.tm_hour,
                                                             gm_time.tm_min,
                                                             gm_time.tm_sec,
                                                             pLI->pszMessage);
      FREE(pLI->pszMessage);
      FREE(pLI);
      }
    }
  }

/*POD
=H log_state()

This function safely returns the actual state of the log. This can be:

=itemize
=item T<LOGSTATE_NORMAL> the log is normal state accepting log items
=item T<LOGSTATE_SHUTTING> the log is currently performing shut down, it does not accept any log item
=item T<LOGSTATE_DEAD> the log is shut down all files are closed
=item T<LOGSTATE_SYNCHRONOUS> the log is synchronous accepting log items
=noitemize

/*FUNCTION*/
int log_state(ptLogger pLOG
  ){
/*noverbatim
CUT*/
  int state;

  /* If the calling program ignores the error returned when the log was initialized
     but cares the state afterwards it gets the state dead. */
  if( pLOG->pszFileName == NULL )return LOGSTATE_DEAD;

  if( pLOG->type == LOGTYPE_SYNCHRONOUS )return LOGSTATE_SYNCHRONOUS;
  thread_LockMutex(&(pLOG->mxState));
  state = pLOG->state;
  thread_UnlockMutex(&(pLOG->mxState));
  return state;
  }

/*POD
=H log_init()

Initialize a log. The function sets the parameters of a logging thread. 
The parameters are the usual memory allocation and deallocation functions
and the log file name format string. This format string can contain at most four
T<%d> as formatting element. This will be passed to T<sprintf> with arguments as
year, month, day and hour in this order. This will ease log rotating.

Note that log file name calculation is a CPU consuming process and therefore it is not
performed for each log item. The log system recalculates the log file name and closes the
old log file and opens a new one whenever the actual log to be written and the last log wrote
is in a different time interval. The time interval is identified by the time stamp value
divided (integer division) by the time span value. This is 3600 when you want to rotate the log
hourly, 86400 if you want to rotate the log daily. Other rotations, like monthly do not work correctly.

To do this the caller has to set the T<TimeSpan> field of the log structure. There is no support function
to set this.

For example:
=verbatim

  if( log_init(&ErrLog,alloc_Alloc,alloc_Free,pM_AppLog,CONFIG("log.err.file"),LOGTYPE_NORMAL) )
    return 1;
  if( cft_GetEx(&MyCONF,"log.err.span",&ConfNode,NULL,&(ErrLog.TimeSpan),NULL,NULL) )
    ErrLog.TimeSpan = 0;

=noverbatim

as you can see in the file T<ad.c> Setting TimeSpan to zero results no log rotation.

Note that it is a good practice to set the TimeSpan value to positive (non zero) even if the
log is not rotated. If you ever delete the log file while the logging application is running
the log is not written anymore until the log file is reopened.

The log type can be T<LOGTYPE_NORMAL> to perform asynchronous high performance logging and
T<LOGTYPE_SYNCHRONOUS> for syncronous, "panic" logging. Panic logging keeps the file continously
opened until the log is shut down and does not perform log rotation.
/*FUNCTION*/
int log_init(ptLogger pLOG,
             void *(*memory_allocating_function)(size_t, void *),
             void (*memory_releasing_function)(void *, void *),
             void *pMemorySegment,
             char *pszLogFileName,
             int iLogType
  ){
/*noverbatim
CUT*/
  THREADHANDLE T;

  pLOG->memory_allocating_function = memory_allocating_function ?
                                     memory_allocating_function
                                               :
                                       _mya;
  pLOG->memory_releasing_function = memory_releasing_function ?
                                    memory_releasing_function
                                               :
                                      _myf;
  pLOG->pMemorySegment = pMemorySegment;
  pLOG->pszFileName = pszLogFileName;
  pLOG->MaxItemLen = 256;
  pLOG->QueueStart = pLOG->QueueEnd = NULL;
  pLOG->state = LOGSTATE_NORMAL;
  if( pszLogFileName == NULL )return 1;
  if( iLogType == LOGTYPE_SYNCHRONOUS ){
    pLOG->type  = LOGTYPE_SYNCHRONOUS;
    pLOG->fp = fopen(pszLogFileName,"a");
    if( pLOG->fp == NULL )return 1;
    return 0;
    }else{
    pLOG->type  = LOGTYPE_NORMAL;
    thread_InitMutex(&(pLOG->mxChain));
    thread_InitMutex(&(pLOG->mxRun));
    thread_InitMutex(&(pLOG->mxState));
    thread_LockMutex(&(pLOG->mxRun));
    thread_CreateThread(&T,log_thread,pLOG);
    pLOG->LastTime = 0;
    pLOG->TimeSpan = 0;
    pLOG->fp = NULL;
    return 0;
    }
  }

/*POD
=H log_printf()

This function can be used to send a formatted log to the log file. The
function creates the formatted string and then puts it onto the log queue.
The log is actually sent to the log file by the asynchronous logger thread.

/*FUNCTION*/
int log_printf(ptLogger pLOG,
               char *pszFormat,
               ...
  ){
/*noverbatim
CUT*/
  va_list marker;
  char *pszMessage;
  ptLogItem pLI;
  int trigger,iState;
  struct tm gm_time;
  long Time;

  /* If the callingprogram ignores the error returned when the log was initialized
     we do net report this as an error any more. This behaves a /dev/null reporting. */
  if( pLOG->pszFileName == NULL )return 0;

  iState = log_state(pLOG);
  /* we do not accept more logs when we are shutting down */
  if( iState == LOGSTATE_SHUTTING || iState == LOGSTATE_DEAD )return 1;

  pszMessage = ALLOC(pLOG->MaxItemLen);
  if( pszMessage == NULL )return 1;

  /* If this is a synchronous logger then simply write the message. */
  if( iState == LOGSTATE_SYNCHRONOUS ){
    time(&Time);
    mygmtime(&Time,&gm_time);
    va_start(marker,pszFormat);
    vsprintf(pszMessage,pszFormat,marker);
    va_end(marker);
    if( pLOG->fp )
      fprintf(pLOG->fp,"%4d.%02d.%02d %02d:%02d:%02d %s\n",gm_time.tm_year+1900,
                                                           gm_time.tm_mon+1,
                                                           gm_time.tm_mday,
                                                           gm_time.tm_hour,
                                                           gm_time.tm_min,
                                                           gm_time.tm_sec,
                                                           pszMessage);
    fflush(pLOG->fp);
    FREE(pszMessage);
    return 0;
    }

  pLI = ALLOC(sizeof(tLogItem));
  if( pLI == NULL ){
    FREE(pszMessage);
    return 1;
    }
  time(&(pLI->Time));
  va_start(marker,pszFormat);
  vsprintf(pszMessage,pszFormat,marker);
  va_end(marker);
  pLI->pszMessage = pszMessage;

  trigger = 0;
  /* chain the message into the queue */
  thread_LockMutex(&(pLOG->mxChain));
  /* append it to the queue end */
  if( pLOG->QueueEnd )
    pLOG->QueueEnd->n = pLI;
  pLI->p = pLOG->QueueEnd;
  pLI->n = NULL;
  pLOG->QueueEnd = pLI;
  if( pLOG->QueueStart == NULL ){
    pLOG->QueueStart = pLI;
    trigger = 1;
    }
  thread_UnlockMutex(&(pLOG->mxChain));
  if( trigger )thread_UnlockMutex(&(pLOG->mxRun));
  return 0;
  }

/*POD
=H log_shutdown()

Calling this function starts the shutdown of a log queue. This function allways return 0 as success.
When the function returns the log queue does not accept more log items, however the queue is not completely
shut down. If the caller wants to wait for the queue to shut down it has to wait and call the function
R<log_state> to ensure that the shutdown procedure has been finished.

/*FUNCTION*/
int log_shutdown(ptLogger pLOG
  ){
/*noverbatim
CUT*/

  /* If the callingprogram ignores the error returned when the log was initialized
     we do net report this as an error any more. This behaves a /dev/null reporting. */
  if( pLOG->pszFileName == NULL )return 0;

  log_printf(pLOG,"Shutting down log.");
  if( pLOG->type == LOGTYPE_SYNCHRONOUS ){
    /* we need not do any sophisticated shut down for synchronous logs */
    pLOG->state = LOGSTATE_DEAD;
    fclose(pLOG->fp);
    pLOG->fp = NULL;
    return 0;
    }
  thread_LockMutex(&(pLOG->mxState));
  pLOG->state = LOGSTATE_SHUTTING; /* signal that we are shutting down this prevents
                                      further log entries. */
  thread_UnlockMutex(&(pLOG->mxState));
  /* Trigger the queue to perform final flush and shutdown. */
  thread_UnlockMutex(&(pLOG->mxRun));
  return 0;
  }
