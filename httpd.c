/*
FILE: httpd.c
HEADER: httpd.h

TO_HEADER:

#ifdef WIN32
typedef u_short port_t;
#else
#ifndef __DARWIN__
typedef int port_t;
#endif
#define closesocket close
#endif

// the default port to listen for the http protocol
#define HTTPD_PORT 80
// the default ip address for the http protocol
#define HTTPD_IP INADDR_ANY

// the listen backlog that we pass to the operating system
#define LISTEN_BACKLOG 200

// the maximum number of concurrent hits that we handle
#define CONCURRENT_HITS 100

// Because this http daemon is designed to serve only GET data
// we maximize the size of a http request
#define HIT_MAX_SIZE 32768
// the maximum number of header lines that we can handle
#define MAX_HEADER_LINES 100
// the maximum length of the query string
#define MAX_QUERY_LENGTH 256

// the call-back function table for the HttpProc threads
struct _fun {
  char *(*pGetServerVariable)();
  int (*pWriteClient)();
  int (*pReadClient)();
  int (*pWriteClientText)();
  int (*pState)();
  int (*pContentType)();
  int (*pHeader)();
  int (*pStartBody)();
  char *(*pGetParam)();
  char *(*pPostParam)();
  char *(*pScriptName)();
  void (*pCloseClient)();
  void (*HttpProc)();
  int  (*FtpProc)();
  };

//Defines that help the programmer writing the HttpProc
// make less number of errors
#define GetServerVariable(X) (pT->pFunctions->pGetServerVariable(pT,(X)))
#define WriteClient(X,Y)     (pT->pFunctions->pWriteClient(pT,(X),(Y)))
#define ReadClient(X,Y)      (pT->pFunctions->pReadClient(pT,(X),(Y)))
#define WriteClientText(X)   (pT->pFunctions->pWriteClientText(pT,(X)))
#define State(X)             (pT->pFunctions->pState(pT,(X)))
#define ContentType(X)       (pT->pFunctions->pContentType(pT,(X)))
#define Header(X,Y)          (pT->pFunctions->pHeader(pT,(X),(Y)))
#define StartBody()          (pT->pFunctions->pStartBody(pT))
#define GetParam(X)          (pT->pFunctions->pGetParam(pT,(X)))
#define PostParam(X)         (pT->pFunctions->pPostParam(pT,(X)))
#define ScriptName()         (pT->pFunctions->pScriptName(pT))
#define CloseClient()        (pT->pFunctions->pCloseClient(pT))

// data that we maintain about each HttpProc thread
typedef struct _ThreadData {
  int                ThreadIndex;      // the index of this element
  int                SocketOpened;     // true when the socket is opened
  int                server_index;     // the index of the server this thread belongs to
  SOCKET             msgsock;          // the messaging socket
  int                NextFree;         // the next free thread data
  void              *pThreadLocalData; // pointer to local data that the thread may use
  THREADHANDLE       T;
  struct sockaddr_in addr;

// In these variables we store the header lines so that the application can access it
  int                  iHeaderLines;            // the number of header lines
  char                *HeaderKey[MAX_HEADER_LINES],
                      *HeaderString[MAX_HEADER_LINES];

// the string of the method
  char                *pszMethod;             // REQUEST_METHOD
// the unaltered query string the way the client has sent to us
  char                *pszQueryString;        // QUERY_STRING

  unsigned char        ClientIP[4];

  unsigned int         cbAvailable;   // Available number of bytes
  unsigned char       *pszData;      // Pointer to cbAvailable bytes
  struct _fun         *pFunctions;          // Call Back Function Control Block
  char                 getparams[MAX_QUERY_LENGTH];
  int                  getparlen;
  char                 script[MAX_QUERY_LENGTH];
  int                  scriptlen;
  char                 buffer[HIT_MAX_SIZE];
  struct _HttpdThread *pHT;  
  // the application thread data pointer
  void                *AppThreadData;
  } ThreadData, *pThreadData;

  // constants to define the individual services
#define SERVER_NONE      0
#define SERVER_HTTP      1
#define SERVER_FTP       2
typedef struct _ServerData {
  port_t        port;
  unsigned long ip;
  int           type;
  char          *salute;
  char          *codebase; // directory where the programs are
  // This is a global variable so that the httpproc shutdown can close it.
  SOCKET         sock;// This is a global variable so that the httpproc shutdown can close it.
  unsigned long cAllowed;// the number of allowed IP masks
  unsigned long *plAllowedIP;
  unsigned long *plAllowedMask;

  unsigned long cDenied;
  unsigned long *plDeniedIP;
  unsigned long *plDeniedMask;

  // ftp parameters, BASIC programs file names to start when the command is issued by the client
  char          *USER;
  char          *PASS;
  char          *ACCT;
  char          *CWD;
  char          *CDUP;
  char          *SMNT;
  char          *REIN;
  char          *QUIT;
  char          *PORT;
  char          *PASV;
  char          *TYPE;
  char          *STRU;
  char          *MODE;
  char          *RETR;
  char          *STOR;
  char          *STOU;
  char          *APPE;
  char          *ALLO;
  char          *REST;
  char          *RNFR;
  char          *RNTO;
  char          *ABOR;
  char          *DELE;
  char          *MKD;
  char          *PWD;
  char          *LIST;
  char          *NLST;
  char          *SITE;
  char          *SYST;
  char          *STAT;
  char          *HELP;
  char          *NOOP;

  } ServerData, *pServerData;

typedef struct _HttpdThread {

  // This is an array of thread data. They are linked together so that we have a list of free threads.
  pThreadData    threads;

  // This variable indexes the first free thread or has the value -1 if there are no free threads
  int            FirstFreeThread;

  // Mutex to lock the variable FirstFreeThread
  MUTEX          mxFirstFreeThread;

  // The number of threads running
  int            cThread;

  // the number of seconds to wait for the threads to stop during shut down
  unsigned long  lWaitSec;

  // the number of times to wait lWaitSec seconds before forcefully exiting
  unsigned long  lWaitCount;

  // the number of ip:port that we can store
#define MAX_SERVERS 100
  ServerData    server[MAX_SERVERS];
  // the actual number of servers configured
  int            c_servers;

  // The maximum number of threads that we start.
  int            threadmax;

  // The listen backlog passed as parameter to listen.
  int            listenmax;

  int            iState; // State of the server.
#define STATE_NORMAL 0
#define STATE_SHUT   1 // The server is shutting down.

  MUTEX          mxState; // mutex to lock the state

  // the application  data pointer
  void          *AppData;
  } HttpdThread, *pHttpdThread;

*/

