#! /usr/bin/scriba -c
include cgi.bas
cgi::Header 200,"text/html"
cgi::FinishHeader
print """<HTML>
<BODY>
<PRE>
"""
print cgi::GetParam("apple")
print """</PRE>
</BODY>
</HTML>
"""
stop
