/* FILE: curlintrfc.c

This file contains code the CURL interface

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

NTLIBS: libcurl.lib ws2_32.lib
UXLIBS: -lcurl -lssl -lcrypto -ldl -lc

*/
#include <stdio.h>
#include <time.h>
#include <curl/curl.h>
#include "../../basext.h"

/**
The CURL module provides easy and still powerful functions to perform various network
oriented actions that are based on URL to access data. For example CURL can perform
getting data via HTTP, HTTPS from web pages, POST data to web pages, transer file to
and from servers via FTP and so on.

To use the module the program has to import the BASIC "header" file named T<curl.bas>. This
should be done using the command import:

=verbatim
import curl.bas
=noverbatim

Note that there are no double quotes around the file name to include the definition of the
C implemented curl functions from the module interface header files include directory.

There are only a few functions defined in this file. The first function that an application
has to call is T<curl::init()> to get a handle to a connection. A single program can use
several connections simultaneous, tough currently there is no possibility to download large
files asynchronous.

Follwing the initialization the program has to set several options calling T<curl::option>, and then
should call the function T<curl::perform> to actually perform the download/upload or other network
action. When a connection is not used anymore the function T<curl::finish> can be called, though this
function is executed automatically for each connection when the interpreter exists. 

=verbatim
import curl.bas
ON ERROR GOTO CURL_ERROR
CURL = curl::init()
curl::option CURL,"URL","http://scriptbasic.com/html/index.html"
curl::option CURL,"FILE","C:\\ScriptBasicMirror\\html\\index.html"
curl::perform CURL
curl::finish
STOP
CURL_ERROR:
PRINT "Some error happened while trying to download ScriptBasic home page. The error message is:\n"
PRINT curl::error()
STOP
=noverbatim

For more information on other functions read the appropriate chapters.

The name CURL is made of the name of the programming language C in which CURL is implemented
and from the word URL. This text that you are reading is the documentation of the ScriptBasic
implementation of CURL. This implementation includes an interface module that containc C coded
BASIC callable function that map most (or some) of the CURL library functions. In other words
using this program yu can access most (or some) of the functions that the CURL library provides.

Altough the CURL library is quite flexible, there are some options that are altered in this
module to ease the use for BASIC programmers. For example the option POSTFIELDS accepts zero 
terminated string by default, because 1.) POST data usually does not contain zero character and 2.)
this is the easy way for C programmers. On the other hand there is a possibility, though more complex
to pass string and length to the option. Because all ScriptBasic strings are arbitrary binary data and
not zero character terminated strings this second method is followed.

Some of the options on the C level accept T<long> numbers from a finite set and CURL C header files define
T<enum> types for the purpose. In these cases the ScriptBasic interface requires strings being specified
by the BASIC program as option and convert to T<long> as appropriate to CURL.

The option names in the original CURL interface are defined in T<enum> and thus options are defined
as T<long> constants. The BASIC interface works with strings, and converts the names specified as
strings to their appropriate value. This is (I assume) simpler for the BASIC programmers and on the other
hand induces only slight overhead as compared to network transfer times.

*/


/*
Here you have to list all the CURL options that get char * parameter and are handles
transparently passed to CURL.

The macro is (re)defined
- in the string parameter declaration,
- initializing the parameters to NULL
- during parameter setting
- and release of memory when finish a curl handle.

That helps you to maintain new options because this is the only place where the name of
the new option is to be present.
*/
#define STRING_OPTIONS \
STRING_OPTION(PROXY)\
STRING_OPTION(URL)\
STRING_OPTION(USERPWD)\
STRING_OPTION(PROXYUSERPWD)\
STRING_OPTION(RANGE)\
STRING_OPTION(REFERER)\
STRING_OPTION(USERAGENT)\
STRING_OPTION(FTPPORT)\
STRING_OPTION(COOKIE)\
STRING_OPTION(SSLCERT)\
STRING_OPTION(SSLCERTPASSWD)\
STRING_OPTION(COOKIEFILE)\
STRING_OPTION(CUSTOMREQUEST)\
STRING_OPTION(INTERFACE)\
STRING_OPTION(KRB4LEVEL)\
STRING_OPTION(WRITEINFO)\
STRING_OPTION(CAINFO)\
STRING_OPTION(RANDOM_FILE)\
STRING_OPTION(EGDSOCKET)\

typedef struct _CurlConnection {
  CURL          *myConnection;
  FILE          *fp;        /* file pointer to file where we read from or store the result             */
  FILE          *hfp;       /* header file pointer where the header is stored                          */
  char          errorbuffer[CURL_ERROR_SIZE];
                            /* buffer to store the error messages from the underlying CURL system      */
  int           fWasPerform;
  char          *pszBuffer; /* buffer to store the downloaded file when not saved to disk              */
  unsigned long cbBuffer;   /* characters in the buffer                                                */
  unsigned long dwBuffer;   /* allocated size of the buffer                                            */
  unsigned long dwStepBf;   /* increase step when fubber is full                                       */
  char          *pszInText; /* the text to upload                                                      */
  unsigned long dwInText;   /* the length of the text                                                  */
  unsigned long cbInText;   /* the number of characters already sent up                                */

  pSupportTable pSt;        /* this is needed to get back to the interpreter
                               environment in the function buffercollect                               */


#define STRING_OPTION(X) char *psz##X;
STRING_OPTIONS
#undef STRING_OPTION

  char *pszPOSTFIELDS;
  struct _CurlConnection *next,*prev;
  struct curl_slist *pHeaderLines;
  struct curl_slist *pQuote;
  struct curl_slist *pPostQuote;
  struct HttpPost *firstitem, *lastitem;
  } CurlConnection,  *pCurlConnection;

typedef struct _CurlClass {
  void *HandleArray;
  pCurlConnection pFirstConnection;
  } CurlClass,*pCurlClass;

SUPPORT_MULTITHREAD

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_SHUTDOWN
  curl_global_cleanup();
besEND

besSUB_START
  pCurlClass pCurl;
  /* This macro increments the thread counter used to help to keep this library in memory 
     even those times when the process is idle and no ScriptBasic threads are running. */
  INC_THREAD_COUNTER
  /* A process level initialization of the module should take place in this function because
     the call-back support functions are not accessible when the  function DllMan or _init is
     called. On the other hand it has to initialize only once.
  */
  INITLOCK /* lock the init mutex */
  if( iFirst ){ /* if it was not initialized yet*/

    curl_global_init(CURL_GLOBAL_ALL);

    iFirst = 0;/* successful initialization was done*/
    }
  INITUNLO /* unlock the init mutex */

  besMODULEPOINTER = besALLOC(sizeof(CurlClass));
  if( besMODULEPOINTER == NULL )return 0;
  pCurl = (pCurlClass)besMODULEPOINTER;
  pCurl->pFirstConnection = NULL;
  pCurl->HandleArray = NULL;
  return COMMAND_ERROR_SUCCESS;
besEND

besSUB_FINISH
  pCurlClass pCurl;
  pCurlConnection pCon;

  pCurl = (pCurlClass)besMODULEPOINTER;
  if( pCurl != NULL ){
    for( pCon = pCurl->pFirstConnection ; pCon ; pCon = pCon->next ){
      if( pCon->pHeaderLines ){
        curl_slist_free_all(pCon->pHeaderLines);
        pCon->pHeaderLines = NULL;
        }
      if( pCon->pQuote ){
        curl_slist_free_all(pCon->pQuote);
        pCon->pQuote = NULL;
        }
      if( pCon->pPostQuote ){
        curl_slist_free_all(pCon->pPostQuote);
        pCon->pQuote = NULL;
        }
      if( pCon->firstitem ){
        curl_formfree(pCon->firstitem);
        pCon->firstitem = pCon->lastitem = NULL;
        }
      curl_easy_cleanup(pCon->myConnection);
      }
    besHandleDestroyHandleArray(pCurl->HandleArray);
    }
  return 0;
besEND

