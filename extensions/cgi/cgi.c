/*
FILE: cgi.c
HEADER: cgi.h

Version: 2.0

This code can be used to hadle web communication between
the server and the application. Using the functions provided
in this file you can write code runs with CGI as well as ISAPI.

This code was developed to be used in the ScriptBasic project, but
this code can be used as a stand-alone product embedded into other
programs. This is GNU GPL protected.

TO_HEADER:

typedef struct _SymbolList {
  char *symbol;
  FILE *fp;   // pointer to the temporary file
  char *file; // in case this is an uploaded file name
  char *value;
  long len;   // length of the value or the saved temporary file
  struct _SymbolList *pHeaders; // in case of multipart
  struct _SymbolList *next;
  } SymbolList, *pSymbolList;

typedef struct _DebugStore {
  char *ServerSoftware;
  char *ServerName;
  char *GatewayInterface;
  char *ServerProtocol;
  char *ServerPort;
  char *RequestMethod;
  char *PathInfo;
  char *PathTranslated;
  char *ScriptName;
  char *QueryString;
  char *RemoteHost;
  char *RemoteAddress;
  char *AuthType;
  char *RemoteUser;
  char *RemoteIdent;
  char *ContentType;
  char *ContentLength;
  char *UserAgent;
  char *Cookie;

  FILE *fpDebugInput;
  } DebugStore, *pDebugStore;

typedef struct _CgiObject {
  void *(*maf)(long size, void *pSegment);
  void (*mrf)(void *MemoryToFree, void *pSegment);
  void *pSegment;

#define CGI_INTERFACE_CGI   0x00000000 // this is the only implemented so far
#define CGI_INTERFACE_ISAPI 0x00000001
#define CGI_INTERFACE_NSAPI 0x00000002
#define CGI_INTERFACE_FCGI  0x00000003
#define CGI_INTERFACE_DEBUG 0x00000004 // read all information from a debug file
  long fInterface;   // constant defining the interface type

#ifdef WIN32
  LPEXTENSION_CONTROL_BLOCK lpECB;
  char *pszNextChar;
  char *pszLocalBuffer;
  DWORD cbAvailable;
  DWORD dwIsapiBufferSize;
#else
#define LPEXTENSION_CONTROL_BLOCK void *
#endif

  void *pEmbed; // embedder pointer for caller defined stdin/stdout and env functions
  int (*pfStdIn)(void *); // user defined stdin function gets pEmbed as argument
  void (*pfStdOut)(int, void*); // user defined stdout function gets pEmbed as second argument
  char *(*pfEnv)(void *,char *,long); //user defined environment function

  char *pszDebugFile; // the debug file if fInterface is CGI_INTERFACE_DEBUG, otherwise ignored
  pDebugStore pDebugInfo; // points to the allocated struct that stores CGI info read from the debug file

  char *pszBoundary; // multipart boundary string
  unsigned long cbBoundary;   // length of the boundary
  unsigned char *pszBuffer;   // buffer to store characters
  unsigned long cbBuffer;     // the actual size of the buffer
  unsigned long cbFill;       // the number of characters stored in the buffer at the moment
  unsigned long lBufferPosition; // the current buffer position when returning characters
  int (*CharacterInput)(struct _CgiObject *p);
  void *pInputParameter; // the parameter the input function can use
  unsigned long lContentLength;  // the length of the POST param (used by the CharacterInput function)

  pSymbolList pGetParameters; // point to GET parameters
  pSymbolList pPostParameters; // point to POST parameters

  unsigned long lBufferIncrease; // increase the buffer using this increment when buffer is small
  unsigned long lBufferMax;      // never exceed this size with the buffer
  unsigned long lContentMax;     // maximal content-length we deal with
  unsigned long lFileMax;        // maximal file length we deal with

#define CGI_METHOD_NONE 0x00000000
#define CGI_METHOD_GET  0x00000001
#define CGI_METHOD_POST 0x00000002
#define CGI_METHOD_UPL  0x00000004 // upload "method" this is a post method,
                                   //but here we treat it if it was a separate method
// these methods are not implemented yet
#define CGI_METHOD_PUT  0x00000008
#define CGI_METHOD_DEL  0x00000010 
#define CGI_METHOD_COPY 0x00000020
#define CGI_METHOD_MOVE 0x00000040
#define CGI_METHOD_HEAD 0x00000080

  long fMethods;        // allowed methods, each bit set is an allowed method
  } CgiObject, *pCgiObject;


#define CGI_ERROR_BUFFER_OVERFLOW 0x00080001
#define CGI_ERROR_BIG_CONTENT     0x00080002
#define CGI_ERROR_INVALID_METHOD  0x00080003
#define CGI_ERROR_NO_DEBUG_FILE   0x00080004
#define CGI_ERROR_NOTIMP          0x00080005
#define CGI_ERROR_EOF             0x00080006
#define CGI_ERROR_ILLF_MULTI      0x00080007
#define CGI_ERROR_FILEMAX         0x00080008
#define CGI_ERROR_MEMORY_LOW      0x00080009
#define CGI_ERROR_METHOD_NOTALL   0x0008000A
#define CGI_ERROR_ILLF_MULTI1     0x00080017
#define CGI_ERROR_ILLF_MULTI2     0x00080027
#define CGI_ERROR_ILLF_MULTI3     0x00080037
#define CGI_ERROR_ILLF_MULTI4     0x00080047
#define CGI_ERROR_ILLF_MULTI5     0x00080057
#define CGI_ERROR_ILLF_MULTI6     0x00080067
#define CGI_ERROR_ILLF_MULTI7     0x00080077

//this is a safe macro implementation of the functions
#define cgi_EmptyBuffer(x) ((x)->cbFill=0)
#define cgi_BufferFull(x)  ((x)->cbFill == (x)->cbBuffer)
*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#include <httpext.h>
#endif

#include "cgi.h"

#define _getenv(X) (pCO->pfEnv ? pCO->pfEnv(pCO->pEmbed,(X),0) : getenv(X))

#if (!defined(_WIN32) && !defined(__MACOS__))
static int stricmp(char *a, char *b){
  char ca,cb;

  while( 1 ){
    ca = *a++;
    cb = *b++;
    ca = isupper(ca) ? tolower(ca) : ca;
    cb = isupper(cb) ? tolower(cb) : cb;
    if( ca == (char)0 && cb == (char)0 )return 0;
    if( ca != cb )return ca-cb;
    }
  }
#endif


#define CR '\r'
#define LF '\n'
/* initial buffer size */
#define BUFFER_INCREASE 1024
#define BUFFER_MAX      10240
/* 10MB */
#define CONTENT_MAX     0xA00000
#define FILE_MAX        0xA00000
/* 40KB */
#define ISAPI_BUFFER    0xA000

#define DEBUGFP (pCO->pDebugInfo->fpDebugInput)

#define GETCHAR() (pCO->CharacterInput(pCO))
#define ALLOC(x) (pCO->maf((x),pCO->pSegment))
#define FREE(x)  (pCO->mrf((x),pCO->pSegment))

static char x2c(char *what){
  register char digit;
#define TOHEX(x)  ((x) >= 'A' ? (((x) & 0xdf) - 'A')+10 : ((x) - '0'))

  digit = TOHEX(*what);
  digit *= 16;
  what++;
  digit += TOHEX(*what);
  return digit;
#undef TOHEX
}

static void unescape(char *s, long *len){
  char *p;
  long rest;

  p  = s;  
  rest = *len;
  while( rest ){
    if( *p == '+' )*p = ' ';
    p++;
    rest --;
    }

  p  = s;  
  rest = *len;

  while( rest ){
    *p = *s;
    if( *p == '%' ){
      s++;rest--;
      *p = x2c(s);
      s++;
      rest--;
      (*len) -= 2;
      }
    p++;
    s++;
    rest--;
    }
  }

