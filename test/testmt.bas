#!/usr/bin/scriba -c
global const nl = "\n"
Const NumberOfCookies = 3

include cgi.bas
include mt.bas

option cgi$Method cgi::Get or cgi::Upload

cgi::Header 200,"text/html"

cgi::FinishHeader

'-------------------------------------------------------
print """<HTML>
<HEAD>
<title>MT module testing</title>
</HEAD>
<BODY><font face="VERDANA" size="2">
<H1>MT module testing</H1>

"""
'-------------------------------------------------------

MTVar = mt::GetVariable("testvariable")
if not isdefined(MTVar) then MTVar = 1
MTVar = MTVar +1
mt::SetVariable "testvariable",MTVar

print "The variable is: ",MTVar,"""
<p>
"""

call mt::SetSessionId "1"
if not isdefined(mt::GetSessionVariable("a")) then mt::SetSessionVariable "a",13
mt::SetSessionVariable "a" ,mt::GetSessionVariable("a")+1
print "The session variable \"a\" is ",mt::GetSessionVariable("a"),"<p>\n"

mt::DeleteSession "1"

call mt::SetSessionId "2"
if not isdefined(mt::GetSessionVariable("a")) then  mt::SetSessionVariable "a",14
mt::SetSessionVariable "a",mt::GetSessionVariable("a")+2
print "The session variable \"a\" is ",mt::GetSessionVariable("a"),"<p>\n"

print "The current directory is ",curdir(),"<p>\n"

print "The new session is:",mt::GetSessionId(),"""
mt:SessionTimeout 20*60,"timeout.bas"
</BODY>
</HTML>
"""
