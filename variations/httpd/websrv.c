#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <process.h>
#include <winsock2.h>
#include <winbase.h>
#include <tlhelp32.h>

#include "service.h"

VOID CmdInstallService();
VOID CmdRemoveService();
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);

#else
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include "../../thread.h"
#include "../../filesys.h"
#include "../../httpd.h"
#include "../../logger.h"
#include "../../scriba.h"

#if WIN32
/* This function decides if the actual process was started as a service. This is a not so sure, but
   much faster way than calling the service dispatcher function */
static BOOL AmIService(void){
  DWORD dwMyPid,dwParentPid;
  PROCESSENTRY32 pe;
  HANDLE         th;

  /* get the current process pid */
  dwMyPid = GetCurrentProcessId();
  th = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if( INVALID_HANDLE_VALUE == th )return FALSE;
  /* Count the processes to allocate the appropriate size array */
  pe.dwSize = sizeof(PROCESSENTRY32);

  if( ! Process32First(th,&pe) )return FALSE;

  /* get the parent process pid */
  while( pe.th32ProcessID != dwMyPid ){
    pe.dwSize = sizeof(PROCESSENTRY32);
    if( ! Process32Next(th,&pe) )break;
    }
  if( pe.th32ProcessID == dwMyPid ){
    dwParentPid = pe.th32ParentProcessID;
    }else{
    return FALSE;
    }

  if( ! Process32First(th,&pe) ){// that could be weird not to have any process running
    return FALSE;
    }

  /* get the parent process parameters */
  while( pe.th32ProcessID != dwParentPid ){
    pe.dwSize = sizeof(PROCESSENTRY32);
    if( ! Process32Next(th,&pe) )break;
    }
  if( pe.th32ProcessID == dwParentPid ){
    // if the parent process exe is named SERVICES.EXE then this is a service
    if( 0 == strnicmp("SERVICES",pe.szExeFile,8) )return TRUE;
    return FALSE;
    }else{
    return FALSE;
    }
  }
#endif

#define FULL_PATH_BUFFER_LENGTH 256
#define INBLEN 10 /* Input Buffer Length */
/*

  Global data for all threads.

*/
typedef struct _WebData {
  pSbProgram pProgramConfig;
  char *pszHome;
  char szPort[10];
  int XForwarded; /* set this falg TRUE in config in case the engine is used behind an Apache
                     That means that the client IP is NOT the client IP (aka the server where
                     Apache is running) but the client Apache passes in the X-Forwarded-For header.

                     The program reading the configuration file will set this variable according
                     to the configuration key 'servers.proxyip'
                     */
   /*
   These are asynchronous and synchronous log handlers. The log handling is implemented in the
   file logger.c

   HitLog logs all hits
   StatLog logs all application performance statistical data (NOT ad statistics)
   AppLog logs all important application actions
   ErrLog logs errors
   PanicLog is an asynchronous error log and is used to log serious errors when the error
            prohibits the usage of the ErrLog

   also see panic.txt in the cwd of the process as a last resort of reporting error
   (note that cwd for WNT services is supposed to be WNT\System32)
   */
  tLogger HitLog,StatLog,AppLog,ErrLog,PanicLog;
  /*
   This program does not start if any of the log files can not be opened properly or
   not configured properly. This is to ensure that if the program starts all error
   conditions are logged fine. Some installation may want to run without any log.
   In that case the application SHOULD configure "http.nolog" to be true.
   */
  int nolog;
  /*
   This fields may point to the message that the server sends on a page not found error.
  */
  char *msg404;
  /*
   This is the error code and error text that the server sends when a page is not found.
   This is the text that is after the HTTP/1.0 and before the new line.
   */
  char *code404;
  /*
   This is the BASIC program to execute when a page is not found. If this is not NULL the
   code404 and msg404 fields are ignored.
   */
  char *run404;

  /* Number of virtual directory definitions */
  unsigned long cVdir;

  /* Virtual directory defintions. */
  char **ppszVdir;

  /* the pid file */
  char *pszPid;

  /* number of seconds to sleep between examining the pid file if that still exists */
  unsigned long lWaitLoop;

  /* where to send error messages bit0 client bit1 errlog */
#define ERRMSGCLIENT 0x01
#define ERRMSGERRLOG 0x02
  unsigned long bErrMsgDest;

  pHttpdThread pHT;
  } WebData, *pWebData;

/*
   Separate data for the individual threads.
*/
typedef struct _ApplicationThreadData {
  char *pszPathTranslated;
  char *pszClientIP;
  char *pszRemoteUser;
  char *pszPassword;
#define OBLEN 20
#define ENVLEN 256 /* the maximal length of an environment variable together with the key and the = sign*/
  unsigned char szBuffer[OBLEN];
  unsigned char szEnvBuf[ENVLEN];
  unsigned char cbBuffer;
  unsigned char szInBuffer[INBLEN];
  unsigned long cbInBuffer;
  unsigned char *pszInBuffer;
  char *pszFtpCommand; /* the actual command including the line terminating \r\n */
  char szThreadIndex[20]; /* the thread index decimal */
  int FirstHeaderLine;
  }ApplicationThreadData, *pApplicationThreadData;

#ifdef WIN32
static pHttpdThread pHT_Service;
#endif

static char six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};
static unsigned char pr2six[256];
static int uudecode(char *bufcoded,
                    unsigned char *bufplain,
                    int outbufsize){
/* single character decode */
#define DEC(c) pr2six[(int)c]
#define MAXVAL 63

  static int first = 1;
  char a[4];
  int j;
  char *bufin = bufcoded;
  unsigned char *bufout = bufplain;

  /* If this is the first call, initialize the mapping table.
   * This code should work even on non-ASCII machines.   */
  if(first) {
    first = 0;
    for(j=0; j<256; j++) pr2six[j] = MAXVAL+1;
    for(j=0; j<64; j++) pr2six[(int)six2pr[j]] = (unsigned char) j;
    }

  bufin = bufcoded;
   
  while( *bufin && outbufsize ){
    for( j = 0 ; j < 4 ; j++ )
      if( *bufin && pr2six[*bufin] <= MAXVAL )a[j] = *bufin++; else a[j] = 'A';
    *(bufout++) = (unsigned char) (DEC(*a) << 2 | DEC(a[1]) >> 4);
    outbufsize--;
    if( outbufsize < 2 ) break;
    *(bufout++) = (unsigned char) (DEC(a[1]) << 4 | DEC(a[2]) >> 2);
    outbufsize--;
    if( outbufsize < 2) break;
    *(bufout++) = (unsigned char) (DEC(a[2]) << 6 | DEC(a[3]));
    outbufsize--;
    if( outbufsize < 2) break;
    }

  *bufout = (char)0;
   
  return 0;
}

/* decide if p is prefix string of t
   if not return NULL
   if yes return the replace string

   zchar and : terminates the prefix string

   This function is used to search the virtual directories.
*/
static char *prefix(char *t, char *p){

  if( t == NULL )return NULL;
  if( p == NULL )return NULL;
  while( *t && *p && *p != ':' ){
    if( *p != *t )return NULL;
    p++;
    t++;
    }
  if( ! *p )return NULL; 
  if( *p == ':' )return ++p;
  return NULL;
  }

/*
 *  Convert a xxx.xxx.xxx.xxx/yyy.yyy.yyy.yyy IP/MASK string into two unsigned
 *  long values. Return zero on success and 1 on failure.
 */
static int maskip(char *s, unsigned long *ip, unsigned long *mask){
  int i;
  int digit;

  *ip = 0;
  for( i = 0 ; i < 4 ; i++ ){
    digit = 0;
    while( isdigit( *s ) ){
      digit = 10*digit + *s++ -'0';
      }
    if( digit > 255 || (*s != '.' && *s != '/' ) )return 1;
    if( *s == '.' )s++;
    *ip = 256* *ip + digit;
    }

  if( *s != '/' )return 1;
  s++;
  *mask = 0;
  for( i = 0 ; i < 4 ; i++ ){
    digit = 0;
    while( isdigit( *s ) ){
      digit = 10*digit + *s++ -'0';
      }
    if( digit > 255 || (*s != '.' && *s != (char)0 ) )return 1;
    if( *s == '.' )s++;
    *mask = 256* *mask + digit;
    }
  return 0;
  }