static void *my_maf(long size, void *p){
  return malloc(size);
  }
static void my_mrf(void *q, void*p){
  free(q);
  }
#ifdef WIN32
static int IsapiGetChar(pCgiObject pCO){
  unsigned char ch; /* it IS important that this is unsigned otherwise a 255
                       would mean EOF */

  if( pCO->lContentLength == 0 )return EOF;
  if( pCO->cbAvailable ){
    pCO->cbAvailable--;
    ch = *(pCO->pszNextChar);
    pCO->lContentLength --;
    pCO->pszNextChar ++;
    return (unsigned int)ch;
    }

  if( pCO->pszLocalBuffer == NULL ){
    if( pCO->dwIsapiBufferSize > pCO->lContentLength )
      pCO->dwIsapiBufferSize = pCO->lContentLength;
    pCO->pszLocalBuffer = ALLOC(pCO->dwIsapiBufferSize);
    if( pCO->pszLocalBuffer == NULL )return EOF;
    }
  pCO->pszNextChar = pCO->pszLocalBuffer;
  pCO->cbAvailable = pCO->dwIsapiBufferSize;
  if( pCO->lpECB->ReadClient(pCO->lpECB->ConnID,pCO->pszLocalBuffer,&(pCO->cbAvailable)) && pCO->cbAvailable ){
    ch = *(pCO->pszNextChar);
    pCO->lContentLength --;
    pCO->cbAvailable--;
    pCO->pszNextChar ++;
    return (unsigned int)ch;
    }
  /* We will get here only if the client pressed the STOP button on the browser. */
  return EOF;
  }
#endif
static int CgiGetChar(pCgiObject pCO){
  int ch;
#ifdef WIN32
  if( ! pCO->pfStdIn ){
    setmode(fileno(stdin), O_BINARY);   /* define stdin as binary */
    _fmode = O_BINARY;                    /* default all file I/O as binary */
     }
#endif
  if( pCO->lContentLength ){
    pCO->lContentLength --;
    if( pCO->pfStdIn )
      ch = pCO->pfStdIn(pCO->pEmbed);
    else
      ch = getchar();

    return ch;
    }
  return EOF;
  }
static void CgiPutChar(pCgiObject pCO, int ch){
  if( pCO->pfStdOut )
    pCO->pfStdOut(ch,pCO->pEmbed);
  else
    putc(ch,stdout);
  }
static int DebugGetChar(pCgiObject pCO){
  int ch;
#ifdef WIN32
  setmode(fileno(DEBUGFP), O_BINARY);   /* define stdin as binary */
  _fmode = O_BINARY;                    /* default all file I/O as binary */
#endif
  ch = getc(DEBUGFP);

  return ch;
  }

/*POD
=H CGI handling

This package contains functions that help you handle CGI (later NSAPI, ISAPI and FCGI)
input in Web programs. The functions are written in a well defined manner, documented,
thread aware.

The functions to be used by external programs (the public functions) have abstract in this
documentation. Other functions are listed and documented here, because they have to be
documented for maintenance purposes. Some very small, almost inline functions are declared
as static and are documented only in the source code.

(zchar string means a string terminted by a character of code zero)

Note that all function names are preceeded with the characters T<cgi_>

Topics:

CUT*/

/*POD
=section InitCgi
=H Initialize a CgiObject with default values
=abstract
This function initializes a CgiObject filling the default values
to handle the http request parameters via the CGI interface.
=end

This function initializes a CgiObject filling the default values
to handle the http request parameters via the CGI interface.

To handle CGI input the caller program should have a variable
of type T<CgiObject>. This T<struct> variable will contain all
information neccessary to handle the CGI input, and all functions
request pointer to this variable as first argument. You can think
of this T<struct> as the container for the class variables (except that
this is written in C and not C++).

T<mrf>

T<maf>

The memory handling functions are initialized to point to a function
using T<malloc> and T<free>. You can change the fields T<maf> and T<mrf>
to point to a T<m>emory T<a>llocation T<f>unction and to a T<m>emory
T<r>eleasing T<f>unction that are similar to T<malloc> and T<free>, but
also accept a second T<void *> argument. This T<void *> argument may help
to use thread local allocation, or to use segmented allocation, where
all memory chunks can be released with a single call using the T<void *>
pointer. (See the http://www.emma.hu/scriptbasic application source and
get the file T<myalloc.c> for example.)

T<CharacterInput>

The field T<CharacterInput> should point to a function that gets a pointer
to the T<CgiObject> variable and returns an int, which is the next character
or EOF in case no more input is available. This field is initialized to
point to local function that reads the standard input (binary mode on Windows)
but may be reseto to point to any other function in case you want to read a file
for debugging purposes instead of the stdin.

T<lBufferIncrease>

The field T<lBufferIncrease> initialized to T<BUFFER_INCREASE>. (1KByte) This is the
initial size of the buffer and this is the increase whenever the buffer seems to be
small.

T<lBufferMax>

The fiels T<lBufferMax> is initialized to T<BUFFER_MAX>. (10KByte) This is the maximal
size of the buffer. If the buffer needs to be larger than this size the program does
not process the CGI input.

The buffer should be large enough to handle POST data (but not uploaded files) and multipart
headers (one at a time).

T<lContentMax>

The fiels T<lContentMax> is initialized to T<CONTENT_MAX>. (10MByte) This is the maximal size
of the CGI input the program handles. Whenever the header field T<Content-Length> is
larger than this size the program does not process the CGI input.

T<lFileMax>

The filed T<lFileMax> is initialized to T<FILE_MAX>. (10MByte) This is the maximal size
of an uploaded file. Whenever a file is larger than this size the program does not process
the CGI input.

=hrule

These parameters can be altered after calling this initialization function. These
values help the program to minimize the effect of a denial of service attack sending
huge amount of data to web scripts that do not expect to handle them.

/*FUNCTION*/
void cgi_InitCgi(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  pCO->maf = my_maf;
  pCO->mrf = my_mrf;
  pCO->pSegment = NULL;

  pCO->fInterface = CGI_INTERFACE_CGI;

  pCO->pszBoundary = NULL;
  pCO->cbBoundary = 0;
  pCO->pszBuffer = NULL;
  pCO->cbBuffer = 0;

  pCO->cbFill = 0;
  pCO->lBufferPosition = 0;

  pCO->CharacterInput = CgiGetChar;
  pCO->pInputParameter = NULL;
  pCO->lContentLength = 0;

  pCO->lBufferIncrease = BUFFER_INCREASE;
  pCO->lBufferMax = BUFFER_MAX;
  pCO->pGetParameters = NULL;
  pCO->pPostParameters = NULL;
  pCO->lContentMax = CONTENT_MAX;
  pCO->lFileMax = FILE_MAX;
#ifdef WIN32
  pCO->dwIsapiBufferSize = ISAPI_BUFFER;
#endif

  pCO->fMethods = CGI_METHOD_GET | CGI_METHOD_POST | CGI_METHOD_UPL;

  pCO->pszDebugFile = NULL;

  pCO->pfStdIn = NULL;
  pCO->pfStdOut = NULL;
  pCO->pfEnv = NULL;
  pCO->pEmbed = NULL;
  }

