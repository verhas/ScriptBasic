import "../sibawa.ini"
use dbg
CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

DB::Connect

AclTypeName = trim(cgi::Param("name"))
AclTypeDescription = cgi::Param("description")

DB::Query "select * from ACLTYPES WHERE NAME ='" & AclTypeName & "'"

if DB::FetchArray(q) then
  DB::Close
  PrintTemplate "admin/acltypeexists.html"
  stop
endif

query = "INSERT INTO ACLTYPES (NAME,DESCRIPTION,"
for i=1 to 32
 query = query & "BIT" & i & ",DESCRIPTION" & i
 if i < 32 then query = query & ","
next i

query = query & ") VALUES('" & AclTypeName & "','" & AclTypeDescription & "',"

for i=1 to 32
 query = query & "'" & trim(cgi::param("bit"&i)) & "','" & cgi::param("description"&i) & "'"
 if i < 32 then query = query & ","
next i
query = query & ")"

DB::Query query

DB::Query "select ID from ACLTYPES WHERE NAME ='" & AclTypeName & "'"
DB::FetchArray q
AclTypeId = q[0]
DB::Close

mt::SetVariable "AclTypeName:" & AclTypeId , AclTypeName
mt::SetVariable "AclTypeId:" & AclTypeName , AclTypeId
mt::SetVariable "AclTypeDescription:" & AclTypeId, AclTypeDescription
for i=1 to 32
  mt::SetVariable "AclTypeBitName:" & AclTypeId & ":" & i , trim(cgi::param("bit"&i))
  mt::SetVariable "AclTypeNameBit:" & AclTypeId & ":" & trim(cgi::param("bit"&i)) , i
  mt::SetVariable "AclTypeBitDescription:" & AclTypeId & ":"  & i , cgi::param("description"&i)
next i

cgi::SymbolName "NAME",AclTypeName
cgi::SymbolName "DESCRIPTION",AclTypeDescription

PrintTemplate "admin/addacltype.html"
stop

end
