/*

  FILE   : interface.c
  HEADER : interface.h
  BAS    : cgi.bas
  AUTHOR: Peter Verhas

  DATE: June 22, 2002.

  CONTENT:
  This is the interface.c file for the ScriptBasic module CGI

NTLIBS:
UXLIBS:
DWLIBS:
MCLIBS:
*/

#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <httpext.h>
#endif

#include "../../basext.h"
#include "cgi.h"

/*
TO_BAS:
' When requestiong SaveFile for a field name that does not exist
const ErrorNoFile           = &H00080000
const ErrorBufferOverflow   = &H00080001
const ErrorContentTooLarge  = &H00080002
const ErrorMethodIsInvalid  = &H00080003
const ErrorNoDebugFile      = &H00080004
const ErrorNotImplemented   = &H00080005
const ErrorEndOfFile        = &H00080006
const ErrorIllFormedUpload  = &H00080007
const ErrorFileTooLarge     = &H00080008
const ErrorMethodNotAllowed = &H0008000A

' constants for setting option cgi$Method
const None   = &H00000000
const Get    = &H00000001
const Post   = &H00000002
const Upload = &H00000006
const Put    = &H00000008
const Del    = &H00000010
const Copy   = &H00000020
const Move   = &H00000040
const Head   = &H00000080

Function Param(ParameterName)
local ParameterValue

  ParameterValue = GetParam(ParameterName)
  If NOT IsDefined(ParameterValue) Then
    ParameterValue = PostParam(ParameterName)
  End If

  Param = ParameterValue
End Function

'
' cgi::ResolveHtml(Text)
' Resolve an HTML text given as argument
'
sub ResolveHtml(HtmlText)
  ResolveHtml = GetHtmlTemplate(HtmlText,1)
end sub

'
' cgi::EmitHtmlTemplate(FileName)
' emit the content of the HTML template to the web client
'
sub EmitHtmlTemplate(FileName)
  print GetHtmlTemplate(FileName)
end sub

sub Header(State,ContentType)
  if isDefined(HeaderWasPrinted) then exit sub
  HeaderWasPrinted = TRUE
  if ServerSoftware() like "Microsoft-IIS*" then
    print "HTTP/1.0 ",State,"\nContent-Type: ",ContentType,"\n"
  else
    print "Status: ",State,"\nContent-Type: ",ContentType,"\n"
  end if
end sub

sub RequestBasicAuthentication(Realm)
  if not IsDefined(Realm) then Realm = "Basic Authentication Realm"
  Header 401,"text/html"
  print "WWW-Authenticate: Basic realm=\"",Realm,"\"\n"
end sub

sub SetCookie(CookieName,CookieValue,CookieDomain,CookiePath,CookieExpires,CookieIsSecure)
  print "Set-cookie: ",CookieName,"=",CookieValue,";"
  if IsDefined( CookieDomain ) then
    print " domain=",CookieDomain,";"
  end if
  if IsDefined( CookiePath ) then
    print " path=",CookiePath,";"
  end if
  if IsDefined(CookieExpires) then
    print FormatDate("expires=WDN, DD-MON-YY HH:mm:ss GMT;",CookieExpires)
  end if
'Wdy, DD-Mon-YY HH:MM:SS GMT
  if CookieIsSecure then
    print " secure"
  end if
  print "\n"
end sub

sub FinishHeader
  if isDefined(HeaderWasFinished) then exit sub
  HeaderWasFinished = TRUE
  print "\n"
end sub

function Cookie(CookieName)
  local RawCookieText,CookieList,i,LikeString,CookieValue

  RawCookieText = RawCookie()
  splita RawCookieText by ";" to CookieList
  LikeString = CookieName & "=*"
  i = 0
  while IsDefined(CookieList[i])
   CookieList[i] = trim(CookieList[i])
   if CookieList[i] LIKE LikeString then
     split CookieList[i] by "=" to CookieName,CookieValue
     Cookie = CookieValue
     exit function
   end if
   i = i + 1
  wend
' by default function return value is undef when cookie is not found
end function

*/

/**
=H Handle CGI input and output

=abstract
Functions that handle web input data for the standalone module running as CGI or
ISAPI or Eszter SB Engine variation.
=end

The module CGI is implemented as an external module written in the language C. Whenever the 
programmer wants to write a program using the language ScriptBasic that uses the CGI interface of the web 
server he or she can use this module. Although this is not a must to use this module for CGI programming 
it IS a wise choice. 

The module comes from the developers of the language interpreter. Most of its code is implemented as C 
code, executing fast and efficient. In its very first version of the module it already supports such 
advanced features as file upload in a robust and secure way. It handles cookies, POST and GET operations 
and will support other interfaces in the future like Apache Module interface or FastCGI in a compatible 
manner.

The basic programs using the ISAPI interpreter should use the module the same way as CGI versions. In 
other words if you write a CGI program it should work executed by the ISAPI version of the interpreter 
without modification.
/**
=section ack
=H Acknowledgements

The core code of the CGI handling routines was not written just looking at the RFC. I have learned a lot 
from other works. The most important to mention is the CGI.pm Perl module written by Lincoln D. Stein 
(http://www-genome.wi.mit.edu/ftp/pub/software/WWW/cgi_docs.html) Another work was named 
cgi-lib.c created by eekim@eekim.com. This piece of code gave me the last push to finish looking for 
readily available free library to interface with ScriptBasic and write instead a new one.

The ISAPI interface was developed using the Microsoft Developer Network CD documentation and I really 
learned a lot while testing the different uses of the function ReadClient until I could figure out that the 
official Microsoft documentation is wrong.

It says: "If more than lpdwSize bytes are immediately available to be read, ReadClient will block until some amount 
of data has been returned, and will return with less than the requested amount of data." In fact it has to be "If less 
than lpdwSize bytes are immediately available to be read, ReadClient will block until some amount of data has been 
returned, and will return with possibly less than the requested amount of data."
*/
#ifdef _DEBUG
static void DBGPR(char *pszFormat,
                  ...
  ){
  va_list marker;
  FILE *fp;

  fp = fopen("E:/MyProjects/sb/debug.txt","at");
  if( fp == NULL )return;
  va_start(marker,pszFormat);
  vfprintf(fp,pszFormat,marker);
  fprintf(fp,"\n");
  va_end(marker);
  fclose(fp);
  return;
  }
