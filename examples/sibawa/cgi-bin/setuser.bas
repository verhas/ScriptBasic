import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

UserName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,REALNAME,PRIV from USERS WHERE NAME ='" & UserName & "'"

DB::Query query

if not DB::FetchArray(q) then
  cgi::SymbolName "NAME",UserName
  PrintTemplate("admin/setnouser.html")
  stop
endif

UserId = q[0]
cgi::SymbolName "REALNAME",cgi::Param("realname")
cgi::SymbolName "PRIV",cgi::Param("priv")
cgi::SymbolName "NAME",cgi::Param("name")

AclName = trim(cgi::Param("acl"))
AclId = GetAclIdFromName(AclName)

UserName = trim(cgi::Param("owner"))
UserId = GetUserIdFromName(UserName)

'
' set attributes with or w/o password altering
'
if cgi::Param("setpassword") then
  '
  ' set attributes with password altering
  '
  if cgi::Param("newpassword") <> cgi::Param("verpassword") then
    cgi::SymbolName "NAME",UserName
    PrintTemplate "admin/mismatch.html"
    stop
  endif
  query = "UPDATE USERS SET REALNAME='" & cgi::Param("realname") & _
                       "', PRIV='"     & cgi::Param("priv") & _
                       "', PASSWORD='" & cgi::Param("newpassword") & _
                       "', ACL=" & AclId & _
                       ",  OWNER=" & UserId & _
                       " WHERE NAME='" & cgi::Param("name") & "'"
  DB::Query query
else
  '
  ' set attributes and leave password unchanged
  '
  query = "UPDATE USERS SET REALNAME='" & cgi::Param("realname") & _
                       "', PRIV='"     & cgi::Param("priv") & _
                       "', ACL=" & AclId & _
                       ",  OWNER=" & UserId & _
                       " WHERE NAME='" & cgi::Param("name") & "'"
  DB::Query query
endif

'
' alter the user group membership
'
query = "DELETE FROM GROUPMEMBERS WHERE user_id=" & UserId
DB::Query query
Iter = undef
GroupId = cgi::PostParamEx("groups",Iter)
while IsDefined(GroupId)
  query = "INSERT INTO GROUPMEMBERS (group_id,user_id) VALUES (" & GroupId & "," & UserId & ")"
  DB::Query query
  GroupId = cgi::PostParamEx("groups",Iter)
wend
DB::Close

PrintTemplate "admin/setuser.html"
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
