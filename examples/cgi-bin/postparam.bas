#! /usr/bin/scriba -c
include cgi.bas
cgi::Header 200,"text/html"
cgi::FinishHeader
print """<HTML>
<BODY>
<PRE>
<FORM METHOD="POST">
<INPUT TYPE="TEXT" NAME="apple">
</FORM>
"""
print cgi::PostParam("apple")
print """</PRE>
</BODY>
</HTML>
"""
stop

