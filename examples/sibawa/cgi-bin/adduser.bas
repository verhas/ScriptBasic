import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

DB::Connect
query = "select * from USERS WHERE NAME ='" & cgi::Param("name") & "'"
DB::Query(query)

if DB::FetchArray(q) then
  DB::Close
  PrintTemplate "admin/userexists.html"
  stop
endif

if cgi::Param("newpassword") <> cgi::Param("verpassword") then
  DB::Close
  PrintTemplate "admin/mismatchnew.html"
  stop
end if

UserName = trim(cgi::Param("name"))

AclName = trim(cgi::Param("acl"))
AclId = GetAclIdFromName(AclName)

OwnerName = trim(cgi::Param("owner"))
if OwnerName <> UserName then
  OwnerId = GetUserIdFromName(OwnerName)

  query = "INSERT INTO USERS (NAME,REALNAME,PASSWORD,PRIV,ACL,OWNER) VALUES('" & _
            cgi::Param("name") & "','" & _
            cgi::Param("realname") & "','" & _
            cgi::Param("newpassword") & "','" & _
            cgi::Param("priv") & "'," & _
            OwnerId & "," & _
            AclId & ")"
  DB::Query query
  DB::Close
else
  ' if the user is going to be the owner of its own user record (not recommended)
  query = "INSERT INTO USERS (NAME,REALNAME,PASSWORD,PRIV,ACL) VALUES('" & _
            cgi::Param("name") & "','" & _
            cgi::Param("realname") & "','" & _
            cgi::Param("newpassword") & "','" & _
            cgi::Param("priv") & "'," & _
            AclId & ")"
  DB::Query query
  ' make it its own owner
  query = "UPDATE USERS SET OWNER=ID WHERE NAME='" & UserName & "'"
  DB::Query query
endif

cgi::SymbolName "REALNAME",cgi::Param("realname")
cgi::SymbolName "PRIV",cgi::Param("priv")
cgi::SymbolName "NAME",UserName
cgi::SymbolName "OWNER",OwnerName
cgi::SymbolName "ACL",AclName
PrintTemplate "admin/adduser.html"

end
