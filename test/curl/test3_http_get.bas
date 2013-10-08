import curl.bas

UseProxy = "http://gate-internal.westel900.hu:8080"

ReStartWithoutProxy:

on error goto curle

URL$ = "http://scriptbasic.com/"

CURL = curl::init()
curl::option CURL,"URL",URL$
if UseProxy then curl::option CURL,"PROXY","http://gate-internal.westel900.hu:8080"
print "Getting the page ", URL$," via proxy:",UseProxy,"\n"
print curl::perform(CURL)
curl::finish CURL
print """Program was executed without error.
"""
stop
curle:
if UseProxy then
  UseProxy = ""
  goto ReStartWithoutProxy  
endif

print "***ERROR: ",curl::error(CURL),"\n"