#ifdef WIN32
/*POD
=section InitIsapi
=H Initialize a CgiObject with default values
=abstract
This function initializes a CgiObject filling the default values
to handle the http request parameters via the ISAPI interface.
=end

This function initializes a CgiObject filling the default values
to handle the http request parameters via the ISAPI interface.
It actually does nothing else, but calls the CGI initialization function
R<InitCgi> and then alters the field T<fInterface> of the CGI object
to signal that this is an ISAPI interface handling case now.

/*FUNCTION*/
void cgi_InitIsapi(pCgiObject pCO,
                   LPEXTENSION_CONTROL_BLOCK lpECB
  ){
/*noverbatim
CUT*/
  cgi_InitCgi(pCO);

  pCO->lpECB = lpECB;
  pCO->CharacterInput = IsapiGetChar;
  pCO->pszNextChar = lpECB->lpbData;
  pCO->cbAvailable = lpECB->cbAvailable;
  pCO->fInterface = CGI_INTERFACE_ISAPI;
  pCO->pszLocalBuffer = NULL;

  pCO->pfStdIn = NULL;
  pCO->pfStdOut = NULL;
  pCO->pfEnv = NULL;
  pCO->pEmbed = NULL;
  }
#else
void cgi_InitIsapi(pCgiObject pCO,
                   LPEXTENSION_CONTROL_BLOCK lpECB
  ){
  /* on UNIX we do nothing */
  }
#endif

/*POD
=section ReadHttpRequest
=H Read the http request data
=abstract
This function reads the http request data and stores in linked lists available to query the
values.
=end

The application program should call this function to process the CGI input data. The parameter
T<pCO> should point to an initialized R<InitCgi> structure. The function decides what type of
input handling is needed, reads the http request and stores them in the appropriate fields
avaiable for later query.

/*FUNCTION*/
long cgi_ReadHttpRequest(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  char LocalBuffer[1024],*s,*r;
  long w;

  switch( pCO->fInterface ){
    case CGI_INTERFACE_ISAPI:
      /* ISAPI methods need tamporary space to store the header
         variables. This is needed because the ISAPI interface does
         not return a pointer to a constant string but rather copies
         the value to a buffer that we have to provide. The pointers
         in the DebugStore are just fine for the purpose.            */
      pCO->pDebugInfo = ALLOC(sizeof(DebugStore));
      if( pCO->pDebugInfo == NULL )return CGI_ERROR_MEMORY_LOW;
      memset(pCO->pDebugInfo,0,sizeof(DebugStore));
    case CGI_INTERFACE_CGI:
    case CGI_INTERFACE_DEBUG:
RetryWithDebugMode:
      if( NULL != (s = cgi_RequestMethod(pCO)) ){
        w = 0;
        if( !strcmp(s,"GET")  )w = CGI_METHOD_GET;
        else
        if( !strcmp(s,"HEAD") )w = CGI_METHOD_HEAD;
        if( w ){
          if( ! (pCO->fMethods&w) )return CGI_ERROR_METHOD_NOTALL;
          return cgi_GetGetParameters(pCO);
          }
        }
      if( s && !strcmp(s,"POST") ){
        if( ! (pCO->fMethods&CGI_METHOD_POST) )return CGI_ERROR_METHOD_NOTALL;
        r=cgi_ContentLength(pCO);
        pCO->lContentLength = r ? atol( r ) : 0 ;
        if( pCO->lContentLength > pCO->lContentMax )return CGI_ERROR_BIG_CONTENT;
        if( (r=cgi_ContentType(pCO)) && ! memcmp(r,"multipart/form-data",19) ){
          if( ! (pCO->fMethods&CGI_METHOD_UPL) )return CGI_ERROR_METHOD_NOTALL;
          return cgi_GetMultipartParameters(pCO);
          }
        else return cgi_GetPostParameters(pCO);
        }
     /* if it is already DEBUG then there is no way out */
     if( pCO->fInterface == CGI_INTERFACE_DEBUG )return CGI_ERROR_INVALID_METHOD;
     /* if this is neither GET nor POST then we try to switch to debug more */
     pCO->fInterface = CGI_INTERFACE_DEBUG;
#define READL(x) if( fgets(LocalBuffer,1024,DEBUGFP) ){\
                 LocalBuffer[w=strlen(LocalBuffer)-2] = (char)0; /* remove terminating \r\n (we read binary)*/ \
                 pCO->pDebugInfo->x = ALLOC(w);\
                 if( pCO->pDebugInfo->x == NULL )return CGI_ERROR_MEMORY_LOW;\
                 strcpy(pCO->pDebugInfo->x,LocalBuffer);\
                 }else pCO->pDebugInfo->x = "";

      pCO->pDebugInfo = ALLOC(sizeof(DebugStore));
      if( pCO->pDebugInfo == NULL )return CGI_ERROR_MEMORY_LOW;
      if( pCO->pszDebugFile == NULL )return CGI_ERROR_NO_DEBUG_FILE;
      DEBUGFP = fopen(pCO->pszDebugFile,"rb");
      if( DEBUGFP == NULL )return CGI_ERROR_NO_DEBUG_FILE;

      READL(ServerSoftware);
      READL(ServerName);
      READL(GatewayInterface);
      READL(ServerProtocol);
      READL(ServerPort);
      READL(RequestMethod);
      READL(PathInfo);
      READL(PathTranslated);
      READL(ScriptName);
      READL(QueryString);
      READL(RemoteHost);
      READL(RemoteAddress);
      READL(AuthType);
      READL(RemoteUser);
      READL(RemoteIdent);
      READL(ContentType);
      READL(ContentLength);
      READL(UserAgent);
      READL(Cookie);

      pCO->CharacterInput = DebugGetChar;
      goto RetryWithDebugMode;
    case CGI_INTERFACE_NSAPI:
      return CGI_ERROR_NOTIMP;
    case CGI_INTERFACE_FCGI:
      return CGI_ERROR_NOTIMP;
    }
  return CGI_ERROR_INVALID_METHOD;
  }

/*POD
=section PostParam
=H Query a POST parameter
=abstract
Use this function to get the value of a POST parameter
=end

The argument T<pszParam> should point to a zchar string
containing the name of a POST parameter. The function returns
the pointer to a constant string containing the value fo the parameter
or NULL if there is no such parameter.

/*FUNCTION*/
char *cgi_PostParam(pCgiObject pCO,
                    char *pszParam
  ){
/*noverbatim
If there are more parameters with the same name only the first occurence is retrieved. Use the
function R<PostParamEx> to iterate through all the parameters with a single name.
CUT*/
  pSymbolList p;

  p = pCO->pPostParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->value ? p->value : "";
    p = p->next;
    }

  return NULL;
  }

/*POD
=section GetParam
=H Query a GET parameter
=abstract
Use this function to get the value of a GET parameter
=end

The argument T<pszParam> should point to a zchar string
containing the name of a GET parameter. The function returns
the pointer to a constant string containing the value fo the parameter
or NULL if there is no such parameter.

/*FUNCTION*/
char *cgi_GetParam(pCgiObject pCO,
                   char *pszParam
  ){
/*noverbatim
If there are more parameters with the same name only the first occurence is retrieved. Use the
function R<GetParamEx> to iterate through all the parameters with a single name.
CUT*/
  pSymbolList p;

  p = pCO->pGetParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->value ? p->value : "";
    p = p->next;
    }

  return NULL;
  }

/*POD
=section PostParamEx
=H Query POST parameter extended
=abstract
Use this function to get the next value of a POST parameter if the parameter is expected to be present
in the HTTP request more than once.
=end

This function gets the next value of the POST parameter. The name of the POST parameter should be
given by the string T<pszParam>. If T<pszParam> is NULL the next parameter is returned without name comparison.
This way a program can retrieve all the CGI parameters.
The pointer T<p> should point to a pointer. This pointer 
(the one that T<p> points to and not T<p>) should be
initialized to NULL before the first call and should not be modified between calls.

The function searches the next occurence of the parameter and returns a pointer to the constant
string containing the value of theparameter or NULL if the parameter has no more occurence.
/*FUNCTION*/
char *cgi_PostParamEx(pCgiObject pCO,
                      char *pszParam,
                      pSymbolList *p
  ){
/*noverbatim
If a parameter is not expected to appear more than once use the function R<PostParam>.
CUT*/
  if( *p == NULL )
    *p = pCO->pPostParameters;
  else
    *p = (*p)->next;
  while( *p ){
    if( !pszParam || !strcmp((*p)->symbol,pszParam) )
      return (*p)->value ? (*p)->value : "";

    *p = (*p)->next;
    }

  return NULL;
  }