#else
static void DBGPR(char *s,...){}
#endif

typedef struct _ModuleObject {
  CgiObject Cgi;
  SymbolTable symboltable;
  void *HandleList;
  }ModuleObject,*pModuleObject;

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND
/**
=H Error Codes


*/
besSUB_ERRMSG

  switch( iError ){
    case 0x00080000: return "The named file is not uploaded.";
    case CGI_ERROR_BUFFER_OVERFLOW: return "Buffer grew too large, probably malformeatted http request.";
    case CGI_ERROR_BIG_CONTENT: return "http request is larger than the allowed and configured maximal size.";
    case CGI_ERROR_INVALID_METHOD: return "Invalid or unknown method, presumably in the CGI debug file.";
    case CGI_ERROR_NO_DEBUG_FILE: return "The CGI debug file is not found";
    case CGI_ERROR_NOTIMP: return "NSAPI and FCGI interfaces are not implemented";
    case CGI_ERROR_FILEMAX: return "File is larger than the configured allowed size.";
    case CGI_ERROR_MEMORY_LOW: return "CGI memory low.";
    case CGI_ERROR_METHOD_NOTALL: return "Method is not allowed.";
    case CGI_ERROR_EOF:
    case CGI_ERROR_ILLF_MULTI:
    case CGI_ERROR_ILLF_MULTI1:
    case CGI_ERROR_ILLF_MULTI2:
    case CGI_ERROR_ILLF_MULTI3:
    case CGI_ERROR_ILLF_MULTI4:
    case CGI_ERROR_ILLF_MULTI5:
    case CGI_ERROR_ILLF_MULTI6:
    case CGI_ERROR_ILLF_MULTI7: return "Ill formatted multipart upload data";
    }
  return "Unknown CGI module error.";
besEND

besSUB_START
  pModuleObject p;
  long lOptionValue,lErrorCode;
  char *s;

  besMODULEPOINTER = besALLOC(sizeof(ModuleObject));
  if( besMODULEPOINTER == NULL )return 0;
  p = (pModuleObject)besMODULEPOINTER;
  p->HandleList = NULL;
  if( *(pSt->pEo->Ver.Variation) == 'W' )/*WINISAPI*/
    cgi_InitIsapi(&(p->Cgi),pSt->pEo->pEmbedder);
  else{
    cgi_InitCgi(&(p->Cgi));
    p->Cgi.pEmbed = pSt->pEo->pEmbedder;
    p->Cgi.pfStdIn = pSt->pEo->fpStdinFunction;
    p->Cgi.pfStdOut = pSt->pEo->fpStdouFunction;
    p->Cgi.pfEnv = pSt->pEo->fpEnvirFunction;
    }
  if( lOptionValue = besOPTION("cgi$bufferincrease") )
    p->Cgi.lBufferIncrease = lOptionValue;
  if( lOptionValue = besOPTION("cgi$buffermax") )
    p->Cgi.lBufferMax = lOptionValue;
  if( lOptionValue = besOPTION("cgi$contentmax") )
    p->Cgi.lContentMax = lOptionValue;
  if( lOptionValue = besOPTION("cgi$filemax") )
    p->Cgi.lFileMax = lOptionValue;
  if( lOptionValue = besOPTION("cgi$method") )
    p->Cgi.fMethods = lOptionValue;

#ifdef WIN32
  if( s = besCONFIG("isapi.buffer") )
    p->Cgi.dwIsapiBufferSize = atol(s);
#endif

  p->Cgi.pszDebugFile = besCONFIG("cgi.debugfile");

  lErrorCode = cgi_ReadHttpRequest(&(p->Cgi));
  /* cgi.c is an autonomous program file and is loosely copled with the Scriba source and therefore it
     uses its own error codes. This error code is converted to the built-in error code of scriba. The other
     codes are module specific. */
  if( lErrorCode == CGI_ERROR_MEMORY_LOW )lErrorCode = COMMAND_ERROR_MEMORY_LOW;

  p->symboltable = besNEWSYMBOLTABLE();
  if( p->symboltable == NULL )return COMMAND_ERROR_MEMORY_LOW;

  return lErrorCode;
besEND

besSUB_FINISH
  pModuleObject p;
  pSymbolList pH;

  p = (pModuleObject)besMODULEPOINTER;
  if( p == NULL )return 0;
  pH = p->Cgi.pPostParameters;
  while( pH ){
    if( pH->fp ){
      fclose(pH->fp);
      pH->fp = NULL;
      }
    pH = pH->next;
    }
  if( p->symboltable != NULL ){
    /* note that this does not free the strings (values) but the whole memory segment is unloaded anyway */
    besFREESYMBOLTABLE( p->symboltable );
    }
  return 0;
besEND

/**
=section GetParam
=H cgi::GetParam("name")

This function returns the value of the GET parameter named in the argument. If there are multiple GET 
parameter with the same name the function returns the first one.

The CGI parameter names are case sensitive according to the CGI standard.
*/
besFUNCTION(getget)
  pModuleObject p;
  VARIABLE Argument;
  char *pszArgument,*pszResult;
  long slen;
  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  pszResult = cgi_GetParam(&(p->Cgi),pszArgument);
  besFREE(pszArgument);
  if( pszResult ){
    slen=strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE ),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE  = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

besEND

/**
=section PostParam
=H cgi::PostParam("name")

This function returns the value of the POST parameter named in the argument. If there are multiple 
POST parameter with the same name the function returns the first one.

The CGI parameter names are case sensitive according to the CGI standard.
*/
besFUNCTION(getpost)
  pModuleObject p;
  VARIABLE Argument;
  char *pszArgument,*pszResult;
  long slen;
  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  pszResult = cgi_PostParam(&(p->Cgi),pszArgument);
  besFREE(pszArgument);
  if( pszResult ){
    slen=strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE ),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE  = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

besEND

