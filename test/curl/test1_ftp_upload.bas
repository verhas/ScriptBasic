import curl.bas

on error goto curle

CURL = curl::init()
curl::option CURL,"URL","ftp://127.0.0.1/test" & now & ".txt"
curl::option CURL,"USERPWD","cvs:cvs_user"
curl::option CURL,"UPLOAD"
curl::option CURL,"TRANSFERTEXT"
curl::option CURL,"INTEXT","""This text is going to go into this file.
The time now is: """ &  FORMATDATE("YEAR MM DD hh:mm:ss am") & """

Have a nice day!
Peter
"""
curl::perform(CURL)
curl::finish CURL
print """Program was executed without error.
See if there is a new file in the account cvs home directory.
"""
stop
curle:
print "***ERROR: ",curl::error(CURL),"\n"