static void httpd_report(void *pEmbedPointer,
                         char *FileName,
                         long LineNumber,
                         unsigned int iErrorCode,
                         int iErrorSeverity,
                         int *piErrorCounter,
                         char *szErrorString,
                         unsigned long *fFlags
  ){
  pThreadData pT = pEmbedPointer;
  pHttpdThread pHT = pT->pThreadLocalData;
  pWebData pWD = pHT->AppData;
  char buf[256],*s;

  if( iErrorSeverity >= REPORT_ERROR && piErrorCounter )(*piErrorCounter)++;

  if( pWD->bErrMsgDest & ERRMSGCLIENT ){
    if( !((*fFlags) & REPORT_F_FRST) ){
      WriteClientText("HTTP/1.0 200 OK\nContent-Type: text/html\n\n"
                                  "<HTML><HEAD>\n"
                                  "<title>Error page, syntax error</title>\n"
                                  "</HEAD><BODY>\n"
                                  "<H1>Error has happened in the code</H1>"
                                  "<pre>\n");
      }
    if( szErrorString && strlen(szErrorString) > 80 )szErrorString[79] = (char)0;
    if( FileName ){
      sprintf(buf,"%s(%ld):",FileName,LineNumber);
      WriteClientText(buf);
      }
    sprintf(buf,(iErrorCode < MAX_ERROR_CODE ? " error &H%x:" : " error 0x%08x:"),iErrorCode);
    WriteClientText(buf);
    if( iErrorCode >= MAX_ERROR_CODE )iErrorCode = COMMAND_ERROR_EXTENSION_SPECIFIC;
    if( szErrorString ){
      sprintf(buf,en_error_messages[iErrorCode],szErrorString);
      WriteClientText(buf);
      WriteClientText("\n");
      }else{
      sprintf(buf,"%s\n",en_error_messages[iErrorCode]);
      WriteClientText(buf);
      }
    }
  if( pWD->bErrMsgDest & ERRMSGERRLOG ){
    if( szErrorString && strlen(szErrorString) > 80 )szErrorString[79] = (char)0;
    s = buf;
    if( FileName ){
      sprintf(s,"%s(%ld):",FileName,LineNumber);
      s += strlen(s);
      }
    sprintf(s,(iErrorCode < MAX_ERROR_CODE ? " error &H%x:" : " error 0x%08x:"),iErrorCode);
    s += strlen(s);
    if( szErrorString ){
      sprintf(s,en_error_messages[iErrorCode],szErrorString);
      }else{
      sprintf(s,"%s",en_error_messages[iErrorCode]);
      }
    s = buf;
    while( *s ){
      if( *s == '\n' )*s = ' ';
      s++;
      }
    log_printf(&(pWD->ErrLog),"%s",buf);
    }

  *fFlags |= REPORT_F_FRST;
  }

static void ftp_report(void *pEmbedPointer,
                       char *FileName,
                       long LineNumber,
                       unsigned int iErrorCode,
                       int iErrorSeverity,
                       int *piErrorCounter,
                       char *szErrorString,
                       unsigned long *fFlags
  ){
  pThreadData pT = pEmbedPointer;
  pHttpdThread pHT = pT->pThreadLocalData;
  pWebData pWD = pHT->AppData;
  char buf[256],*s;

  if( iErrorSeverity >= REPORT_ERROR && piErrorCounter )(*piErrorCounter)++;

  if( pWD->bErrMsgDest & ERRMSGCLIENT ){
    if( !((*fFlags) & REPORT_F_FRST) ){
      WriteClientText("421 Service not available, closing control connection.\r\n");
      }
    }

  if( pWD->bErrMsgDest & ERRMSGERRLOG ){
    if( szErrorString && strlen(szErrorString) > 80 )szErrorString[79] = (char)0;
    s = buf;
    if( FileName ){
      sprintf(s,"%s(%ld):",FileName,LineNumber);
      s += strlen(s);
      }
    sprintf(s,(iErrorCode < MAX_ERROR_CODE ? " error &H%x:" : " error 0x%08x:"),iErrorCode);
    s += strlen(s);
    if( szErrorString ){
      sprintf(s,en_error_messages[iErrorCode],szErrorString);
      }else{
      sprintf(s,"%s",en_error_messages[iErrorCode]);
      }
    s = buf;
    while( *s ){
      if( *s == '\n' )*s = ' ';
      s++;
      }
    log_printf(&(pWD->ErrLog),"%s",buf);
    }

  *fFlags |= REPORT_F_FRST;
  }

#define CONFIG(X) cft_GetString(pWD->pProgramConfig->pCONF,(X))