/**
=section GetParamEx
=H cgi::GetParamEx("name",q) cgi::PostParamEx("name",q)

=verbatim
q = undef
cgi::GetParamEx("param",q)
cgi::GetParamEx("param",q)

q = undef
cgi::PostParamEx("param",q)
cgi::PostParamEx("param",q)
=noverbatim

These functions can be used to iteratively fetch all parameter values passed with the same name. While 
the functions Param, GetParam and PostParam return the value of the first parameter of the name these 
functions can be used to retrieve the second, third and so on parameters.

The first parameter is the name of the CGI variable.

The second argument is an iteration variable that the function uses internally. This argument is passed 
by value and therefore it should be a variable to reach proper functioning. This variable should be undef 
when first calling the function. Later the value of this variable is set to a string that represents 
an internal value that the basic code SHOULD NOT alter. The value can be moved from one variable to another, 
but should not be changed.

The function returns undef when there are no more CGI variables of the name and the iteration variable 
is also set to hold the undef value.

The CGI parameter names are case sensitive according to the CGI standard.
*/
besFUNCTION(getgetex)
  pModuleObject p;
  VARIABLE Argument,*pSymVAR,SymVAR;
  pSymbolList pSYM;
  char *pszArgument,*pszResult;
  long slen;
  unsigned long __refcount_;

  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  SymVAR = besARGUMENT(2);
  besDEREFERENCE(Argument);
  besLEFTVALUE(SymVAR,pSymVAR);
  if( pSymVAR ){
    SymVAR = *pSymVAR;
    if( SymVAR && TYPE(SymVAR) == VTYPE_LONG ){
      pSYM = besHandleGetPointer(p->HandleList,LONGVALUE(SymVAR));
      besHandleFreeHandle(p->HandleList,LONGVALUE(SymVAR));
      }else{
      pSYM = NULL;
      }
    }else{
    pSYM = NULL;
    }

  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  pszResult = cgi_GetParamEx(&(p->Cgi),pszArgument,&pSYM);
  besFREE(pszArgument);

  if( pSymVAR ){
    if( *pSymVAR && TYPE(*pSymVAR) == VTYPE_LONG )
      LONGVALUE(*pSymVAR) = besHandleGetHandle(p->HandleList,pSYM);
    else{
      besRELEASE( *pSymVAR );
      *pSymVAR = besNEWLONG;
      if( *pSymVAR == NULL )return COMMAND_ERROR_MEMORY_LOW;
      LONGVALUE(*pSymVAR) = besHandleGetHandle(p->HandleList,pSYM);
      }
    }

  if( pszResult ){
    slen=strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE ),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE  = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

besEND

/*
=section PostParamEx
*/
besFUNCTION(getpostex)
// char *cgi_PostParamEx(pModuleObject pCO, char *pszParam, pSymbolList *p);
  pModuleObject p;
  VARIABLE Argument,*pSymVAR,SymVAR;
  pSymbolList pSYM;
  char *pszArgument,*pszResult;
  long slen;
  unsigned long __refcount_;

  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  SymVAR = besARGUMENT(2);
  besDEREFERENCE(Argument);
  besLEFTVALUE(SymVAR,pSymVAR);
  if( pSymVAR ){
    SymVAR = *pSymVAR;
    if( SymVAR && TYPE(SymVAR) == VTYPE_LONG ){
      pSYM = besHandleGetPointer(p->HandleList,LONGVALUE(SymVAR));
      besHandleFreeHandle(p->HandleList,LONGVALUE(SymVAR));
      }else{
      pSYM = NULL;
      }
    }else{
    pSYM = NULL;
    }

  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  pszResult = cgi_PostParamEx(&(p->Cgi),pszArgument,&pSYM);
  besFREE(pszArgument);

  if( pSymVAR ){
    if( *pSymVAR && TYPE(*pSymVAR) == VTYPE_LONG )
      LONGVALUE(*pSymVAR) = besHandleGetHandle(p->HandleList,pSYM);
    else{
      besRELEASE( *pSymVAR );
      *pSymVAR = besNEWLONG;
      if( *pSymVAR == NULL )return COMMAND_ERROR_MEMORY_LOW;
      LONGVALUE(*pSymVAR) = besHandleGetHandle(p->HandleList,pSYM);
      }
    }

  if( pszResult ){
    slen=strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE ),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE  = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

besEND

/**
=section SaveFile
=H cgi::SaveFile("param","filename")
This function can be used to save an uploaded file into a file on disk. The first argument is the name of 
the CGI parameter. The second argument is the desired file name. This file will be created and the content
of the uploaded data is written into it. If the file already existed it is going to be deleted.

Note that the first argument is the name of the CGI parameter and not the name of the file. This is the 
string that appears in the tag name in the input field of type file. In the following example the argument 
should be T<"FILE-UPLOAD-NAME">.

=verbatim
<FORM METHOD="POST" ACTION="echo.bas" ENCTYPE="multipart/form-data">
<INPUT TYPE="FILE" VALUE="FILE-UPLOAD-VALUE" NAME="FILE-UPLOAD-NAME">
<INPUT TYPE="SUBMIT" NAME="SUBMIT-BUTTON" VALUE="UPLOAD FILE">
</FORM>
=noverbatim

Files are uploaded in binary format. This means that applications accepting text file uploads should take 
care of the conversion. The current version of the CGI module does not support the conversion process. 

The CGI parameter names are case sensitive according to the CGI standard.
*/
besFUNCTION(savefile)
  pModuleObject p;
  VARIABLE Argument,FileName;
  char *pszArgument,*pszFileName;
  FILE *fp,*fi;
  int ch;

  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  FileName = besARGUMENT(2);
  besDEREFERENCE(Argument);
  besDEREFERENCE(FileName);
  Argument = besCONVERT2STRING(Argument);
  FileName = besCONVERT2STRING(FileName);
  besCONVERT2ZCHAR(Argument,pszArgument);

  fi = cgi_FILEp(&(p->Cgi),pszArgument);
  besFREE(pszArgument);
  if( fi == NULL ){
    return 0x00080000;
    }
  besCONVERT2ZCHAR(FileName,pszFileName);

  fp = besHOOK_FOPEN(pszFileName,"wb");
DBGPR("file %s handle is %p\n",pszFileName,fp);
  besFREE(pszFileName);
  if( fp == NULL ){
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }

  while( (ch=fgetc(fi)) != EOF )besFPUTC(ch,fp);
  besFCLOSE(fp);

besEND

