import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

if cgi::Param("surecheck") <> 1 then
  cgi::SymbolName "NAME",cgi::Param("name")
  PrintTemplate "admin/suregdelete.html"
  stop
endif

GroupName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "SELECT ID FROM GROUPS WHERE NAME='" & GroupName & "'"
DB::Query(query)
if DB::FetchArray(q) then
  GroupId = q[0]
  ' delete the group from the groups table
  query = "delete from GROUPS WHERE NAME='" & GroupName & "'"
  DB::Query(query)
  ' delet the user from the groups where it was member
  query = "delete from GROUPMEMBERS WHERE group_id=" & GroupId 
  DB::Query(query)
  DB::Close
  PrintTemplate "admin/delgroup.html"
else
  cgi::SymbolName "NAME",GroupName
  PrintTemplate "admin/delnogroup.html"
end if
stop
AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
