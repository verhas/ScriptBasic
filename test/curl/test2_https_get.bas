import curl.bas

on error goto curle

URL$ = "https://www.otpbank.hu/otpportal2000/"

CURL = curl::init()
curl::option CURL,"URL",URL$
curl::option CURL,"PROXY","http://gate-internal.westel900.hu:8080"
print "Getting the page ", URL$,"\n"
print curl::perform(CURL)
curl::finish CURL
print """Program was executed without error.
"""
stop
curle:
print "***ERROR: ",curl::error(CURL),"\n"

