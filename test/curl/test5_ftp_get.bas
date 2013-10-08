import curl.bas
import re.bas

on error goto curle

CONTENT$ ="""
This is just to have there something.
"""

CURL = curl::init()
curl::option CURL,"URL","ftp://127.0.0.1/test.txt"
curl::option CURL,"USERPWD","cvs:cvs_user"
curl::option CURL,"UPLOAD"
curl::option CURL,"INTEXT",CONTENT$
curl::perform(CURL)
curl::finish CURL

CURL = curl::init()
curl::option CURL,"URL","ftp://127.0.0.1/test.txt"
curl::option CURL,"USERPWD","cvs:cvs_user"
curl::option CURL,"FILE","test.txt"
curl::perform(CURL)
curl::finish CURL

print """Now there should be a file names test.txt containing the string:"""
print CONTENT$
print """If it is there you will see it now as I execute SYSTEM(\"cat test.txt\")
----------------------------------------------\n"""
a = execute("cat test.txt",60,PID)

print "----------------------------------------------\nwas it?\n"
print "Now I delete this file\n"
delete "test.txt"
stop
curle:
print "***ERROR: ",curl::error(CURL),"\n"