#define GET_CURL_HANDLE \
  pCurl = (pCurlClass)besMODULEPOINTER;\
  Argument = besARGUMENT(1);\
  besDEREFERENCE(Argument);\
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;\
  pCon = besHandleGetPointer(pCurl->HandleArray,besGETLONGVALUE(Argument));\
  if( pCon == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;


/**
=section curl_init
=H curl::init

Call this function before any other curl action. This function will return a handle
to the curl session. This handle should be used as first argument to any further curl calls.


Usage:
=verbatim
CURL = curl::init()
=noverbatim

The handle that the function returns is a small integer. However the program should not alter this value in any
way. The good practice is to store the value in a variable and use it as it is without caring about the real
value or type of the content.

If the initialization of the connection can not be performed T<MEMORY_LOW> error occurs.
*/
besFUNCTION(sb_curl_init)
  pCurlClass pCurl;
  pCurlConnection pCon;
  long res;

  pCurl = besMODULEPOINTER;

  /* allocate the space to store the new connection */
  pCon = besALLOC(sizeof(CurlConnection));
  if( pCon == NULL )return COMMAND_ERROR_MEMORY_LOW;

  /* get the connection */
  pCon->myConnection = curl_easy_init();
  if( pCon->myConnection == NULL ){
    besFREE(pCon);
    return COMMAND_ERROR_MEMORY_LOW;
    }

  pCon->fp           = NULL; /* no output file opened                                           */
  pCon->hfp          = NULL; /* no input file opened                                            */
  pCon->fWasPerform  = 0;    /* no perform was done yet (check before calling curl_easy_getinfo */
  pCon->pHeaderLines = NULL; /* no extra header lines                                           */
  pCon->pQuote       = NULL; /* no QUOTE commands for FTP                                       */
  pCon->pPostQuote   = NULL; /* no post QUOTE commands for FTP                                  */
  pCon->firstitem    = NULL; /* POST parameters first item                                      */
  pCon->lastitem     = NULL; /* POST parameters last item                                       */

  pCon->pszBuffer    = NULL; /* no buffer is allocated by default                               */
  pCon->dwBuffer     = 0;    /* as I said: no buffer is allocated by default                    */
  pCon->cbBuffer     = 0;    /* there is zero number of chars in the nonallocated buffer        */
  pCon->dwStepBf     = 0;    /* by default allocate space as bytes come                         */
  pCon->pszInText    = NULL; /* no text is stored for upload                                    */
  pCon->dwInText     = 0;    /* as I said: you know                                             */
  pCon->cbInText     = 0;    /* no bytes were sent from the nonexistent infile text             */

  pCon->pSt          = pSt;

#define STRING_OPTION(X) pCon->psz##X = NULL;
STRING_OPTIONS
#undef STRING_OPTION
  pCon->pszPOSTFIELDS = NULL;

  memset(pCon->errorbuffer,0,CURL_ERROR_SIZE);
  res = curl_easy_setopt(pCon->myConnection, CURLOPT_ERRORBUFFER, pCon->errorbuffer);
  if( res )return 0x00081100+res;

  /* link the new connection tot he head of the list of connections */
  pCon->next               = pCurl->pFirstConnection;
  pCon->prev               = NULL;
  if( pCon->next )
    pCon->next->prev       = pCon;
  pCurl->pFirstConnection  = pCon;

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = besHandleGetHandle(pCurl->HandleArray,pCon);
  return COMMAND_ERROR_SUCCESS;

besEND

static size_t buffersend(char *buffer, size_t size, size_t count, pCurlConnection pCon){
  unsigned long dwChunkSize;

  dwChunkSize = size*count;
  if( dwChunkSize > pCon->dwInText - pCon->cbInText )dwChunkSize = pCon->dwInText - pCon->cbInText;
  memcpy(buffer,pCon->pszInText,dwChunkSize);
  pCon->cbInText += dwChunkSize;
  return dwChunkSize;
  }

/**
=section curl_option
=H curl::option

Call this function to set the various options of the actual curl session.

Usage:
=verbatim
curl::option CURL, "option" [, value]
=noverbatim

The various options that the current version of the module supports are listed in the following sections.
Note that the documentation is mainly copied from the original CURL lib documentation.

Some of the options require a string as a parameter. The strings usually should not contain zero character.
The exception is the option T<POSTFIELDS>, which is handled in a special way to allow the programmer to send
arbitrary binary data as HTTP POST parameter.

The option names in the section titles in this document are followed by one of the words I<string>, I<integer> and I<flag>.

=itemize
=item
Options followed by the word I<string> need string argument. If the option value is not specified zero length
string is used. Because this is nonsense in most cases, you better use a third string argument for these options.
=item
Options followed by the word I<flag> are yes/no type options. Presenting them without value means switching the
option on. To switch such an option off the programmer has to explicitely specify zero value as third argument.
In other words omitting the option value in such a case is the same as specifying T<1> or T<TRUE> or any other
non-zero value. To switch off a I<flag> option you can specify the value as T<FALSE> (which is zero in ScriptBasic)
for more readability.
=verbatim
curl::option CURL,"VERBOSE"
curl::option CURL,"VERBOSE",TRUE
curl::option CURL,"VERBOSE",1
=noverbatim
are the same as well as the oposit
=verbatim
curl::option CURL,"VERBOSE",FALSE
curl::option CURL,"VERBOSE",0
=noverbatim
are the same.
=item
Options followed by the word I<integer> accept integer value. If the value is missing for such an option zero is used.
For example:
=verbatim
curl::option CURL,"RESUME_FROM"
curl::option CURL,"RESUME_FROM",0
=noverbatim
are the same.
=noitemize


The option names are implemented case sensitive in this module, thus you can not use T<"verbose">
instead of T<"VERBOSE">. Also the programmer should type the option names precisely, no mispelling is
tolerated by the program.

=subsection BUFFER_SIZE integer

=verbatim
curl::option CurlHandle,"BUFFER_SIZE",1024
=noverbatim

When a file is downloaded but not stored in a file the function T<curl::perform(CURL)> returns the content
of the downloaded file as a huge string. During the download this string is stored ina temporary buffer. The
size of this buffer increases gradually as more and more bytes come. If there are 10 bytes coming at a time then
the buffer will grow only ten bytes. This also means a new buffer allocation and copying the content of the buffer,
which consumes system resources, especially for large files.

If you happen to know the estimated size of the file, you can set the initial size of the buffer to a huge value using this
option. For example if you know that the file is 1024 bytes, you can set this option as in the example above. In that case
when the first byte comes from the URL the 1024 byte length buffer is increased and when the consecutive bytes come there is
space to store them without reallocating the buffer.

You need not worry about using this buffer when you handle small files, like web pages. If you see performace or memory
shortage problems, then you may consider this option along with the option T<FILE> that helps you store the downloaded file
on disk.

=subsection CAINFO string

=verbatim
curl::option CurlHandle,"CAINFO","c:\\certs\mycert.pem"
=noverbatim

Pass a string file naming holding the certificate to
verify the peer with. This only makes sense when used in combination with the
CURLOPT_SSL_VERIFYPEER option.

=subsection COOKIE string

=verbatim
curl::option CurlHandle,"COOKIE","mycookie=is rather long"
=noverbatim

Pass a string as parameter. It will be used to
set a cookie in the http request. The format of the string should be
T<[NAME]=[CONTENTS];> Where T<NAME> is the cookie name.

=subsection COOKIEFILE string

=verbatim
curl::option CurlHandle,"COOKIEFILE","c:\\WINDOWS\\cookies.txt"
=noverbatim

Pass a string as parameter. It should contain the
name of your file holding cookie data. The cookie data may be in Netscape /
Mozilla cookie data format or just regular HTTP-style headers dumped to a
file.

=subsection CUSTOMREQUEST string

=verbatim
curl::option CurlHandle,"CUSTOMREQUEST","MOVE"
=noverbatim

Pass a string as parameter. It will be user
instead of GET or HEAD when doing the HTTP request. This is useful for doing
DELETE or other more obscure HTTP requests. Don't do this at will, make sure
your server supports the command first.

=subsection FILE

=verbatim
curl::option CurlHandle,"FILE","file_name"
=noverbatim

Use this option to set the file name of the file where the result will be saved. When you set this option
the file is opened and truncated assumed that the program has appropriate privileges. Thus if there was a
file with the name the file will be overwrtitten even if the T<curl::perform> function is not called. The file
is opened, when the option is set and kept opened so long as the connection is "finished" or another
T<INFILE> or T<FILE> option is specified for the connection.

If you donot specify any file to store the downloaded result, then the function T<curl::perform(CURL)>
will return the file as a single huge string.

Comment on internals (if you do not understand what I am talking about, most probably you do not need to):

The underlying CURL library requests an opened file handle passed to the library and a function that
performs the file writing. The ScriptBasic interface gets the name of the file, opens the file, passes
the opened file pointer to the library and specifies the stadard T<fwrite> function to write the file.

The pointer to the function T<fwrite> is taken from the support table, thus is any preloaded module
altered this before setting this option the module function will be used.

The file is opened calling the system function T<fopen> via the ScriptBasic support function calling stacks.
This means that if some module implements a hook function to control file access that will be taken into account
the same way as it is taken into accoung in the case of the BASIC command T<OPEN>.

=subsection FTPPORT string

=verbatim
curl::option CurlHandle,"FTPPORT","222"
=noverbatim

Pass a string as parameter. It will be used to
get the IP address to use for the ftp PORT instruction. The PORT instruction
tells the remote server to connect to our specified IP address. The string may
be a plain IP address, a host name, an network interface name (under unix) or
just a '-' letter to let the library use your systems default IP address.

=subsection HEADERFILE string

=verbatim
curl::option CurlHandle,"HEADERFILE","file_name"
=noverbatim

Use this option to set the file name of the file where the header coming from the server will be saved.
When you set this option the file is opened and truncated assumed that the program has appropriate privileges.
Thus if there was a file with the name the file will be overwrtitten even if the T<curl::perform> function is
not called. The file is opened, when the option is set and kept opened so long as the connection is "finished" 
or another T<HEADERFILE> option is specified for the connection.

Note: You need this option if you want to get the header and the body of a downloaded file into two separate
files. You can easily get the header and the body into a single file if you use the option T<HEADER>. Altough
there is nothing that prevents, I see no real use of the two options aka T<HEADERFILE> and flag T<HEADER>) together.
But you are free to use them together.

Comment on internals (if you do not understand what I am talking about, most probably you do not need to):

The underlying CURL library requests an opened file handle passed to the library and a function that
performs the file writing. The ScriptBasic interface gets the name of the file, opens the file, passes
the opened file pointer to the library and specifies the stadard T<fwrite> function to write the file.

The pointer to the function T<fwrite> is taken from the support table, thus is any preloaded module
altered this before setting this option the module function will be used.

The file is opened calling the system function T<fopen> via the ScriptBasic support function calling stacks.
This means that if some module implements a hook function to control file access that will be taken into account
the same way as it is taken into accoung in the case of the BASIC command T<OPEN>.

=subsection INFILE string

=verbatim
curl::option CurlHandle,"INFILE","file_name"
=noverbatim

Use this option to set the file name of the file where the source is for file upload. When you set this option
the file is opened for reading, thus the file should exist and should be readable for the program. The file
is opened, when the option is set and kept opened so long as the connection is "finished" or another
T<INFILE> or T<FILE> option is specified for the connection.

You cannot use the options T<INTEXT> and T<INFILE> at the same time for the same connection. If you use both
the latter specified will be used.

Comment on internals (if you do not understand what I am talking about, most probably you do not need to):

The underlying CURL library requests an opened file handle passed to the library and a function that
performs the file writing. The ScriptBasic interface gets the name of the file, opens the file, passes
the opened file pointer to the library and specifies the stadard T<fread> function to read the file.

The pointer to the function T<fread> is taken from the support table, thus is any preloaded module
altered this before setting this option the module function will be used.

Curl optionally acepts the size of the file to report to the upload server. The implementation of this
option interfacing automatically gets the size of the file specified in the option and sets the underlying
option.

The file is opened calling the system function T<fopen> via the ScriptBasic support function calling stacks.
This means that if some module implements a hook function to control file access that will be taken into account
the same way as it is taken into accoung in the case of the BASIC command T<OPEN>.

=subsection INTERFACE string

=verbatim
curl::option CurlHandle,"INTERFACE","16.193.68.55"
=noverbatim

Pass a string as parameter. This set the interface name to use as outgoing
network interface. The name can be an interface name, an IP address or a host
name.

=subsection INTEXT string

=verbatim
curl::option CurlHandle,"INTEXT","""This is the content of the file
that we will upload.
"""
=noverbatim

Use this option to set the content of the file for any kind of upload. This option can be used to upload 
small files via HTTP or FTP, which are generated on-the-fly by the BASIC program, and there is no need to
store the files locally.

You cannot use the options T<INTEXT> and T<INFILE> at the same time for the same connection. If you use both
the latter specified will be used.

=subsection KRB4LEVEL string

=verbatim
curl::option CurlHandle,"KRB4LEVEL","confidential"
=noverbatim

Pass a string as parameter. Set the krb4 security level, this also enables
krb4 awareness.  This is a string, 'clear', 'safe', 'confidential' or
'private'.  If the string is set but doesn't match one of these, 'private'
will be used. Set the string to NULL to disable kerberos4. The kerberos
support only works for FTP.

=subsection PROXY string

=verbatim
curl::option CurlHandle,"PROXY","www.my-proxy.com:8080"
=noverbatim

If you need libcurl to use a http proxy to access the outside world, set the
proxy string with this option. To specify port number in this string, append T<:[port]> to
the end of the host name. The proxy string may be prefixed with
T<[protocol]://> since any such prefix will be ignored.

=subsection PROXYPORT integer

=verbatim
curl::option CurlHandle,"PROXYPORT",8080
=noverbatim

Use this option to set the proxy port to use unless it is
specified in the proxy string T<PROXY>.

=subsection PROXYUSERPWD string

=verbatim
curl::option CurlHandle,"PROXYUSERPWD","verhas:m23kkdUT"
=noverbatim

Pass a string as parameter, which should be T<username:password> to use for
the connection to the HTTP proxy. If the password is left out, you will be
prompted for it.

=subsection RANDOM_FILE string

=verbatim
curl::option CurlHandle,"RANDOM_FILE","c:\\WINNT4\pagefile.sys"
=noverbatim

Pass a string to a zero terminated file name. The file will be used to read
from to seed the random engine for SSL. The more random the specified file is,
the more secure will the SSL connection become.

=subsection RANGE string

=verbatim
curl::option CurlHandle,"RANGE","3321-3322"
=noverbatim

Pass a string as parameter, which should contain the specified range you
want. It should be in the format "X-Y", where X or Y may be left out. HTTP
transfers also support several intervals, separated with commas as in
T<"X-Y,N-M">. Using this kind of multiple intervals will cause the 
HTTP server to send the response document in pieces.

=subsection REFERER string

=verbatim
curl::option CurlHandle,"REFERER","http://www.scriptbasic.com"
=noverbatim

Pass a string as parameter. It will be used to
set the T<Referer:> header in the http request sent to the remote server. This
can be used to fool servers or scripts.

=subsection SSLCERT string

=verbatim
curl::option CurlHandle,"SSLCERT","???"
=noverbatim

Pass a string as parameter. The string should be
the file name of your certficicate in PEM format.

=subsection SSLCERTPASSWD string

=verbatim
curl::option CurlHandle,"SSLCERTPASSWD","m23kkdUT"
=noverbatim

Pass a string as parameter. It will be used as
the password required to use the T<SSLCERT> certificate. If the password
is not supplied, you will be prompted for it.

=subsection URL string

=verbatim
curl::option CurlHandle,"URL","http://curl.haxx.se"
=noverbatim

The actual URL to deal with. 

NOTE: this option is required to be set before T<curl::perform()> is called.

=subsection USERAGENT string

=verbatim
curl::option CurlHandle,"USERAGENT","CURL 7.9.5 with SB Interface; v10b29"
=noverbatim

Pass a string as parameter. It will be used to
set the T<User-Agent:> header in the http request sent to the remote server. This
can be used to fool servers or scripts.

=subsection USERPWD string

=verbatim
curl::option CurlHandle,"USERPWD","verhas:m23kkdUT"
=noverbatim

Pass a string as parameter, which should be T<username:password> to use for
the connection. If the password is left out, you will be prompted for it.

=subsection WRITEINFO string

=verbatim
curl::option CurlHandle,"WRITEINFO",""
=noverbatim

Pass a string as parameter. It will be used to
report information after a successful request.

As report and progress callback is not implemented in ScriptBasic CURL module, there
is not much use of this option.

=subsection EGDSOCKET string

=verbatim
curl::option CurlHandle,"EGDSOCKET","\\.\\edg"
=noverbatim

Pass a string path name to the Entropy Gathering Daemon
socket. It will be used to seed the random engine for SSL.

=subsection POSTFIELDS string

=verbatim
curl::option CurlHandle,"POSTFIELDS","name=Edmund+Nielsen+Bohr&address=Downing+street+11"
=noverbatim

Pass a string as parameter, which should be the full data to post in a HTTP
post operation.

Note that CURL library also implements the option T<CURLOPT_POSTFIELDSIZE> for C programmers.
That option is automatically called by the interface function, thus any binary BASIC string
can be used as post parameter.

=subsection HTTPPROXYTUNNEL flag

=verbatim
curl::option CurlHandle,"HTTPPROXYTUNNEL"
=noverbatim

Set the parameter to get the library to tunnel all non-HTTP
operations through the given HTTP proxy. Do note that there is a big
difference to use a proxy and to tunnel through it. If you don't know what
this means, you probably don't want this tunnel option.

=subsection VERBOSE flag

=verbatim
curl::option CurlHandle,"VERBOSE"
=noverbatim

Set the parameter to get the library to display a lot of verbose
information about its operations. Very useful for libcurl and/or protocl
debugging and understanding.

=subsection NOPROGRESS flag

=verbatim
curl::option CurlHandle,"NOPROGRESS"
=noverbatim

Setting the parameter tells the library to shut of the built-in progress meter
completely. (NOTE: future versions of the lib is likely to not have any
built-in progress meter at all).

=subsection HEADER flag

=verbatim
curl::option CurlHandle,"HEADER"
=noverbatim

Setting the parameter tells the library to include the header in the
output. This is only relevant for protocols that actually has a header
preceeding the data (like HTTP).

=subsection NOBODY flag

=verbatim
curl::option CurlHandle,"NOBODY"
=noverbatim

Setting the parameter tells the library to not include the body-part in the
output. This is only relevant for protocols that have a separate header and
body part.

=subsection FAILONERROR flag

=verbatim
curl::option CurlHandle,"FAILONERROR"
=noverbatim

Setting the parameter tells the library to fail silently if the HTTP code
returned is equal or larger than 300. The default action would be to return
the page normally, ignoring that code.

=subsection UPLOAD flag

=verbatim
curl::option CurlHandle,"UPLOAD"
=noverbatim

Setting the parameter tells the library to prepare for an upload. The
option T<INFILE> is also interesting for uploads.

=subsection POST flag

=verbatim
curl::option CurlHandle,"POST"
=noverbatim

Setting the parameter tells the library to do a regular HTTP post. This is a
normal T<application/x-www-form-urlencoded> kind, which is the most commonly used
one by HTML forms. See the option T<POSTFIELDS> option for how to specify the
data to post.

=subsection CRLF flag

=verbatim
curl::option CurlHandle,"CRLF"
=noverbatim

Convert unix newlines to CRLF newlines on FTP uploads.

=subsection FTPLISTONLY flag

=verbatim
curl::option CurlHandle,"FTPLISTONLY"
=noverbatim

Setting the parameter tells the library to just list the names of an ftp
directory, instead of doing a full directory listin that would include file
sizes, dates etc.

=subsection FTPAPPEND flag

=verbatim
curl::option CurlHandle,"FTPAPPEND"
=noverbatim

Setting the parameter tells the library to append to the remote file instead of
overwrite it. This is only useful when uploading to a ftp site.

=subsection NETRC flag

=verbatim
curl::option CurlHandle,"NETRC"
=noverbatim

The parameter tells the library to scan your
T<~/.netrc>
file to find user name and password for the remote site you are about to
access. Do note that curl does not verify that the file has the correct
properties set (as the standard unix ftp client does), and that only machine
name, user name and password is taken into account (init macros and similar
things aren't supported).

=subsection FOLLOWLOCATION flag

=verbatim
curl::option CurlHandle,"FOLLOWLOCATION"
=noverbatim

Setting the parameter tells the library to follow any Location: header that the
server sends as part of a HTTP header. NOTE that this means that the library
will resend the same request on the new location and follow new Location:
headers all the way until no more such headers are returned.

=subsection TRANSFERTEXT flag

=verbatim
curl::option CurlHandle,"TRANSFERTEXT"
=noverbatim

Setting the parameter tells the library to use ASCII mode for ftp transfers,
instead of the default binary transfer. For LDAP transfers it gets the data in
plain text instead of HTML and for win32 systems it does not set the stdout to
binary mode. This option can be useable when transfering text data between
system with different views on certain characters, such as newlines or
similar.

=subsection CLOSEPOLICY

=verbatim
curl::option CurlHandle,"CLOSEPOLICY"
=noverbatim

This option sets what policy libcurl should use when the
connection cache is filled and one of the open connections has to be closed to
make room for a new connection. This must be T<OLD> or T<FRESH>. If you specify T<OLD>
as argument libcurl close the
oldest connection, the one that was created first among the ones in the
connection cache. If you specify T<FRESH> libcurl close the
connection that was least recently used, that connection is also least likely
to be capable of re-use.

Example

=verbatim
curl::option CURL,"CLOSEPOLICY","OLD"
curl::option CURL,"CLOSEPOLICY","FRESH"
=noverbatim

Note that the values are checked by the module case sensitive, thus you can not write T<"old">
or T<"fresh"> or mixed case words. Also the module B<does not tolerate> any other form of the words,
like T<OLDEST> or T<NEW>.

=subsection PUT flag

=verbatim
curl::option CurlHandle,"PUT"
=noverbatim

Setting the parameter tells the library to use HTTP PUT a file. The file to put
must be set with option T<INFILE>.

=subsection SSL_VERIFYPEER flag

=verbatim
curl::option CurlHandle,"SSL_VERIFYPEER"
=noverbatim

Set the flag value to make curl verify the peer's
certificate. The certificate to verify against must be specified with the
option T<CAINFO> option.

=subsection FILETIME flag

=verbatim
curl::option CurlHandle,"FILETIME"
=noverbatim

Pass a long. If it is a non-zero value, libcurl will attempt to get the
modification date of the remote document in this operation. This requires that
the remote server sends the time or replies to a time querying command. The
curl_easy_getinfo() function with the CURLINFO_FILETIME argument can be used
after a transfer to extract the received time (if any). (Added in 7.5)

=subsection FRESH_CONNECT flag

=verbatim
curl::option CurlHandle,"FRESH_CONNECT"
=noverbatim

Set the option to make the next transfer use a new connection by
force. If the connection cache is full before this connection, one of the
existinf connections will be closed as according to the set policy. This
option should be used with caution and only if you understand what it
does. Set to 0 to have libcurl attempt re-use of an existing connection.

=subsection FORBID_REUSE flag

=verbatim
curl::option CurlHandle,"FORBID_REUSE"
=noverbatim

Set the option to make the next transfer explicitly close the
connection when done. Normally, libcurl keep all connections alive when done
with one transfer in case there comes a succeeding one that can re-use them.
This option should be used with caution and only if you understand what it
does. Set to 0 to have libcurl keep the connection open for possibly later
re-use.

=subsection HTTPGET flag

=verbatim
curl::option CurlHandle,"HTTPGET"
=noverbatim

Set thisoption to force the HTTP request to get back
to T<GET>. Only really usable if T<POST>, T<PUT> or a custom request have been used
previously using the same curl handle.


=subsection TIMEOUT integer

=verbatim
' for example set the timeout to ten minutes
curl::option CurlHandle,"TIMEOUT",600
=noverbatim

Pass an integer parameter containing the maximum time in seconds that you allow
the libcurl transfer operation to take. Normally, name lookups can take a
considerable time and limiting operations to less than a few minutes risk
aborting perfectly normal operations. This option will cause curl to use the
SIGALRM to enable timeouting system calls. B<NOTE>
that this does not work in multi-threaded programs!

=subsection LOW_SPEED_LIMIT integer

=verbatim
curl::option CurlHandle,"LOW_SPEED_LIMIT",100
=noverbatim

Pass an integer as parameter. It contains the transfer speed in bytes per second
that the transfer should be below during T<LOW_SPEED_TIME> seconds for
the library to consider it too slow and abort.

=subsection LOW_SPEED_TIME integer

=verbatim
curl::option CurlHandle,"LOW_SPEED_TIME",60
=noverbatim

Pass an integer as parameter. It contains the time in seconds that the transfer
should be below the T<LOW_SPEED_LIMIT> for the library to consider it too
slow and abort.

=subsection RESUME_FROM integer

=verbatim
curl::option CurlHandle,"RESUME_FROM",3321
=noverbatim

Pass an integer as parameter. It contains the offset in number of bytes that you
want the transfer to start from.

=subsection SSLVERSION integer

=verbatim
curl::option CurlHandle,"SSLVERSION",3
=noverbatim

Pass an integer as parameter. Set what version of SSL to attempt to use, 2 or
3. By default, the SSL library will try to solve this by itself although some
servers make this difficult why you at times will have to use this option.

=subsection TIMECONDITION string

=verbatim
curl::option CurlHandle,"TIMECONDITION","IFMODSINCE"
=noverbatim

Pass a string as parameter. This defines how the T<TIMEVALUE> time value is
treated. You can set this parameter to T<IFMODSINCE> or
T<IFUNMODSINCE>. This is aa HTTP-only feature.

Example

=verbatim
curl::option CURL,"TIMECONDITION","IFMODSINCE"
curl::option CURL,"TIMECONDITION","IFUNMODSINCE"
=noverbatim

Note that the values are checked by the module case sensitive, thus you can not write T<"ifmodsince">
or T<"ifmodsince"> or mixed case words.

=subsection TIMEVALUE integer

=verbatim
curl::option CURL,"TIMECONDITION","IFMODSINCE"
curl::option CurlHandle,"TIMEVALUE",curl::getdate("2 days ago")
=noverbatim

Pass an integer as parameter. This should be the time in seconds since 1 jan 1970,
and the time will be used as specified in the option T<TIMECONDITION> or if that
isn't used, it will be T<IFMODSINCE> by default. (In other words curl will fetch the
page only if that is newer than the specified time.)

To conveniently get such time-stamp values as accepted by this function as argument you
can use the ScriptBasic function T<TIMEVALUE>.

=subsection MAXREDIRS integer

=verbatim
curl::option CurlHandle,"MAXREDIRS",3
=noverbatim

Pass an integer as parameter. The set number will be the redirection limit. If that many
redirections have been followed, the next redirect will cause an error. This
option only makes sense if the option T<FOLLOWLOCATION> is used at the same
time.

=subsection MAXCONNECTS integer

=verbatim
curl::option CurlHandle,"MAXCONNECTS",10
=noverbatim

Pass an integer as parameter. The set number will be the persistant connection cache size. The
set amount will be the maximum amount of simultaneous connections that libcurl
may cache between file transfers. Default is 5, and there isn't much point in
changing this value unless you are perfectly aware of how this work and
changes libcurl's behaviour. Note: if you have already performed transfers
with this curl handle, setting a smaller T<MAXCONNECTS> than before may cause
open connections to unnecessarily get closed.

=subsection CONNECTTIMEOUT integer

=verbatim
curl::option CurlHandle,"CONNECTTIMEOUT",10
=noverbatim

Pass an integer as parameter. It should contain the maximum time in seconds that you allow the
connection to the server to take.  This only limits the connection phase, once
it has connected, this option is of no more use. Set to zero to disable
connection timeout (it will then only timeout on the system's internal
timeouts). Specifying no value for this option is the same as specifying zero as all integer value options.
See also the T<TIMEOUT> option.

B<NOTE> that this does not work in multi-threaded programs!

=subsection HTTPHEADER string

=verbatim
curl::option CurlHandle,"HTTPHEADER","Accept: image/gif"
=noverbatim

Use this option to specify a header to be sent to the server. If you add a header that is 
otherwise generated and used by libcurl internally, your added one will be used instead.
If you add a header with no contents as in T<Accept:>, the internally used header will
just get disabled. Thus, using this option you can add new headers, replace
internal headers and remove internal headers.

You should call the function with this option for each extra header line that you want to add.

=subsection NOHTTPHEADER flag

=verbatim
curl::option CurlHandle,"NOHTTPHEADER"
=noverbatim

Use this option to delete all previous T<HTTPHEADER> options. This may be needed when you use a single
CURL connection to download several pages from the same server and did not call T<curl::finish> and
T<curl::init()> again. Until you call T<curl::finish> or specify this option all header lines remain effective.

After setting this option you can set various T<HTTPHEADER> option strings to build up a new list.

=subsection QUOTE string

=verbatim
curl::option CurlHandle,"QUOTE","pwd"
=noverbatim

Use this option to set arbitrary FTP commands that will be passed to the server prior to
your ftp request. You can set more than one values for this option. The commands will be 
executed in the order they are specified.

=subsection NOQUOTE flag

=verbatim
curl::option CurlHandle,"NOQUOTE"
=noverbatim

Use this option to delete all previously set T<QUOTE> FTP command strings. Read the documentation of
the option T<HTTPDEADER>.

=subsection POSTQUOTE string

=verbatim
curl::option CurlHandle,"POSTQUOTE","site chmod 777 uploadedfile.txt"
=noverbatim

Use this option to set arbitrary FTP commands that will be passed to the server after
your ftp request. You can set more than one values for this option. The commands will be 
executed in the order they are specified.

=subsection NOPOSTQUOTE flag

=verbatim
curl::option CurlHandle,"NOPOSTQUOTE"
=noverbatim

Use this option to delete all previously set T<POSTQUOTE> FTP command strings. Read the documentation of
the option T<HTTPDEADER>.

=subsection HTTPPOST string

=verbatim
curl::option CurlHandle,"HTTPPOST","name=Desade+Marquis"
=noverbatim

Use this option to specify form data in case of complex HTTP POST operation. As third argument specify a string
as POST data. You can specify many post data calling the T<curl::option> function with this option in successive calls.
The string argument should have the following format:

=itemize
=item T<name=content>

This can be used to specify textual form data. This is like what the browser sends to the web server when an
<input type=text name="name" value="content"> is used.

=item T<name=@@filename>

This can be used to specify a file to upload. This is like what the browser sends to the web server when an
<input type=file name="name"> is used.

=item T<name=@@filename1,filename2,...>

Add a form field named T<name> with the contents as read from the local files
named T<filename1> and T<filename2>. This is identical to the previous, except that
you get the contents of several files in one section.

=item T<name=@@filename;type=content-type>

Whenever you specify a file to read from, you can optionally specify the
content-type as well. The content-type is passed to the server together with
the contents of the file. The underlying CURL library will guess content-type for a
number of well-known extensions and otherwise it will set it to binary. You
can override the internal decision by using this option.

=item T<name=@@filename1,filename2,...;type=content-type>

When you specify several files to read the contents from, you can set the
content-type for all of them in the same way as with a single file.

=noitemize

=subsection ERRORBUFFER not implemented

This option is implemented in the original CURL library, but is not implemented in this interface.
The ScriptBasic interface sets this option automatically and you can access the error message calling the
BASIC function T<curl::error()>.

=subsection Options not implemented
=itemize
=item T<CURLOPT_STDERR> is used in the CURL library to specify an alternate strean instead of T<stderr>
for erro output. This is not supported by the ScriptBasic interface.

=item T<CURLOPT_PROGRESSFUNCTION> is used in conjunction with T<CURLOPT_PROGRESSDATA> to specify a progress
function that CURLIB library calls from time to time to allow progress indication for the user. This is
not implemented in the ScriptBasic interface.

=item T<CURLOPT_PASSWDFUNCTION> is used along with the option T<CURLOPT_PASSWDDATA> to specify a function
that returns the password for connections that need password, but no password is specified. This is not
implemented in ScriptBasic.

*/
besFUNCTION(sb_curl_option)
  pCurlClass pCurl;
  pCurlConnection pCon;
  VARIABLE Argument;
  char *pszOption;
  long value;
  long res;
  struct curl_slist *pNewList;

  GET_CURL_HANDLE
  /* get the option name */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
  Argument = besCONVERT2STRING(Argument);

#define CHECK_OPTION(X) if( STRLEN(Argument) == strlen(#X) && memcmp(STRINGVALUE(Argument),#X,strlen(#X)) == 0 )

  CHECK_OPTION(FILE){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    if( pCon->fp )besFCLOSE(pCon->fp);
    pCon->fp = besFOPEN(pszOption,"wb");
    if( pCon->fp == NULL )return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_FILE,pCon->fp);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_WRITEFUNCTION,pSt->fwrite);
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(HEADERFILE){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    if( pCon->hfp )besFCLOSE(pCon->hfp);
    pCon->hfp = besFOPEN(pszOption,"wb");
    if( pCon->hfp == NULL )return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_WRITEHEADER,pCon->hfp);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_HEADERFUNCTION,pSt->fwrite);
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(INFILE){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    if( pCon->fp )besFCLOSE(pCon->fp);
    besFREE(pCon->pszInText);

    /* if there was any previous but not used string: do not loose memory! */
    pCon->pszInText = NULL;
    pCon->cbInText = 0;
    pCon->dwInText = 0;

    pCon->fp = besFOPEN(pszOption,"rb");
    if( pCon->fp == NULL )return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_INFILE,pCon->fp);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_READFUNCTION,pSt->fread);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_INFILESIZE,besSIZE(pszOption));
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(INTEXT){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    Argument = besCONVERT2STRING(Argument);
    if( pCon->fp )besFCLOSE(pCon->fp);

    /* if there was any previous but not used string: do not loose memory! */
    besFREE(pCon->pszInText);
    pCon->pszInText = NULL;
    pCon->cbInText = 0;
    pCon->dwInText = 0;

    pCon->pszInText = besALLOC(STRLEN(Argument));
    if( pCon->pszInText == NULL )return COMMAND_ERROR_MEMORY_LOW;
    pCon->dwInText = STRLEN(Argument);
    memcpy(pCon->pszInText,STRINGVALUE(Argument),pCon->dwInText);
    pCon->cbInText = 0;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_INFILE,pCon);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_READFUNCTION,buffersend);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_INFILESIZE,pCon->dwInText);
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(POSTFIELDS){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    Argument = besCONVERT2STRING(Argument);
    pCon->pszPOSTFIELDS = besALLOC(STRLEN(Argument));
    if( pCon->pszPOSTFIELDS == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(pCon->pszPOSTFIELDS,STRINGVALUE(Argument),STRLEN(Argument));
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_POSTFIELDS,pCon->pszPOSTFIELDS);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_POSTFIELDSIZE,STRLEN(Argument));
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(CLOSEPOLICY){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    Argument = besCONVERT2STRING(Argument);
    if( STRLEN(Argument) == 3 && memcmp(STRINGVALUE(Argument),"OLD",3) == 0 ){
      value = CURLCLOSEPOLICY_OLDEST;
      }else
    if( STRLEN(Argument) == 5 && memcmp(STRINGVALUE(Argument),"FRESH",5) == 0 ){
      value = CURLCLOSEPOLICY_LEAST_RECENTLY_USED;
      }else return COMMAND_ERROR_ARGUMENT_RANGE;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_CLOSEPOLICY,value);
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(TIMECONDITION){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    Argument = besCONVERT2STRING(Argument);
    if( STRLEN(Argument) == 10 && memcmp(STRINGVALUE(Argument),"IFMODSINCE",10) == 0 ){
      value = TIMECOND_IFMODSINCE;
      }else
    if( STRLEN(Argument) == 12 && memcmp(STRINGVALUE(Argument),"IFUNMODSINCE",12) == 0 ){
      value = TIMECOND_IFUNMODSINCE;
      }else return COMMAND_ERROR_ARGUMENT_RANGE;
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_TIMECONDITION,value);
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;
    return 0x00081100+res;
    }

  CHECK_OPTION(HTTPHEADER){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    pNewList = curl_slist_append(pCon->pHeaderLines,pszOption);
    besFREE(pszOption);
    if( pNewList == NULL ){
      curl_slist_free_all(pCon->pHeaderLines);
      pCon->pHeaderLines = NULL;
      return COMMAND_ERROR_MEMORY_LOW;
      }
    pCon->pHeaderLines = pNewList;
    return COMMAND_ERROR_SUCCESS;
    }

  CHECK_OPTION(QUOTE){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    pNewList = curl_slist_append(pCon->pQuote,pszOption);
    besFREE(pszOption);
    if( pNewList == NULL ){
      curl_slist_free_all(pCon->pQuote);
      pCon->pQuote = NULL;
      return COMMAND_ERROR_MEMORY_LOW;
      }
    pCon->pQuote = pNewList;
    return COMMAND_ERROR_SUCCESS;
    }

  CHECK_OPTION(POSTQUOTE){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    pNewList = curl_slist_append(pCon->pPostQuote,pszOption);
    besFREE(pszOption);
    if( pNewList == NULL ){
      curl_slist_free_all(pCon->pPostQuote);
      pCon->pPostQuote = NULL;
      return COMMAND_ERROR_MEMORY_LOW;
      }
    pCon->pPostQuote = pNewList;
    return COMMAND_ERROR_SUCCESS;
    }

  /* delete all gathered HTTP header lines that may have remained from the previous network action */
  CHECK_OPTION(NOHTTPHEADER){
    curl_slist_free_all(pCon->pHeaderLines);
    pCon->pHeaderLines = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  /* delete all previously gathered FTP quote commands */
  CHECK_OPTION(NOQUOTE){
    curl_slist_free_all(pCon->pQuote);
    pCon->pQuote = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  /* delete all previously gathered FTP post quote commands */
  CHECK_OPTION(NOPOSTQUOTE){
    curl_slist_free_all(pCon->pPostQuote);
    pCon->pPostQuote = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  CHECK_OPTION(HTTPPOST){
    Argument = besARGUMENT(3);
    besDEREFERENCE(Argument);
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
    besCONVERT2ZCHAR(Argument,pszOption);
    curl_formparse(pszOption,&(pCon->firstitem),&(pCon->lastitem));
    besFREE(pszOption);
    return COMMAND_ERROR_SUCCESS;
    }

  CHECK_OPTION(BUFFER_SIZE){
    value=0;
    if( besARGNR >= 3 ){
      Argument = besARGUMENT(3);
      besDEREFERENCE(Argument);
      value = besGETLONGVALUE(Argument);
      }
    pCon->dwStepBf = value;
    return COMMAND_ERROR_SUCCESS;
    }

#define STRING_OPTION(X) \
  CHECK_OPTION(X){\
    Argument = besARGUMENT(3);\
    besDEREFERENCE(Argument);\
    if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;\
    if( pCon->psz##X )besFREE(pCon->psz##X);\
    besCONVERT2ZCHAR(Argument,pCon->psz##X);\
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_##X,pCon->psz##X);\
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;\
    return 0x00081100+res;\
    }
STRING_OPTIONS
#undef STRING_OPTION

#define HANDLE_FLAG(X)\
  CHECK_OPTION(X){\
    value=1;\
    if( besARGNR >= 3 ){\
      Argument = besARGUMENT(3);\
      besDEREFERENCE(Argument);\
      value = besGETLONGVALUE(Argument);\
      }\
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_##X,value);\
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;\
    return 0x00081100+res;\
    }

#define HANDLE_LONG(X)\
  CHECK_OPTION(X){\
    value=0;\
    if( besARGNR >= 3 ){\
      Argument = besARGUMENT(3);\
      besDEREFERENCE(Argument);\
      value = besGETLONGVALUE(Argument);\
      }\
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_##X,value);\
    if( res == CURLE_OK ) return COMMAND_ERROR_SUCCESS;\
    return 0x00081100+res;\
    }

HANDLE_FLAG(HTTPPROXYTUNNEL)
HANDLE_FLAG(VERBOSE)
HANDLE_FLAG(NOPROGRESS)
HANDLE_FLAG(NOBODY)
HANDLE_FLAG(FAILONERROR)
HANDLE_FLAG(UPLOAD)
HANDLE_FLAG(POST)
HANDLE_FLAG(FTPLISTONLY)
HANDLE_FLAG(FTPAPPEND)
HANDLE_FLAG(NETRC)
HANDLE_FLAG(FOLLOWLOCATION)
HANDLE_FLAG(TRANSFERTEXT)
HANDLE_FLAG(PUT)
HANDLE_FLAG(HEADER)
HANDLE_FLAG(SSL_VERIFYPEER)
HANDLE_FLAG(FILETIME)
HANDLE_FLAG(FRESH_CONNECT)
HANDLE_FLAG(FORBID_REUSE)
HANDLE_FLAG(HTTPGET)
HANDLE_FLAG(CRLF)

HANDLE_LONG(TIMEOUT)
HANDLE_LONG(LOW_SPEED_LIMIT)
HANDLE_LONG(LOW_SPEED_TIME)
HANDLE_LONG(RESUME_FROM)
HANDLE_LONG(SSLVERSION)
HANDLE_LONG(TIMEVALUE)
HANDLE_LONG(MAXREDIRS)
HANDLE_LONG(MAXCONNECTS)
HANDLE_LONG(CONNECTTIMEOUT)
HANDLE_LONG(PROXYPORT)

  besCONVERT2ZCHAR(Argument,pszOption);
  sprintf(pCon->errorbuffer,"CURL Error in 'curl::option'. Unkown option \"%s\"",pszOption);
  besFREE(pszOption);
  return 0x00081001;
besEND

static size_t buffercollect(char *buffer,size_t size,size_t count,pCurlConnection pCon){
  long dwChunkSize;
  pSupportTable pSt;
  char *pszNewBuffer;
  unsigned long dwNewSize;

  /* set the pSt variable so we can easily use the bes macros */
  pSt = pCon->pSt;

  dwChunkSize = size*count;

  /* if the buffer can not store the new chunk then reallocate it */
  if( dwChunkSize+pCon->cbBuffer > pCon->dwBuffer ){
    dwNewSize = pCon->dwBuffer+pCon->dwStepBf;
    if( dwNewSize < dwChunkSize+pCon->cbBuffer )dwNewSize = dwChunkSize+pCon->cbBuffer;
    pszNewBuffer = besALLOC(dwNewSize);
    if( pszNewBuffer == NULL )return 0;
    pCon->dwBuffer = dwNewSize;
    /* copy the characters that were already read into the new buffer */
    memcpy(pszNewBuffer,pCon->pszBuffer,pCon->cbBuffer);
    besFREE(pCon->pszBuffer);
    pCon->pszBuffer = pszNewBuffer;
    }

  memcpy(pCon->pszBuffer+pCon->cbBuffer,buffer,dwChunkSize);
  pCon->cbBuffer += dwChunkSize;
  return dwChunkSize;
  }

/**
=section curl_perform
=H curl::perform

Call this function to perform the action that was initialized using the function
T<curl::init()> and T<curl::option()>

Usage:
=verbatim
curl::perform CURL
=noverbatim

*/
besFUNCTION(sb_curl_perform)
  pCurlClass pCurl;
  pCurlConnection pCon;
  VARIABLE Argument;
  int res;

  GET_CURL_HANDLE

  /* if there were header lines specified then apply them. */
  if( pCon->pHeaderLines ){
    curl_easy_setopt(pCon->myConnection,CURLOPT_HTTPHEADER,pCon->pHeaderLines);
    }

  if( pCon->pQuote ){
    curl_easy_setopt(pCon->myConnection,CURLOPT_QUOTE,pCon->pQuote);
    }

  if( pCon->pPostQuote ){
    curl_easy_setopt(pCon->myConnection,CURLOPT_POSTQUOTE,pCon->pPostQuote);
    }

  if( pCon->firstitem ){
    curl_easy_setopt(pCon->myConnection,CURLOPT_HTTPPOST,pCon->firstitem);
    }

  /* if there was no file specified for the download than we have to return the result as string */
  if( pCon->fp == NULL ){
    res = curl_easy_setopt(pCon->myConnection,CURLOPT_FILE,pCon);
    if( res == CURLE_OK )res = curl_easy_setopt(pCon->myConnection,CURLOPT_WRITEFUNCTION,buffercollect);
    }

  res = curl_easy_perform(pCon->myConnection);

  /* if there was no download file specified and there is some downloaded string then we have to return it */
  if( pCon->cbBuffer > 0 ){
    besALLOC_RETURN_STRING(pCon->cbBuffer);
    memcpy(STRINGVALUE(besRETURNVALUE),pCon->pszBuffer,STRLEN(besRETURNVALUE));
    besFREE(pCon->pszBuffer);
    pCon->pszBuffer = NULL;
    pCon->dwBuffer = 0;
    pCon->cbBuffer = 0;
    }

  if( pCon->pszInText ){
    besFREE(pCon->pszInText);
    pCon->pszInText = NULL;
    pCon->cbInText = 0;
    pCon->dwInText = 0;
    }

  if( res == CURLE_OK ){
    pCon->fWasPerform = 1;
    return COMMAND_ERROR_SUCCESS;
    }

  return 0x00081100+res;;

besEND

/**
=section curl_info
=H curl::info

Call this function to get information on the performed action
after T<curl::perform> was called

Usage:
=verbatim
INFO = curl::info(CURL,info)
=noverbatim

The argument T<info> should be string and can take one of the valueslisted in the following sections. The return
value is either a string, integer or real value according to the type of the requested information. The possible
string arguments should be used in upper case letters, and should be precisely typed, no abbreviations or alternate
forms of the words are recognized by the subroutine.

Note that the following sections were mainly compiled from the documentation of the original C language CURL package
documentation T<curl_easy_getinfo.3> man page.

=subsection EFFECTIVE_URL string
The function returns the string of the effective URL of the last transfer.

=subsection HTTP_CODE integer
The function returns the integer status value of the lasttransfer.

=subsection FILETIME integer
The function returns the remote time of the retrieved document. If the result is zero it means that the time
can not be determined for some reson. It may happen that the server hides the time, or does not support the
command that is used to retrieve the file time.

=subsection TOTAL_TIME real
The function returns a real number; the total transaction time in seconds for the previous transfer. The returned
value is real and not integer because the fractional seconds are also taken into account.

=subsection NAMELOOKUP_TIME real
The function returns a real number; the time, in seconds, it took from the start until the name resolving 
was completed.

=subsection CONNECT_TIME real
The function returns a real number; 
the time, in seconds, it took from the
start until the connect to the remote host (or proxy) was completed.

=subsection PRETRANSFER_TIME real
The function returns a real number; 
the time, in seconds, it took from the
start until the file transfer is just about to begin. This includes all
pre-transfer commands and negotiations that are specific to the particular
protocol(s) involved.

=subsection SIZE_UPLOAD real
The function returns a real number; 
the total amount of bytes that were
uploaded.

I see no reason why this is a real number but the underlying CURL library returns a T<double>.

=subsection SIZE_DOWNLOAD real
The function returns a real number; 
total amount of bytes that were
downloaded.

I see no reason why this is a real number but the underlying CURL library returns a T<double>.

=subsection SPEED_DOWNLOAD real
The function returns a real number; 
the average download speed, in bytes/seconds, that curl
measured for the complete download.

=subsection SPEED_UPLOAD real
The function returns a real number; 
the average upload speed, in bytes/seconds, that curl
measured for the complete upload.

=subsection HEADER_SIZE integer
The function returns an integer;
the toal number of bytes received in all headers during the transfer.

=subsection REQUEST_SIZE integer
The function returns an integer;
the total number of bytes of the issued
requests. This is so far only for HTTP requests. Note that this may be more
than one request if T<FOLLOWLOCATION> was set using T<curl::option>.

=subsection SSL_VERIFYRESULT integer
The function returns an integer;
the result of the certification verification that was requested (using T<curl::option> with the option T<SSL_VERIFYPEER>).

=subsection CONTENT_LENGTH_DOWNLOAD real
The function returns a real number;
the content-length of the download. This
is the value read from the T<Content-Length:> field.

I see no reason why this is a real number but the underlying CURL library returns a T<double>.

=subsection CONTENT_LENGTH_UPLOAD real
The function returns a real number;
the specified size of the upload.
*/
besFUNCTION(sb_curl_info)
  pCurlClass pCurl;
  pCurlConnection pCon;
  VARIABLE Argument;
  int res;
  char *pszOption;
  long lOption;
  double dOption;

  GET_CURL_HANDLE

  if(  ! pCon->fWasPerform ){
    sprintf(pCon->errorbuffer,"CURL Error: no successful curl::perform was called before curl::info");
    return 0x00081002;
    }
  /* get the option name */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;
  Argument = besCONVERT2STRING(Argument);

#define CHECK_INFOPT_SZ(X) if( STRLEN(Argument) == strlen(#X) && memcmp(STRINGVALUE(Argument),#X,strlen(#X)) == 0 ){\
    res = curl_easy_getinfo(pCon->myConnection,CURLINFO_##X,&pszOption);\
    besALLOC_RETURN_STRING(strlen(pszOption));\
    memcpy(STRINGVALUE(besRETURNVALUE),pszOption,STRLEN(besRETURNVALUE));\
    return COMMAND_ERROR_SUCCESS;\
    }
#define CHECK_INFOPT_L(X) if( STRLEN(Argument) == strlen(#X) && memcmp(STRINGVALUE(Argument),#X,strlen(#X)) == 0 ){\
    res = curl_easy_getinfo(pCon->myConnection,CURLINFO_##X,&lOption);\
    besALLOC_RETURN_LONG;\
    LONGVALUE(besRETURNVALUE) = lOption;\
    return COMMAND_ERROR_SUCCESS;\
    }
#define CHECK_INFOPT_D(X) if( STRLEN(Argument) == strlen(#X) && memcmp(STRINGVALUE(Argument),#X,strlen(#X)) == 0 ){\
    res = curl_easy_getinfo(pCon->myConnection,CURLINFO_##X,&dOption);\
    besALLOC_RETURN_DOUBLE;\
    DOUBLEVALUE(besRETURNVALUE) = dOption;\
    return COMMAND_ERROR_SUCCESS;\
    }

CHECK_INFOPT_SZ(EFFECTIVE_URL)
CHECK_INFOPT_L (HTTP_CODE)
CHECK_INFOPT_L (FILETIME)
CHECK_INFOPT_D (TOTAL_TIME)
CHECK_INFOPT_D (NAMELOOKUP_TIME)
CHECK_INFOPT_D (CONNECT_TIME)
CHECK_INFOPT_D (PRETRANSFER_TIME)
CHECK_INFOPT_D (SIZE_UPLOAD)
CHECK_INFOPT_D (SIZE_DOWNLOAD)
CHECK_INFOPT_D (SPEED_DOWNLOAD)
CHECK_INFOPT_D (SPEED_UPLOAD)
CHECK_INFOPT_L (HEADER_SIZE)
CHECK_INFOPT_L (REQUEST_SIZE)
CHECK_INFOPT_L (SSL_VERIFYRESULT)
CHECK_INFOPT_D (CONTENT_LENGTH_DOWNLOAD)
CHECK_INFOPT_D (CONTENT_LENGTH_UPLOAD)

  besCONVERT2ZCHAR(Argument,pszOption);
  sprintf(pCon->errorbuffer,"CURL Error in 'curl::info'. Unkown info type \"%s\"",pszOption);
  besFREE(pszOption);
  return 0x00081003;

besEND

/**
=section curl_finish
=H curl::finish

You can call this function to close the connection that was used by CURL and to release
all resources that were allocated to handle the connection. To call this function is not
a must as all the releasing tasks are automatically performed as soon as the interpreter
finishes execution the BASIC program. However it is a good practice to call this functions
especially if you want to access the file the perform routine downloaded.

Usage:
=verbatim
curl::finish CURL
=noverbatim

This function closes the connection to the remote URL, closes the local file (if there was opened any), and
releases the memory that was used for the connection.

*/
besFUNCTION(sb_curl_finish)
  pCurlClass pCurl;
  pCurlConnection pCon;
  VARIABLE Argument;

  GET_CURL_HANDLE

  curl_easy_cleanup(pCon->myConnection);

  if( pCon->pHeaderLines ){
    curl_slist_free_all(pCon->pHeaderLines);
    pCon->pHeaderLines = NULL;
    }

  if( pCon->pQuote ){
    curl_slist_free_all(pCon->pQuote);
    pCon->pQuote = NULL;
    }

  if( pCon->pPostQuote ){
    curl_slist_free_all(pCon->pPostQuote);
    pCon->pPostQuote = NULL;
    }

  if( pCon->firstitem ){
    curl_formfree(pCon->firstitem);
    pCon->firstitem = pCon->lastitem = NULL;
    }

  if( pCon->fp )besFCLOSE(pCon->fp);

  /* unlink the handle from the list */
  if( pCon->prev )
    pCon->prev->next       = pCon->next;
  else
    pCurl->pFirstConnection = pCon->next;

  if( pCon->next )
    pCon->next->prev     = pCon->prev;

#define STRING_OPTION(X) if( pCon->psz##X )besFREE(pCon->psz##X);
STRING_OPTIONS
#undef STRING_OPTION
  if( pCon->pszPOSTFIELDS )besFREE(pCon->pszPOSTFIELDS);

  /* free the connection hanlde structure */
  besFREE(pCon);

besEND

/**
=section curl_error
=H curl::error

If any error has happened this function can be used to retrieve the error message that the underlying
CURL library supplied.

Usage:
=verbatim
print curl::error(CURL)
=noverbatim

*/
besFUNCTION(sb_curl_error)
  pCurlClass pCurl;
  pCurlConnection pCon;
  VARIABLE Argument;

  GET_CURL_HANDLE

  besALLOC_RETURN_STRING(strlen(pCon->errorbuffer));
  memcpy(STRINGVALUE(besRETURNVALUE),pCon->errorbuffer,STRLEN(besRETURNVALUE));
besEND


/**
=section curl_escape
=H curl::escape

This function will convert the given input string to an URL encoded string and
return that string. All input characters that are not a-z,
A-Z or 0-9 will be converted to their "URL escaped" version.

Usage:
=verbatim
URLencoded = curl::escape(URLnonencoded)
=noverbatim

*/
besFUNCTION(sb_curl_escape)
  VARIABLE Argument;
  char *s;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  /* undef argument returns undef */
  if( Argument == NULL )return COMMAND_ERROR_SUCCESS;
  Argument = besCONVERT2STRING(Argument);
  if( STRLEN(Argument) == 0 ){
    besALLOC_RETURN_STRING(1);
    STRLEN(besRETURNVALUE) = 0;
    }
  s = curl_escape(STRINGVALUE(Argument),STRLEN(Argument));
  if( s == NULL )return COMMAND_ERROR_MEMORY_LOW;
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE),s,STRLEN(besRETURNVALUE));
  free(s);
besEND

/**
=section curl_unescape
=H curl::unescape

This function will convert the given URL encoded input string to a "plain
string" and return that string. All input characters that are encoded T<%XX>
where T<XX> is a to digit hexadecimal character will be converted to their
plain text version. All T<+> characters that are after the (or a) T<?> character
are converted to space.

Usage:
=verbatim
URLencoded = curl::escape(URLnonencoded)
=noverbatim

This behaviour of this function makes it easy to unescape URLs that contain not
only the CGI GET parameter of the URL, but the whole URL. For exaple:

=verbatim
print curl::unescape("http://www.kuka+muka.com/mypage.asp?name=Linus+Nielsen")
=noverbatim

will print

=verbatim
http://www.kuka+muka.com/mypage.asp?name=Linus Nielsen
=noverbatim

which is much better than having the web server name converted to something containing a space.

*/
besFUNCTION(sb_curl_unescape)
  VARIABLE Argument;
  char *s;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  /* undef argument returns undef */
  if( Argument == NULL )return COMMAND_ERROR_SUCCESS;
  Argument = besCONVERT2STRING(Argument);
  if( STRLEN(Argument) == 0 ){
    besALLOC_RETURN_STRING(1);
    STRLEN(besRETURNVALUE) = 0;
    }
  s = curl_unescape(STRINGVALUE(Argument),STRLEN(Argument));
  if( s == NULL )return COMMAND_ERROR_MEMORY_LOW;
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE),s,STRLEN(besRETURNVALUE));
  free(s);
besEND

/**
=section curl_getdate
=H curl::getdate

This function returns the number of seconds since January 1st 1970, for the
date and time that the I<datestring> parameter specifies. 
Read further in the date string parser section below.

If the input string can not be parsed the function return zero.

The following is copied from the original T<curl_getdate> manual page.

B<PARSING DATES AND TIMES>

A T<"date"> is a string, possibly empty, containing many items separated by
whitespace. The whitespace may be omitted when no ambiguity arises.  The
empty string means the beginning of today (i.e., midnight).  Order of the
items is immaterial.  A date string may contain many flavors of items:

=itemize
=item calendar date items
This can be specified in a number of different ways. 
Including T<1970-09-17>, T<70-9-17>, T<70-09-17>, T<9/17/72>,
T<24 September 1972>, T<24 Sept 72>, T<24 Sep 72>, T<Sep 24, 1972>,
T<24-sep-72>, T<24sep72>.
The year can also be omitted, for example: T<9/17> or T<sep 17>.

=item time of the day items
This string specifies the time on a given day. Syntax supported includes:
T<18:19:0>, T<18:19>, T<6:19pm>, T<18:19-0500>
(for specifying the time zone as well).

=item time zone items
Specifies international time zone. There are a few acronyms supported, but in
general you should instead use the specific realtive time compared to
UTC. Supported formats include: T<-1200>, T<MST>, T<+0100>.

=item day of the week items
Specifies a day of the week. If this is mentioned alone it means that day of
the week in the future.

Days of the week may be spelled out in full: T<Sunday>, T<Monday>, etc or they
may be abbreviated to their first three letters, optionally followed by a
period.  The special abbreviations T<Tues> for T<Tuesday>, T<Wednes> for
T<Wednesday> and T<Thur> or T<Thurs> for T<Thursday> are also allowed.

A number may precede a day of the week item to move forward supplementary
weeks.  It is best used in expression like T<third monday>.  In this context,
T<last DAY> or T<next DAY> is also acceptable; they move one week before or
after the day that DAY by itself would represent.

=item relative items
A relative item adjusts a date (or the current date if none) forward or
backward. Example syntax includes: T<1 year>, T<1 year ago>, T<2 days>, 
T<4 weeks>.

The string T<tomorrow> is worth one day in the future (equivalent to T<day>),
the string T<yesterday> is worth one day in the past (equivalent to T<day ago>).

=item pure numbers
If the decimal number is of the form T<YYYYMMDD> and no other calendar date item
appears before it in the date string, then T<YYYY> is read as the year, T<MM> as the
month number and T<DD> as the day of the month, for the specified calendar date.
=noitemize

B<AUTHORS>

Originally written by Steven M. Bellovin T<smb@research.att.com> while at the
University of North Carolina at Chapel Hill.  Later tweaked by a couple of
people on Usenet.  Completely overhauled by Rich $alz T<rsalz@bbn.com> and Jim
Berets T<jberets@bbn.com> in August, 1990.

*/
besFUNCTION(sb_curl_getdate)
  VARIABLE Argument;
  char *s;
  time_t t;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  besCONVERT2ZCHAR(Argument,s);
  t = time(&t);
  t = curl_getdate(s,&t);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = t;

besEND

/**
=section curl_version
=H curl::version

Returns a human readable string with the version number of
libcurl and some of its important components (like OpenSSL
version) that were used to create the ScriptBasic module
T<curl.dll> under Windows NT or T<curl.so> under UNIX.

The module links the CURL library static thus you have to
recompile it or download a newer binary of the module if
there is a newer version of the CURL library even if you
have installed a newer version of the library itself.
*/
besFUNCTION(sb_curl_version)
  char *s;

  s = curl_version();  
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE),s,STRLEN(besRETURNVALUE));