#ifndef __NO_SOCKETS__

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <winbase.h>
#define SLEEPER 1000
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#define Sleep sleep
#define SLEEPER 1
#endif

#include "thread.h"
#include "filesys.h"
#include "httpd.h"
#include "getopt.h"

#define MAXBINDTRIAL 1200 /* so many times do we try the bind*/
#define BINDSLEEP    1   /* so many seconds we sleep after an unsuccessful bind */

/*POD
=H httpd module

This module is used only by the standalone webserver variation of ScriptBasic.

The module contains a function R<httpd> that the main application should start. This
function calls the initialization function R<AppInit> and the application starting
functios R<AppStart>. After R<AppStart> returns it starts to listen on the configured
port and accepts http requests and passes the requests to R<HttpProc>.

CUT*/

/*POD
=section AppInit
=H AppInit
=bold
This function is not implemented in this module. This function is used by this module
and the program using this module should provide this function.
=nobold

This function is called by the function R<httpd> practically before anything
is done.

=verbatim
int AppInit(int argc,char *argv[],pHttpdThread pHT,void **AppData),
=noverbatim

The R<httpd> function passes the command line arguments back as it gets them plain. The
pointer T<pApp> points to an applicatrion specific T<void> pointer that is initialized to be
T<NULL> and is guaranteed not been changed. The pointer to the same T<void> pointer is passed
also to R<AppStart>. This pointer should be used to pass data between T<AppInit> and R<AppStart>.

The pointer T<pHT> points to a T<HttpThread> structure and the function T<AppInit> can change
the values of this structure.

The entry point of the function T<AppInit> should be given to the function R<httpd> as argument.

CUT*/

/*POD
=section AppStart
=H AppStart

=bold
This function is not implemented in this module. This function is used by this module
and the program using this module should provide this function.
=nobold

This function is called by R<httpd> after binding on the desired port, and after forking
to background on UNIX. This function should start all threads instead of R<AppInit>, otherwise the
forking looses all threads except the main thread. The first version of this
code started the logger threads before the fork and the parent exited with the
running logger threads whil the child daemon did not run the logger threads.

=verbatim
int AppStart(void **pApp);
=noverbatim
CUT*/

