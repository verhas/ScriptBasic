import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

if cgi::Param("surecheck") <> 1 then
  cgi::SymbolName "NAME",cgi::Param("name")
  PrintTemplate "admin/suacltypedelete.html"
  stop
endif

AclTypeName = trim(cgi::Param("name"))
on error goto AdminDataBaseError
DB::Connect
query = "SELECT ID FROM ACLTYPES WHERE NAME='" & AcltypeName & "'"
DB::Query(query)

if not DB::FetchArray(q) then
  cgi::SymbolName "NAME",AclTypeName
  PrintTemplate "admin/delnoacltype.html"
end if
AclTypeId = q[0]
query = "SELECT ID FROM ACLS WHERE ACLTYPE=" & AcltypeId
DB::Query(query)
if DB::FetchArray(q) then
  cgi::SymbolName "NAME",AclTypeName
  PrintTemplate "admin/acltypeused.html"
  DB::Close
  stop
endif
query = "DELETE FROM ACLTYPES WHERE ID=" & AcltypeId
DB::Query(query)
DB::Close

mt::SetVariable "AclTypeName:" & AclTypeId , undef
mt::SetVariable "AclTypeId:" & AclTypeName , undef
mt::SetVariable "AclTypeDescription:" & AclTypeId, undef
for i=1 to 32
  mt::SetVariable "AclTypeBitName:" & AclTypeId & ":" & i , undef
  mt::SetVariable "AclTypeNameBit:" & AclTypeId & ":" & q[2*(i-1)+3] , undef
  mt::SetVariable "AclTypeBitDescription:" & AclTypeId & ":"  & i , undef
next i


PrintTemplate "admin/delacltype.html"

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
