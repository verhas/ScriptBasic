import cgi.bas

q = undef
cgi::Header 200,"text/html"
cgi::FinishHeader

print """
<HTML>
<HEAD>
<title>GetParamEx test</title>
</HEAD>
<BODY>
<FONT FACE="Verdana" SIZE="2">
<H1>GetParamEx testing</H1>
This is a test that checks the cgi module function GetParamEx. Call this script
using the URL <FONT SIZE="3">
<pre>
/cgi-bin/cgimodtest.bas?a=1&amp;a=2&amp;a=anything
</pre>
</font>
<p>
"""
do

param = cgi::GetParamEx("a",q)
print "Param is ",param," q is ",q,"<p>\n"

loop while IsDefined(param)

print """That is all
</BODY>
</HTML>
"""
