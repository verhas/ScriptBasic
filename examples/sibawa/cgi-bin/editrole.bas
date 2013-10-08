import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

RoleId = cgi::Param("role")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,DESCRIPTION from ROLES WHERE ID =" & RoleId
DB::Query query

if DB::FetchArray(q) then
  RoleId = q[0]
  RoleName = q[1]
  cgi::SymbolName "ID",RoleId
  cgi::SymbolName "NAME",RoleName
  cgi::SymbolName "DESCRIPTION",q[2]
  PrintTemplate("admin/editrole.html")
else
  cgi::SymbolName "NAME",RoleId
  PrintTemplate("admin/editnorole.html")
endif

DB::Close

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end

