/* FILE: mt.c

This file contains code fot the multi-thread support module.

For further information read the accopaining documentation that 
I am going to write when I am ready programming this module. Right 
now I just start to write it.

Peter Verhas March 6, 2001.

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

NTLIBS:
UXLIBS:
DWLIBS: -init __init

*/
#include <stdio.h>
#include <time.h>
#include "../../basext.h"

/**
=H The module MT
=abstract
Support for multi-thread implementation web applications.
=end

This module delivers functions that can be used by ScriptBasic
programs executed by a multi-thread basic interpreter. This means
that the programs are executed in the same process one after the
other or in synchron. The typical example is the Eszter SB Engine
that is a standalone webserver execution BASIC programs to answer
http questions. Those programs may ask this module to store
session variables, wait for eachother.

*/
/* This declaraton declares the counter and the mutex protecting it that
   counts the number of threads loading this library. Note that only the
   first call to LoadLibrary/dlopen loads this library the rest is only
   returns the pointer to the already loaded library.

   This library was designed to support multithread implementations so that
   it allows different threads to access data stored in memory. However each
   thread call FreeLibrary/dlclose to release this library. If there is a point
   in time when no thread uses this library then the library is unloaded and all
   data allocated is lost.

   To keep the library in memory the function besSUB_KEEP returns zero when the
   library would be unloaded. Thus exactly one LoadLibrary/dlopen is not followed
   by FreeLibrary/dlclose and thus this library remains in memory so long as long
   the process is alive.                                                               */
SUPPORT_MULTITHREAD

static SymbolTable MtVariables,SessionTable;
static MUTEX mxMTVariables,mxSessionCounter,mxSessionTable;
static unsigned long lSessionCounter;

/* Information that maintains a session. */
typedef struct _SessionInfo {
  SHAREDLOCK lckSession;       /* lock for read when used, lock for write when being deleted */
  MUTEX mxSession;             /* lock whenever access session symbol table                  */
  void *pMemSeg;               /* memory segment for session related memory allocations      */
  SymbolTable stSession;       /* session variables symbol table                             */
  unsigned long lPingTime;     /* timestamp when the session was alive last time             */
  unsigned long lTimeout;      /* timestamp when the session times out (set by BASIC program)*/
  unsigned long lTimeStart;    /* timestamp when the session was started                     */
  char *pszId;                 /* pointer into the symbol table to the if of the session     */
  struct _SessionInfo *prev,*next;
  } SessionInfo, *pSessionInfo;

static pSessionInfo pSessionRoot;

typedef struct _MtClass {
  pSessionInfo AssignedSession;
  char *pszSessionId;
  } MtClass, *pMtClass;

typedef struct _MTVariable{
  SHAREDLOCK lckV;
  VARIABLE vV;
  }MTVariable,*pMTVariable;


#define MT_ERROR_NOSESSION 0x00080001
#define MT_ERROR_BAD_PROG  0x00080002
#define MT_ERROR_BAD_SESS  0x00080003

/* Allocate a new session information structure.
   Note that this function is NOT reentrant but is called when the MUTEX mxSessionTable is locked.
*/
static pSessionInfo AllocNewSession(pSupportTable pSt,void *p){
  pSessionInfo pS;
  pS = besPROCALLOC(sizeof(SessionInfo));
  if( ! pS )return NULL;
  pS->pszId = SymbolName(p);
  pS->lTimeStart = time(NULL);
  /* hook into the double linked list */
  pS->next = pSessionRoot;
  pS->prev = NULL;
  pSessionRoot = pS;
  return pS;
  }

static void ReleaseSession(pSupportTable pSt, pSessionInfo pS){

  besLockMutex(&mxSessionTable);

  /* unhook from the double linked list */
  if( pS->prev )pS->prev->next = pS->next; else pSessionRoot = pS->next;
  if( pS->next )pS->next->prev = pS->prev;

  besPROCFREE(pS);

  besUnlockMutex(&mxSessionTable);
  }

/*
This function is executed as a separate thread when the module is initialized the first time.
It starts the timeout scripts when their time comes.
*/
static void time_out_work_thread(void *p){
  pSupportTable pSt;
  unsigned long lSleepLen;
  pSessionInfo pS;
  pSt = p;

  if( besCONFIGEX(pSt->pEo->pConfig,"httpd.mt.sleep",NULL,NULL,&lSleepLen,NULL,NULL) )
    lSleepLen = 60;

  while(1){
    besSLEEP(lSleepLen);
    pS = pSessionRoot;
    }


  }

#define IsUndef(pVar)  ((pVar) == NULL || (pVar)->vType == VTYPE_UNDEF)


besSUB_SHUTDOWN

besEND

besSUB_START
  pMtClass pMT;

  /* This macro increments the thread counter used to help to keep this library in memory 
     even those times when the process is idle and no ScriptBasic threads are running. */
  INC_THREAD_COUNTER
  /* A process level initialization of the module should take place in this function because
     the call-back support functions are not accessible when the  function DllMain or _init is
     called. On the other hand it has to initialize only once.
  */
  INITLOCK /* lock the init mutex */
  if( iFirst ){ /* if it was not initialized yet*/

    MtVariables = pSt->NewSymbolTable(pSt->Alloc,besPROCMEMORYSEGMENT);
    if( MtVariables == NULL )return COMMAND_ERROR_MEMORY_LOW;

    besInitMutex(&mxMTVariables);

    SessionTable = pSt->NewSymbolTable(pSt->Alloc,besPROCMEMORYSEGMENT);
    if( SessionTable == NULL )return COMMAND_ERROR_MEMORY_LOW;
    besInitMutex(&mxSessionCounter);
    besInitMutex(&mxSessionTable);
    /* time is just to introduce some randomity */
    lSessionCounter = (unsigned long)time(NULL);

    pSessionRoot = NULL;

    /* create the thread that handles timeout script execution */
    /* besCreateThread(&hT,time_out_work_thread,pSt->pEo->pSTI); */

    iFirst = 0;/* successful initialization was done*/
    }
  INITUNLO /* unlock the init mutex */

  besMODULEPOINTER = besALLOC(sizeof(MtClass));
  if( besMODULEPOINTER == NULL )return 0;
  pMT = (pMtClass)besMODULEPOINTER;
  pMT->AssignedSession = NULL;
  pMT->pszSessionId = NULL;

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section session
=H What are session and variables
=abstract
Some description on what is a session and what are session and MT variables.
=end