/**
=section FileName
=H cgi::FileName("name")

This function returns the name of the uploaded file. This is the file name that the file has on the client 
computer. Based on the client computer operating system and used browser this value may contain 
spaces, may contain backslash as path separator character, may but need not contain the full path of the 
file or even may be OpenVMS format file name specification. Applications using this function should be 
prepared to handle the various client file formats.
*/
besFUNCTION(filename)
  pModuleObject p;
  VARIABLE Argument;
  char *pszArgument,*pszResult;
  long slen;
  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  pszResult = cgi_OriginalFileName(&(p->Cgi),pszArgument);
  besFREE(pszArgument);
  if( pszResult ){
    slen=strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE ),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE  = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
besEND

/**
=section FileLength
=H cgi::FileLength("param")

This function returns the length of the uploaded file. This is zero if the file field was not filled by the user 
or if a zero length file was uploaded. Files are uploaded in binary format. The length is the length of the 
binary data that may be more or less of the size of the final file if it is a text file converted to the natural 
format of the operating system running the application.
*/
besFUNCTION(filelen)
  pModuleObject p;
  VARIABLE Argument;
  char *pszArgument;
  long lResult;
  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszArgument);
  lResult = cgi_FileLength(&(p->Cgi),pszArgument);
  besFREE(pszArgument);
  besRETURNVALUE = besNEWMORTALLONG;
  if( besRETURNVALUE == NULL )return COMMAND_ERROR_MEMORY_LOW;
  LONGVALUE(besRETURNVALUE) = lResult;
  return COMMAND_ERROR_SUCCESS;
besEND

#define ENVIRFUNC(XXX) \
besFUNCTION(XXX)\
  char *pszResult;\
  long slen;\
  pModuleObject p;\
  p = (pModuleObject)besMODULEPOINTER;\
  pszResult = cgi_##XXX( &(p->Cgi) );\
  if( pszResult ){\
    slen = strlen(pszResult);\
    besALLOC_RETURN_STRING(slen);\
    memcpy(STRINGVALUE(besRETURNVALUE),pszResult,slen);\
    return COMMAND_ERROR_SUCCESS;\
    }else{\
    besRETURNVALUE = NULL;\
    return COMMAND_ERROR_SUCCESS;\
    }\
besEND

/**
=section EnvironmentFunctions
=H Environment functions
=abstract
CGI programs gain a great wealth of information from environment variables. This data is available to 
the ScriptBasic program via module functions. The CGI program is encouraged to use these functions 
instead of the function environ(). The reason to use these functions is that later versions of the CGI 
module may support ISAPI, NSAPI, FastCGI and other web server interface modes. Calling the function 
environ() to get the value of these variables will not work in that case, while calling the functions
provided by the CGI module still works.

The value returned by any of these functions is string even when the value is numeric by its nature. This 
is usually not an issue, because ScriptBasic automatically converts the values from numeric to string and 
back.
=end
*/

