#!/usr/bin/scriba -c
use dbg
global const nl = "\n"
Const NumberOfCookies = 3

include cgi.bas

option cgi$Method cgi::Get or cgi::Upload

' cgi::RequestBasicAuthentication "Test Realm"
cgi::Header 200,"text/html"

'
' We are setting several cookies. The expiry time is ten seconds so you can test that
' the cookies are sent by the browser if you press some of the buttons fast enough,
' but it does not if you are slow
'
for i=1 to NumberOfCookies 
  ' cookie(i) is i, no domain is defined, path is /, expires after 5 seconds, not secure
  cgi::SetCookie "cookie" & i,i,undef,"/",gmtime()+10,false
next i

cgi::FinishHeader

'-------------------------------------------------------
print """<HTML>
<HEAD>
<title>CGI parameter testing</title>
</HEAD>
<BODY><font face="VERDANA" size="2">
<H1>View CGI Parameters</H1>
This page shows the cgi parameters the way it was uploaded.
<!-- here is the result of the previous HTTP request -->
<FONT SIZE="3">
<PRE>
CGI system variables
--------------------

"""
'-------------------------------------------------------

print "ServerSoftware  = ",cgi::ServerSoftware(), nl
print "ServerName      = ",cgi::ServerName(), nl
print "GatewayInterface= ",cgi::GatewayInterface(),nl
print "ServerProtocol  = ",cgi::ServerProtocol(), nl
print "ServerPort      = ",cgi::ServerPort(), nl
print "RequestMethod   = ",cgi::RequestMethod(), nl
print "PathInfo        = ",cgi::PathInfo(), nl
print "PathTranslated  = ",cgi::PathTranslated(), nl
print "ScriptName      = ",cgi::ScriptName(), nl
print "QueryString     = ",cgi::QueryString(), nl
print "RemoteHost      = ",cgi::RemoteHost(), nl
print "RemoteAddress   = ",cgi::RemoteAddress(), nl
print "AuthType        = ",cgi::AuthType(), nl
print "RemoteUser      = ",cgi::RemoteUser(), nl
print "RemoteIdent     = ",cgi::RemoteIdent(), nl
print "ContentType     = ",cgi::ContentType(), nl
print "ContentLength   = ",cgi::ContentLength(), nl
print "UserAgent       = ",cgi::UserAgent(), nl
print "Cookie          = ",cgi::RawCookie(), nl

print "Referer         = ",cgi::Referer(),nl
print "Password        = ",Environ("HTTP_PASSWORD"),nl
print "Full auth string= ",Environ("HTTP_AUTHORIZATION"),nl
print "\nCookies:\n"
for i=1 to NumberOfCookies
  print "cookie" & i," ",cgi::Cookie("cookie" & i),"\n"
next i

print "Text field using Param(\"TEXT-FIELD\") is ",cgi::Param("TEXT-FIELD"),nl,nl


if cgi::RequestMethod() = "GET" then
  print "GET text field using GetParam(\"TEXT-FIELD\") is ",cgi::GetParam("TEXT-FIELD"),nl
end if

if cgi::RequestMethod() = "POST" then
  print "POST text field using PostParam(\"TEXT-FIELD\") is ",cgi::PostParam("TEXT-FIELD"),nl
  if cgi::ContentType() like "multipart*" then
    print "Original file name is ",cgi::FileName("FILE-UPLOAD-NAME"),nl
    if cgi::FileLength("FILE-UPLOAD-NAME") > 0 then
      print "File of length ",cgi::FileLength("FILE-UPLOAD-NAME")," bytes is saved\n"
      on error goto NoSave
      cgi::SaveFile "FILE-UPLOAD-NAME","E:/MyProjects/sb/upload.txt"
    else
      print "There is no uploaded file."
    end if
  end if
end if

print """</PRE><TABLE><TR><TD BORDER=0 BGCOLOR="EEEEEE"><PRE>
A simple form to POST parameters:<BR>
<FORM METHOD="POST" ACTION="echo.bas">
<INPUT TYPE="TEXT" VALUE="DEFAULT TEXT" NAME="TEXT-FIELD">
<INPUT TYPE="SUBMIT" NAME="SUBMIT-BUTTON" VALUE=" POST ">
</FORM>
</PRE></TD><TD BORDER=1 width="20">&nbsp;</TD><TD BORDER=0 BGCOLOR="EEEEEE"><PRE>
A simple form to GET parameters:<BR>
<FORM METHOD="GET" ACTION="echo.bas">
<INPUT TYPE="TEXT" VALUE="DEFAULT TEXT" NAME="TEXT-FIELD">
<INPUT TYPE="SUBMIT" NAME="SUBMIT-BUTTON" VALUE=" GET ">
</FORM>
</PRE></TD></TR></TABLE><PRE>
<hr>
A simple form to UPLOAD a file:<BR>
<FORM METHOD="POST" ACTION="echo.bas" ENCTYPE="multipart/form-data">
<INPUT TYPE="TEXT" VALUE="DEFAULT TEXT" NAME="TEXT-FIELD">
<INPUT TYPE="FILE" VALUE="FILE-UPLOAD-VALUE" NAME="FILE-UPLOAD-NAME">
<INPUT TYPE="SUBMIT" NAME="SUBMIT-BUTTON" VALUE="UPLOAD FILE">
</FORM>
<hr>
</BODY>
</HTML>
"""
stop
NoSave:

print "An error has happened saving the file. Code =",error(),nl

resume next
