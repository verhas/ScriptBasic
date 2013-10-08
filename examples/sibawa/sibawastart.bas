'
' sibawastart.bas
'
' This program is started by the SIBAWA system when the application starts.
'
' This program loads the cache authentication and access control into memory
' and also maintains the session data handling session time out
' use dbg
import mt.bas
import mysql.bas

'
' Load all ACLTYPES into memory
'
on error goto sibawaDataBaseError
dbh = mysql::Connect("auth")

query = "select ID,NAME,DESCRIPTION,"
for i=1 to 32
 query = query & "BIT" & i & ",DESCRIPTION" & i
 if i < 32 then query = query & ","
next i
query = query & " from ACLTYPES"

mysql::query dbh,query

while isDefined(mysql::fetcharray(dbh,q))
  AclTypeId          = q[0]
  AclTypeName        = q[1]
  AclTypeDescription = q[2]

  mt::SetVariable "AclTypeName:" & AclTypeId , AclTypeName
  mt::SetVariable "AclTypeId:" & AclTypeName , AclTypeId
  mt::SetVariable "AclTypeDescription:" & AclTypeId, AclTypeDescription
  for i=1 to 32
    mt::SetVariable "AclTypeBitName:" & AclTypeId & ":" & i , q[2*(i-1)+3]
    mt::SetVariable "AclTypeNameBit:" & AclTypeId & ":" & q[2*(i-1)+3] , i
    mt::SetVariable "AclTypeBitDescription:" & AclTypeId & ":"  & i , q[2*(i-1)+4]
  next i
wend
mysql::close dbh

stop

sibawaDataBaseError:
print "A database error has happened."