/**
=section ServerSoftware
=H ServerSoftware
This function returns the string identifying the web server software. This is "Microsoft-IIS/4.0" under 
Windows NT using the Microsoft web server. The ScriptBasic program usually does not need this function 
only if there is some special feature that the web server software provides.
*/
besFUNCTION(ServerSoftware)
  char *pszResult;
  long slen;
  pModuleObject p;
  p = (pModuleObject)besMODULEPOINTER;
  pszResult = cgi_ServerSoftware(&(p->Cgi));
  if( pszResult ){
    slen = strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
besEND

/**
=section ServerName
=H cgi::ServerName

The internet name of the server machine running the web server.
*/
/*
besFUNCTION(ServerName)
*/
ENVIRFUNC(ServerName)
/**
=section GatewayInterface
=H cgi::GatewayInterface
This is the name and version of the interface between the program and the web server. This is usually 
"CGI/1.1"
*/
/*
besFUNCTION(GatewayInterface)
*/
ENVIRFUNC(GatewayInterface)
/**
=section ServerProtocol
=H cgi::ServerProtocol
This is the protocol name and version that the server uses. This is usually "HTTP/1.1", but old servers 
may still use HTTP/1.0.
*/
/*
besFUNCTION(ServerProtocol)
*/
ENVIRFUNC(ServerProtocol)
/**
=section ServerPort
=H cgi::ServerPort
The internet port that the web server is listening. This value is numeric by its nature, but the functions 
returns the string representation of the port number as decimal value. There can be more than one web 
servers running on a machine all listening on different ip numbers and ports. The returned value is the 
port number of the web server that started the CGI application.
*/
/*
besFUNCTION(ServerPort)
*/
ENVIRFUNC(ServerPort)
/**
=section RequestMethod
=H cgi::RequestMethod
This is the HTTP method the request uses. This can be T<"GET">, T<"HEAD"> or T<"POST">. There are other methods that 
web servers may support but there is no clear definition how these may interact with CGI programs.

Note that most CGI programs are not prepared to handle HEAD requests. Therefore this method is NOT allowed by default.
The allowed methods are T<GET>, T<POST> and the special type of T<POST> that uploads one or more files. If
your program is prepared to handle T<HEAD> requests then you can explicitely allow this method

=verbatim
option cgi$Method cgi::Get or cgi::Upload or cgi::Head
=noverbatim

setting the appropriate value of the run-time option T<cgi$Method>.

*/
/*
besFUNCTION(RequestMethod)
*/
ENVIRFUNC(RequestMethod)
/**
=section PathInfo
=H cgi::PathInfo
This is the value CGI variable PATH_INFO. The web servers often use this variable in many different 
ways. PATH_INFO and PATH_TRANSLATE variables may appear to work incorrectly in Internet 
Information Server (IIS) version 4.0. These CGI environment variables return the physical path to the file that 
was passed to the CGI application as part of the GET statement. Instead, IIS returns the path to the CGI 
script.
*/
/*
besFUNCTION(PathInfo)
*/
ENVIRFUNC(PathInfo)
/**
=section PathTranslated
=H cgi::PathTranslated
This is the value of the CGI variable PATH_TRANSLATED. This is supposed to be the absolute path to the 
application.
*/
/*
besFUNCTION(PathTranslated)
*/
ENVIRFUNC(PathTranslated)
/**
=section ScriptName
=H cgi::ScriptName
This is value of the CGI variable SCRIPT_NAME. This is usually the name of the script. This may contain 
full or partial path elements before the script name.
*/
/*
besFUNCTION(ScriptName)
*/
ENVIRFUNC(ScriptName)
/**
=section QueryString
=H cgi::QueryString
This is the query string of a GET request. This string usually appears after the ? mark on the URL and it 
is automatically created when the request method is specified to be "GET" in a form. ScriptBasic 
programs rarely need this value, as there are other more flexible and more powerful methods to handle CGI 
input values.
*/
/*
besFUNCTION(QueryString)
*/
ENVIRFUNC(QueryString)
/**
=section RemoteHost
=H cgi::RemoteHost
This is the internet name of the remote client. If the name of the client can not be determined this 
variable usually holds the ip number of the client. To have an ip name in this variable depends on the client 
and also on the configuration of the web server. The http request does not include the name of the 
client. It only holds the ip number, and the web server should issue a reverse lookup request toward the 
DNS server to determine the ip name of the client. This is used sometimes for security reasons 
disallowing all clients that have no ip name. On the other hand this is a slow process and may severely impact 
the performance of the web server. Usually this reverse lookup is switched off.
*/
/*
besFUNCTION(RemoteHost)
*/
ENVIRFUNC(RemoteHost)
/**
=section RemoteAddress
=H cgi::RemoteAddress
This is the ip address of the client machine that issued the http request. This can be the ip number of the 
client or the ip number of the proxy server that the client uses.
*/
/*
besFUNCTION(RemoteAddress)
*/
ENVIRFUNC(RemoteAddress)
/**
=section AuthType
=H cgi::AuthType
This is the type of the authentication the web server performed when the CGI program was started. It is 
undef if there was no user authentication. It is "Basic" if the web server used the basic authentication 
and is "NTLM" if the authentication is Windows NT challenge response authentication. Other web servers 
may use different authentication schemas and this variable may have other values.
*/
/*
besFUNCTION(AuthType)
*/
ENVIRFUNC(AuthType)
/**
=section RemoteUser
=H cgi::RemoteUser
This is the value of the CGI variable REMOTE_USER. This variable usually holds the user name supplied 
during authentication. Note that this name may have the format DOMAIN\USER under Windows NT. 
*/
/*
besFUNCTION(RemoteUser)
*/
ENVIRFUNC(RemoteUser)
/**
=section RemoteIdent
=H cgi::RemoteIdent
This is the value of the CGI variable REMOTE_IDENT. This is rarely implemented.
*/
/*
besFUNCTION(RemoteIdent)
*/
ENVIRFUNC(RemoteIdent)
/**
=section ContentType
=H cgi::ContentType
This is the value of the CGI variable CONTENT_TYPE. This is undef in case of a GET request. Otherwise 
this is the MIME type of the http request body. This is application/x-www-form-urlencoded for normal 
form posting and is multipart/form-data in case of file upload.
*/
/*
besFUNCTION(ContentType)
*/
ENVIRFUNC(ContentType)
/**
=section ContentLength
=H cgi::ContentLength
This is the value of the CGI variable CONTENT_LENGTH. This gives the length of the http request data 
sent to the web server in the POST request.
*/
/*
besFUNCTION(ContentLength)
*/
ENVIRFUNC(ContentLength)
/**
=section UserAgent
=H cgi::UserAgent
This is the value of the CGI variable USER_AGENT. The web browsers send their identification strings to 
the server in the http request. This browser identification string can be retrieved using this function. This 
string is "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT)" for an Internet Explorer 5.0 under 
Windows NT.
*/
/*
besFUNCTION(UserAgent)
*/
ENVIRFUNC(UserAgent)
/**
=section RawCookie
=H cgi::RawCookie
The cookies that the client has sent in the http request. This string contains the cookies without any 
processing in raw format. ScriptBasic programs usually do not need this function because there are more 
powerful functions to handle cookies.
*/
besFUNCTION(Cookie)
  char *pszResult;
  long slen;
  pModuleObject p;
  p = (pModuleObject)besMODULEPOINTER;
  pszResult = cgi_Cookie( &(p->Cgi) );
  if( pszResult ){
    slen = strlen(pszResult);
    besALLOC_RETURN_STRING(slen);
    memcpy(STRINGVALUE(besRETURNVALUE),pszResult,slen);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
besEND

/**
=section Referer
=H cgi::Referer
The browser usually sends the URL of the referring page in a http header field to the server. Using this 
function the CGI application can access this URL. Note that according to the http spelling Referer is 
spelled with simple r and not Referrer.

This function was missing prior to version 3.0 of the module.
*/
/*
besFUNCTION(Referer)
*/
ENVIRFUNC(Referer)

/**
=section Header
=H cgi::Header

=verbatim
cgi::Header 200,"text/html"
=noverbatim

This function is implemented in basic and the source of the function can be found in the file
T<cgi.bas>. This function accepts two arguments. The first argument is the code of the state 
of the http answer that the application is sending. This is usually T<200> for normal answers. 
The second argument is the mime type of the answer. This is T<text/html> for HTML pages.

The state of the http answer should be defined differently using Microsoft IIS. This function 
automatically takes care of this.
*/

/**
=section SetCookie
=H cgi::SetCookie

=verbatim
cgi::SetCookie CookieName,CookieValue,CookieDomain,CookiePath,CookieExpires,CookieIsSecure
=noverbatim

This function should be used to set a cookie. The arguments are the name and value of the cookie. These 
arguments are needed. Other arguments may be missing or explicitly hold the undef value.

*/
/**
=section FinishHeader
=H cgi::FinishHeader

This function should be called after the last http answer header creating function. In the current 
implementation this function only prints an empty line that separates the header from the body, but later 
versions may do more actions.

Note that this function is implemented as BASIC code in the file T<cgi.bas>, therefore you can easily read and
understand how it works without the need for reading and understanding any C code.
*/

/*
=section RequestBasicAuthentication
=H cgi::RequestBasicAuthentication Realm

This function should be called if the application requires user authentication. This function should be 
called before sending any header or body information to the client. The function sends an error code 
“401 Not Authorized” along with the appropriate header fields requesting basic authentication. There are 
more sophisticated types of authentication requests that are supported by various web servers.

The application calling this function should call function FinishHeader before ending its execution.

Real CGI applications may access only the user name part of the authentication information. In that case 
the authentication is done by the webserver and the password is not passed to the application. In this 
case the application should rely on the web server and accept the user as authentication if there is a not 
undef RemoteUser or RemoteIdent value. The function T<RequestBasicAuthentication> should be used if 
there is not satisfactory user name or in case the application wants the user to re-authenticate with 
presumably different user name.

When ScriptBasic is embedded in the web server Eszter SB Application Engine remote user name and password is passed to the 
BASIC application. The web server accepts any username and password and this is the responsibility of 
the application to accept or reject a username and password pair.

In this case the user name can be accessed using the RemoteUser function of the cgi module and the 
environment variable T<HTTP_PASSWORD> holds the password available for the BASIC program. Note that this 
environment variable is specific to the web server Eszter and aceessing this environment variable is not portable.
However it is more likely that whatever web server an installation uses the Eszter SB Engine will be used 
to run ScriptBasic applications behind the web server as an application server.

Note that this function is implemented as BASIC code in the file T<cgi.bas>, therefore you can easily read and
understand how it works without the need for reading and understanding any C code.
*/

/**
=section Cookie
=H Cookie("myCookie")

This function returns the value of the cookie named as the argument. This function handles the cookies 
that the browser has sent in the http request and not the cookies that the application sends to the client.

*/

/**
=section templates
=H Template handling

CGI programs should output HTML text. Embedding this text into the code is a bad practice. In our
practice we usually use HTML templates that the program reads, modifies, inserts values and outputs. The 
CGI module support the usage of such templates.

A template is an HTML text with parameters. The parameters are placed in HTML comments, therefore 
you can easily edit these templates with the HTML editor you like. Each comment may contain a symbol 
name. The program should specify the actual value for the symbol and the module reads the template 
with the actual values replacing the comments. For example the template:

This is a template text with a
=verbatim
<!--alma-->
=noverbatim
symbol.

will be presented as
=verbatim
This is a template text with a defined symbol.
=noverbatim
assuming that the actual value of the symbol alma is the string "defined". If the value of the symbol is 
not defined by the program the comment is replaced by an empty string.

To handle symbols, and templates there are several functions in ScriptBasic. You can define a symbol 
calling the function cgi::SymbolName. To define the symbol alma you have to write:

=verbatim
cgi::SymbolName "alma" , "defined"
=noverbatim

You can also tell the module that the actual string of the symbol can be found in a file, saying:

=verbatim
cgi::SymbolFile "symbol","file name"
=noverbatim

To get the template file already with resolved symbol values you should say:

=verbatim
HtmlTemplate$ = cgi::GetHtmlTemplate("filename")
=noverbatim

or if you want to hard wire the template text into the code:

=verbatim
HtmlTemplate$ = cgi::ResolveHtml("template text to be resolved")
=noverbatim

When you are finished sending a resolved template to the client you may want to define other symbols, 
but before doing that it is safe to undefine the symbols used by the previous template. You can do that 
calling the function

=verbatim
cgi::ResetSymbols
=noverbatim

Calling this function also releases the space occupied by the symbols and their values.
For more information see the sample code.

Note that modern approach to this issue is to generate XML format output from the program and use XSLT
transformation to create the desired XHTML output.

*/
/**
=section options
=H Options that alter the behavour of the module CGI

Options can be set using the ScriptBasic command option. Each option has a 32bit value. The options 
should be set before calling the very first function of this module.

These options are:

B<Name:> T<cgi$BufferIncrease>

B<Default value:> 1024 byte

The size of the internal buffer that is used to hold the 
header lines of a multipart upload. This is the initial size 
of the buffer and this is the size of the automatic
increase whenever the buffer needs size increase.

B<Name:> T<cgi$BufferMax>

B<Default value:> 10240 byte

This is the maximal size of the buffer. If the buffer gets 
larger than this size the cgi handling process fails. This 
helps you stop attacks that send endless header fields in 
a multipart upload.

B<Name:> T<cgi$ContentMax>

B<Default value:> 10Mbyte

The maximal size of the http request.

B<Name:> T<cgi$FileMax>

B<Default value:> 10Mbyte

The maximal size of an uploaded file. If there are more 
than one file files uploaded in a single http request their 
total size should also be less than the configuration parameter cgi$ContentMax.

B<Name:> T<cgi$Method>

B<Default value:> GET, POST, UPLOAD

The allowed configuration methods. To set the value of 
this option you can use the constants defined in the file 
cgi.bas. These are:
=verbatim
const None   = &H00000000
const Get    = &H00000001
const Post   = &H00000002
const Upload = &H00000006
const Put    = &H00000008
const Del    = &H00000010
const Copy   = &H00000020
const Move   = &H00000040
const Head   = &H00000080
=noverbatim

If a client starts your CGI program with a method that is not allowed the
cgi modul will not handle that and stop with error.

You can allow more than one methods for the program. In that case the
different options should be given in an expression with biwise T<OR>
connected. For example, if you want to allow T<GET> and T<POST>
operation handled by your CGI program, but do not want to allow upload
you can use the following code segment:

=verbatim
import cgi.bas
option cgi$Method cgi::Get or cgi::Post
=noverbatim

When you want to allow upload you can write

=verbatim
import cgi.bas
option cgi$Method cgi::Upload
=noverbatim

because T<cgi::Upload> is a special kind of POST operation and therefore
the bits controlling the methods permissions are set according to this.

*/
/*
.function SymbolName
Define a named symbol passing the value.

*/
besFUNCTION(defsname)
  pModuleObject p;
  VARIABLE Argument;
  char *pszName,*pszValue;
  void **q;

  p = (pModuleObject)besMODULEPOINTER;
  pszName = pszValue = NULL;

  if( besARGNR >= 1 ){
    Argument = besARGUMENT(1);
    besDEREFERENCE(Argument);
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszName);
    }

  if( besARGNR >= 2 ){
    Argument = besARGUMENT(2);
    besDEREFERENCE(Argument);
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszValue);
    }

  if( p->symboltable == NULL ){
    p->symboltable = besNEWSYMBOLTABLE();
    if( p->symboltable == NULL )return COMMAND_ERROR_MEMORY_LOW;
    }
 
 q = besLOOKUPSYMBOL(pszName,p->symboltable,1);
 /* if there was any name defined for this symbol */
 if( *q )besFREE(*q);
 *q = pszValue;
 besFREE(pszName);
