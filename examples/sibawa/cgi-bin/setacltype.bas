import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

AclTypeName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,DESCRIPTION from ACLTYPES WHERE NAME ='" & AclTypeName & "'"

DB::Query query

if not DB::FetchArray(q) then
  cgi::SymbolName "NAME",AclTypeName
  PrintTemplate("admin/setnoacltype.html")
  stop
endif
AclTypeId = q[0]

if q[1] <> cgi::Param("description") then
  query = "UPDATE ACLTYPES SET DESCRIPTION='" & cgi::Param("description") & "' WHERE ID=" & AclTypeId
  DB::Query query
  mt::SetVariable "AclTypeDescription:" & AclTypeId , cgi::Param("description")
endif

query = "UPDATE ACLTYPES SET "

for i=1 to 32
  query = query & "BIT" & i & "='" & cgi::Param("bit" & i ) & "', "
  query = query & "DESCRIPTION" & i & "='" & cgi::Param("description" & i ) & "'"
  if i < 32 then query = query & ", "
  mt::SetVariable "AclTypeBitName:" & AclTypeId & ":" & i , cgi::Param("bit" & i )
  mt::SetVariable "AclTypeNameBit:" & AclTypeId & ":" & cgi::Param("bit" & i ) , i
  mt::SetVariable "AclTypeBitDescription:" & AclTypeId & ":"  & i , cgi::Param("description" & i )
next i

DB::Query query

DB::Close

cgi::SymbolName "DESCRIPTION",cgi::Param("description")
PrintTemplate "admin/setacltype.html"

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
