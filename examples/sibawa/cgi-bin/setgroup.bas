import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

GroupName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,DESCRIPTION from GROUPS WHERE NAME ='" & GroupName & "'"

DB::Query query

if DB::FetchArray(q) then

  AclName = trim(cgi::Param("acl"))
  AclId = GetAclIdFromName(AclName)
  OwnerName = trim(cgi::Param("owner"))
  OwnerId = GetUserIdFromName(OwnerName)
  GroupId = q[0]
  DB::Query "UPDATE GROUPS SET DESCRIPTION='" & cgi::Param("description") & "', ACL=" & AclId & ", OWNER=" & OwnerId & " WHERE ID=" & GroupId

  DB::Query "DELETE FROM GROUPMEMBERS WHERE GROUP_ID=" & GroupId
  Iter = undef
  UserId = cgi::PostParamEx("members",Iter)
  while IsDefined(UserId)
    query = "INSERT INTO GROUPMEMBERS (group_id,user_id) VALUES (" & GroupId & "," & UserId & ")"
    DB::Query query
    UserId = cgi::PostParamEx("members",Iter)
  wend
  DB::Close

  cgi::SymbolName "DESCRIPTION",cgi::Param("description")
  cgi::SymbolName "ACL",AclName
  PrintTemplate "admin/setgroup.html"
  stop
else
  cgi::SymbolName "NAME",GroupName
  PrintTemplate("admin/setnogroup.html")
  stop
endif

DB::Close

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