besEND

static void freesymbol_callback(char *SymbolName, char *SymbolValue, void *p){
  pSupportTable pSt=p;
  if( SymbolValue )
    besFREE(SymbolValue);
  }

/*
.function ResetSymbols
Reset the symbol table removing all symbols and releasing memory.

*/
besFUNCTION(resetsymt)

  pModuleObject p;

  p = (pModuleObject)besMODULEPOINTER;
  if( p->symboltable != NULL ){
    besTRAVERSESYMBOLTABLE(p->symboltable,(void (*)(signed char *,void *,void *))freesymbol_callback,pSt);
    besFREESYMBOLTABLE(p->symboltable);
    p->symboltable = NULL;
    }

besEND

/*
.function SymbolFile
 define a named symbol by the file it is contained
*/
besFUNCTION(defsfile)
  pModuleObject p;
  VARIABLE Argument;
  char *pszName,*pszValue,*pszFile;
  void **q;
  FILE *fp;
  long lFileLength,i;
  int ch;

  p = (pModuleObject)besMODULEPOINTER;
  pszName = pszValue = NULL;

  if( besARGNR >= 1 ){
    Argument = besARGUMENT(1);
    besDEREFERENCE(Argument);
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszName);
    }

  if( besARGNR >= 2 ){
    Argument = besARGUMENT(2);
    besDEREFERENCE(Argument);
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszFile);
    }

  fp = besFOPEN(pszFile,"rb");
  if( fp == NULL ){
    besFREE(pszName);
    besFREE(pszFile);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }
  lFileLength = besSIZE(pszFile);
  besFREE(pszFile);
  pszValue = besALLOC(lFileLength+1);
  for( i=0 ; i<lFileLength && (ch=besFGETC(fp)) != EOF ; i++ ){
    pszValue[i] = ch;
    }
  besFCLOSE(fp);
  pszValue[i] = (char)0;

  if( p->symboltable == NULL ){
    p->symboltable = besNEWSYMBOLTABLE();
    if( p->symboltable == NULL )return COMMAND_ERROR_MEMORY_LOW;
    }
 
 q = besLOOKUPSYMBOL(pszName,p->symboltable,1);
 /* if there was any name defined for this symbol */
 if( *q )besFREE(*q);
 *q = pszValue;
 besFREE(pszName);