/*POD
=section HttpProc
=H HttpProc

=bold
This function is not implemented in this module. This function is used by this module
and the program using this module should provide this function.
=nobold

This function is called by R<httpd> for each hit in a separate thread.

=verbatim
void HttpProc(pHttpdThread pHT,pThreadData ThisThread);
=noverbatim

CUT*/
/*POD
=section FtpProc
=H FtpProc

=bold
This function is not implemented in this module. This function is used by this module
and the program using this module should provide this function.
=nobold

This function is called by R<httpd> for each ftp command.

=verbatim
void FtProc(pHttpdThread pHT,pThreadData ThisThread, char *pszCommand);
=noverbatim

CUT*/
#ifdef WIN32
/* This is a total DUMMY under Windows NT */
void InitSignalHandlers(){}
#else
void SEGV_handle(int i) {
    char fname[2000];
    int pid;
    pid=getpid();
    fclose(fopen(fname,"w+"));
    sleep(1000000);
}

/*POD
=section InitSignalHandlers
=H Initialize the signal handlers under UNIX

This function is called under UNIX to initialize the signal handlers.
This makes the application ignoring if the socket pipe is broken by the
client application.

Under Win32 there is a dummy function doing nothing in place of this function.

/*FUNCTION*/
void InitSignalHandlers(
  ){
/*noverbatim
CUT*/
  signal(SIGPIPE,SIG_IGN);
  signal(SIGSEGV,SEGV_handle); 
 
  }

#endif

static struct _StateCode {
  int Code;
  char *StateMessage;
  }StateMessages[]={
  200, "OK",
  201, "CREATED",
  202, "Accepted",
  203, "Partial Information",
  204, "No Response",
  301, "Found, but moved",
  302, "Found, but data resides under different URL (add a /)",
  303, "Method",
  304, "Not Modified",
  400, "Bad request",
  401, "Unauthorized",
  402, "PaymentRequired",
  403, "Forbidden",
  404, "Not found",
  500, "Internal Error",
  501, "Not implemented",
  502, "Service temporarily overloaded",
  503, "Gateway timeout ",
  600, "Bad request",
  601, "Not implemented",
  602, "Connection failed",
  603, "Timed out",
  000, NULL
  };

static int unhex(int t){
  if( t >= 'A' && t <= 'F' )return t-'A'+10;
  if( t >= 'a' && t <= 'f' )return t-'a'+10;
  return t-'0';
  }

static int strIcmp(char *a, char *b){
  int ca,cb;

  while( (ca=*a) && (cb=*b) ){           
    if( isupper(ca) )ca = tolower(ca);
    if( isupper(cb) )cb = tolower(cb);
    if( ca != cb )return ca-cb;
    a++,b++;
    }
  return 0;
  }

static char *_GetServerVariable(pThreadData ThisThread,
                            char *VariableName){
  int i;

  for( i = 0 ; i < ThisThread->iHeaderLines ; i++ ){
    if( ! strIcmp(ThisThread->HeaderKey[i],VariableName) )return ThisThread->HeaderString[i];
    }
  return NULL;
  }

static void _CloseClient(pThreadData ThisThread){
  if( ThisThread->SocketOpened ){
    closesocket(ThisThread->msgsock);
    ThisThread->SocketOpened = 0;
    }
  }

static int _WriteClient(pThreadData ThisThread,
                    char *pszBuffer,
                    int cbBuffer){
  int i;
  fd_set writefds;
  struct timeval timeout;

  FD_ZERO(&writefds);
  FD_SET(ThisThread->msgsock,&writefds);
  timeout.tv_sec=60;
  timeout.tv_usec=0;
  i = select(FD_SETSIZE,NULL,&writefds,NULL,&timeout);
  if( i == 0 )return 1;
  return cbBuffer != send(ThisThread->msgsock,pszBuffer,cbBuffer,0);
  }

static int _WriteClientText(pThreadData ThisThread,
                            char *pszBuffer){
  int i;
  fd_set writefds;
  struct timeval timeout;

  FD_ZERO(&writefds);
  FD_SET(ThisThread->msgsock,&writefds);
  timeout.tv_sec=60;
  timeout.tv_usec=0;
  i = select(FD_SETSIZE,NULL,&writefds,NULL,&timeout);
  if( i == 0 )return 1;

  if( pszBuffer == NULL )
    return 4 != send(ThisThread->msgsock,"null",4,0);

  i = strlen(pszBuffer);
  return i != send(ThisThread->msgsock,pszBuffer,i,0);
  }