int AppInit(int argc,char *argv[],pHttpdThread pHT,void **AppData){
  pWebData pWD;
  long myV,i;
  CFT_NODE ClientRootNode,ServerRootNode,ServerStartNode,Node;
  char *s;
  void *pM_Log;
  FILE *fpanic;
  int iError;

  iError = 0;/* No error happened so far. When an error happens this variable is set to 1.
                When the function returns the value of this variable is returned indicating
                error or success. When an error occures we work on to recognize further possible
                errors in the log file and report it into the panic log.                          */

  pWD = malloc(sizeof(WebData));
  if( pWD == NULL )return 1;

  pWD->pHT = pHT;

  *AppData = pWD;

  /* default server values */
  pWD->XForwarded = 0;
  pWD->nolog = 0;

  /* create the pseudo program that will hold the configuration information */
  pWD->pProgramConfig = scriba_new(malloc,free);
  if( pWD->pProgramConfig == NULL ) return 2;

  if( scriba_LoadConfiguration(pWD->pProgramConfig,NULL) ){
     fprintf(stderr,"Can not initialize the configuration management system.\n");
     fprintf(stderr,"The expected configuration file \"%s\" can not be read.\n",pWD->pProgramConfig->pCONF->pszConfigFileName);
     fpanic = fopen("panic.txt","w");
     if( fpanic ){
       fprintf(fpanic,"Can not initialize the configuration management system.\n");
       fprintf(fpanic,"The expected configuration file \"%s\" can not be read.\n",pWD->pProgramConfig->pCONF->pszConfigFileName);
       fclose(fpanic);
       }
     return 1;
     }

  scriba_InitModuleInterface(pWD->pProgramConfig);

  /* the first server is configured by default. The rest has to be configured. */
  for( i= 1 ; i < MAX_SERVERS ; i++ ){
    pHT->server[i].ip = HTTPD_IP;   /* IP is configured by default */
    pHT->server[i].port = 0;        /* no port is configured by default */
    pHT->server[i].type = SERVER_NONE;
    }
  /* you have to explicitely configure the server not to log. Otherwise the server will not
     start if it can not open the vital log files for security reasons. */
  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.nolog",NULL,NULL,&myV,NULL,NULL) )
    pWD->nolog = myV;

  pWD->bErrMsgDest = 2;
  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.errmsgdest",NULL,NULL,&myV,NULL,NULL) )
    pWD->bErrMsgDest = myV;

  if( pWD->nolog ){
    log_init(&(pWD->PanicLog),NULL,NULL,NULL,NULL,LOGTYPE_SYNCHRONOUS);
    log_init(&(pWD->AppLog),NULL,NULL,NULL,NULL,LOGTYPE_SYNCHRONOUS);
    }else{
    /* if we have config then open the panic log as soon as possible so we can report further
       configuration errors to the panic log. The panic log is a synchronous log, therefore we can
       open it in AppInit before the main thread performs fork under UNIX. Asynchronous logs can
       only be started in AppStart that is called after forking into background. */
    pM_Log = alloc_InitSegment(malloc,free);
    if( pM_Log == NULL ){
      fprintf(stderr,"Low memory allocating segment for panic logging.\n");
      fpanic = fopen("panic.txt","w");
       if( fpanic ){
         fprintf(fpanic,"Low memory allocating segment for panic logging.\n");
         fclose(fpanic);
         }
      return 1;
      }

    if( (s = CONFIG("servers.log.panic.file")) == NULL ||
        log_init(&(pWD->PanicLog),
                 alloc_Alloc,
                 alloc_Free,
                 pM_Log,
                 s,
                 LOGTYPE_SYNCHRONOUS) ){
      fprintf(stderr,"Can not open panic log \"%s\".\n",s);
      fpanic = fopen("panic.txt","w");
      if( fpanic ){
        fprintf(fpanic,"Can not open panic log \"%s\".\n",s);
        fclose(fpanic);
        }
      return 1;
      }

    /* open the application log in synchronous mode and close it afterwards to
       reopen it in asynchronous mode */
    if( (s = CONFIG("servers.log.app.file")) == NULL ||
        log_init(&(pWD->AppLog),
                 alloc_Alloc,
                 alloc_Free,
                 pM_Log,
                 s,
                 LOGTYPE_SYNCHRONOUS) ){
      log_printf(&(pWD->PanicLog),
                 "Can not open the application log >>%s<<",
                 CONFIG("servers.log.app.file"));
      iError = 1;
      }

    log_printf(&(pWD->AppLog),"Eszter SB Application Engine start up initiated.");
    }

  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.port",NULL,NULL,&myV,NULL,NULL) )
    pHT->server[0].port = (port_t)myV;

  /* get the ip address that we have to bind the IP address on */
  if( s = CONFIG("servers.ip") )
    pHT->server[0].ip = inet_addr(s);

  sprintf(pWD->szPort,"%d",pHT->server[0].port);
  log_printf(&(pWD->AppLog),"Listening on port %d",pHT->server[0].port);

  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.threads",NULL,NULL,&myV,NULL,NULL) )
    pHT->threadmax = myV;
  log_printf(&(pWD->AppLog),"Maximum number of serving threads: %d",pHT->threadmax);

  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.proxyip",NULL,NULL,&myV,NULL,NULL) )
    pWD->XForwarded = myV;
  log_printf(&(pWD->AppLog), pWD->XForwarded 
                                  ? "Using Apache X-Forwarded-For header to determine client IP"
                                  : "Using TCP/IP reported IP to determine client IP" );

  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.listenbacklog",NULL,NULL,&myV,NULL,NULL) )
    pHT->listenmax = myV;
  log_printf(&(pWD->AppLog),"Listen backlog is %d",pHT->listenmax);

  pWD->pszHome = CONFIG("servers.home");
  if( pWD->pszHome == NULL )pWD->pszHome = "./";
  log_printf(&(pWD->AppLog),"http home directory is %s",pWD->pszHome);

  pWD->code404 = CONFIG("servers.code404");
  if( pWD->code404 )
    log_printf(&(pWD->AppLog),"Page not found error code is %s",pWD->code404);

  pWD->msg404 = CONFIG("servers.msg404");
  log_printf(&(pWD->AppLog), pWD->msg404 
                            ? "404 error page text is defined." 
                            : "404 error page text is default.");

  pWD->run404 = CONFIG("servers.run404");
  if( pWD->run404 && strlen(pWD->run404) > FULL_PATH_BUFFER_LENGTH ){
    log_printf(&(pWD->PanicLog),"run404 file name is too long, ignored.");
    pWD->run404 = NULL;
    }
  log_printf(&(pWD->AppLog), pWD->run404 
                            ? "404 error program is %s" 
                            : "no 404 error program defined",pWD->run404);

  pHT->lWaitSec = 10;
  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.pid.wait.period",NULL,NULL,&myV,NULL,NULL) )
    pHT->lWaitSec = myV;

  pHT->lWaitCount = 2;
  if( ! cft_GetEx(pWD->pProgramConfig->pCONF,"servers.pid.wait.length",NULL,NULL,&myV,NULL,NULL) )
    pHT->lWaitCount = myV;

  /* count the servers configured */
  ServerRootNode = cft_FindNode(pWD->pProgramConfig->pCONF,CFT_ROOT_NODE,"servers");
  ServerRootNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ServerRootNode);
  pHT->c_servers = 0;
  for( ;  ServerRootNode ; ServerRootNode = cft_EnumNext(pWD->pProgramConfig->pCONF,ServerRootNode) ){
    s = cft_GetKey(pWD->pProgramConfig->pCONF,ServerRootNode);
    if( ! strcmp(s,"server") )
      pHT->c_servers++;
    }
  if( pHT->c_servers >= MAX_SERVERS ){
    log_printf(&(pWD->ErrLog),"There are %d servers configured. Max %d are allowed.",pHT->c_servers,MAX_SERVERS);
    return 1;
    }
  log_printf(&(pWD->AppLog),"There are %d server(s) configured",pHT->c_servers);
  ServerRootNode = cft_FindNode(pWD->pProgramConfig->pCONF,CFT_ROOT_NODE,"servers");
  ServerRootNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ServerRootNode);
  i = 0;
  for(  ;  ServerRootNode ; ServerRootNode = cft_EnumNext(pWD->pProgramConfig->pCONF,ServerRootNode) ){
    s = cft_GetKey(pWD->pProgramConfig->pCONF,ServerRootNode);
    if( ! strcmp(s,"server") ){

      ServerStartNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ServerRootNode);

      Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"port");
      if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,NULL,&myV,NULL,NULL) )
        pHT->server[i].port = (port_t)myV;

      Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"ip");
      if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) )
        pHT->server[i].ip = inet_addr(s);

      Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"protocol");
      if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) ){
        if( ! stricmp(s,"http") )
          pHT->server[i].type = SERVER_HTTP;
        if( ! stricmp(s,"ftp") )
          pHT->server[i].type = SERVER_FTP;
        }else
          pHT->server[i].type = SERVER_HTTP;

      Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"salute");
      if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) )
        pHT->server[i].salute = s;
      else
        pHT->server[i].salute = "220 Eszter SB application Engine";

      Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"codebase");
      if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) )
        pHT->server[i].codebase = s;
      else
        pHT->server[i].codebase = NULL;