Formally a session is the series of consecutive execution of ScriptBasic scripts
that belong to the same calculation. Typically this is the series of web script
executions that serve the same client.

When a client meets the web server the first time the program creates a new session and
when the user goes on with the web application the same session information is used. The
session ID is usually sent to the client in HTTP cookie and when the browser sends
the cookie back the server application remembers that this client is the same as the one
before. What the application really remembers is that state of the transaction the user
performed in the session. The tipical example is the consumer basket in an eShop. The user
behind the browser puts several things into the basket while shopping and the application
has to remember what is in the basket.

Conventional CGI applications usually store the session information in plain text files
or in database. That is usually a slow solution unless you need to store these information
for a longer period. But session state is usually not stored for long time. 

When a browser first comes to the site a new session is created calling R<NewSessionId>. Later
when the user gets to the next page the session id is sent from the browser and the application
checks that the session really exists calling R<CheckSessionId> and if it exists it tells the
MT extension module calling R<SetSessionID> that the actual session served by the currently 
running interpreter thread is this.

To store a value that belongs to that session the application can call R<SetSessionVariable> and
to retrieve the value call R<GetSessionVariable>. Session variables are kept in memory and can
be reached extremely fast. They are available until the session is deleted calling R<DeleteSession>.

There can be not two interpreter threads running concurrently accessing the same session data.
Each session has its own variable set and in case a session has a variable named "BASKET" this
has nothing to do with the other sessions variable of the same name.

MT variables on the other hand are shared by all sessions. An MT variable has a name, and in case
a session modifies the MT variable named "BASKET" the other session looking at the MT variable "BASKET"
will see the changed value. MT variables are global among the sessions.

When accessing an MT variable the variable can not be accesses by other interpreter threads. Thus
there is no chance that an MT variable gets up messed. What is more a program may decide to lock an
MT variable preventing other programs to alter it or even to read its value until the variable
is unlocked. For this the program should call R<LockWrite>, R<LockRead> and to unlock R<UnlockWrite>,
R<UnlockRead>

*/