static int _State(pThreadData ThisThread,
                   int StateCode){
  int i;
  char buffer[80];

  for( i = 0 ; StateMessages[i].StateMessage && 
               StateMessages[i].Code <= StateCode ; i++ ){
    if( StateMessages[i].Code == StateCode ){
      sprintf(buffer,"HTTP/1.0 %d %s\n",StateCode,StateMessages[i].StateMessage);
      return _WriteClientText(ThisThread,buffer);
      }
    }
  sprintf(buffer,"HTTP/1.0 %d\n",StateCode);
  return _WriteClientText(ThisThread,buffer);
  }

static int _ContentType(pThreadData ThisThread,
                        char *pszContentType){
  _WriteClient(ThisThread,"Content-Type: ",14);
  _WriteClientText(ThisThread,pszContentType);
  return _WriteClient(ThisThread,"\n",1);
  }

static int _Header(pThreadData ThisThread,
                   char *pszHeaderKey,
                   char *pszHeaderValue){
  _WriteClientText(ThisThread,pszHeaderKey);
  _WriteClient(ThisThread,": ",2);
  _WriteClientText(ThisThread,pszHeaderValue);
  return _WriteClient(ThisThread,"\n",1);
  }

static int _StartBody(pThreadData ThisThread){
  return _WriteClient(ThisThread,"\n",1);
  }

static int _ReadClient(pThreadData ThisThread,
                   char *pszBuffer,
                   int cbBuffer){
  int i;
  fd_set readfds;
  struct timeval timeout;

  FD_ZERO(&readfds);
  FD_SET(ThisThread->msgsock,&readfds);
  timeout.tv_sec=60;
  timeout.tv_usec=0;
  i = select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);
  if( i == 0 )return 0;

  return recv(ThisThread->msgsock,pszBuffer,cbBuffer,0);
  }

static char *_ScriptName(pThreadData ThisThread){
  int i,j;

  if( ! ThisThread->script[0] ){
    for( i = 0 ; ThisThread->pszQueryString[i] ; i++ )
      if( ThisThread->pszQueryString[i] == '?' )break;
    while( i && ThisThread->pszQueryString[i] != '/' )i--;
    if( ThisThread->pszQueryString[i] == '/' )i++;
    for( j = 0 ; ThisThread->pszQueryString[i] &&
                 ThisThread->pszQueryString[i] != '?' ; i++, j++ ){
      ThisThread->script[j] = ThisThread->pszQueryString[i];
      if( j >= MAX_QUERY_LENGTH ){
        ThisThread->script[0] = (char)0;
        return NULL;
        }
      }
    ThisThread->script[j] = (char)0;
    }
  return ThisThread->script;
  }

static char *_GetParam(pThreadData ThisThread,
                     char *key){
  char *s;
  int i,j;

  if( ! ThisThread->getparams[0] ){
    /* when first called it contains no data */
    s = ThisThread->pszQueryString;
    while( *s && *s != '?' )s++;
    if( ! *s )return NULL;
    s++; /* step over the ? */
    /* check the size before copiing */
    ThisThread->getparlen = strlen(s);
    if( ThisThread->getparlen >= MAX_QUERY_LENGTH )return NULL;
    strcpy(ThisThread->getparams,s);
    s = ThisThread->getparams;
    for( i=j=0 ; ; i++ ){
       s[i] = s[j];
       if( ! s[j] )break;
       if( s[j] == '%' && s[j+1] && s[j+2] ){
         s[i] = unhex(s[j+1])*16+unhex(s[j+2]);
         j += 3;
         }else j++;
       }
    ThisThread->getparlen = i;
    s = ThisThread->getparams;
    while( *s ){
      if( *s == '&' )*s = (char)0;
      s++;
      }
    }
  /* Now we have the get params in ThisThread->getparams */
  s = ThisThread->getparams;
  for( i = 0 ; i < ThisThread->getparlen ; i++ ){
    for( j = 0 ; key[j] && s[i] && s[i] != '=' ; j++, i++ ){
      if( s[i] != key[j] ){
        while( s[i] )i++;
        break;
        }
      }
    if( s[i] )return s+i+1;
    }
  return NULL;
  }

static char *_PostParam(pThreadData ThisThread,
                      char *key){
  return NULL;
  }