/*POD
=section GetParamEx
=H Query GET parameter extended
=abstract
Use this function to get the next value of a GET parameter if the parameter is expected to be present
in the HTTP request more than once.
=end

This function gets the next value of the GET parameter. The name of the GET parameter should be
given by the string T<pszParam>. If T<pszParam> is NULL the next parameter is returned without name comparison.
This way a program can retrieve all the CGI parameters.
The pointer T<p> should point to a pointer. This pointer 
(the one that T<p> points to and not T<p>) should be
initialized to NULL before the first call and should not be modified between calls.

The function searches the next occurence of the parameter and returns a pointer to the constant
string containing the value of theparameter or NULL if the parameter has no more occurence.
/*FUNCTION*/
char *cgi_GetParamEx(pCgiObject pCO,
                     char *pszParam,
                     pSymbolList *p
  ){
/*noverbatim
If a parameter is not expected to appear more than once use the function R<GetParam>.
CUT*/
  if( *p == NULL )
    *p = pCO->pGetParameters;
  else
    *p = (*p)->next;
  while( *p ){
    if( !pszParam || !strcmp((*p)->symbol,pszParam) )
      return (*p)->value ? (*p)->value : "";

    *p = (*p)->next;
    }

  return NULL;
  }

/*POD
=section FILEp
=H Query a POST parameter
=abstract
Use this function to get the file pointer to the temporary uploaded file.
=end

The argument T<pszParam> should point to a zchar string
containing the name of a POST parameter, which is an uploaded
file and is stored in a temporary file.

/*FUNCTION*/
FILE *cgi_FILEp(pCgiObject pCO,
                char *pszParam
  ){
/*noverbatim
CUT*/
  pSymbolList p;

  p = pCO->pPostParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->fp;
    p = p->next;
    }
  return NULL;
  }

/*POD
=section OriginalFileName
=H Query a POST parameter
=abstract
Use this function to get the file name of an uploaded file.
=end

The argument T<pszParam> should point to a zchar string
containing the name of a POST parameter, which is an uploaded
file and is stored in a temporary file.

The return value points to a zchar string containing the original
file name as it was given in the header T<Content-Disposition>.

/*FUNCTION*/
char *cgi_OriginalFileName(pCgiObject pCO,
                char *pszParam
  ){
/*noverbatim
CUT*/
  pSymbolList p;

  p = pCO->pPostParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->file;
    p = p->next;
    }

  return NULL;
  }

/*POD
=section FileLength
=H Query the length of an uploaded file
=abstract
Use this function to get the length of an uploaded file.
=end

The argument T<pszParam> should point to a zchar string
containing the name of a POST parameter, which is an uploaded
file and is stored in a temporary file.

The return value is a long, which is the number of characters in the
uploaded file.

/*FUNCTION*/
long cgi_FileLength(pCgiObject pCO,
                char *pszParam
  ){
/*noverbatim
CUT*/
  pSymbolList p;

  p = pCO->pPostParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->len;
    p = p->next;
    }

  return 0;
  }

/*POD
=section PartHeader
=H Query a POST parameter
=abstract
Use this function to get the pointer to the header of a multipart part.
=end

The argument T<pszParam> should point to a zchar string
containing the name of a POST parameter, which is an uploaded
file and is stored in a temporary file.

The return value points to header list. This pointer should be used as an argument to
the function R<Header>.

/*FUNCTION*/
pSymbolList cgi_PartHeader(pCgiObject pCO,
                     char *pszParam
  ){
/*noverbatim
CUT*/
  pSymbolList p;

  p = pCO->pPostParameters;
  while( p ){
    if( !strcmp(p->symbol,pszParam) )return p->pHeaders;
    p = p->next;
    }

  return NULL;
  }

/*POD
=section Header
=H Get a header value

This function returns a pointer to a zero terminated string containing
the value associated with the reqiested header field, or returns NULL
if the header was not present.

The argument T<symbol> shoud point to a ZCHAR string containing the
header symbol we search. The header is searched case insensitive, therefore
T<Content-Type> and T<Content-type> are equivalent.

B<Never use the trailing colon> after the header name.

The third parameter T<pHeader> should point to a header list. Such a hreader
list ids returned by the function R<ReadHeader> when a multipart form data
is read and stored for the uploaded file information.
/*FUNCTION*/
char *cgi_Header(pCgiObject pCO,
                 char *symbol,
                 pSymbolList pHeader
  ){
/*noverbatim
CUT*/
  while( pHeader ){
    if( ! stricmp(pHeader->symbol,symbol) )return pHeader->value;
    pHeader = pHeader->next;
    }
  return NULL;
  }

/*POD
=section Environment
=H Functions to return the usual environment variables
=abstract
Get the values of several CGI environment variables.
=end

These functions return the usual environment variables
of a CGI program. The CGI code should call these functions
and don't call T<getenv> directly because calling these
functions hide the interface and the application programs
can be easily ported to run using ISAPI, NSAPI, FCGI etc.
The functions provided:

=itemize
=item T<ServerSoftware>
=item T<ServerName>
=item T<GatewayInterface>
=item T<ServerProtocol>
=item T<ServerPort>
=item T<RequestMethod>
=item T<PathInfo>
=item T<PathTranslated>
=item T<ScriptName>
=item T<QueryString>
=item T<RemoteHost>
=item T<RemoteAddress>
=item T<AuthType>
=item T<RemoteUser>
=item T<RemoteIdent>
=item T<ContentType>
=item T<ContentLength>
=item T<UserAgent>
=item T<Cookie>
=noitemize

CUT*/

#define ISAPIVAR(X,Y) \
      if( pCO->pDebugInfo->X )return pCO->pDebugInfo->X;\
      { DWORD cbBuffer;\
      cbBuffer = 0;\
      pCO->lpECB->GetServerVariable(pCO->lpECB->ConnID,Y,NULL,&cbBuffer);\
      if( cbBuffer == 0 )return NULL;\
      pCO->pDebugInfo->X = ALLOC(cbBuffer);\
      if( pCO->pDebugInfo->X == NULL )return NULL;\
      pCO->lpECB->GetServerVariable(pCO->lpECB->ConnID,Y,pCO->pDebugInfo->X,&cbBuffer);\
      return pCO->pDebugInfo->X;}