#define GET_FTP_PARAMETER(X)  Node = cft_FindNode(pWD->pProgramConfig->pCONF,ServerStartNode,"programs." #X);\
                              if( ! cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) ){\
                                pHT->server[i].X = s;\
                                }else pHT->server[i].X = NULL;
      if( SERVER_FTP == pHT->server[i].type ){
        GET_FTP_PARAMETER(USER)
        GET_FTP_PARAMETER(PASS)
        GET_FTP_PARAMETER(ACCT)
        GET_FTP_PARAMETER(CWD)
        GET_FTP_PARAMETER(CDUP)
        GET_FTP_PARAMETER(SMNT)
        GET_FTP_PARAMETER(REIN)
        GET_FTP_PARAMETER(QUIT)
        GET_FTP_PARAMETER(PORT)
        GET_FTP_PARAMETER(PASV)
        GET_FTP_PARAMETER(TYPE)
        GET_FTP_PARAMETER(STRU)
        GET_FTP_PARAMETER(MODE)
        GET_FTP_PARAMETER(RETR)
        GET_FTP_PARAMETER(STOR)
        GET_FTP_PARAMETER(STOU)
        GET_FTP_PARAMETER(APPE)
        GET_FTP_PARAMETER(ALLO)
        GET_FTP_PARAMETER(REST)
        GET_FTP_PARAMETER(RNFR)
        GET_FTP_PARAMETER(RNTO)
        GET_FTP_PARAMETER(ABOR)
        GET_FTP_PARAMETER(DELE)
        GET_FTP_PARAMETER(MKD)
        GET_FTP_PARAMETER(PWD)
        GET_FTP_PARAMETER(LIST)
        GET_FTP_PARAMETER(NLST)
        GET_FTP_PARAMETER(SITE)
        GET_FTP_PARAMETER(SYST)
        GET_FTP_PARAMETER(STAT)
        GET_FTP_PARAMETER(HELP)
        GET_FTP_PARAMETER(NOOP)

        }

      ClientRootNode = cft_FindNode(pWD->pProgramConfig->pCONF,cft_EnumFirst(pWD->pProgramConfig->pCONF,ServerRootNode),"client");
      ClientRootNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ClientRootNode);
      pHT->server[i].cAllowed = 0;
      pHT->server[i].cDenied  = 0;
      for( ; ClientRootNode ; ClientRootNode = cft_EnumNext(pWD->pProgramConfig->pCONF,ClientRootNode) ){
        s = cft_GetKey(pWD->pProgramConfig->pCONF,ClientRootNode);
        if( ! strcmp(s,"allowed") )
          pHT->server[i].cAllowed++;
        else
        if( ! strcmp(s,"denied") )
          pHT->server[i].cDenied++;
        }

      log_printf(&(pWD->AppLog),
                 "Allowed %d Denied %d IP/MASK pairs configured for server %d",
                 pHT->server[i].cAllowed,pHT->server[i].cDenied,i+1);

      if( pHT->server[i].cAllowed ){
        pHT->server[i].plAllowedIP = (unsigned long *)alloc_Alloc(pHT->server[i].cAllowed *  sizeof(unsigned long),pWD->pProgramConfig->pMEM);
        if( pHT->server[i].plAllowedIP == NULL ){
          log_printf(&(pWD->PanicLog),"Memory allocation error 0001");
          return 1;
          }
        pHT->server[i].plAllowedMask = (unsigned long *)alloc_Alloc(pHT->server[i].cAllowed *  sizeof(unsigned long),pWD->pProgramConfig->pMEM);
        if( pHT->server[i].plAllowedMask == NULL ){
          log_printf(&(pWD->PanicLog),"Memory allocation error 0002");
          return 1;
          }
        }

      if( pHT->server[i].cDenied ){
        pHT->server[i].plDeniedIP = (unsigned long *)alloc_Alloc(pHT->server[i].cDenied *  sizeof(unsigned long),pWD->pProgramConfig->pMEM);
        if( pHT->server[i].plDeniedIP == NULL ){
          log_printf(&(pWD->PanicLog),"Memory allocation error 0003");
          return 1;
          }
        pHT->server[i].plDeniedMask = (unsigned long *)alloc_Alloc(pHT->server[i].cDenied *  sizeof(unsigned long),pWD->pProgramConfig->pMEM);
        if( pHT->server[i].plDeniedMask == NULL ){
          log_printf(&(pWD->PanicLog),"Memory allocation error 0004");
          return 1;
          }
        }
      pHT->server[i].cAllowed = 0;
      pHT->server[i].cDenied  = 0;
      ClientRootNode = cft_FindNode(pWD->pProgramConfig->pCONF,cft_EnumFirst(pWD->pProgramConfig->pCONF,ServerRootNode),"client");
      ClientRootNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ClientRootNode);
      for( ; ClientRootNode ; ClientRootNode = cft_EnumNext(pWD->pProgramConfig->pCONF,ClientRootNode) ){
        if( ! strcmp(cft_GetKey(pWD->pProgramConfig->pCONF,ClientRootNode),"allowed") ){
          if( cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&ClientRootNode,&s,NULL,NULL,NULL) ){
            log_printf(&(pWD->PanicLog),
                      "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
            fprintf(stderr,
                    "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
            return 1;/* this is serious return immediately */
            }
          if( maskip(s,pHT->server[i].plAllowedIP+pHT->server[i].cAllowed,pHT->server[i].plAllowedMask+pHT->server[i].cAllowed) ){
            log_printf(&(pWD->PanicLog),"Invalid allowed ip/mask syntax: %s",s);
            iError = 1;
            }
          pHT->server[i].cAllowed++;
          }else
        if( ! strcmp(cft_GetKey(pWD->pProgramConfig->pCONF,ClientRootNode),"denied") ){
          if( cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&ClientRootNode,&s,NULL,NULL,NULL) ){
            log_printf(&(pWD->PanicLog),
                      "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
            fprintf(stderr,
                    "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
            return 1;/* this is serious return immediately */
            }
          if( maskip(s,pHT->server[i].plDeniedIP+pHT->server[i].cDenied,pHT->server[i].plDeniedMask+pHT->server[i].cDenied) ){
            log_printf(&(pWD->PanicLog),"Invalid denied ip/mask syntax: %s",s);
            iError = 1;
            }
          pHT->server[i].cDenied++;
          }
        }
      i++;
      }
    }

  /* Count the virtual directories. */
  ClientRootNode = cft_FindNode(pWD->pProgramConfig->pCONF,CFT_ROOT_NODE,"servers.vdirs");
  ClientRootNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ClientRootNode);
  pWD->cVdir = 0;
  for( Node = ClientRootNode ;  Node ; Node = cft_EnumNext(pWD->pProgramConfig->pCONF,Node) ){
    s = cft_GetKey(pWD->pProgramConfig->pCONF,Node);
    if( ! strcmp(s,"dir") )
      pWD->cVdir++;
    }
  if( pWD->cVdir ){
    pWD->ppszVdir = (char **)alloc_Alloc(pWD->cVdir*sizeof(char *),pWD->pProgramConfig->pMEM);
    if( pWD->ppszVdir == NULL ){
      log_printf(&(pWD->PanicLog),"Memory allocation error 0005");
      return 1;
      }
    }
  else
    pWD->ppszVdir = NULL;
  if( pWD->cVdir ){
    i = 0;
    for( Node = ClientRootNode ;  Node ; Node = cft_EnumNext(pWD->pProgramConfig->pCONF,Node) ){
      s = cft_GetKey(pWD->pProgramConfig->pCONF,Node);
      if( ! strcmp(s,"dir") ){
        if( cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL) ){
          log_printf(&(pWD->PanicLog),
                     "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
          fprintf(stderr,
                  "cgt_GetEx( ..., %s, ... ) retuned error second time. This is an internal error.");
          return 1;/* this is serious return immediately */
          }
        pWD->ppszVdir[i++] = s;
        }
      }
    }

  log_printf(&(pWD->AppLog),"AppInit returns %d",iError);
  /* App log was opened for syncronous logging. It will be reopened asynchronous after the possible fork */
  log_shutdown(&(pWD->AppLog));
  return iError;
  }

#ifdef WIN32
static void sleep(int x){ Sleep(1000*x); }
#endif
/*

This GuardThread is started from the function AppStart and periodically looks at
the PID file if that exists. When the system administrator deletes the PID file
this thread alters the state of the httpd daemon and the http daemon stops executing
more requests. However the requests currently serving are not affected.

*/
static void GuardThread(void *p){
  pHttpdThread pHT;
  pWebData pWD;  
  FILE *fp;
  SOCKET Client;
  char buf[80];
  int i;

  pHT = p;
  pWD = pHT->AppData;

  while(1){
    fp = fopen(pWD->pszPid,"r");
    if( fp == NULL ){
      thread_LockMutex(&(pHT->mxState));
      pHT->iState = STATE_SHUT;
      thread_UnlockMutex(&(pHT->mxState));
      /* we have to connect to the web server port to help it out of the
         waiting loop. For the purpose I use this quick&dirty solution.
         It works and because this is not in a loop it does not matter. */
     for( i=0 ; i < pHT->c_servers ; i++ ){
       struct in_addr sip;
       memcpy(&sip,&(pHT->server[i].ip),sizeof(struct in_addr));
       sprintf(buf,"%s:%d",inet_ntoa(sip),pHT->server[i].port);
       log_printf(&(pWD->AppLog),"Connecting to server %s",buf);
       file_tcpconnect(&Client,buf);
       }
      //sprintf(buf,"127.0.0.1:%s",pWD->szPort);
      //file_tcpconnect(&Client,buf);
      return; /* exit from thread */
      }
    fclose(fp);
    sleep(pWD->lWaitLoop);
    }
  }

struct _RunServiceProgram {
  pWebData pWD;
  char *pszProgramFileName;
  int iRestart;
  };

static void ExecuteProgramThread(void *p){
  pSbProgram pProgram;
  int binarycode;
  char szInputFile[FULL_PATH_BUFFER_LENGTH];
  int iErrorCode;
  struct _RunServiceProgram *pRSP;

  pRSP = p;
  while( 1 ){
    strcpy(szInputFile,pRSP->pszProgramFileName);
    pProgram = scriba_new(malloc,free);
    if( pProgram == NULL )return;

    scriba_SetProcessSbObject(pProgram,pRSP->pWD->pProgramConfig);
    scriba_SetEmbedPointer(pProgram,pRSP->pWD);
    scriba_SetFileName(pProgram,szInputFile);
    binarycode = 0;
    if( scriba_UseCacheFile(pProgram) == SCRIBA_ERROR_SUCCESS )binarycode = 1;
    if( binarycode || scriba_IsFileBinaryFormat(pProgram) ){
      scriba_LoadBinaryProgram(pProgram);
      }else{
      if( scriba_RunExternalPreprocessor(pProgram,NULL) ){
        log_printf(&(pRSP->pWD->ErrLog),"External preprocessor failed for %s",szInputFile);
        return;
        }
      if( scriba_LoadSourceProgram(pProgram) ){
        log_printf(&(pRSP->pWD->ErrLog),"Source program loading error %s",szInputFile);
        return;
        }
      scriba_SaveCacheFile(pProgram);
      }
    log_printf(&(pRSP->pWD->AppLog),"Executing %s",szInputFile);
    iErrorCode = scriba_Run(pProgram,NULL);
    if( iErrorCode )
      log_printf(&(pRSP->pWD->ErrLog),"Program %s terminated with error %d",szInputFile,iErrorCode);
    scriba_destroy(pProgram);
    if( ! pRSP->iRestart )return;
    }
  } 

