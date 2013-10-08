' FILE: testodbc.bas

import odbc.bas

' on error goto ErrorHandler

DB = ODBC::RealConnect("test","","")

print "Connected to DSN=\"test\", connection id is ",DB,"\n"

ODBC::Query 13," "

ODBC::Query DB,"DELETE FROM test;"

print "Deleted rows: ",ODBC::AffectedRows(DB),"\n"
print

for i=1 to 20
print "inserting ",i,"\n"
' ODBC::Query DB,"INSERT INTO test VALUES ('"&STRING(600,"A")&i&"','haliho',"&i&");"
ODBC::Query DB,"INSERT sss INTO test VALUES ('A"&i&"','haliho',"&i&");"
next i

ODBC::Query DB,"SELECT COUNT(*) FROM test;"
ODBC::FetchHash DB,arr
print "The number of rows selected: ",arr[0]," ",arr[1],"\n"

ODBC::Query DB,"SELECT * FROM test"
print "selected rows: ",ODBC::AffectedRows(DB)

while ODBC::FetchHash(DB,hash)

  for i=lbound(hash) to ubound(hash) step 2
    print hash[i],"=",hash[i+1],", "
  next i
  print "length of name is =",len(hash{"name"})
  print

wend

print "---------------------------------------------\n"

ODBC::Query DB,"SELECT * FROM test"
print "selected rows: ",ODBC::AffectedRows(DB),"\n"

while ODBC::FetchArray(DB,hash)

  for i=lbound(hash) to ubound(hash)
    print hash[i],", "
  next i
  print

wend


ODBC::Close DB


STOP

ErrorHandler:
print "An error has happened. The ODBC error message is\n"
print ODBC::Error(DB)
print
print "The SB error code is:",HEX(error())
error error()