/*POD
=section httpd
=H httpd

This is the main entry point of the module. This function should be called by the
main function passing the command line arguments and the three callback functions:

=itemize
=item R<AppInit> is called to initialize the webserver. Should read configuration.
=item R<AppStart> should start the worker threads.
=item R<HttpProc> should handle the individual http requests.
=item R<FtpProc> should handle the individual ftp commands.
=noitemize

The function T<httpd> never returns.
/*FUNCTION*/
int httpd(int argc,
          char *argv[],
          int (*AppInit)(int argc,char *argv[],pHttpdThread pHT,void **AppData),
          int (*AppStart)(void **AppData),
          void (*HttpProc)(pHttpdThread pHT,pThreadData ThisThread),
          int (*FtpProc)(pHttpdThread pHT,pThreadData ThisThread, char *pszCommand)
){
/*noverbatim
CUT*/
  int addr_size;
  int i,j,so;
  int length;
  struct _fun Functions;
  struct sockaddr_in server;
  pThreadData ThisThread;
  HttpdThread HT;
  int iState;
  int cThread;
  unsigned long iL;
#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
#else
  int cpid;
#endif
  char *optarg,opt;
  int OptionIndex;
  fd_set acceptfds;
  struct timeval timeout;


#ifdef WIN32
  wVersionRequested = MAKEWORD( 2, 2 );
  err = WSAStartup( wVersionRequested, &wsaData );
  if( err != 0 ){
    fprintf(stderr,"Error initializing the Windows Socket subsystem\n");
    exit(1);
    }
#endif
  HT.server[0].port      = HTTPD_PORT;
  HT.server[0].ip        = HTTPD_IP;
  HT.c_servers           = 1;
  HT.threadmax           = CONCURRENT_HITS;
  HT.listenmax           = LISTEN_BACKLOG;
  HT.server[0].cAllowed  = 0;
  HT.server[0].cDenied   = 0;

  /* init the signal handlerts to SIG_IGN so that a brokern pipe does not kill us */
  InitSignalHandlers();

  /* This is just a pointer that we guarantee not to be altered. */
  HT.AppData = NULL;
  /* AppInit should return zero on success. */
  if( i=AppInit(argc,argv,&HT,&(HT.AppData)) ){
    fprintf(stderr,"AppInit returned %d\n",i);
    exit(i);
    }

  OptionIndex = 0;
  while( (opt = getoptt(argc, argv, "p:h:",&optarg,&OptionIndex)) != ':'){
    switch( opt ){
      case 'p' : /* you can specify a port number on the command line to bind on */
                 /* this overrides the configuration port */
         HT.server[0].port  = atoi(optarg);
         HT.c_servers = 1;
         break;
      case 'h' : /* you can specify a single host IP to bind on */
                 /* this overrides the configuration ip */
         HT.server[0].ip     = inet_addr(optarg);
         HT.c_servers  = 1;
         break;
      }
    }

  for( i=0 ; i < HT.c_servers ; i++ ){
    HT.server[i].sock = socket(AF_INET, SOCK_STREAM, 0);
    so=1;
    setsockopt(HT.server[i].sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&so),sizeof(i));
    if( HT.server[i].sock < 0 ){
      fprintf(stderr, "Error at socket");
      exit(1);
      }
    server.sin_family = AF_INET;   
    server.sin_addr.s_addr = HT.server[i].ip;
    server.sin_port = htons(HT.server[i].port);

    for( j=0 ; j<MAXBINDTRIAL ; j++ ){
      if( ! bind(HT.server[i].sock, (struct sockaddr *)&server, sizeof(server)) )break;
      if( j == MAXBINDTRIAL-1 ){
        fprintf(stderr, "\nError at bind.");
        exit(1);
        }
      if( j== 0 )fprintf(stderr,"Bind failed on %s:%d, retrying at most %d times\n.",
                            inet_ntoa(server.sin_addr), ntohs(server.sin_port),MAXBINDTRIAL);
      else fprintf(stderr,".");
      if( j%40 == 39 )fprintf(stderr,"\n");
      Sleep(BINDSLEEP);
      }
    if( j )fprintf(stderr,"\nBind finally successful after %d trials\n",j);
    
    length = sizeof(server);
    if( getsockname(HT.server[i].sock, (struct sockaddr *)&server, &length) ){ 
      fprintf(stderr, "Error at getsockname.");
      exit(1);
      }

    listen(HT.server[i].sock, HT.listenmax);
    }
  HT.iState = STATE_NORMAL;

  if( j=AppStart(&(HT.AppData)) ){
    fprintf(stderr,"Appstart returned %d\n",j);
    exit(j);
    }

  HT.threads = (pThreadData)malloc(HT.threadmax*sizeof(ThreadData));
  if( HT.threads == NULL ){
    fprintf(stderr,"Not enough memory\n");
    exit(1);
    }

  /* Initialize the list of thread data
  */
  for( i=0 ; i < HT.threadmax ; i++ ){
    HT.threads[i].ThreadIndex = i;
    HT.threads[i].pFunctions = &Functions;
    HT.threads[i].NextFree   = i+1;
    HT.threads[i].pHT        = &HT;
    }
  HT.cThread = 0;
  /* make it dead end */
  HT.threads[HT.threadmax-1].NextFree = -1;
  HT.FirstFreeThread = 0;
  thread_InitMutex(&(HT.mxFirstFreeThread));
  Functions.pGetServerVariable = _GetServerVariable;
  Functions.pWriteClient       = _WriteClient;
  Functions.pWriteClientText   = _WriteClientText;
  Functions.pReadClient        = _ReadClient;
  Functions.pState             = _State;
  Functions.pContentType       = _ContentType;
  Functions.pHeader            = _Header;
  Functions.pStartBody         = _StartBody;
  Functions.pGetParam          = _GetParam;
  Functions.pPostParam         = _PostParam;
  Functions.pCloseClient       = _CloseClient;
  Functions.pScriptName        = _ScriptName;
  Functions.HttpProc           = HttpProc;
  Functions.FtpProc            = FtpProc;
  while(1){
    ThisThread = GetFreeThread(&HT);

    FD_ZERO(&acceptfds);
    for( i=0 ; i < HT.c_servers ; i++ )
      FD_SET(HT.server[i].sock,&acceptfds);
    timeout.tv_sec=60;
    timeout.tv_usec=0;
    i = select(FD_SETSIZE,&acceptfds,NULL,NULL,&timeout);
    for( i=0 ; i < HT.c_servers ; i++ ){
      if( FD_ISSET(HT.server[i].sock,&acceptfds) ){
        do{
          addr_size=sizeof(struct sockaddr);
          ThisThread->msgsock = accept(HT.server[i].sock,
                                      (struct sockaddr *)&(ThisThread->addr),
                                      &addr_size);
          }while(
    #ifdef WIN32
                  ThisThread->msgsock == INVALID_SOCKET
    #else
                  ThisThread->msgsock <= 0
    #endif
                  );
        thread_LockMutex(&(HT.mxState));
        iState = HT.iState;
        thread_UnlockMutex(&(HT.mxState));
        if( iState == STATE_SHUT ){/* we have to stop */
          /* wait for all threads to stop */
          for( iL = 0 ; iL < HT.lWaitCount ; iL++ ){
            thread_LockMutex(&(HT.mxFirstFreeThread));
            cThread = HT.cThread;
            thread_UnlockMutex(&(HT.mxFirstFreeThread));
            if( cThread == 1 )break;
            Sleep(HT.lWaitSec*SLEEPER);
            }
          return 0;
          }
        ThisThread->SocketOpened = 1;
        ThisThread->server_index = i;
        thread_CreateThread(&(ThisThread->T),HitHandler,ThisThread);
        }
      }
    }
  /* we never get here */
  return 0;
  }