static void ExecuteProgram(pWebData pWD,
                           char *pszProgramFileName,
                           int iRestart ){
  struct _RunServiceProgram *pRSP;
  THREADHANDLE T;

  pRSP = (struct _RunServiceProgram *)malloc( sizeof(struct _RunServiceProgram) );
  if( pRSP == NULL ){
    log_printf(&(pRSP->pWD->ErrLog),
       "No memory to allocate RSP structure when trying to run %s",pszProgramFileName);
    return;
    }
  pRSP->pszProgramFileName = (char *)malloc(strlen(pszProgramFileName) + 1);
  if( pRSP->pszProgramFileName == NULL ){
    log_printf(&(pRSP->pWD->ErrLog),
       "No memory to allocate pszProgramFileName when trying to run %s",pszProgramFileName);
    return;
    }
  strcpy(pRSP->pszProgramFileName,pszProgramFileName);
  pRSP->iRestart = iRestart;
  pRSP->pWD = pWD;
  thread_CreateThread(&T,ExecuteProgramThread,pRSP);
  }

int AppStart(void **AppData){
  void *pM_HitLog,*pM_StatLog,*pM_AppLog,*pM_ErrLog;
  pWebData pWD;
  CFT_NODE ConfNode,Node;
  int iError;
  FILE *fp;
  THREADHANDLE hThread;
  char *s;
  int iRestart;
#ifndef WIN32
  uid_t uid;
  struct passwd *pw;
  char *pszEffectiveUser;
#endif

  iError = 0; /* read comment on the variable of the same name in AppInit */
  pWD = *AppData;

  if( pWD->nolog ){
    log_init(&(pWD->HitLog),NULL,NULL,NULL,NULL,LOGTYPE_NORMAL);
    log_init(&(pWD->StatLog),NULL,NULL,NULL,NULL,LOGTYPE_NORMAL);
    log_init(&(pWD->AppLog),NULL,NULL,NULL,NULL,LOGTYPE_NORMAL);
    log_init(&(pWD->ErrLog),NULL,NULL,NULL,NULL,LOGTYPE_NORMAL);
    }else{
    /* allocate memory segments for each log. Memory allocation is serious enough not to go on. */
    pM_HitLog = alloc_InitSegment(malloc,free);
    if( pM_HitLog == NULL ){
      log_printf(&(pWD->PanicLog),"Memory error allocating pM_HitLog");
      return 1;
      }
    pM_StatLog = alloc_InitSegment(malloc,free);
    if( pM_StatLog == NULL ){
      log_printf(&(pWD->PanicLog),"Memory error allocating pM_StatLog");
      return 1;
      }
    pM_AppLog = alloc_InitSegment(malloc,free);
    if( pM_AppLog == NULL ){
      log_printf(&(pWD->PanicLog),"Memory error allocating pM_AppLog");
      return 1;
      }
    pM_ErrLog = alloc_InitSegment(malloc,free);
    if( pM_ErrLog == NULL ){
      log_printf(&(pWD->PanicLog),"Memory error allocating pM_ErrLog");
      return 1;
      }

#ifndef WIN32
  pszEffectiveUser = CONFIG("servers.user");
  if( pszEffectiveUser ){
    pw = getpwnam(pszEffectiveUser);
    if( pw ){
      if( setuid(pw->pw_uid) ){
        log_printf(&(pWD->PanicLog),"ESBAE configured to run as user (%s/%d). I can not set this uid for some reason.",
                  pszEffectiveUser,uid);
        iError = 1;
        }
      }else{
      log_printf(&(pWD->PanicLog),"ESBAE configured to run as user (%s). This user does not exists on this machine.",pszEffectiveUser);
      iError = 1;
      }
    }
#endif

    if( log_init(&(pWD->ErrLog),alloc_Alloc,alloc_Free,pM_ErrLog,CONFIG("servers.log.err.file"),LOGTYPE_NORMAL) ){
      log_printf(&(pWD->PanicLog),"Can not initialize error log.");
      return 1;
      }
    if( cft_GetEx(pWD->pProgramConfig->pCONF,"servers.log.err.span",&ConfNode,NULL,&(pWD->ErrLog.TimeSpan),NULL,NULL) )
    pWD->ErrLog.TimeSpan = 0;

    if( log_init(&(pWD->HitLog),alloc_Alloc,alloc_Free,pM_HitLog,CONFIG("servers.log.hit.file"),LOGTYPE_NORMAL) ){
      log_printf(&(pWD->PanicLog),"HitLog (%s) cannot be opened.",CONFIG("servers.log.hit.file"));
      return 1;
      }
    if( cft_GetEx(pWD->pProgramConfig->pCONF,"servers.log.hit.span",&ConfNode,NULL,&(pWD->HitLog.TimeSpan),NULL,NULL) )
      pWD->HitLog.TimeSpan = 0;
  
    if( log_init(&(pWD->StatLog),alloc_Alloc,alloc_Free,pM_StatLog,CONFIG("servers.log.stat.file"),LOGTYPE_NORMAL) ){
      log_printf(&(pWD->PanicLog),"StatLog (%s) cannot be opened.",CONFIG("servers.log.stat.file"));
      return 1;
      }
    if( cft_GetEx(pWD->pProgramConfig->pCONF,"servers.log.stat.span",&ConfNode,NULL,&(pWD->StatLog.TimeSpan),NULL,NULL) )
      pWD->StatLog.TimeSpan = 0;
  
    if( log_init(&(pWD->AppLog),alloc_Alloc,alloc_Free,pM_AppLog,CONFIG("servers.log.app.file"),LOGTYPE_NORMAL) ){
      log_printf(&(pWD->PanicLog),"AppLog (%s) cannot be opened.",CONFIG("servers.log.app.file"));
    return 1;
    }
    if( cft_GetEx(pWD->pProgramConfig->pCONF,"servers.log.app.span",&ConfNode,NULL,&(pWD->AppLog.TimeSpan),NULL,NULL) )
      pWD->AppLog.TimeSpan = 0;

    log_printf(&(pWD->AppLog),"All logs successfully intialized.");
    }

  pWD->pszPid = CONFIG("servers.pid.file");
  fp = NULL;
  if( pWD->pszPid )fp = fopen(pWD->pszPid,"w");
  if( fp == NULL ){
    log_printf(&(pWD->PanicLog),"Pid file (%s) cannot be opened.",pWD->pszPid);
    if( ! pWD->nolog )iError = 1;
    pWD->pszPid = NULL;
    }else{
    fprintf(fp,"%d\n",getpid());
    fclose(fp);
    fp = NULL;
    }

  thread_InitMutex(&(pWD->pHT->mxState));
  pWD->pHT->iState = STATE_NORMAL;

  if( cft_GetEx(pWD->pProgramConfig->pCONF,"servers.pid.delay",&ConfNode,NULL,&(pWD->lWaitLoop),NULL,NULL) )
    pWD->lWaitLoop  = 5; /* five seconds delay */

  if( !iError && pWD->pszPid )
    thread_CreateThread(&hThread,GuardThread,pWD->pHT);

#ifdef WIN32
  pHT_Service = pWD->pHT;
#endif

  ConfNode = cft_FindNode(pWD->pProgramConfig->pCONF,CFT_ROOT_NODE,"servers.run");
  if( ConfNode )ConfNode = cft_EnumFirst(pWD->pProgramConfig->pCONF,ConfNode);
  if( ConfNode ){/* if there is any servers.run command configured */
    for( Node = ConfNode ;  Node ; Node = cft_EnumNext(pWD->pProgramConfig->pCONF,Node) ){
      s = cft_GetKey(pWD->pProgramConfig->pCONF,Node);
      if( ! strcmp(s,"start") )iRestart = 0;
      else if( ! strcmp(s,"restart") )iRestart = 1;
      else{
        log_printf(&(pWD->ErrLog),"Unknow RUN command in the config file %s",s);
        continue;
        }
      cft_GetEx(pWD->pProgramConfig->pCONF,NULL,&Node,&s,NULL,NULL,NULL);
      ExecuteProgram(pWD,s,iRestart);
      }
    }


  log_printf(&(pWD->AppLog),"AppStart returns %d",iError);
  return iError;
  }

static void pfFtpOut(int ch,
                     void *pE){
  pThreadData pT = pE;

  ch = (char)ch;

  WriteClient(&ch,1);
  return;
  }
static int pfFtpIn(void *pE){
  return EOF;
  }

