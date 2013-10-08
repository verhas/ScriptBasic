import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

on error goto AdminDataBaseError
DB::Connect
query = "select * from GROUPS WHERE NAME ='" & cgi::Param("name") & "'"
DB::Query(query)

if DB::FetchArray(q) then
  DB::Close
  PrintTemplate "admin/groupexists.html"
  stop
endif

AclName = trim(cgi::Param("acl"))
AclId = GetAclIdFromName(AclName)

OwnerName = trim(cgi::Param("owner"))
OwnerId = GetUserIdFromName(OwnerName)

if not isDefined(AclId) then
  DB::Close
  cgi::SymbolName "ACL",AclName
  PrintTemplate "admin/nosacl.html"
  stop
endif


query = "INSERT INTO GROUPS (NAME,DESCRIPTION,ACL,OWNER) VALUES('" & _
        cgi::Param("name") & "','" & _
        cgi::Param("description") & "'," & _
        AclId & "," & OwnerId & ")"

DB::Query query
DB::Close

cgi::SymbolName "NAME",cgi::Param("name")
cgi::SymbolName "DESCRIPTION",cgi::Param("description")
cgi::SymbolName "ACL",AclName
cgi::SymbolName "OWNER",OwnerName
PrintTemplate "admin/addgroup.html"
stop
AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
