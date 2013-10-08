import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

UserName = cgi::Param("user")

on error goto AdminDataBaseError
DB::Connect

DB::Query "select ID,NAME,REALNAME,PRIV,ACL,OWNER FROM USERS WHERE NAME ='" & UserName & "'"

if DB::FetchArray(q) then
  UserId = q[0]
  UserName = q[1]
  cgi::SymbolName "NAME",UserName
  cgi::SymbolName "REALNAME",q[2]
  cgi::SymbolName "PRIV",q[3]
  AclId = q[4]
  AclName = GetAclNameFromId(AclId)
  cgi::SymbolName "ACL",AclName
  OwnerId = q[5]
  OwnerName = GetUserNameFromId(OwnerId)
  cgi::SymbolName "OWNER",OwnerName
  PrintTemplate "admin/edituserhead.html"

  query = """
SELECT g.ID, g.NAME, gm.USER_ID
 FROM GROUPS g LEFT JOIN GROUPMEMBERS gm ON gm.GROUP_ID = g.ID AND gm.USER_ID=""" & UserId & """, USERS u
 WHERE u.ID=""" & UserId
  DB::Query query
  while DB::FetchArray(q)
    cgi::ResetSymbols
    cgi::SymbolName "ID",q[0]
    cgi::SymbolName "NAME",q[1]
    if q[2] = UserId then  cgi::SymbolName "SELECTED"," SELECTED"
    PrintTemplate("admin/editgroupse.html")
  wend

  cgi::SymbolName "NAME",UserName
  PrintTemplate("admin/editusertail.html")
else
  cgi::SymbolName "NAME",UserName
  PrintTemplate("admin/editnouser.html")
endif

DB::Close

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