static void pfStdOut(int ch,
                     void *pE){
  pThreadData pT;
  pApplicationThreadData pATD;

  ch = (char)ch;

  pT = pE;
  pATD = pT->AppThreadData;
  if( ! pATD->FirstHeaderLine ){
    WriteClient(&ch,1);
    return;
    }

  if( pATD->cbBuffer == OBLEN ){
    WriteClient(pATD->szBuffer,pATD->cbBuffer);
    pATD->cbBuffer = 0;
    pATD->FirstHeaderLine = 0;
    WriteClient(&ch,1);
    return;
    }

  pATD->szBuffer[pATD->cbBuffer++] = ch;
  /* if this is the first header line convert 'Status:' to 'HTTP/1.0' */
  if( ch == '\n' ){
    pATD->FirstHeaderLine = 0;
    if( pATD->cbBuffer > 7 && ! strncmp(pATD->szBuffer,"Status:",7) ){
      WriteClient("HTTP/1.0",8);
      WriteClient(pATD->szBuffer+7,pATD->cbBuffer-7);
      pATD->cbBuffer = 0;
      return;
      }else{
      WriteClient(pATD->szBuffer,pATD->cbBuffer);
      pATD->cbBuffer = 0;
      }
    }

  }

static int pfStdIn(void *pE){
  pThreadData pT;
  pApplicationThreadData pATD;  

  pT = pE;
  if( pT->cbAvailable ){
    pT->cbAvailable--;
    return (unsigned int)*(pT->pszData++);
    }

  pATD = pT->AppThreadData;

  if( pATD->cbInBuffer ){
    pATD->cbInBuffer--;
    return (unsigned int)*pATD->pszInBuffer++;
    }

  /* if there is no available bytes we have to fill the buffer */
  pATD->cbInBuffer = ReadClient(pATD->szInBuffer,INBLEN);
  pATD->pszInBuffer = pATD->szInBuffer;
  if( pATD->cbInBuffer ){
    pATD->cbInBuffer--;
    return (unsigned int)*(pATD->pszInBuffer++);
    }else return EOF;
  }

static char *pfFtpEnv(void *pE, char *pszEVN, long lEVSN){
  pThreadData pT;
  pApplicationThreadData pATD;
  int isLong;
  char *pszRet;

  pT = pE;
  pATD = pT->AppThreadData;

  if( isLong = (pszEVN == NULL) )
    switch( lEVSN ){
#define X(a,b) case a: pszEVN = b; strcpy(pATD->szEnvBuf,b); strcat(pATD->szEnvBuf,"=");break;
X(0,"FTP_COMMAND")
X(1,"THREAD_INDEX")
      default: return NULL;
      }
#define envRETURN(X) do{ pszRet=X;\
                      if( isLong ){\
                        if( pszRet && strlen(pszRet) < ENVLEN - strlen(pATD->szEnvBuf) -1 )\
                          strcat(pATD->szEnvBuf,pszRet);\
                        return pATD->szEnvBuf;\
                        }else return pszRet;\
                     }while(0)

  if( !strcmp(pszEVN,"FTP_COMMAND") ){
    envRETURN( pATD->pszFtpCommand );
    }
  if( !strcmp(pszEVN,"THREAD_INDEX") ){
    envRETURN( pATD->szThreadIndex );
    }

  return NULL;
  }

static char *pfEnv(void *pE, char *pszEVN, long lEVSN){
  pThreadData pT;
  pApplicationThreadData pATD;
  pWebData pWD;
  char *s;
  int isLong;
  char *pszRet;

  pT = pE;
  pATD = pT->AppThreadData;
  pWD = pT->pHT->AppData;

  if( isLong = (pszEVN == NULL) )
    switch( lEVSN ){
#define X(a,b) case a: pszEVN = b; strcpy(pATD->szEnvBuf,b); strcat(pATD->szEnvBuf,"=");break;
X(0,"HTTP_COOKIE")
X(1,"SERVER_SOFTWARE")
X(2,"SERVER_NAME")
X(3,"GATEWAY_INTERFACE")
X(4,"SERVER_PROTOCOL")
X(5,"SERVER_PORT")
X(6,"REQUEST_METHOD")
X(7,"PATH_INFO")
X(8,"PATH_TRANSLATED")
X(9,"SCRIPT_NAME")
X(10,"QUERY_STRING")
X(11,"REMOTE_HOST")
X(12,"REMOTE_ADDR")
X(13,"AUTH_TYPE")
X(14,"REMOTE_USER")
X(15,"REMOTE_IDENT")
X(16,"CONTENT_TYPE")
X(17,"CONTENT_LENGTH")
X(18,"HTTP_USER_AGENT")
X(19,"HTTP_REFERER")
X(20,"HTTP_PASSWORD")
X(21,"HTTP_AUTHORIZATION")
#undef X
      default: return NULL;
      }

#define envRETURN(X) do{ pszRet=X;\
                      if( isLong ){\
                        if( pszRet && strlen(pszRet) < ENVLEN - strlen(pATD->szEnvBuf) -1 )\
                          strcat(pATD->szEnvBuf,pszRet);\
                        return pATD->szEnvBuf;\
                        }else return pszRet;\
                     }while(0)

  if( !strcmp(pszEVN,"HTTP_COOKIE") ){
    envRETURN( GetServerVariable("Cookie") );
    }
  if( !strcmp(pszEVN,"SERVER_SOFTWARE") ){
    envRETURN( "Eszter SB Engine 1.0" );
    }
  if( !strcmp(pszEVN,"SERVER_NAME") ){
    if( isLong )return "SERVER_NAME=";
    return NULL;
    }
  if( !strcmp(pszEVN,"GATEWAY_INTERFACE") ){
    envRETURN( "CGI/1.1" );
    }
  if( !strcmp(pszEVN,"SERVER_PROTOCOL") ){
    envRETURN( "HTTP/1.1" );
    }
  if( !strcmp(pszEVN,"SERVER_PORT") ){
    
    envRETURN( pWD->szPort );
    }
  if( !strcmp(pszEVN,"REQUEST_METHOD") ){
    envRETURN( pT->pszMethod );
    }
  if( !strcmp(pszEVN,"PATH_INFO") ){
    if( isLong )return "PATH_INFO=";
    return NULL;
    }
  if( !strcmp(pszEVN,"PATH_TRANSLATED") ){
    envRETURN( pATD->pszPathTranslated );
    }
  if( !strcmp(pszEVN,"SCRIPT_NAME") ){
    envRETURN( ScriptName() );
    }
  if( !strcmp(pszEVN,"QUERY_STRING") ){
    s = pT->pszQueryString;
    while( *s && *s != '?' )s++;
    if( *s )s++;
    envRETURN( s );
    }
  if( !strcmp(pszEVN,"REMOTE_HOST") ){
    envRETURN( pATD->pszClientIP );
    }
  if( !strcmp(pszEVN,"REMOTE_ADDR") ){
    envRETURN( pATD->pszClientIP );
    }
  if( !strcmp(pszEVN,"AUTH_TYPE") ){
    envRETURN( GetServerVariable("Authorization") );
    }
  if( !strcmp(pszEVN,"REMOTE_USER") ){
    envRETURN( pATD->pszRemoteUser );
    }
  if( !strcmp(pszEVN,"REMOTE_IDENT") ){
    if( isLong )return "REMOTE_IDENT=";
    return NULL;
    }
  if( !strcmp(pszEVN,"CONTENT_TYPE") ){
    envRETURN( GetServerVariable("Content-Type") );
    }
  if( !strcmp(pszEVN,"CONTENT_LENGTH") ){
    envRETURN( GetServerVariable("Content-Length") );
    }
  if( !strcmp(pszEVN,"HTTP_USER_AGENT") ){
    envRETURN( GetServerVariable("User-Agent") );
    }
  if( !strcmp(pszEVN,"HTTP_REFERER") ){
    envRETURN( GetServerVariable("Referer") );
    }
  if( !strcmp(pszEVN,"HTTP_PASSWORD") ){
    envRETURN( pATD->pszPassword );
    }
  if( !strcmp(pszEVN,"HTTP_AUTHORIZATION") ){
    envRETURN( GetServerVariable("Authorization") );
    }

  return NULL;
  }

/* send the 404 error page to the client */
static void send404(pHttpdThread pHT,pThreadData pT){
  pWebData pWD;

  pWD = pHT->AppData;
  if( pWD->code404 ){
    WriteClientText("HTTP/1.0 ");
    WriteClientText(pWD->code404);
    WriteClientText("\nContent-Type: text/html\n\n");
    }else{
    WriteClientText("HTTP/1.0 404 Page not found\nContent-Type: text/html\n\n");
    }
  if( pWD->msg404 ){
    WriteClientText(pWD->msg404);
    }else{
    /* The shortest possible correct error message that we can send to the client. */
    WriteClientText(
"<HTML><HEAD><TITLE>Page not found error 404</TITLE></HEAD>"
"<BODY>The requested page is not found on this server.</BODY></HTML>"
);
    }
  
  }

