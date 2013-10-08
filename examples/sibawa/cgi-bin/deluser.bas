import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

if cgi::Param("surecheck") <> 1 then
  cgi::SymbolName "NAME",cgi::Param("name")
  PrintTemplate "admin/suredelete.html"
  stop
endif

UserName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "SELECT ID FROM USERS WHERE NAME='" & UserName & "'"
DB::Query(query)
if DB::FetchArray(q) then
  UserId = q[0]
  ' delete the user from the users table
  query = "delete from USERS WHERE NAME ='" & UserName & "'"
  DB::Query(query)
  ' delet the user from the groups where it was member
  query = "delete from GROUPMEMBERS WHERE user_id=" & UserId 
  DB::Query(query)
  DB::Close
  PrintTemplate "admin/deluser.html"
else
  cgi::SymbolName "NAME",UserName
  PrintTemplate "admin/delnouser.html"
end if
stop
AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