/*FUNCTION*/
pThreadData GetFreeThread(pHttpdThread pHT
  ){
  pThreadData t;
  /* get exclusive access to the first free thread index */
  thread_LockMutex(&(pHT->mxFirstFreeThread));
  /* while there is no free thread wait a second and try again */
  while( pHT->FirstFreeThread == -1 ){
    thread_UnlockMutex(&(pHT->mxFirstFreeThread));
    Sleep(1*SLEEPER);
    thread_LockMutex(&(pHT->mxFirstFreeThread));
    }
  t = pHT->threads+pHT->FirstFreeThread;
  pHT->FirstFreeThread = pHT->threads[pHT->FirstFreeThread].NextFree;
  pHT->cThread++;
  thread_UnlockMutex(&(pHT->mxFirstFreeThread));

  return t;
  }

/*FUNCTION*/
void ReleaseThreadData(pHttpdThread pHT,
                       int index
  ){
  thread_LockMutex(&(pHT->mxFirstFreeThread));
  pHT->threads[index].NextFree = pHT->FirstFreeThread;
  pHT->FirstFreeThread = index;
  pHT->cThread--;
  thread_UnlockMutex(&(pHT->mxFirstFreeThread));
  }

#define MSPACE(x) while( *x && isspace(*x)) x++
#define SSPACE(x) if( *x && isspace(*x)) x++

