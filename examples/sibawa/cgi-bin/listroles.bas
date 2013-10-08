import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

RoleName = cgi::Param("role")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,DESCRIPTION from ROLES"
if IsDefined(RoleName) then
  query = query & " WHERE NAME LIKE '" & RoleName & "%'"
endif

PrintTemplate("admin/rolelisthead.html")

DB::Query query

while DB::FetchArray(q)
  cgi::SymbolName "ID",q[0]
  cgi::SymbolName "NAME",q[1]
  cgi::SymbolName "DESCRIPTION",q[2]
  PrintTemplate("admin/rolelistline.html")
  cgi::ResetSymbols
wend

DB::Close
PrintTemplate("admin/rolelisttail.html")
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end