int FtpProc(pHttpdThread pHT,pThreadData pT,char *Buffer){
  char *s,*r,last_c;
  int inpfnlen;
  pSbProgram pProgram;
  char szInputFile[FULL_PATH_BUFFER_LENGTH];
  int binarycode;
  pWebData pWD;
  pApplicationThreadData pATD;
  unsigned long iErrorCode;
  FILE *fp;

  pWD = pHT->AppData;
  if( NULL == pT->AppThreadData ){
    pT->AppThreadData = malloc( sizeof(ApplicationThreadData) );
    if( NULL == pT->AppThreadData )return 1;
    }
  pATD = pT->AppThreadData;
  pT->pThreadLocalData = pHT;
  
  s = NULL;
#define GET_BAS_P_NAME(X,Y) if( ! strnicmp(Buffer,#X,Y) )s = pHT->server[pT->server_index].X;
  GET_BAS_P_NAME(USER,4)
  GET_BAS_P_NAME(PASS,4)
  GET_BAS_P_NAME(ACCT,4)
  GET_BAS_P_NAME(CWD,3)
  GET_BAS_P_NAME(CDUP,4)
  GET_BAS_P_NAME(SMNT,4)
  GET_BAS_P_NAME(REIN,4)
  GET_BAS_P_NAME(QUIT,4)
  GET_BAS_P_NAME(PORT,4)
  GET_BAS_P_NAME(PASV,4)
  GET_BAS_P_NAME(TYPE,4)
  GET_BAS_P_NAME(STRU,4)
  GET_BAS_P_NAME(MODE,4)
  GET_BAS_P_NAME(RETR,4)
  GET_BAS_P_NAME(STOR,4)
  GET_BAS_P_NAME(STOU,4)
  GET_BAS_P_NAME(APPE,4)
  GET_BAS_P_NAME(ALLO,4)
  GET_BAS_P_NAME(REST,4)
  GET_BAS_P_NAME(RNFR,4)
  GET_BAS_P_NAME(RNTO,4)
  GET_BAS_P_NAME(ABOR,4)
  GET_BAS_P_NAME(DELE,4)
  GET_BAS_P_NAME(MKD,3)
  GET_BAS_P_NAME(PWD,3)
  GET_BAS_P_NAME(LIST,4)
  GET_BAS_P_NAME(NLST,4)
  GET_BAS_P_NAME(SITE,4)
  GET_BAS_P_NAME(SYST,4)
  GET_BAS_P_NAME(STAT,4)
  GET_BAS_P_NAME(HELP,4)
  GET_BAS_P_NAME(NOOP,4)

  if( NULL == s ){
    /* If there is no program name configured then the name of the command is used
       in lower case value.

       In this case codebase should be defined for the server.
    */
    if( NULL == pHT->server[pT->server_index].codebase ){
      if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
      log_printf(&(pWD->ErrLog),"no code base is defined for server %d and the command \"%s\" is not handled",pT->server_index,Buffer);
      return 1;
      }
    if( strlen(pHT->server[pT->server_index].codebase) >= FULL_PATH_BUFFER_LENGTH ){
      if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
      log_printf(&(pWD->ErrLog),"code base for server %d is too long, max %d is allowed",pT->server_index,FULL_PATH_BUFFER_LENGTH);
      return 1;
      }
    strcpy(szInputFile,pHT->server[pT->server_index].codebase);
    inpfnlen=strlen(szInputFile);
    r = szInputFile+inpfnlen;
    if( inpfnlen )
      last_c = *( r-1 );
    else
      last_c = (char)0;
    if( inpfnlen >= FULL_PATH_BUFFER_LENGTH ){
      if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
      log_printf(&(pWD->ErrLog),"file name becomes too long server %d, max %d chars are allowed",pT->server_index,FULL_PATH_BUFFER_LENGTH);
      return 1;
      }
    if( '\\' != last_c && '/' != last_c )strcat(szInputFile,"/");
    s = Buffer;
    while( *s && !isspace(*s) ){
      inpfnlen++;
      if( inpfnlen >= FULL_PATH_BUFFER_LENGTH-4 ){
        if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
        log_printf(&(pWD->ErrLog),"file name becomes too long server %d, max %d chars are allowed",pT->server_index,FULL_PATH_BUFFER_LENGTH);
        return 1;
        }
      *r++ = isupper(*s) ? tolower(*s) : *s ;
      *r = (char)0;
      s++;
      }
    strcpy(r,".bas");
    }else{
    if( strlen(s) >= FULL_PATH_BUFFER_LENGTH ){
        if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
        log_printf(&(pWD->ErrLog),"Configured file name for basic program is too long for server %d, max %d chars are allowed.",pT->server_index,FULL_PATH_BUFFER_LENGTH);
        return 1;
        }
    strcpy(szInputFile,s);
    }

  /* check that the file exists and readable */
  if( NULL == (fp = file_fopen(szInputFile,"r")) ){
    WriteClientText("500 Syntax error, command unrecognized really.\r\n");
    return 0;
    }
  file_fclose(fp);

  pProgram = scriba_new(malloc,free);
  if( pProgram == NULL ){
    log_printf(&(pWD->ErrLog),"scriba_new returned null");
    if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
    return 1;
    }

  scriba_SetProcessSbObject(pProgram,pWD->pProgramConfig);
  scriba_SetStdin(pProgram,pfFtpIn);
  scriba_SetStdout(pProgram,pfFtpOut);

  pATD->pszFtpCommand = Buffer;
  sprintf(pATD->szThreadIndex,"%d",pT->ThreadIndex);
  scriba_SetEnvironment(pProgram,pfFtpEnv);
  scriba_SetEmbedPointer(pProgram,pT);
  scriba_SetReportPointer(pProgram,pT);
  scriba_SetReportFunction(pProgram,ftp_report);

  scriba_SetFileName(pProgram,szInputFile);

  binarycode = 0;
  if( scriba_UseCacheFile(pProgram) == SCRIBA_ERROR_SUCCESS )binarycode = 1;
  if( binarycode || scriba_IsFileBinaryFormat(pProgram) ){
    scriba_LoadBinaryProgram(pProgram);
    }else{
    if( scriba_RunExternalPreprocessor(pProgram,NULL) ){
      log_printf(&(pWD->ErrLog),"External preprocessor failed for %s",szInputFile);
      if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
      return 1;
      }

    if( scriba_LoadSourceProgram(pProgram) ){/* if there is some error loading the program */
      scriba_destroy(pProgram);
      if( pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
      log_printf(&(pWD->ErrLog),"There was error loading the program \"%s\"",szInputFile);
      return 1; /* error was already reported, just return */
      }
    scriba_SaveCacheFile(pProgram);
    }
  log_printf(&(pWD->HitLog),"Executing %s",szInputFile);
  iErrorCode = scriba_Run(pProgram,NULL);
  scriba_destroy(pProgram);
  if( iErrorCode && pT->AppThreadData ){ free(pT->AppThreadData);pT->AppThreadData = NULL;}
  return iErrorCode;
  }

void HttpProc(pHttpdThread pHT,pThreadData pT){
  char *sn,*s;
  pSbProgram pProgram;
  char szInputFile[FULL_PATH_BUFFER_LENGTH];
  char szOriginalInputFile[FULL_PATH_BUFFER_LENGTH]; /* to store PathTranslated when 404 code is executed */
  char szAuthStringBuffer[FULL_PATH_BUFFER_LENGTH];
  char szClientIP[16]; /* xxx.xxx.xxx.xxx\0 */
  int binarycode;
  pWebData pWD;
  ApplicationThreadData ATD;
  unsigned long i,iErrorCode;
  int iErrorCounter;
  FILE *fProgramSource;

  pWD = pHT->AppData;
  pT->AppThreadData = &ATD;
  pT->pThreadLocalData = pHT;
  
  if( pWD->XForwarded ){
    ATD.pszClientIP = GetServerVariable("X-Forwarded-For");
    }else{
    sprintf(szClientIP,"%d.%d.%d.%d",pT->ClientIP[0],pT->ClientIP[1],pT->ClientIP[2],pT->ClientIP[3]);
    ATD.pszClientIP = szClientIP;
    }
  ATD.cbBuffer = 0;
  ATD.FirstHeaderLine = 1;
  ATD.cbInBuffer = 0;

  /*
   *  Get the BASIC authentication parameters
   */
  s = GetServerVariable("Authorization");
  if( s ){
    while( *s && isspace(*s) )s++;    /* step over the space after the :   */
    while( *s && ! isspace(*s) )s++;  /* step over the word 'Basic'        */
    while( *s && isspace(*s) )s++;    /* step over the space after 'Basic' */
    uudecode(s,szAuthStringBuffer,FULL_PATH_BUFFER_LENGTH);
    }else *szAuthStringBuffer = (char)0;
  ATD.pszRemoteUser = szAuthStringBuffer;
  s = szAuthStringBuffer;
  while( *s && *s != ':' )s++;
  if( *s )*s++ = (char)0;
  ATD.pszPassword = s;

  /* get the name of the BASIC program */
  sn = ScriptName();

  pProgram = scriba_new(malloc,free);
  if( pProgram == NULL )return;

  scriba_SetProcessSbObject(pProgram,pWD->pProgramConfig);
  scriba_SetCgiFlag(pProgram);
  scriba_SetStdin(pProgram,pfStdIn);
  scriba_SetStdout(pProgram,pfStdOut);
  scriba_SetEnvironment(pProgram,pfEnv);
  scriba_SetEmbedPointer(pProgram,pT);
  scriba_SetReportPointer(pProgram,pT);
  scriba_SetReportFunction(pProgram,httpd_report);

  s = NULL;
  for( i = 0 ; i < pWD->cVdir ; i++ ){
    if( s = prefix(pT->pszQueryString,pWD->ppszVdir[i]) )
      break;
    }

  if( s ){
    strcpy(szInputFile,s);
    strcat(szInputFile,sn);
    }else{
    strcpy(szInputFile,pWD->pszHome);
    s = sn;
    if( *s == '/' )s++;
    strcat(szInputFile,s);
    }

  scriba_SetFileName(pProgram,szInputFile);
  ATD.pszPathTranslated = szInputFile;
  binarycode = 0;
  if( scriba_UseCacheFile(pProgram) == SCRIBA_ERROR_SUCCESS )binarycode = 1;
  if( binarycode || scriba_IsFileBinaryFormat(pProgram) ){
    scriba_LoadBinaryProgram(pProgram);
    }else{
    if( scriba_RunExternalPreprocessor(pProgram,NULL) ){
      log_printf(&(pWD->ErrLog),"External preprocessor failed for %s",szInputFile);
      return;
      }

    /* open the program source code to test that we can open the file (readable and exists) */
    if( NULL == (fProgramSource = file_fopen(pProgram->pszFileName,"r")) ){
      if( ! (pWD->bErrMsgDest & ERRMSGERRLOG) )
        log_printf(&(pWD->ErrLog),"Source program loading error %s",szInputFile);
      if( pWD->run404 ){/* if the program can not be loaded and there is a run404 BASIC program */
        strcpy(szOriginalInputFile,szInputFile);
        strcpy(szInputFile,pWD->run404);
        scriba_SetFileName(pProgram,szInputFile);
        ATD.pszPathTranslated = szOriginalInputFile; /* WE LET THE 404 PROGRAM HAVE THE ORIGINAL PATH TO SEE */
        binarycode = 0;
        if( scriba_UseCacheFile(pProgram) == SCRIBA_ERROR_SUCCESS )binarycode = 1;
        if( binarycode || scriba_IsFileBinaryFormat(pProgram) ){
          scriba_LoadBinaryProgram(pProgram);
          }else{
          if( scriba_RunExternalPreprocessor(pProgram,NULL) ){
            log_printf(&(pWD->ErrLog),"External preprocessor failed for \"%s\"",szInputFile);
            scriba_destroy(pProgram);
            return;
            }
          if( scriba_LoadSourceProgram(pProgram) ){
            if( ! (pWD->bErrMsgDest & ERRMSGERRLOG) ){
              log_printf(&(pWD->ErrLog),"404 program loading error \"%s\"",szInputFile);
              send404(pHT,pT);
              scriba_destroy(pProgram);
              return;
              }
            }
          }
        }else{/* if there is no run404 program configured */
          if( ! (pWD->bErrMsgDest & ERRMSGCLIENT) ){
            log_printf(&(pWD->ErrLog),"404 program loading error \"%s\"",szInputFile);
            }
          send404(pHT,pT);
          return;
          }
        }
    /* close the file handle that was opened for testing purpose only */
    if( fProgramSource )file_fclose(fProgramSource);
    if( scriba_LoadSourceProgram(pProgram) ){/* if there is some error loading the program */
      scriba_destroy(pProgram);
      return; /* error was already reported, just return */
      }
    scriba_SaveCacheFile(pProgram);
    }
  log_printf(&(pWD->HitLog),"Executing %s",szInputFile);
  iErrorCode = scriba_Run(pProgram,NULL);
  if( iErrorCode )
    httpd_report(pT,szInputFile,0,iErrorCode,REPORT_ERROR,&iErrorCounter,
                 pProgram->pEXE->pszModuleError ? pProgram->pEXE->pszModuleError : "program terminated with error",&iErrorCode);
  scriba_destroy(pProgram);
  }

#if BCC32
char *_pgmptr;
#endif

main(int argc, char *argv[]){
  int do_fork, do_safe;
  int i;
#ifdef WIN32
  SERVICE_TABLE_ENTRY dispatchTable[] ={
        { "sbhttpd", (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };
#else
  int cpid;
#endif

  /* Because the command CHDIR calls the system function that changes the current directory
     of the process, and because this application implements several ScriptBasic interpreters
     in several threads thus it is better to disable the command for all programs that
     runs in this application. */
  CommandFunction[CMD_CHDIR - START_CMD ] = NULL;

#ifdef WIN32

#if BCC32
  _pgmptr = argv[0];
#endif
  if( argc > 1 && !strcmp(argv[1],"-install")){
    CmdInstallService();
    return 0;
    }

  if( argc > 1 && !strcmp(argv[1],"-remove")){
    CmdRemoveService();
    return 0;
    }

  /* if the program was loaded as a service then this function
     registers the service_main entry point and returns TRUE */
  if( (argc > 1 && ! strcmp(argv[1],"-start")) || AmIService() )
    if( StartServiceCtrlDispatcher(dispatchTable) )return 0;

#endif

  scriba_InitStaticModules();

  do_fork = argc > 1 && !strcmp(argv[1],"-start");
  do_safe = argc > 1 && !strcmp(argv[1],"-safe");
  if( do_safe )do_fork = 1;

#ifndef WIN32
  /* we have to do the fork here before calling AppInit, because AppInit
     starts threads and fork does NOT duplicate the threads of the parent
     process except the thread calling fork() .*/
  if( do_fork ){
    if( fork() )exit(0);
    }
  if( do_safe ){
    while( cpid = fork() ){
      waitpid( cpid , NULL , WUNTRACED );
      }
    }
#endif

  /* if -start or -safe was specified then delete them from the command line and the rest of the command line
     is passed to the function httpd */
  if( argc > 1 && ( !strcmp(argv[1],"-start") || !strcmp(argv[1],"-safe") ) ){
    for( i = 1 ; i < argc - 1 ; i++ )argv[i] = argv[i+1];
      argc--;
    }

  httpd(argc,argv,AppInit,AppStart,HttpProc,FtpProc);
  return 0;
  }

#ifdef WIN32
VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv){
  char *argv[2] = { "sbhttpd","-start" };

  if( ReportStatusToSCMgr(SERVICE_RUNNING,NO_ERROR,0) ){
    main(2,argv);
    }
}
VOID ServiceStop(){
  pHttpdThread pHT;
  pWebData pWD;  
  SOCKET Client;
  char buf[80];
  int i;

  pHT = pHT_Service;
  pWD = pHT->AppData;

  thread_LockMutex(&(pHT->mxState));
  pHT->iState = STATE_SHUT;
  thread_UnlockMutex(&(pHT->mxState));
  /* we have to connect to the web server port to help it out of the
     waiting loop. For the purpose I use this quick&dirty solution.
     It works. */
  log_printf(&(pWD->AppLog),"Shutting down %d servers",pHT->c_servers);
  for( i=0 ; i < pHT->c_servers ; i++ ){
     struct in_addr sip;
     memcpy(&sip,&(pHT->server[i].ip),sizeof(struct in_addr));
     sprintf(buf,"%s:%d",inet_ntoa(sip),pHT->server[i].port);
     log_printf(&(pWD->AppLog),"Connecting to server %s",buf);
     file_tcpconnect(&Client,buf);
     }
  }
#endif