/*FUNCTION*/
int CheckAllowDeny(pThreadData ThisThread
  ){
  int fAllowed;
  pHttpdThread pHT;
  unsigned long h;
  unsigned long lClientIP;
  pServerData ThisServer;

  pHT = ThisThread->pHT;
  /*
   * check that the client is on the allowed list and is not on the denied list
   */
  fAllowed = 1;/* allowed if there is no allowed/denied client list */

  /* pointer to the server data that this thread belongs to */
  ThisServer = pHT->server + ThisThread->server_index;

  if( ThisServer->cAllowed || ThisServer->cDenied ){

    lClientIP = ThisThread->ClientIP[3] + 256*(
                ThisThread->ClientIP[2] + 256*(
                ThisThread->ClientIP[1] + 256*(
                ThisThread->ClientIP[0] )));
    if( ThisServer->cAllowed ){
      fAllowed = 0;/* allowed only if any of the allowed host list matches */
      for( h=0 ; h < ThisServer->cAllowed ; h++ ){
        if( (lClientIP&ThisServer->plAllowedMask[h]) == 
            (ThisServer->plAllowedIP[h]&ThisServer->plAllowedMask[h]) ){
          fAllowed = 1;
          break;
          }
        }
      }
    if( fAllowed && ThisServer->cDenied ){
      for( h=0 ; h < ThisServer->cDenied ; h++ ){
        if( (lClientIP&ThisServer->plDeniedMask[h]) == 
            (ThisServer->plDeniedIP[h]&ThisServer->plDeniedMask[h]) ){
          fAllowed = 0;
          break;
          }
        }
      }
    }
    return fAllowed;
  }
/*POD
=section FinishConnection
=H FinishConnection

This function closes the client connection and finishes the thread this is
running in. Thus this fucntion does not return. This function should be called
from the hit handling functions that are started in asynchronous thread when
the connection was handled or should for some reason abandoned.

The function T<FinishConnection> never returns.

/*FUNCTION*/
void FinishConnection(pThreadData ThisThread
  ){
/*noverbatim
CUT*/
  if( ThisThread->SocketOpened ){
    closesocket(ThisThread->msgsock);
    ThisThread->SocketOpened = 0;
    }
  ReleaseThreadData(ThisThread->pHT,ThisThread->ThreadIndex);
  thread_ExitThread();
  }


/*FUNCTION*/
void HitHandler(void *t
  ){
  pThreadData ThisThread;
  pServerData ThisServer;
  pHttpdThread pHT;

  ThisThread = (pThreadData)t;
  pHT = ThisThread->pHT;

  ThisThread->AppThreadData = NULL;

#ifdef WIN32
  ThisThread->ClientIP[0] = ThisThread->addr.sin_addr.S_un.S_un_b.s_b1;
  ThisThread->ClientIP[1] = ThisThread->addr.sin_addr.S_un.S_un_b.s_b2;
  ThisThread->ClientIP[2] = ThisThread->addr.sin_addr.S_un.S_un_b.s_b3;
  ThisThread->ClientIP[3] = ThisThread->addr.sin_addr.S_un.S_un_b.s_b4;
#else
  memcpy(ThisThread->ClientIP,&(ThisThread->addr.sin_addr.s_addr),4);
#endif

  if( ! CheckAllowDeny(ThisThread) )FinishConnection(ThisThread);

  /* pointer to the server data that this thread belongs to */
  ThisServer = pHT->server + ThisThread->server_index;
  switch( ThisServer->type ){
    case SERVER_HTTP:
      HandleHttpHit(ThisThread);
    case SERVER_FTP:
      HandleFtpHit(ThisThread);
    }
  FinishConnection(ThisThread);
  }

/*FUNCTION*/
void HandleFtpHit(pThreadData ThisThread
  ){
  pServerData ThisServer;
  fd_set readfds;
  struct timeval timeout;
  pHttpdThread pHT;
  char *Buffer;
  int cbBuffer,cbCharsRead,i;

  Buffer = ThisThread->buffer;
  cbBuffer = HIT_MAX_SIZE;

  /* pointer to the server data that this thread belongs to */
  pHT = ThisThread->pHT;
  ThisServer = pHT->server + ThisThread->server_index;

  send(ThisThread->msgsock,ThisServer->salute,strlen(ThisServer->salute),0);
  send(ThisThread->msgsock,"\r\n",2,0);

  while(1){
    FD_ZERO(&readfds);
    FD_SET(ThisThread->msgsock,&readfds);
    timeout.tv_sec=60;
    timeout.tv_usec=0;
    i = select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);
    if( i == 0 )FinishConnection(ThisThread);
    cbCharsRead = recv(ThisThread->msgsock,Buffer,cbBuffer,0);
    /* some clients send such packets, but I could not figure out how */
    if( cbCharsRead == 0 )FinishConnection(ThisThread);
    if( cbCharsRead < 0 )FinishConnection(ThisThread);
    Buffer[cbCharsRead] = (char)0;
    /* note that FinishConnection function not only closes the socket but also terminates the this thread thus
       there us no need to get out of the loop and return from this function */
    if( ThisThread->pFunctions->FtpProc(ThisThread->pHT,ThisThread,Buffer) )FinishConnection(ThisThread);
    }
  }