/*FUNCTION*/
char *cgi_Referer(pCgiObject pCO
  ){
  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("HTTP_REFERER");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(Cookie,"HTTP_REFERER")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return NULL;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_Cookie(pCgiObject pCO
  ){
  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("HTTP_COOKIE");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(Cookie,"HTTP_COOKIE")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->Cookie;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ServerSoftware(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("SERVER_SOFTWARE");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ServerSoftware,"SERVER_SOFTWARE")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ServerSoftware;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ServerName(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("SERVER_NAME");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ServerName,"SERVER_NAME")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ServerName;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_GatewayInterface(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("GATEWAY_INTERFACE");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      return "ISAPI";
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->GatewayInterface;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ServerProtocol(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("SERVER_PROTOCOL");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ServerProtocol,"SERVER_PROTOCOL")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ServerProtocol;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ServerPort(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("SERVER_PORT");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ServerPort,"SERVER_PORT")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ServerPort;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_RequestMethod(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("REQUEST_METHOD");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(RequestMethod,"REQUEST_METHOD");
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->RequestMethod;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_PathInfo(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("PATH_INFO");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(PathInfo,"PATH_INFO")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->PathInfo;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_PathTranslated(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("PATH_TRANSLATED");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(PathTranslated,"PATH_TRANSLATED")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->PathTranslated;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ScriptName(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("SCRIPT_NAME");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ScriptName,"SCRIPT_NAME")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ScriptName;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_QueryString(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("QUERY_STRING");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(QueryString,"QUERY_STRING")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->QueryString;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_RemoteHost(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("REMOTE_HOST");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(RemoteHost,"REMOTE_HOST")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->RemoteHost;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_RemoteAddress(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("REMOTE_ADDR");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(RemoteAddress,"REMOTE_ADDR")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->RemoteAddress;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_AuthType(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("AUTH_TYPE");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(AuthType,"AUTH_TYPE")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->AuthType;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_RemoteUser(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("REMOTE_USER");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(RemoteUser,"REMOTE_USER")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->RemoteUser;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_RemoteIdent(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("REMOTE_IDENT");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      return NULL;
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->RemoteIdent;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ContentType(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("CONTENT_TYPE");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ContentType,"CONTENT_TYPE")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ContentType;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_ContentLength(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("CONTENT_LENGTH");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      ISAPIVAR(ContentLength,"CONTENT_LENGTH")
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->ContentLength;
    }
  return NULL;
  }

/*FUNCTION*/
char *cgi_UserAgent(pCgiObject pCO
  ){

  switch( pCO->fInterface ){
    case CGI_INTERFACE_CGI:
      return _getenv("HTTP_USER_AGENT");
#ifdef WIN32
    case CGI_INTERFACE_ISAPI:
      return NULL;
#endif
    case CGI_INTERFACE_NSAPI:
      return NULL;
    case CGI_INTERFACE_FCGI:
      return NULL;
    case CGI_INTERFACE_DEBUG:
      return pCO->pDebugInfo->UserAgent;
    }
  return NULL;
  }

/*POD
=section ------------------------------------------------------------------------------------------
CUT*/

/*POD
=section Buffer
=H Input handling buffer

CGI input handling is a sophisticated task especially when upload data
in form of multi-part data is handled. To handle the data efficiently a
buffer is implemented in this module. The buffer is a piece of memory
pointed by the field T<pszBuffer>. The actual size of the buffer is given
by the field T<cbBuffer>. The first T<cbFill> characters are actually
filled in the buffer, the rest of the buffer should be treated as garbage.
The start characters of the buffer upto T<lBufferPosition> should be
treated as already removed from the buffer. The "string" in the buffer is
binary and is NOT zchar terminated.

Not all the functions handle the buffer exactly the same way, but they are
consistent as they cooperate.

There are simple operations that the program performs with the buffer.
CUT*/

/*POD
=section ResizeBuffer
=H Resize the object buffer

The cgi object buffer is resized to accomodate at least T<lNewSize> bytes.
If the buffer is already larger it just returns. If this is smaller then
new space is allocated and the valid characters are copied from the start of the old buffer
up to T<cbFill> characters.

If there is not enough memory the buffer is untouched and the return value is
zero. On success the return value is 1.

/*FUNCTION*/
int cgi_ResizeBuffer(pCgiObject pCO,
                     unsigned long lNewSize
  ){
/*noverbatim
CUT*/
  char *s;

  if( lNewSize <= pCO->cbBuffer )return 1;

  s = (char *)pCO->pszBuffer;
  pCO->pszBuffer = ALLOC(lNewSize);
  if( pCO->pszBuffer == NULL ){
    pCO->pszBuffer = (unsigned char *)s;
    return 0;
    }
  if( s )
    memcpy(pCO->pszBuffer,s,pCO->cbFill);
  pCO->cbBuffer = lNewSize;
  if( s )
    FREE(s);
  return 1;
  }

/*POD
=section FillBuffer
=H Fill the buffer

Fill the buffer reading the input until the buffer is full or
the end of the file is reached. The buffer is NOT zchar terminated,
only T<cbFill> contains the number of characters in the buffer.

The function returns the number of characters read. This is zero
if EOF is reached or if the buffer is full.

/*FUNCTION*/
long cgi_FillBuffer(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  int ch;
  long lchcount;

  lchcount = 0;
  while( pCO->cbFill < pCO->cbBuffer ){
    ch = GETCHAR();
    if( ch == EOF )return lchcount;
    lchcount ++;
    pCO->pszBuffer[pCO->cbFill++] = ch;
    }
  return lchcount;
  }

/*POD
=section ShiftBuffer
=H Remove characters from buffer

Remove the first T<nch> characters from the buffer and shift the
rest B<valid> characters to the start of the buffer.

Also the field T<lBufferPosition> is moved with the characters,
so it will point to the same character as before the move or
to the start of the buffer if the pointed character is shifted off the
buffer.

The field T<cbFill> is also corrected.

/*FUNCTION*/
void cgi_ShiftBuffer(pCgiObject pCO,
                     unsigned long nch
  ){
/*noverbatim
CUT*/
  unsigned long i,j;

  if( nch == 0 )return;
  for( i=0, j=nch ; j < pCO->cbFill ; i++, j++ )
    pCO->pszBuffer[i] = pCO->pszBuffer[j];
  if( pCO->cbFill > nch )
    pCO->cbFill -= nch;
  else
    pCO->cbFill = 0;
  if( pCO->lBufferPosition > nch )
    pCO->lBufferPosition -= nch;
  else
    pCO->lBufferPosition = 0;
  return;
  }

/*POD
=section NormalizeBuffer
=H Remove characters already passed from buffer

Remove the characters from the buffer that has already been passed
and are before the position T<lBufferPosition>.

This function simply calls R<ShiftBuffer> to shift T<lBufferPosition>
characters off from the buffer.

/*FUNCTION*/
void cgi_NormalizeBuffer(pCgiObject pCO
  ){
/*noverbatim
CUT*/

  if( pCO->lBufferPosition == 0 )return;
  cgi_ShiftBuffer(pCO,pCO->lBufferPosition);
  }

/*POD
=section SkipAfterBoundary
=H Read up to and after the next boundary

This function reads the input until the next boundary is found.

Return 0 if the boundary was found and the next character is after the
boundary. Return error code if the end of the file has been reached without
finding a boundary or the memory allocation failed.

The function uses the buffer to store the input characters and leaves the
buffer filled and normalized when returns. EOF is recognized when the
physical EOF is reached (the input function returns EOF) or when the
boundary string followed by -- is found.

/*FUNCTION*/
long cgi_SkipAfterBoundary(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  unsigned long i;

  /* Assure that the buffer is large enough to hold the --boundary and the
     CR/LF at the end of the line                                            */
  if( !cgi_ResizeBuffer(pCO,pCO->cbBoundary+4) )return CGI_ERROR_MEMORY_LOW; /* if memory fails */

  do{
    for( i=0 ; i+1 < pCO->cbFill ; i++ ){
      if( pCO->pszBuffer[i] == '-' && pCO->pszBuffer[i+1] == '-'){
        cgi_ShiftBuffer(pCO,i+2);
        cgi_FillBuffer(pCO);

        /* the buffer is large enough, therefore this happens only if we have reached the end
           of the file and there is not enough characters left to match the boundary */
        if( pCO->cbFill < pCO->cbBoundary )return CGI_ERROR_EOF;

        /* we had -- but the following characters do not match the boundary string */
        if( memcmp(pCO->pszBuffer,pCO->pszBoundary,pCO->cbBoundary) )
          continue;
        else{
          /* we have found a boundary string */
          if( (pCO->cbBoundary < pCO->cbFill && pCO->pszBuffer[pCO->cbBoundary] == '-') &&
              (pCO->cbBoundary+1 < pCO->cbFill && pCO->pszBuffer[pCO->cbBoundary+1] == '-' ) ){
            /* --boundary-- means EOF*/
            return CGI_ERROR_EOF;
            }
          if( (pCO->cbBoundary < pCO->cbFill && pCO->pszBuffer[pCO->cbBoundary] != CR) || 
              (pCO->cbBoundary+1 < pCO->cbFill && pCO->pszBuffer[pCO->cbBoundary+1] != LF) ){
            /* --boundary is not followed by CR/LF, and it should be unless we have reached end of file */
            continue;
            }
          if( !(pCO->cbBoundary < pCO->cbFill) )return CGI_ERROR_EOF;/* we have reached the end of file */
          cgi_ShiftBuffer(pCO,pCO->cbBoundary+2);/* boundary + CR/LF */
          cgi_FillBuffer(pCO);
          if( pCO->cbFill == 0 )return CGI_ERROR_EOF;/* we have reached end of file */
          pCO->lBufferPosition = 0;
          return 0;
          }
        }/*if*/
      }/*for*/
    /* we have reached the end of the buffer and did not find anything that looks like a
       boundary. Now drop the buffer and fill it again and go on. */
    cgi_EmptyBuffer(pCO);
    /* if there is no more characters to process, we have reached the end of the file. */
    if( ! cgi_FillBuffer(pCO) )return CGI_ERROR_EOF;
    }while(1);
  }

/*POD
=section GetNextByte
=H Get the next byte from the current part

Get the next byte from the input buffer when reading binary data of a multipart
upload. 

This function automatically gets a byte from the input buffer and fills the buffer
with new data when neccesary. This function should only be used when reading the
multipart data bytes. When the reading reaches a boundary the function returns EOF.

Calling this function after getting EOF does not step over the boundary, the function
will stay at the position on the input stream and return EOF many times. You have to call
R<SkipAfterBoundary> to get after the boundary and start to read the next header.

/*FUNCTION*/
int cgi_GetNextByte(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  
  /* Assure that the buffer is large enough to hold the boundary and the
     CR/LF at the end and at the start of the line
  */
  if( !cgi_ResizeBuffer(pCO,pCO->cbBoundary+6) )return EOF; /* if memory fails */

  /* if the buffer is empty and we can not get more characters */
  if( pCO->cbFill == 0  && cgi_FillBuffer(pCO) == 0 )return EOF;

  /* After these two lines we will have at least two characters in the buffer
     after the position lBufferPosition to check --boundary */
  if( pCO->lBufferPosition + 4 > pCO->cbFill )cgi_NormalizeBuffer(pCO);
  if( pCO->cbFill == 0  && cgi_FillBuffer(pCO) == 0 )return EOF;
  if( pCO->cbFill < 4 )cgi_FillBuffer(pCO); /* we should have at least four (CR/LF--) characters */
//  if( pCO->cbFill < 4 )DebugBreak();


  if( pCO->pszBuffer[pCO->lBufferPosition] == CR &&
      pCO->lBufferPosition+1 < pCO->cbFill && /* this may be false only if there is no character on the input enough */
      pCO->pszBuffer[pCO->lBufferPosition+1] == LF &&
      pCO->lBufferPosition+2 < pCO->cbFill && 
      pCO->pszBuffer[pCO->lBufferPosition+2] == '-' &&
      pCO->lBufferPosition+3 < pCO->cbFill &&
      pCO->pszBuffer[pCO->lBufferPosition+3] == '-' ){
    /* do the check for the boundary */
    cgi_NormalizeBuffer(pCO);

    /* the buffer is large enough, therefore this happens only if we have reached the end
       of the file and there is not enough characters left to match the boundary */
    if( pCO->cbFill >= pCO->cbBoundary+4 &&
        !memcmp(pCO->pszBuffer+4,pCO->pszBoundary,pCO->cbBoundary) &&
        (
            (pCO->pszBuffer[pCO->cbBoundary+4] == CR && pCO->pszBuffer[pCO->cbBoundary+5] == LF )
          ||
            (pCO->pszBuffer[pCO->cbBoundary+4] == '-' && pCO->pszBuffer[pCO->cbBoundary+5] == '-' )
        )
      )return EOF;
    }
  return pCO->pszBuffer[pCO->lBufferPosition++];
  }

/*POD
=section GetNextChar
=H Get the next character from the current part

Get the next character from the input stream via the buffer.

This function should be used when getting the next character
during a normal POST operation. This function simply fills the buffer
when neccesary and returns the next character until EOF. When EOF is
reached the function returns EOF.

No multipart handling or anything like that.

/*FUNCTION*/
int cgi_GetNextChar(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  
  if( pCO->cbBuffer == 0  && !cgi_ResizeBuffer(pCO,pCO->lBufferIncrease) )return EOF; /* if memory fails */

  /* if the buffer is empty and we can not get more characters */
  if( pCO->cbFill == 0  && cgi_FillBuffer(pCO) == 0 )return EOF;

  if( pCO->lBufferPosition >= pCO->cbFill ){
    cgi_EmptyBuffer(pCO);
    if( cgi_FillBuffer(pCO) == 0 )return EOF;
    }
  return pCO->pszBuffer[pCO->lBufferPosition++];
  }

/*POD
=section ReadHeader
=H Read header information until CR/LF/CR/LF sequence is found

This function is used to read the header information of a
multipart form data encoded upload. When the multipart form
data format is used each part has a header after the preceeding
boundary string separated by double CR/LF from the body of the part.

Return 0 if OK,

error code if error occured.

The argument T<pHeader> should point to a pointer (not the pointer, but
the address of the pointer). The pointer that the argument points to should
be initialized with NULL (or point to a list which is appended to the result list).

The pointer will point to the head of the header list upon return.

/*FUNCTION*/
long cgi_ReadHeader(pCgiObject pCO,
                   pSymbolList *pHeader
  ){
/*noverbatim
CUT*/
  unsigned long i,j,k,w;
  char *pszHeader,*pszValue;
  char *pszHeaderBuffer;

  cgi_NormalizeBuffer(pCO);
  /* search the CR/LF/CR/LF sequence and set i to point to the first CR 
     increase the buffer to hold all the header. */
  i = 0;
  do{
    if( i+4 >= pCO->cbFill ){/* we want to look 4 characters ahead */
      if( pCO->cbFill == pCO->cbBuffer )/* the buffer is full we have to increase it */
        if( pCO->cbBuffer+pCO->lBufferIncrease > pCO->lBufferMax )return CGI_ERROR_BUFFER_OVERFLOW;
        if( !cgi_ResizeBuffer(pCO,pCO->cbBuffer+pCO->lBufferIncrease) )return CGI_ERROR_MEMORY_LOW;
      if( cgi_FillBuffer(pCO) == 0 )return CGI_ERROR_ILLF_MULTI1;/* The buffer is not full, it is EOF */
      }
    if( pCO->pszBuffer[i+0] == CR &&
        pCO->pszBuffer[i+1] == LF &&
        pCO->pszBuffer[i+2] == CR &&
        pCO->pszBuffer[i+3] == LF )break;
    i++;
    }while(1);

  /* Now convert continuation lines of the header. */
  for( j = 0 ; j < i ; j++ ){
    if( pCO->pszBuffer[j+0] == CR &&
        pCO->pszBuffer[j+1] == LF &&
        isspace(pCO->pszBuffer[j+2]) ){
      pCO->pszBuffer[j] = ' ';
      /* search the first non space on the continuation line */
      for( k=j+3 ; k < i && isspace(pCO->pszBuffer[k]) ; k++ )continue;
      /* pull down the rest */
      for( w=j+1 ; k < pCO->cbFill ; w++, k++ )
        pCO->pszBuffer[w] = pCO->pszBuffer[k];
      /* correct cbFill and i */
      pCO->cbFill -= k-w;
      i -= k-w;
      }
    }

  pszHeaderBuffer = ALLOC(sizeof(char)*i+1);/* the final terminating zero is the +1 */
  if( pszHeaderBuffer == NULL )return CGI_ERROR_MEMORY_LOW;
  memcpy(pszHeaderBuffer,pCO->pszBuffer,i+1);/* shift off the header and the CR/LF/CR/LF */
  cgi_ShiftBuffer(pCO,i+4);

  k = 0;
  while( k<=i ){
    pszHeader = pszHeaderBuffer+k;
    while( k<=i && pszHeaderBuffer[k] != ':' )k++;
    if( !(k<=i) )return CGI_ERROR_ILLF_MULTI2;
    pszHeaderBuffer[k] = (char)0;/* terminate the header name */
    k++;
    while( k<=i && isspace(pszHeaderBuffer[k]) )k++;
    if( !(k<=i) )return CGI_ERROR_ILLF_MULTI3;
    pszValue = pszHeaderBuffer+k;
    while( k<=i && pszHeaderBuffer[k] != CR )k++;
    if( !(k<=i) )return CGI_ERROR_ILLF_MULTI4;
    pszHeaderBuffer[k] = (char)0;/* terminate the header value */
    *pHeader = ALLOC(sizeof(SymbolList));
    if( *pHeader == NULL )return CGI_ERROR_ILLF_MULTI5;
    (*pHeader)->symbol = pszHeader;
    (*pHeader)->value  = pszValue;
    (*pHeader)->fp  = NULL;
    (*pHeader)->file = NULL;
    (*pHeader)->next = NULL;
    pHeader = &((*pHeader)->next);
    if( !(k<=i) )return 0;
    k++;
    while( k<=i && (pszHeaderBuffer[k] == CR || pszHeaderBuffer[k] == LF) )k++;
    }
  return 0;
  }

/*POD
=section ResizeThisBuffer
=H Resize a buffer other than the CGI input buffer

This function is to be used to allocate a new buffer or
to resize an existing buffer. The argument T<ppszBuffer>
points to the old buffer, T<plOldSize> points to the size
of the buffer. A new memory is allocated, the old content
is copied to the new location, and the size of the buffer
is set to the allocated value. The return value is true
upon success and false if memory allocation fails.

/*FUNCTION*/
int cgi_ResizeThisBuffer(pCgiObject pCO,
                         char **ppszBuffer,
                         long *plOldSize,
                         long lNewSize
  ){
/*noverbatim
CUT*/
  char *s;

  if( lNewSize <= *plOldSize )return 1;

  s = *ppszBuffer;
  *ppszBuffer = ALLOC(lNewSize);
  if( *ppszBuffer == NULL ){
    *ppszBuffer = s;
    return 0;
    }
  memcpy(*ppszBuffer,s,*plOldSize);
  *plOldSize = lNewSize;
  if( s )
    FREE(s);
  return 1;
  }

/*POD
=section FillSymbolAndFile
=H Fill the symbol and file field of a header struct

This aux function gets a string that stands in the header field
T<Content-Disposition> of a part of a multi part form data upload
and fills the T<symbol> and T<file> field of the structure that holds
the data of the actual POST parameter. If the T<name=> is missing the
filed T<symbol> is untouched. If the T<file=> is missing the field
T<file> remains untouched.

There is no return value. This function is used only at a single location
and is a separate function to ease readability.
/*FUNCTION*/
void cgi_FillSymbolAndFile(pCgiObject pCO,
                           char *pszContentDisposition,
                           pSymbolList pHeader
  ){
/*noverbatim
CUT*/
  char *s,*r;
  long Len;

  if( pszContentDisposition == NULL )return;

  s = pszContentDisposition;
  while( *s && memcmp(s,"name=",5) )s++;
  if( !*s )goto HandleFileName;
  s += 5;
  while( *s && isspace(*s) )s++;
  if( *s == '\"' ){
    s++;
    r = s;
    while( *r && *r != '\"' )r++;
    }else{
    r = s;
    while( *r && *r != ';' )r++;
    }
  Len = r-s;
  pHeader->symbol = ALLOC(Len+1);
  if( pHeader->symbol == NULL )return;
  memcpy(pHeader->symbol,s,Len);
  pHeader->symbol[Len] = (char)0;

HandleFileName:;
  s = pszContentDisposition;
  while( *s && memcmp(s,"filename=",9) )s++;
  if( !*s )return;
  s += 9;
  while( *s && isspace(*s) )s++;
  if( *s == '\"' ){
    s++;
    r = s;
    while( *r && *r != '\"' )r++;
    }else{
    r = s;
    while( *r && *r != ';' )r++;
    }
  Len = r-s;
  pHeader->file = ALLOC(Len+1);
  if( pHeader->file == NULL )return;
  memcpy(pHeader->file,s,Len);
  pHeader->file[Len] = (char)0;
  }

/*POD
=section GetMultipartParameters
=H Get parameters from an upload

This function reads the configured input and stores
the POST parameters in case the content type is
multipart form data.

The data read is stored in the into a linked list
pointed by the field T<pPostParameters>.

The files uploaded are stored in temporary file. The
temporary file is opened, the content is written into them,
and the file pointer stored in the field T<fp> is moved to the
start of the file. Closing this pointer using T<closey is
supposed to delete the temporary file according to POSIX.
/*FUNCTION*/
long cgi_GetMultipartParameters(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  char *s,*pszContentDisposition;
  unsigned long i;
  int ch,lErrorCode;
  pSymbolList pThisHeader;
  pSymbolList *pParamp;
  char *pszBuffer;
  unsigned long cbBuffer;

  /* there can be some ?a=1&b=2 part after the URL even when POST is used */
  cgi_GetGetParameters(pCO);

  s = cgi_ContentType(pCO) + 19;/* after multipart-form-data */
  while( *s && memcmp(s,"boundary=",9) )s++;
  if( *s ){
    pCO->pszBoundary = s+9;
    pCO->cbBoundary = strlen(pCO->pszBoundary);
    if( (lErrorCode=cgi_SkipAfterBoundary(pCO)) == CGI_ERROR_EOF )return CGI_ERROR_ILLF_MULTI6; /* EOF ? A zero part-for-data ?*/
    if( lErrorCode == CGI_ERROR_MEMORY_LOW )return CGI_ERROR_MEMORY_LOW;
    }else{
    /* the content type does not contain the boundary
       try to guess the boundary using the first line
       of the content that is supposed to hold the boundary */
    cgi_ResizeBuffer(pCO,pCO->lBufferIncrease);
    i = 0;
    do{
      if( ! cgi_FillBuffer(pCO) )
        /* we have read the whole input and no any CR/LF was found */
        return CGI_ERROR_ILLF_MULTI7;

      while( i >= pCO->cbFill-1 ){
        /* return if buffer grew too large or no more memory is available */
        if( pCO->cbBuffer+pCO->lBufferIncrease > pCO->lBufferMax )return CGI_ERROR_BUFFER_OVERFLOW;
        if( !cgi_ResizeBuffer(pCO,pCO->cbBuffer+pCO->lBufferIncrease) )return CGI_ERROR_MEMORY_LOW;
        }
      while( i < pCO->cbFill-1 ){
        if( pCO->pszBuffer[i] == CR && pCO->pszBuffer[i+1] == LF ){
          pCO->pszBoundary = ALLOC(i-2);
          if( pCO->pszBoundary == NULL )return CGI_ERROR_MEMORY_LOW;
          pCO->cbBoundary = i-2;
          memcpy(pCO->pszBoundary,pCO->pszBuffer+2,i);
          cgi_ShiftBuffer(pCO,i+2);
          goto OUTER_BREAK;
          }
        i++;
        }
      }while(1);
OUTER_BREAK:;
    }
  /* now we have the boundary and are after the first boundary */
  cgi_ResizeBuffer(pCO,pCO->lBufferIncrease);
  pszBuffer = NULL;
  cbBuffer = 0L;
  pParamp = &(pCO->pPostParameters);
  while( 1 ){
    pThisHeader = NULL;
    if( lErrorCode = cgi_ReadHeader(pCO,&pThisHeader) )return lErrorCode;
    *pParamp = ALLOC(sizeof(SymbolList));
    if( *pParamp == NULL )return CGI_ERROR_MEMORY_LOW;
    (*pParamp)->symbol = NULL;
    (*pParamp)->file = NULL;
    (*pParamp)->value = NULL;
    (*pParamp)->fp  = NULL;
    (*pParamp)->file = NULL;
    (*pParamp)->next = NULL;
    (*pParamp)->pHeaders = pThisHeader;

    pszContentDisposition = cgi_Header(pCO,"Content-Disposition",(*pParamp)->pHeaders);
    /* if there is content disposition field then fill the name and the file fileds. */
    cgi_FillSymbolAndFile(pCO,pszContentDisposition,*pParamp);
    if( (*pParamp)->file ){
      (*pParamp)->fp = tmpfile();
      if( (*pParamp)->fp == NULL )return CGI_ERROR_MEMORY_LOW;
      i = 0;
      while( (ch=cgi_GetNextByte(pCO)) != EOF ){
        putc(ch,(*pParamp)->fp);
        if( ++i > pCO->lFileMax )return CGI_ERROR_FILEMAX;
        }
      (*pParamp)->len = i;
      fseek((*pParamp)->fp,0L,SEEK_SET);
      }else{
      cgi_ResizeThisBuffer(pCO,&pszBuffer,&cbBuffer,pCO->lBufferIncrease);
      i = 0;
      while( (ch=cgi_GetNextByte(pCO)) != EOF ){
        while( i >= cbBuffer-1 ){/* -1 to have space for termiating zero */
          if( pCO->lBufferIncrease + cbBuffer > pCO->lBufferMax )return CGI_ERROR_BUFFER_OVERFLOW;
          cgi_ResizeThisBuffer(pCO,&pszBuffer,&cbBuffer,cbBuffer+pCO->lBufferIncrease);
          }
        pszBuffer[i++] = ch;
        }
      pszBuffer[i] = (char)0;
      (*pParamp)->value = ALLOC(i+1);
      if( (*pParamp)->value == NULL )return CGI_ERROR_MEMORY_LOW;
      memcpy((*pParamp)->value,pszBuffer,i+1);
      }
    pParamp = &((*pParamp)->next);
    if( (lErrorCode = cgi_SkipAfterBoundary(pCO)) == CGI_ERROR_EOF ){
      FREE(pszBuffer);
      return 0;
      }

    if( lErrorCode == CGI_ERROR_MEMORY_LOW )return CGI_ERROR_MEMORY_LOW;
    }
  }

/*POD
=section GetGetParameters
=H Get the GET parameters

This function gets the GET parameters and stores them into a linked list
pointed by the field T<pGetParameters>.
/*FUNCTION*/
long cgi_GetGetParameters(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  char *s,*Symbol,*Value;
  long Len;
  pSymbolList *pParamp;

  s = cgi_QueryString(pCO);
  pParamp = &(pCO->pGetParameters);
  while( s && *s ){
    Symbol = s;
    while( *s && *s != '=' && *s != '&' )s++;
    Len = s - Symbol;
    *pParamp = ALLOC(sizeof(SymbolList));
    if( *pParamp == NULL )return CGI_ERROR_MEMORY_LOW;
    (*pParamp)->fp  = NULL;
    (*pParamp)->file = NULL;
    (*pParamp)->value = NULL;
    (*pParamp)->next = NULL;
    (*pParamp)->pHeaders = NULL;
    (*pParamp)->symbol = ALLOC(Len+1);
    if( (*pParamp)->symbol == NULL )return CGI_ERROR_MEMORY_LOW;
    memcpy((*pParamp)->symbol,Symbol,Len);
    (*pParamp)->symbol[Len] = (char)0;
    Len++;
    unescape((*pParamp)->symbol,&Len);
    if( !*s )break;
    if( *s == '=' )s++;
    Value = s;
    while( *s && *s != '&' )s++;
    Len = s - Value;
    (*pParamp)->value = ALLOC(Len+1);
    if( (*pParamp)->value == NULL )return CGI_ERROR_MEMORY_LOW;
    memcpy((*pParamp)->value,Value,Len);
    (*pParamp)->value[Len] = (char)0;
    Len++;
    unescape((*pParamp)->value,&Len);
    pParamp = &((*pParamp)->next);
    if( *s )s++;
    }
  return 0;
  }

/*POD
=section GetPostParameters
=H Get the POST parameters

This function gets the POST parameters and stores them into a linked list
pointed by the field T<pPostParameters>.
/*FUNCTION*/
long cgi_GetPostParameters(pCgiObject pCO
  ){
/*noverbatim
CUT*/
  char *s,*Symbol,*Value;
  long Len;
  pSymbolList *pParamp;

  /* there can be some ?a=1&b=2 part after the URL even when POST is used */
  cgi_GetGetParameters(pCO);

  if( ! cgi_ResizeBuffer(pCO,pCO->lBufferIncrease) )return CGI_ERROR_MEMORY_LOW;
  /* read all characters into the buffer */
  while( cgi_FillBuffer(pCO) ){
    if( pCO->cbBuffer+pCO->lBufferIncrease > pCO->lBufferMax )return CGI_ERROR_BUFFER_OVERFLOW;
    if( !cgi_ResizeBuffer(pCO,pCO->cbBuffer+pCO->lBufferIncrease) )return CGI_ERROR_MEMORY_LOW;
    }
  s = (char *)pCO->pszBuffer;
  /* terminate the buffer with a zchar. We actually have pCO->lBufferIncrease number of bytes
     to do this */
  s[ pCO->cbFill ] = (char)0;

  pParamp = &(pCO->pPostParameters);
  while( *s ){
    Symbol = s;
    while( *s && *s != '=' && *s != '&' )s++;
    Len = s - Symbol;
    *pParamp = ALLOC(sizeof(SymbolList));
    if( *pParamp == NULL )return CGI_ERROR_MEMORY_LOW;
    (*pParamp)->fp  = NULL;
    (*pParamp)->file = NULL;
    (*pParamp)->value = NULL;
    (*pParamp)->next = NULL;
    (*pParamp)->pHeaders = NULL;
    (*pParamp)->symbol = ALLOC(Len+1);
    if( (*pParamp)->symbol == NULL )return CGI_ERROR_MEMORY_LOW;
    memcpy((*pParamp)->symbol,Symbol,Len);
    (*pParamp)->symbol[Len] = (char)0;
    Len++;
    unescape((*pParamp)->symbol,&Len);
    if( !*s )break;
    if( *s == '=' )s++;
    Value = s;
    while( *s && *s != '&' )s++;
    Len = s - Value;
    (*pParamp)->value = ALLOC(Len+1);
    if( (*pParamp)->value == NULL )return CGI_ERROR_MEMORY_LOW;
    memcpy((*pParamp)->value,Value,Len);
    (*pParamp)->value[Len] = (char)0;
    Len++;
    unescape((*pParamp)->value,&Len);
    pParamp = &((*pParamp)->next);
    if( *s )s++;
    }
  return 0;
  }
