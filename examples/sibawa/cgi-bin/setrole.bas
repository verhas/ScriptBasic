import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

RoleName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,DESCRIPTION from ROLES WHERE NAME ='" & RoleName & "'"

DB::Query query

if DB::FetchArray(q) then
  cgi::SymbolName "DESCRIPTION",cgi::Param("description")
  RoleId = q[0]
  query = "UPDATE ROLES SET DESCRIPTION='" & cgi::Param("description") & "' WHERE ID=" & RoleId
  DB::Query query
  DB::Close
  PrintTemplate "admin/setrole.html"
  stop
else
  cgi::SymbolName "NAME",RoleName
  PrintTemplate("admin/setnorole.html")
  stop
endif

DB::Close

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