besEND

/*
.function GetHtmlTemplate
 get a file resolving the references inside it
*/
besFUNCTION(getfile)
  pModuleObject p;
  VARIABLE Argument;
  void **q;
  char *pszFile,
       *pszName,
       *pszValue,
       *pszSymbolValue,
       *pszResult,
       cSave;
  FILE *fp;
  long lFileLength,i,j;
  int ch;
  long cRemovedChars,cInsertedChars,cTotalChars;

  p = (pModuleObject)besMODULEPOINTER;
  if( besARGNR >= 1 ){
    Argument = besARGUMENT(1);
    besDEREFERENCE(Argument);
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszFile);
    }else return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
 
  if( besARGNR >= 2 ){
    pszValue = pszFile;
   }else{
    fp = besFOPEN(pszFile,"rb");
    if( fp == NULL ){
      besFREE(pszFile);
      return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
      }
    lFileLength = besSIZE(pszFile);
    besFREE(pszFile);
    pszValue = besALLOC(lFileLength+1);
    for( i=0 ; i < lFileLength && (ch=besFGETC(fp)) != EOF ; i++ ){
      pszValue[i] = ch;
      }
    besFCLOSE(fp);
    pszValue[i] = (char)0;
    }

  cRemovedChars = 0;
  cInsertedChars = 0;
  /* here it has to resolve the references iteratively and return the result */

  /* loop counting the characters in the comments and counting the characters that are to be inserted */
  for( i=0 ; pszValue[i] ; i++ ){/*COUNT LOOP*/
    /* skip the <!--- comments --> (note: starts with three dashes!) */
    if( pszValue[i+0] == '<' && 
        pszValue[i+1] == '!' && 
        pszValue[i+2] == '-' && 
        pszValue[i+3] == '-' && 
        pszValue[i+4] == '-' ){
        i += 5;/* step over the <!--- */
        cRemovedChars += 5;
        while( pszValue[i] &&
               ! ( pszValue[i+0] == '-' &&
                   pszValue[i+1] == '-' &&
                   pszValue[i+2] == '>') )i++,cRemovedChars++;
        if( pszValue[i] ){
          i += 2;
          cRemovedChars += 3;
          }else i--;
        continue;
        }

    if( pszValue[i+0] == '<' && 
        pszValue[i+1] == '!' && 
        pszValue[i+2] == '-' && 
        pszValue[i+3] == '-' ){
      i += 4; /* step over the <!-- */
      cRemovedChars += 4;
      /* step over the spaces */
      while( pszValue[i] && isspace(pszValue[i]) )i++,cRemovedChars++;
      pszName = pszValue+i;
      while( pszValue[i] &&
             !isspace(pszValue[i]) &&
             pszValue[i] != '=' &&
             pszValue[i] != '-' )i++,cRemovedChars++;
      cSave = pszValue[i];
      pszValue[i] = (char)0;
      q = besLOOKUPSYMBOL(pszName,p->symboltable,0);
      pszValue[i] = cSave;
      if( q && *q ){
        /* there is a symbol defined */
        pszSymbolValue = (char *)*q;
        /* get the length of the string to be inserted */
        while( *pszSymbolValue )cInsertedChars++,pszSymbolValue++;
        /* go to the end of the comment */
        while( pszValue[i] &&
             ! ( pszValue[i+0] == '-' &&
                 pszValue[i+1] == '-' &&
                 pszValue[i+2] == '>') )i++,cRemovedChars++;
        if( pszValue[i] ){
          i += 2;
          cRemovedChars += 3;
          }else i--;
        continue;
        }else{
        /* the symbol is not defined in the symbol table */
        /*step over the optional spaces between the name and the = */
        while( pszValue[i] && isspace(pszValue[i]) )i++,cRemovedChars++;
        if( pszValue[i] == '=' ){/* there is some default value */
          i++,cRemovedChars++;
          /*step over the optional spaces between the = and the default value */
          while( pszValue[i] && isspace(pszValue[i]) )i++,cRemovedChars++;
          if( pszValue[i] && (pszValue[i] == '\'' || pszValue[i] == '\"') )
            cRemovedChars++,cSave = pszValue[i++];
          else
            cSave = ' ';
          while( pszValue[i] ){
            if( pszValue[i] == cSave )break;
            if( cSave == ' ' &&
                pszValue[i+0] == '-' &&
                pszValue[i+1] == '-' &&
                pszValue[i+2] == '>' )break;
            i++; /* we could also increment cRemovedChars and cInsertedChars */
            }
          }
        /* go to the end of the comment */
        while( pszValue[i] &&
             ! ( pszValue[i+0] == '-' &&
                 pszValue[i+1] == '-' &&
                 pszValue[i+2] == '>') )i++,cRemovedChars++;
          if( pszValue[i] ){
            i += 2;
            cRemovedChars += 3;
            }else i--;
          continue;
        }
      }
    }/*COUNT LOOP*/


  cTotalChars = i - cRemovedChars + cInsertedChars;
  besALLOC_RETURN_STRING(cTotalChars);
  pszResult = STRINGVALUE(besRETURNVALUE );

  /* loop replacing the symbols with the values of the referenced symbols  */
  for( j=i=0 ; pszValue[i] ; i++ ){/*REPLACE LOOP*/
    /* skip the <!--- comments --> (note: starts with three dashes!) */
    if( pszValue[i+0] == '<' && 
        pszValue[i+1] == '!' && 
        pszValue[i+2] == '-' && 
        pszValue[i+3] == '-' && 
        pszValue[i+4] == '-' ){
        i += 5;/* step over the <!--- */
        while( pszValue[i] &&
               ! ( pszValue[i+0] == '-' &&
                   pszValue[i+1] == '-' &&
                   pszValue[i+2] == '>') )i++;
        if( pszValue[i] )i += 2; else i--;
        continue;
        }

    if( pszValue[i+0] == '<' && 
        pszValue[i+1] == '!' && 
        pszValue[i+2] == '-' && 
        pszValue[i+3] == '-' ){
      i += 4; /* step over the <!-- */
      /* step over the spaces */
      while( pszValue[i] && isspace(pszValue[i]) )i++;
      pszName = pszValue+i;
      while( pszValue[i] &&
             !isspace(pszValue[i]) &&
             pszValue[i] != '=' &&
             pszValue[i] != '-' )i++;
      cSave = pszValue[i];
      pszValue[i] = (char)0;
      q = besLOOKUPSYMBOL(pszName,p->symboltable,0);
      pszValue[i] = cSave;
      if( q && *q ){
        /* there is a symbol defined */
        pszSymbolValue = (char *)*q;
        /* get the length of the string to be inserted */
        while( *pszSymbolValue )pszResult[j++] = *pszSymbolValue++;
        /* go to the end of the comment */
        while( pszValue[i] &&
             ! ( pszValue[i+0] == '-' &&
                 pszValue[i+1] == '-' &&
                 pszValue[i+2] == '>') )i++;
        if( pszValue[i] ) i += 2; else i--;
        continue;
        }else{
        /* the symbol is not defined in the symbol table */
        /*step over the optional spaces between the name and the = */
        while( pszValue[i] && isspace(pszValue[i]) )i++,cRemovedChars++;
        if( pszValue[i] == '=' ){/* there is some default value */
          i++;
          /*step over the optional spaces between the = and the default value */
          while( pszValue[i] && isspace(pszValue[i]) )i++;
          if( pszValue[i] && (pszValue[i] == '\'' || pszValue[i] == '\"') )
            cSave = pszValue[i++];
          else
            cSave = ' ';
          while( pszValue[i] ){
            if( pszValue[i] == cSave )break;
            if( cSave == ' ' &&
                pszValue[i+0] == '-' &&
                pszValue[i+1] == '-' &&
                pszValue[i+2] == '>' )break;
            pszResult[j++] = pszValue[i++];
            }
          }
        /* go to the end of the comment */
        while( pszValue[i] &&
             ! ( pszValue[i+0] == '-' &&
                 pszValue[i+1] == '-' &&
                 pszValue[i+2] == '>') )i++,cRemovedChars++;
          if( pszValue[i] ){
            i += 2;
            cRemovedChars += 3;
            }else i--;
          continue;
        }/*if( q && *q ) */
      }/* if( <!-- ... */
    pszResult[j++] = pszValue[i];
    }/*REPLACE LOOP*/

  return 0;
