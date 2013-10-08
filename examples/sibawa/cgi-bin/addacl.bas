import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

on error goto AdminDataBaseError
DB::Connect
query = "select * from ACLS WHERE NAME ='" & cgi::Param("name") & "'"
DB::Query(query)

if DB::FetchArray(q) then
  DB::Close
  PrintTemplate "admin/aclexists.html"
  stop
endif

AclName = trim(cgi::Param("acl"))
AclId = GetAclIdFromName(AclName)

AclTypeName = trim(cgi::Param("acltype"))
AclTypeId = GetAclTypeIdFromName(AclTypeName)

OwnerName = trim(cgi::Param("owner"))
OwnerId = GetUserIdFromName(OwnerName)

query = "INSERT INTO ACLS (ACLID,ACLTYPE,NAME,DESCRIPTION,OWNER) VALUES(" & AclId & "," & AclTypeId & ",'" & _
        cgi::Param("name") & "','" & _
        cgi::Param("description") & "'," & _
        OwnerId & ")"

DB::Query query
DB::Close

cgi::SymbolName "NAME",cgi::Param("name")
cgi::SymbolName "DESCRIPTION",cgi::Param("description")
cgi::SymbolName "ACL",cgi::Param("acl")
cgi::SymbolName "OWNER",cgi::Param("owner")
cgi::SymbolName "ACLTYPE",AclTypeName
PrintTemplate "admin/addacl.html"
stop
AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
