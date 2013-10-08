#!/usr/bin/scriba -c

'
' Call this program from several browsers and test locking and unlockings
' seeing how the different programs run and return an answer
'


global const nl = "\n"

include cgi.bas
include mt.bas

option cgi$Method cgi::Get or cgi::Upload

cgi::Header 200,"text/html"

cgi::FinishHeader

'-------------------------------------------------------
print """<HTML>
<HEAD>
<title>MT module lock testing</title>
</HEAD>
<BODY><font face="VERDANA" size="2">
<H1>MT module lock testing</H1>

"""
'-------------------------------------------------------

if isdefined(cgi::Param("lockw")) then
  mt::LockWrite cgi::Param("lockw")
end if

if isdefined(cgi::Param("lockr")) then
  mt::LockRead cgi::Param("lockr")
end if

if isdefined(cgi::Param("unlockw")) then
  mt::UnlockWrite cgi::Param("unlockw")
end if

if isdefined(cgi::Param("unlockr")) then
  mt::UnlockRead cgi::Param("unlockr")
end if

print """This is OK
</BODY>
</HTML>
"""
