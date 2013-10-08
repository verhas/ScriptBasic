#! /usr/bin/scriba -c
include cgi.bas
cgi::Header 200,"text/html"
cgi::FinishHeader
print """<HTML>
<BODY>
<PRE>
"""
i = 0
while IsDefined( Environ(i) )
  print i," ",Environ(i),"\n"
  i = i+1
wend
print """</PRE>
</BODY>
</HTML>
"""
stop
