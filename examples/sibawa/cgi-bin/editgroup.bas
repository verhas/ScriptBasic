import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

GroupId = cgi::Param("group")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,DESCRIPTION,ACL,OWNER from GROUPS WHERE ID =" & GroupID
DB::Query query

if DB::FetchArray(q) then
  GroupId     = q[0]
  GroupName   = q[1]
  Description = q[2]
  AclId       = q[3]
  OwnerId     = q[4]
  AclName   = GetAclNameFromId(AclId)
  OwnerName = GetUserNameFromId(OwnerId)
  cgi::SymbolName "ACL",AclName
  cgi::SymbolName "ID",GroupId
  cgi::SymbolName "NAME",GroupName
  cgi::SymbolName "DESCRIPTION",q[2]
  cgi::SymbolName "OWNER",OwnerName
  PrintTemplate("admin/editgrouphead.html")
  query = """
SELECT u.ID, u.NAME, gm.GROUP_ID
 FROM  USERS u LEFT JOIN GROUPMEMBERS gm ON gm.USER_id = u.ID AND gm.GROUP_ID=""" & GroupId & """, GROUPS g
 WHERE g.ID=""" & GroupId
  DB::Query query
  while DB::FetchArray(q)
    cgi::ResetSymbols
    cgi::SymbolName "ID",q[0]
    cgi::SymbolName "NAME",q[1]
    if q[2] = GroupId then  cgi::SymbolName "SELECTED"," SELECTED"
    PrintTemplate("admin/editgroupse.html")
  wend
  cgi::SymbolName "NAME",GroupName
  PrintTemplate("admin/editgrouptail.html")
else
  cgi::SymbolName "NAME",UserName
  PrintTemplate("admin/editnouser.html")
endif

DB::Close

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