/*FUNCTION*/
void HandleHttpHit(pThreadData ThisThread
  ){
  pServerData ThisServer;
  int j,i,cbBuffer,cbCharsRead,cbCharsReadTotal;
  char *Buffer;
  char *s;
  void MyHttpExtensionProc(pThreadData);
  fd_set readfds;
  struct timeval timeout;
  pHttpdThread pHT;

  Buffer = ThisThread->buffer;
  cbBuffer = HIT_MAX_SIZE;

  /* pointer to the server data that this thread belongs to */
  pHT = ThisThread->pHT;
  ThisServer = pHT->server + ThisThread->server_index;
  
  cbCharsReadTotal = 0;
  while(1){
    FD_ZERO(&readfds);
    FD_SET(ThisThread->msgsock,&readfds);
    timeout.tv_sec=60;
    timeout.tv_usec=0;
    i = select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);
    if( i == 0 )FinishConnection(ThisThread);
    cbCharsRead = recv(ThisThread->msgsock,Buffer,cbBuffer,0);
    /* some clients send such packets, but I could not figure out how */
    if( cbCharsRead == 0 )FinishConnection(ThisThread);
    if( cbCharsRead < 0 )FinishConnection(ThisThread);

    /* If this is the first chunk read then we are searching for the header terminating
       empty line from the first character. If this is not the first chunk then we start the
       search from the end of the previous chunk. */
    if( cbCharsReadTotal < 3 )j = 4 - cbCharsReadTotal; else j = 1;

    cbCharsReadTotal += cbCharsRead;

    /* if we have found the empty line following the header */
    for( ; j <= cbCharsRead ; j++ ){
      if( Buffer[j-1] == '\n' && Buffer[j-2] == '\r' &&
          Buffer[j-3] == '\n' && Buffer[j-4] == '\r' ){
        Buffer[j-4] = (char)0;
        ThisThread->pszData = Buffer+j;
        ThisThread->cbAvailable = cbCharsRead - j;
        goto HEADER_IS_READ;
        }
      }
    cbBuffer -= i;
    Buffer += i;
    if( cbBuffer <= 0 )FinishConnection(ThisThread);
    }
HEADER_IS_READ:;

  /* Here we have the total HTTP GET header. */
  s = ThisThread->pszMethod = ThisThread->buffer;
  while( *s && ! isspace(*s) )s++;
  if( *s )
    *s++ = (char)0;
  else FinishConnection(ThisThread);

  ThisThread->pszQueryString = s;
  while( *s && ! isspace(*s) )s++;
  if( *s )
    *s++ = (char)0;
  else FinishConnection(ThisThread);

  /* if there is anything after the URL until the new line then skip it */
  while( *s && *s != '\n' )s++;
  /* skip the CR/LF */
  while( *s == '\n' || *s == '\r' )s++;

  ThisThread->iHeaderLines = 0;
  /* now s points to the start of the first header line */
  while( *s ){

    ThisThread->HeaderKey[ThisThread->iHeaderLines] = s;
    while( *s && ! isspace(*s) && *s != ':' )s++;
    if( *s )*s++ = (char)0;
    while( isspace(*s) )s++;
    if( *s ){
      ThisThread->HeaderString[ThisThread->iHeaderLines] = s;
      while( *s && *s != '\n' && *s != '\r' )s++;
      if( *s )*s++ = (char)0;
      while( *s == '\n' || *s == '\r' )s++;
      ThisThread->iHeaderLines++;
      }
    }

  ThisThread->getparams[0] = (char)0;
  ThisThread->script[0] = (char)0;
  ThisThread->pThreadLocalData = NULL;

  ThisThread->pFunctions->HttpProc(ThisThread->pHT,ThisThread);

  FinishConnection(ThisThread);
  }
#endif /* __NO_SOCKETS__ */