/**
=section SetSessionId
=H mt::SetSessionId "session id"

Set the session for the script. This function can and should be used to specify the session that the
program actions belong to. Before calling this function and specifying the session the program can not
access the session variables.

The session ID is usually given by the client program by some means. Typically web browser cookies hold
the session identifier.
*/
besFUNCTION(setsession)
  char *pszSession;
  pMtClass pMT;
  void **p;
  VARIABLE Argument;
  pSessionInfo pS;
  unsigned long i;

  pMT = besMODULEPOINTER;
  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  /* check that the session ID does not contain zero character inside (for security reasons) */
  for( i = 0 ; i < STRLEN(Argument) ; i++ )
    if( ! STRINGVALUE(Argument)[i] )return MT_ERROR_BAD_SESS;

  besCONVERT2ZCHAR(Argument,pszSession);
  /* if this thread already has a session assigned to it then release the session */
  pS = pMT->AssignedSession;
  if( pMT->pszSessionId )besFREE(pMT->pszSessionId);
  if( pS )besUnlockSharedRead(&(pS->lckSession));
  pS = pMT->AssignedSession = NULL; /* pS to NULL also just for safety */

  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(pszSession,SessionTable,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  if( p == NULL ){
    besUnlockMutex(&mxSessionTable);
    besFREE(pszSession);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  if( *p == NULL ){/* this is a brand new session, most probably it is */
    /* count the creation of the new session */
    besLockMutex(&mxSessionCounter);
    lSessionCounter++;
    besUnlockMutex(&mxSessionCounter);

    *p = AllocNewSession(pSt,p);
    if( *p == NULL ){
      besUnlockMutex(&mxSessionTable);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    pS = pMT->AssignedSession = *p;
    besInitMutex(&(pS->mxSession));
    besInitSharedLock(&(pS->lckSession));
    besLockSharedRead(&(pS->lckSession));
    /* Create a new memory segment for the session. */
    pS->pMemSeg = besINIT_SEGMENT(pSt->pEo->pMemorySegment,NULL);
    if( pS->pMemSeg == NULL )return COMMAND_ERROR_MEMORY_LOW;
    pS->stSession = pSt->NewSymbolTable(pSt->Alloc,pS->pMemSeg);
    if( pS->stSession == NULL )return COMMAND_ERROR_MEMORY_LOW;
    besUnlockMutex(&mxSessionTable);
    }else{/* this is an already existing session */
    pS = *p;
    besLockSharedRead(&(pS->lckSession));
    besUnlockMutex(&mxSessionTable);
    }

  /* the session id string should be copied because a program may think that it is executing a session and
     another thread deletes the session in the mean time, but can not know that there is a thread using the
     session */
  pMT->pszSessionId = besALLOC(strlen(SymbolName(p))+1);
  if( pMT->pszSessionId == NULL )return COMMAND_ERROR_MEMORY_LOW;
  strcpy(pMT->pszSessionId,SymbolName(p));
  pMT->AssignedSession = pS;
  pS->lPingTime = time(NULL);
  besUnlockSharedRead(&(pS->lckSession));

besEND

/**
=section CheckSessionId
=H mt::CheckSessionId("sessionid")

Checks that a session already exists. The function returns T<TRUE>
if the session id already exists.

This function can and should be used to check that a session is valid or not based
on its existence. If the session is not existent the program can still call the
function R<SetSessionId> to set the session and the new session is autmatically created.

However calling this function is a good chanhe to check if any session initialization is
needed, for example password check.

*/
besFUNCTION(chksession)
  char *pszSession;
  pMtClass pMT;
  void **p;
  VARIABLE Argument;
  pSessionInfo pS;
  unsigned long i;

  besRETURNVALUE = NULL;
  pMT = besMODULEPOINTER;
  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  /* check that the session ID does not contain zero character inside (for security reasons) */
  for( i = 0 ; i < STRLEN(Argument) ; i++ )
    if( ! STRINGVALUE(Argument)[i] )return MT_ERROR_BAD_SESS;

  besCONVERT2ZCHAR(Argument,pszSession);

  pS = pMT->AssignedSession = NULL;
  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(pszSession,SessionTable,0,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besUnlockMutex(&mxSessionTable);
  besFREE(pszSession);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = p == NULL ? 0 : -1 ;
  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section SessionCount
=H mt::SessionCount()

This function returns the number of the active sessions.
*/
besFUNCTION(sessioncount)
  besALLOC_RETURN_LONG;
  /* count the creation of the new session */
  besLockMutex(&mxSessionCounter);
  LONGVALUE(besRETURNVALUE) = lSessionCounter;
  besUnlockMutex(&mxSessionCounter);
  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section NewSessionId
=H mt::NewSessionId(["optional random string"])

This function creates a new session and returns the ID of the newly created
session and returns the id of the session.

If no argument is provided the function just count the session and 
creates a 32 bytes session id using MD5.

If there is an optional argument that is also used to create the
MD5 session id. That may provide some more randimity from the
application level.
*/
besFUNCTION(newsession)
  unsigned long lThisSession;
  MD5_CTX Context;
  unsigned char digest[16];
  int i;
  pMtClass pMT;
  void **p;
  pSessionInfo pS;
  VARIABLE Argument;

  pMT = besMODULEPOINTER;
  Argument = NULL;
  if( besARGNR > 0 ){
    Argument = besARGUMENT(1);
    besDEREFERENCE(Argument);
    if( Argument){
      Argument = besCONVERT2STRING(Argument);
      }
    }

  /* count the creation of the new session */
  besLockMutex(&mxSessionCounter);
  lThisSession = lSessionCounter++;
  besUnlockMutex(&mxSessionCounter);

  /* create a new session id */
  besMD5INIT(&Context);
  besMD5UPDATE(&Context,(unsigned char *)&lThisSession,sizeof(unsigned long));
  if( Argument )besMD5UPDATE(&Context,(unsigned char *)STRINGVALUE(Argument),STRLEN(Argument));
  besMD5FINAL(digest,&Context);
  besALLOC_RETURN_STRING(33);
  STRLEN(besRETURNVALUE) = 32;
  /* convert the digest to ASCII */
  for( i = 0 ; i < 16 ; i++ ){
    STRINGVALUE(besRETURNVALUE)[2*i] = 'A' + (digest[i]&0x0F);
    digest[i] >>= 4;
    STRINGVALUE(besRETURNVALUE)[2*i+1] = 'A' + (digest[i]&0x0F);
    }
  STRINGVALUE(besRETURNVALUE)[32] = (char)0;/* terminate it to be used in the symbol table */

  pMT->AssignedSession = NULL;
  if( pMT->pszSessionId )besFREE(pMT->pszSessionId);
  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(STRINGVALUE(besRETURNVALUE),SessionTable,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  if( p == NULL ){
    besUnlockMutex(&mxSessionTable);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  if( *p == NULL ){/* this is a brand new session, most probably it is */
    *p = AllocNewSession(pSt,p);
    if( *p == NULL ){
      besUnlockMutex(&mxSessionTable);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    pS = pMT->AssignedSession = *p;
    pS-> lPingTime = time(NULL);
    besInitMutex(&(pS->mxSession));
    besInitSharedLock(&(pS->lckSession));
    besLockSharedRead(&(pS->lckSession));
    /* Create a new memory segment for the session. */
    pS->pMemSeg = besINIT_SEGMENT(pSt->pEo->pMemorySegment,NULL);
    if( pS->pMemSeg == NULL )return COMMAND_ERROR_MEMORY_LOW;
    pS->stSession = pSt->NewSymbolTable(pSt->Alloc,pS->pMemSeg);
    if( pS->stSession == NULL )return COMMAND_ERROR_MEMORY_LOW;
    besUnlockMutex(&mxSessionTable);
    }else{
    pS = *p;
    besLockSharedRead(&(pS->lckSession));
    besUnlockMutex(&mxSessionTable);
    }
  pMT->pszSessionId = besALLOC(strlen(SymbolName(p))+1);
  if( pMT->pszSessionId == NULL )return COMMAND_ERROR_MEMORY_LOW;
  strcpy(pMT->pszSessionId,SymbolName(p));
  pMT->AssignedSession = pS;
  pS->lPingTime = time(NULL);
  besUnlockSharedRead(&(pS->lckSession));

besEND

static void FinishSegmentCallBack(char *SymbolName, void *SymbolValue, void *f){
  pSupportTable pSt;
  pMTVariable p;

  /* we need this variable to use the bes macros */
  pSt = f;
  p = SymbolValue;
  besFinishSharedLock(&(p->lckV));
  }

/**
=section DeleteSession
=H mt::DeleteSession ["sessionid"]

Call this function to delete a session. The function deletes the session and releases
all memory that was allocated to store session variables.

If no argumentum is supplied then the function deletes the actual session.
*/
besFUNCTION(delsession)
  char *pszSession;
  void **p;
  VARIABLE Argument;
  pSessionInfo pS;
  pMtClass pMT;
  int bSessDef; /* TRUE if the session was the default (actual) session 
                   This variable also shows that pszSession should be realeased */

  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  besRETURNVALUE = NULL;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  bSessDef = 0;
  if( Argument == NULL ){
    /* In this case we delete the actual session. */
    bSessDef = 1;
    pszSession = pMT->pszSessionId;
    if( pszSession == NULL )return MT_ERROR_NOSESSION;
    }else{
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszSession);
    }

  /* if this is the actual session that we delete either because the user
     supplied no argument or because the user supplied the id of the actual
     session then we have to release the read lock on the session           */
  if( bSessDef || (pMT->pszSessionId && !strcmp(pszSession,pMT->pszSessionId)) ){
    besUnlockSharedRead(&(pS->lckSession));
    pMT->pszSessionId;
    pMT->AssignedSession = NULL;
    }

  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(pszSession,SessionTable,0,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);

  /* there is no such session, nothing to delete */
  if( p == NULL ){
    besUnlockMutex(&mxSessionTable);
    if( ! bSessDef )besFREE(pszSession);
    return COMMAND_ERROR_SUCCESS;
    }
  /* store the pointer to the session data */
  pS = *p;

  /* delete the symbol and the symbol tabl entry */
  pSt->DeleteSymbol(pszSession,SessionTable,pSt->Free,besPROCMEMORYSEGMENT);
  besUnlockMutex(&mxSessionTable);

  /* this should not happen. If the session id is in the table it HAS to be initialized */
  if( pS == NULL ){
    /* note that we check it here after the symbol has been deleted */
    if( ! bSessDef )besFREE(pszSession);
    return EXE_ERROR_INTERNAL;
    }

  /*  This lock waits for all executing read locks. Because we have already deleted
      the session symbol from the symbol table no new read locks may start waiting. */
  besLockSharedWrite(&(pS->lckSession));

  /* This call releases all locks using besFinishSharedLock. We should do this because some
     lock implementation in some operating system may loose some resources if we do not
     call the system function releasing a seaphore or mutex but only release the memory.
     If an operating system does not loose any resource if we just release the memory
     allocated for a mutex/semaphore then this function call may be deleted. */
  besTRAVERSESYMBOLTABLE(pS->stSession,FinishSegmentCallBack,pSt);

  /* Delete all memory that was assigned to the session. That is all session variables and their
     corresponding locks. */
  besFINISH_SEGMENT(pS->pMemSeg);
  /* Finish the mutex that has protected the symbol table. */
  besFinishMutex(&(pS->mxSession));
  /* we may unlock this shared lock (we could do it before, but who cares...) 
     there should be no one waiting for it */
  besUnlockSharedWrite(&(pS->lckSession));
  /* finish the session shard lock */
  besFinishSharedLock(&(pS->lckSession));
  /* release the session information finally */
  ReleaseSession(pSt,pS);

  /* If there was a pszSession allocated then release it. If it was the default then the variable
     pszSession already points to a memory location that was released when the session data was
     released. */
  if( ! bSessDef )besFREE(pszSession);
besEND

/**
=section GetSessionId
=H mt::GetSessionId()

This function returns the actual session id. This is usually known by
the program because this is the id that was set calling R<SetSessionId> or
was created and returned by R<NewSessionId>. However some programs may need
to call this function.
*/
besFUNCTION(getsession)
  pMtClass pMT;
  char *s;

  pMT = besMODULEPOINTER;
  s = pMT->pszSessionId;
  besRETURNVALUE = NULL;
  if( pMT->AssignedSession == NULL )return COMMAND_ERROR_SUCCESS;
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE),s,strlen(s));
  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section SetSessionVariable
=H mt::SetSessionVariabe "variablename",value

Call this function to set the value of a session variable. The first
argument has to be a string that names the session variable. The second
argument is the new value of the session variable.
*/
besFUNCTION(setsvariable)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  pMtClass pMT;
  pSessionInfo pS;

  besRETURNVALUE = NULL;
  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&(pS->mxSession));
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,pS->stSession,1,pSt->Alloc,pSt->Free,pS->pMemSeg);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&(pS->mxSession));
    return COMMAND_ERROR_MEMORY_LOW;
    }
  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = pSt->Alloc(sizeof(MTVariable),pS->pMemSeg);
    if( *p == NULL ){
      besUnlockMutex(&(pS->mxSession));
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    (*p)->vV = NULL;
    }
  besUnlockMutex(&(pS->mxSession));

  besLockSharedWrite(&((*p)->lckV));

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);

  if( (*p)->vV && TYPE((*p)->vV) == VTYPE_STRING && STRINGVALUE((*p)->vV) ){
    pSt->Free(STRINGVALUE((*p)->vV),pS->pMemSeg);
    STRINGVALUE((*p)->vV) = NULL;
    }

  /* we do not need memory to store undef */
  if( IsUndef(Argument) ){
    if( (*p)->vV )pSt->Free((*p)->vV,pS->pMemSeg);
    (*p)->vV = NULL;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( (*p)->vV == NULL )(*p)->vV = pSt->Alloc(sizeof(FixSizeMemoryObject),pS->pMemSeg);
  if( (*p)->vV == NULL ){
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_MEMORY_LOW;
    }

  (*p)->vV->sType = 0;             /* not used */
  (*p)->vV->State = STATE_UNKNOWN; /* not used */
  (*p)->vV->next = NULL;           /* not used */
  (*p)->vV->link.prev = NULL;           /* not used */
  (*p)->vV->ArrayHighLimit = 0;
  (*p)->vV->ArrayLowLimit  = 1;

  if( TYPE(Argument) == VTYPE_STRING ){
    (*p)->vV->Value.pValue = pSt->Alloc(STRLEN(Argument) ? STRLEN(Argument) : 1 ,pS->pMemSeg);
    if( (*p)->vV->Value.pValue == NULL ){
      pSt->Free((*p)->vV,pS->pMemSeg);
      (*p)->vV = NULL;
      besUnlockSharedWrite(&((*p)->lckV));
      return COMMAND_ERROR_MEMORY_LOW;
      }
    memcpy((*p)->vV->Value.pValue,STRINGVALUE(Argument),STRLEN(Argument));
    (*p)->vV->Size = STRLEN(Argument);
    TYPE((*p)->vV) = VTYPE_STRING;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE(Argument) == VTYPE_LONG || TYPE(Argument) == VTYPE_DOUBLE ){
    TYPE((*p)->vV) = TYPE(Argument);
    (*p)->vV->Size = 0;
    (*p)->vV->Value = Argument->Value;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }
  
  besUnlockSharedWrite(&((*p)->lckV));
  return EXE_ERROR_INTERNAL;
besEND

/**
=section GetSessionVariable
=H mt::GetSessionVariabe("variablename")

Get the value of a session variable. The argument has to be a string that
identifies the session variable.

The value of a session variable can be set using R<SetSessionVariable>.
*/
besFUNCTION(getsvariable)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  pMtClass pMT;
  pSessionInfo pS;

  besRETURNVALUE = NULL;
  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&(pS->mxSession));
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,pS->stSession,0,pSt->Alloc,pSt->Free,pS->pMemSeg);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&(pS->mxSession));
    return COMMAND_ERROR_SUCCESS; /* the value is undef, cause it was not defined */
    }

  if( *p == NULL ){
    besUnlockMutex(&(pS->mxSession));
    return COMMAND_ERROR_SUCCESS; /* the value is undef, cause it was defined to be undef */
    }
  besUnlockMutex(&(pS->mxSession));

  if( (*p)->vV == NULL )return COMMAND_ERROR_SUCCESS;

  besLockSharedRead(&((*p)->lckV));

  if( TYPE((*p)->vV) == VTYPE_STRING ){
    besALLOC_RETURN_STRING(STRLEN( (*p)->vV ));
    memcpy(STRINGVALUE(besRETURNVALUE),STRINGVALUE( (*p)->vV ),STRLEN( (*p)->vV) );
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE((*p)->vV) == VTYPE_LONG ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = LONGVALUE((*p)->vV);
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE((*p)->vV) == VTYPE_DOUBLE ){
    besALLOC_RETURN_DOUBLE;
    DOUBLEVALUE(besRETURNVALUE) = DOUBLEVALUE((*p)->vV);
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  return EXE_ERROR_INTERNAL;
besEND

/**
=section SetVariable
=H mt::SetVariable "variablename",value

Set the value of an MT variable. The first argument has to be a string that
identifies the variable. The second argument is the new value of the
variable.

MT variables are global, all running scripts like the same values.
*/
besFUNCTION(setmtvariable)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_MEMORY_LOW;
    }

  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = besPROCALLOC(sizeof(MTVariable));
    if( *p == NULL ){
      besUnlockMutex(&mxMTVariables);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    /* make the variable initially undef */
    (*p)->vV = NULL;
    }
  besUnlockMutex(&mxMTVariables);

  besLockSharedWrite(&((*p)->lckV));

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);

  /* release the old value because we are going to replace that anyway */
  if( (*p)->vV && TYPE((*p)->vV) == VTYPE_STRING && STRINGVALUE((*p)->vV) ){
    besPROCFREE(STRINGVALUE((*p)->vV));
    STRINGVALUE((*p)->vV) = NULL;
    }

  /* we do not need memory to store undef */
  if( IsUndef(Argument) ){
    if( (*p)->vV )besPROCFREE((*p)->vV);
    (*p)->vV = NULL;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( (*p)->vV == NULL )(*p)->vV = besPROCALLOC(sizeof(FixSizeMemoryObject));
  if( (*p)->vV == NULL ){
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_MEMORY_LOW;
    }

  (*p)->vV->sType = 0;             /* not used */
  (*p)->vV->State = STATE_UNKNOWN; /* not used */
  (*p)->vV->next = NULL;           /* not used */
  (*p)->vV->link.prev = NULL;           /* not used */
  (*p)->vV->ArrayHighLimit = 0;
  (*p)->vV->ArrayLowLimit  = 1;

  if( TYPE(Argument) == VTYPE_STRING ){
    (*p)->vV->Value.pValue = besPROCALLOC(STRLEN(Argument) ? STRLEN(Argument) : 1 );
    if( (*p)->vV->Value.pValue == NULL ){
      besUnlockSharedWrite(&((*p)->lckV));
      return COMMAND_ERROR_MEMORY_LOW;
      }
    memcpy((*p)->vV->Value.pValue,STRINGVALUE(Argument),STRLEN(Argument));
    (*p)->vV->Size = STRLEN(Argument);
    TYPE((*p)->vV) = VTYPE_STRING;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE(Argument) == VTYPE_LONG || TYPE(Argument) == VTYPE_DOUBLE ){
    TYPE((*p)->vV) = TYPE(Argument);
    (*p)->vV->Size = 0;
    (*p)->vV->Value = Argument->Value;
    besUnlockSharedWrite(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }
  
  besUnlockSharedWrite(&((*p)->lckV));
  return EXE_ERROR_INTERNAL;
besEND

/**
=section GetVariable
=H mt::GetVariable("variablename")

Get Mt variable. This function gets the value of an Mt variable.
*/
besFUNCTION(getmtvariable)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,0,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_SUCCESS; /* the value is undef, cause it was not defined */
    }

  if( *p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_SUCCESS; /* the value is undef, cause it was defined to be undef */
    }
  besUnlockMutex(&mxMTVariables);

  if( (*p)->vV == NULL )return COMMAND_ERROR_SUCCESS;

  besLockSharedRead(&((*p)->lckV));

  if( TYPE((*p)->vV) == VTYPE_STRING ){
    besALLOC_RETURN_STRING(STRLEN( (*p)->vV ));
    memcpy(STRINGVALUE(besRETURNVALUE),STRINGVALUE( (*p)->vV ),STRLEN( (*p)->vV) );
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE((*p)->vV) == VTYPE_LONG ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = LONGVALUE((*p)->vV);
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  if( TYPE((*p)->vV) == VTYPE_DOUBLE ){
    besALLOC_RETURN_DOUBLE;
    DOUBLEVALUE(besRETURNVALUE) = DOUBLEVALUE((*p)->vV);
    besUnlockSharedRead(&((*p)->lckV));
    return COMMAND_ERROR_SUCCESS;
    }

  return EXE_ERROR_INTERNAL;
besEND

/**
=section LockWrite
=H mt::LockWrite "variablename"

Lock mt variable for write protection. While the variable is locked no programs can access it, not even
the one that locked the variable.

Usually there is no need to alter the MT variables, as the variables are automatically locked while
their value is altered. Locking is needed when a program needs to alter more than one MT variable in
a coincise manner. Another possibility is when a program wants to alter a variable and the new value depends
on the previous value. As a simplest example is when aprogram wants to increment a variable.

In either case the programs have to have a pseudo MT variable, which is used in the locking and
unlocking functions. Usually the programs use these variables only to lock and to release
and do not care their values, albeit there is nothing that would prohibit using the values.

For example a program (X) wants to increment the MT variable "B" and it has to keep track of the 
variable "BB" to ensure that "BB" is the double of "B".

In the meantime another (Y) program wants to decrement "BB" by two and set "B" to the half of "BB" to
keep the rule.

Program X:
=verbatim
mt::GetVariable "B",B
B = B+1
mt::SetVariable "B",B
mt::SetVariable "BB",2*B
=noverbatim

Program Y:
=verbatim
mt::GetVariable "BB",BB
BB = BB-2
mt::SetVariable "BB",BB
mt::SetVariable "B",B \ 2
=noverbatim

The lines of the two programs can execute in any mixed order.

See the following scenario:
=itemize
=item Program X examines B and sees that B is 3 (for example)
=item Program Y examines BB and sees that BB is 6.
=item Program X increments B to be 4.
=item Program Y decrements BB to be 4.
=item Program Y sets B to be the half of BB to 2.
=item Program X sets the variable BB to the double of B, which is 8 for that program.
=end itemize

Instead a variable called "BLCK" has to be used (BLCK is just for example):

Program X:
=verbatim
mt::LockWrite "BLCK"
mt::GetVariable "B",B
B = B+1
mt::SetVariable "B",B
mt::SetVariable "BB",2*B
mt::UnlockWrite "BLCK"
=noverbatim

Program Y:
=verbatim
mt::LockWrite "BLCK"
mt::GetVariable "BB",BB
BB = BB-2
mt::SetVariable "BB",BB
mt::SetVariable "B",B \ 2
mt::UnlockWrite "BLCK"
=noverbatim

The locking at the start of the critical code segments prevents two programs to enter the critical part at the
same time.

*/
besFUNCTION(lockmtwrite)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = besPROCALLOC(sizeof(MTVariable));
    if( *p == NULL ){
      besUnlockMutex(&mxMTVariables);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    (*p)->vV = NULL;
    }
  besUnlockMutex(&mxMTVariables);

  besLockSharedWrite(&((*p)->lckV));

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section LockRead
=H mt::LockRead "variablename"

Lock mt variable for read protection. Several concurrent programs can read lock a variable,
but so long as long there are read locks no program can write lock a variable.

The function will wait until it can lock the variable. A variable can not be read locked if
the variable is write locked or there is a write lock waiting for the variable.
*/
besFUNCTION(lockmtread)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = besPROCALLOC(sizeof(MTVariable));
    if( *p == NULL ){
      besUnlockMutex(&mxMTVariables);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    (*p)->vV = NULL;
    }
  besUnlockMutex(&mxMTVariables);

  besLockSharedRead(&((*p)->lckV));

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section UnlockWrite
=H mt::UnlockWrite "variablename"

Unlock mt variable from write protection.
*/
besFUNCTION(unlockmtwrite)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = besPROCALLOC(sizeof(MTVariable));
    if( *p == NULL ){
      besUnlockMutex(&mxMTVariables);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    (*p)->vV = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  besUnlockMutex(&mxMTVariables);

  besUnlockSharedWrite(&((*p)->lckV));

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section UnlockRead
=H mt::UnlockRead "variablename"
Unlock mt variable from read protection
*/
besFUNCTION(unlockmtread)
  VARIABLE Argument;
  char *pszVariableName;
  pMTVariable *p;
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszVariableName);

  besLockMutex(&mxMTVariables);
  p = (pMTVariable *)pSt->LookupSymbol(pszVariableName,MtVariables,1,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);
  besFREE(pszVariableName);
  if( p == NULL ){
    besUnlockMutex(&mxMTVariables);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  /* if this is the first call regarding this variable then create it*/
  if( *p == NULL ){
    *p = besPROCALLOC(sizeof(MTVariable));
    if( *p == NULL ){
      besUnlockMutex(&mxMTVariables);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    besInitSharedLock(&((*p)->lckV));
    (*p)->vV = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  besUnlockMutex(&mxMTVariables);

  besUnlockSharedRead(&((*p)->lckV));

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section SessionTimeout
=H mt::SessionTimeout [sec]

Call this function to set session timeout (20 minutes in the example)
=verbatim
mt::SessionTimeout 20*60
=noverbatim
or
=verbatim
mt::SessionTimeout
=noverbatim
cancelling session timeout.

*/
besFUNCTION(sessionto)
  VARIABLE Argument;
  pSessionInfo pS;
  pMtClass pMT;

  besRETURNVALUE = NULL;
  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  /* if there is no actual session selected */
  if( pS == NULL )return MT_ERROR_NOSESSION;

  /* If there is no any argument then it means that the session
     timeout should be cancelled. */
  if( besARGNR < 1 ){
    pS->lTimeout = 0;
    return COMMAND_ERROR_SUCCESS;
    }

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  /* the timeout delay should not be undefined */
  if( Argument == NULL )return COMMAND_ERROR_FEW_ARGS;
  Argument = besCONVERT2LONG(Argument);
  /* convert the delay seconds to absolute GMT time stamp (well timestamp is GMT per def) */
  pS->lTimeout= time(NULL) + LONGVALUE(Argument);

  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section GetSessionTimeout
=H mt::GetSessionTimeout ["sessionid"]

This function returns the time when the session times out. This value is
expressed as GMT seconds since January 1, 1970. 0:00.

If the argument is missing the actual argument is used.
*/
besFUNCTION(getsesto)
  char *pszSession;
  void **p;
  VARIABLE Argument;
  pSessionInfo pS;
  pMtClass pMT;
  int bSessDef; /* TRUE if the session was the default (actual) session 
                   This variable also shows that pszSession should be realeased */

  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  besRETURNVALUE = NULL;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    bSessDef = 1;
    pszSession = pMT->pszSessionId;
    if( pszSession == NULL )return MT_ERROR_NOSESSION;
    }else{
    bSessDef = 0;
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszSession);
    }

  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(pszSession,SessionTable,0,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);

  /* there is no such session, there is no timeout */
  if( p == NULL ){
    besUnlockMutex(&mxSessionTable);
    if( ! bSessDef )besFREE(pszSession);
    besRETURNVALUE = NULL; /* return undef */
    if( ! bSessDef )besFREE(pszSession);
    return COMMAND_ERROR_SUCCESS;
    }
  /* store the pointer to the session data */
  pS = *p;

  besUnlockMutex(&mxSessionTable);

  /* this should not happen. If the session id is in the table it HAS to be initialized */
  if( pS == NULL ){
    /* note that we check it here after the symbol has been deleted */
    if( ! bSessDef )besFREE(pszSession);
    besRETURNVALUE = NULL;
    if( ! bSessDef )besFREE(pszSession);
    return EXE_ERROR_INTERNAL;
    }

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = pS->lTimeout;
  /* If there was a pszSession allocated then release it. If it was the default then the variable
     pszSession already points to a memory location that was released when the session data was
     released. */
  if( ! bSessDef )besFREE(pszSession);
besEND

/**
=section GetSessionPingTime
=H mt::GetSessionPingTime ["sessionid"]

This function returns the time when the session was last used (assigned to a script).
This value is expressed as GMT seconds since January 1, 1970. 0:00.

If the argument is missing the actual argument is used.
*/
besFUNCTION(getsespt)
  char *pszSession;
  void **p;
  VARIABLE Argument;
  pSessionInfo pS;
  pMtClass pMT;
  int bSessDef; /* TRUE if the session was the default (actual) session 
                   This variable also shows that pszSession should be realeased */

  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

  besRETURNVALUE = NULL;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    bSessDef = 1;
    pszSession = pMT->pszSessionId;
    if( pszSession == NULL )return MT_ERROR_NOSESSION;
    }else{
    bSessDef = 0;
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszSession);
    }

  besLockMutex(&mxSessionTable);
  p = pSt->LookupSymbol(pszSession,SessionTable,0,pSt->Alloc,pSt->Free,besPROCMEMORYSEGMENT);

  /* there is no such session, there is no ping time */
  if( p == NULL ){
    besUnlockMutex(&mxSessionTable);
    if( ! bSessDef )besFREE(pszSession);
    besRETURNVALUE = NULL; /* return undef */
    if( ! bSessDef )besFREE(pszSession);
    return COMMAND_ERROR_SUCCESS;
    }
  /* store the pointer to the session data */
  pS = *p;

  besUnlockMutex(&mxSessionTable);

  /* this should not happen. If the session id is in the table it HAS to be initialized */
  if( pS == NULL ){
    /* note that we check it here after the symbol has been deleted */
    if( ! bSessDef )besFREE(pszSession);
    besRETURNVALUE = NULL;
    if( ! bSessDef )besFREE(pszSession);
    return EXE_ERROR_INTERNAL;
    }

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = pS->lPingTime;
  /* If there was a pszSession allocated then release it. If it was the default then the variable
     pszSession already points to a memory location that was released when the session data was
     released. */
  if( ! bSessDef )besFREE(pszSession);
besEND

/**
=section ListSessions
=H mt::ListSessions

This subroutine collects the session ids.

=verbatim
  call mt::ListSessions Array,Sm,SM,Pm,PM,Tm,TM
=noverbatim

The first argument to the subroutine should be a variable. This variable will loose its previous
value and gets a new array value containing the session strings.

If there is no more arguments to the subroutine call this array will contain all the
existing sessions in the MT module. However the programmer may define six optional
parameters that select only certain sessions that have time value parameters that
are between the specified values. These are

=itemize
=item T<Sm> required minimal start time of the session
=item T<SM> required maximal start time of the session
=item T<Pm> required minimal ping time of the session
=item T<PM> required maximal ping time of the session
=item T<Tm> required minimal time-out time of the session
=item T<TM> required maximal time-out time of the session
=noitemize

These time values have to be expressed as GTM time stamps. If a value is zero or T<undef>
the value in the constraint is ignored. Unspecified arguments are T<undef> by ScriptBasic behaviour.

For example to get the list of all sessions that are older than an hour

=verbatim
call mt::ListSession Array,undef,GmTime()-60*60
=noverbatim

To get all session IDs that timed out:

=verbatim
call mt::ListSession Array,undef,undef,undef,undef,undef,GmTime()
=noverbatim

To get all sessions that are idle for more than ten minutes

=verbatim
call mt::ListSession Array,undef,undef,undef,GmTime()-600
=noverbatim

Note that this subroutine is rarely used in "cgi" scripts. This
subroutine has to be used in scripts that are run by the Eszter
SB Engine asynchronously started using the configuration
key T<httpd.run.[re]start>. Those scripts start before the
engine starts to listen on a port and run so long as long
the engine runs and are able to scan the sessions from time to
time and delete the old sessions freeing the memoryof the module
MT.

*/
besFUNCTION(listses)
  VARIABLE Argument,*Lval;
  long cSessionList;
  pSessionInfo pS;
  /* Timeout and Ping time min and max crtiteria values */
  unsigned long lToMin,lToMax,lPtMin,lPtMax,lStMin,lStMax;
  int iError;
  long __refcount_;

  Argument = besARGUMENT(1);
  besLEFTVALUE(Argument,Lval);
  if( ! Lval )return COMMAND_ERROR_ARGUMENT_TYPE;
  besRELEASE(*Lval);
  iError = besGETARGS "*[iiiiii]" , &lStMin, &lStMax, &lPtMin, &lPtMax, &lToMin, &lToMax besGETARGE

  if( iError )return iError;

#define SESSION__CONSTRAINT \
        ( lToMin < pS->lTimeout )  && \
        ( lPtMin < pS->lPingTime)  && \
        ( lStMin < pS->lTimeStart) &&\
        ( (!lToMax) || lToMax > pS->lTimeout)  && \
        ( (!lPtMax) || lPtMax > pS->lPingTime) && \
        ( (!lStMax) || lStMax > pS->lTimeStart)

  pS = pSessionRoot;
  cSessionList = 0;
  besLockMutex(&mxSessionTable);
  while( pS ){
    if( SESSION__CONSTRAINT )cSessionList ++;
    pS = pS->next;
    }

  if( cSessionList == 0 ){
    *Lval = NULL;
    besUnlockMutex(&mxSessionTable);
    return COMMAND_ERROR_SUCCESS;
    }
  *Lval = besNEWARRAY(1,cSessionList);
  if( *Lval == NULL ){
    besUnlockMutex(&mxSessionTable);
    return COMMAND_ERROR_MEMORY_LOW;
    }

  pS = pSessionRoot;
  cSessionList = 0;
  while( pS ){
    if( SESSION__CONSTRAINT ){
      (*Lval)->Value.aValue[cSessionList] = besNEWSTRING(strlen(pS->pszId));
      if( (*Lval)->Value.aValue[cSessionList] == NULL ){
        besUnlockMutex(&mxSessionTable);
        return COMMAND_ERROR_MEMORY_LOW;
        }
      memcpy(STRINGVALUE((*Lval)->Value.aValue[cSessionList]),pS->pszId,strlen(pS->pszId));
      cSessionList ++;
      }
    pS = pS->next;
    }

  besUnlockMutex(&mxSessionTable);


besEND


besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND


besSUB_FINISH
  pSessionInfo pS;
  pMtClass pMT;

  pMT = besMODULEPOINTER;
  pS = pMT->AssignedSession;

 /* This macro decrements the thread counter used to help to keep this library in memory 
    even those times when the process is idle and no ScriptBasic threads are running. */
  DEC_THREAD_COUNTER

  /* Release the session from the read lock (if any) */
  if( pS )besUnlockSharedRead(&(pS->lckSession));

besEND

besDLL_MAIN

/* This function is used to tell the ScriptBasic interpreter whether to call FreeLibrary/dlclose
   or not. When this function returns TRUE the interpreter unloads the library. If this function
   returns FALSE the interpreter does not call FreeLibrary/dlclose and thus the library remains
   in memory even when there is no any thread actually using it. */
besSUB_KEEP
  long lTC;

  /* get the actual value of the thread counter that counts the number of threads using the library
     currently */
  GET_THREAD_COUNTER(lTC);
  /* if this was the last thread then we will return 0 not to unload the library and therefore
     this thread finish should not be counted */
  if( lTC == 0 ){
    INC_THREAD_COUNTER
    }
  /* conver the long counter to (int) boolean */
  return lTC ? 1 : 0;
besEND

besSUB_PROCESS_START
  INIT_MULTITHREAD
  return 1;
besEND

besSUB_PROCESS_FINISH
  FINISH_MULTITHREAD
besEND

SLFST MT_SLFST[] ={

{ "keepmodu" , keepmodu },
{ "shutmodu" , shutmodu },
{ "bootmodu" , bootmodu },
{ "setsession" , setsession },
{ "chksession" , chksession },
{ "sessioncount" , sessioncount },
{ "newsession" , newsession },
{ "delsession" , delsession },
{ "getsession" , getsession },
{ "setsvariable" , setsvariable },
{ "getsvariable" , getsvariable },
{ "setmtvariable" , setmtvariable },
{ "getmtvariable" , getmtvariable },
{ "lockmtwrite" , lockmtwrite },
{ "lockmtread" , lockmtread },
{ "unlockmtwrite" , unlockmtwrite },
{ "unlockmtread" , unlockmtread },
{ "sessionto" , sessionto },
{ "getsesto" , getsesto },
{ "getsespt" , getsespt },
{ "listses" , listses },
{ "versmodu" , versmodu },
{ "finimodu" , finimodu },
{ "keepmodu" , keepmodu },
{ "_init" , _init },
{ "_fini" , _fini },
{ NULL , NULL }
  };