besEND

SLFST CGI_SLFST[] ={

{ "versmodu" , versmodu },
{ "bootmodu" , bootmodu },
{ "finimodu" , finimodu },
{ "emsgmodu" , emsgmodu },
{ "getget" , getget },
{ "getpost" , getpost },
{ "getgetex" , getgetex },
{ "getpostex" , getpostex },
{ "savefile" , savefile },
{ "filename" , filename },
{ "filelen" , filelen },
{ "ServerName" , ServerName },
{ "GatewayInterface" , GatewayInterface },
{ "ServerProtocol" , ServerProtocol },
{ "ServerPort" , ServerPort },
{ "RequestMethod" , RequestMethod },
{ "PathInfo" , PathInfo },
{ "PathTranslated" , PathTranslated },
{ "ScriptName" , ScriptName },
{ "QueryString" , QueryString },
{ "RemoteHost" , RemoteHost },
{ "RemoteAddress" , RemoteAddress },
{ "AuthType" , AuthType },
{ "RemoteUser" , RemoteUser },
{ "RemoteIdent" , RemoteIdent },
{ "ContentType" , ContentType },
{ "ContentLength" , ContentLength },
{ "UserAgent" , UserAgent },
{ "Cookie" , Cookie },
{ "Referer" , Referer },
{ "ServerSoftware" , ServerSoftware },
{ "defsname" , defsname },
{ "resetsymt" , resetsymt },
{ "defsfile" , defsfile },
{ "getfile" , getfile },
{ NULL , NULL }
  };