besEND

besDLL_MAIN

besSUB_PROCESS_START
  INIT_MULTITHREAD
  return 1;
besEND

besSUB_PROCESS_FINISH
  FINISH_MULTITHREAD
besEND

START_FUNCTION_TABLE(CURL_SLFST)

EXPORT_MODULE_FUNCTION(shutmodu)
EXPORT_MODULE_FUNCTION(bootmodu)
EXPORT_MODULE_FUNCTION(versmodu)
EXPORT_MODULE_FUNCTION(sb_curl_error)
EXPORT_MODULE_FUNCTION(sb_curl_finish)
EXPORT_MODULE_FUNCTION(sb_curl_option)
EXPORT_MODULE_FUNCTION(sb_curl_perform)
EXPORT_MODULE_FUNCTION(sb_curl_init)
EXPORT_MODULE_FUNCTION(sb_curl_info)
EXPORT_MODULE_FUNCTION(sb_curl_version)
EXPORT_MODULE_FUNCTION(sb_curl_getdate)
EXPORT_MODULE_FUNCTION(sb_curl_escape)
EXPORT_MODULE_FUNCTION(sb_curl_unescape)
EXPORT_MODULE_FUNCTION(_init)
EXPORT_MODULE_FUNCTION(_fini)

END_FUNCTION_TABLE
