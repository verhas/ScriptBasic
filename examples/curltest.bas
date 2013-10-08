import curl.bas

on error goto curle

CURL = curl::init()
curl::option CURL,"URL","ftp://127.0.0.1/test" & now & ".txt"
curl::option CURL,"USERPWD","verhas:ofolnajtofi7"
curl::option CURL,"UPLOAD"
curl::option CURL,"TRANSFERTEXT"
' curl::option CURL,"CRLF"
curl::option CURL,"INTEXT","""Ezt a szöveget fogjuk feltölteni ebbe a fájlba.
Most az idõ: """ &  FORMATDATE("YEAR MM DD hh:mm:ss am")
print curl::perform(CURL)
curl::finish CURL
print "OK"
stop
curle:
print "***ERROR: ",curl::error(CURL),"\n"

