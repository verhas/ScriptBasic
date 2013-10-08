import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

GroupName = cgi::Param("group")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,DESCRIPTION from GROUPS"
if IsDefined(GroupName) then
  query = query & " WHERE NAME LIKE '" & GroupName & "%'"
endif

PrintTemplate("admin/grouplisthead.html")

DB::Query query


while DB::FetchArray(q)
  cgi::SymbolName "ID",q[0]
  cgi::SymbolName "NAME",q[1]
  cgi::SymbolName "DESCRIPTION",q[2]
  PrintTemplate("admin/grouplistline.html")
  cgi::ResetSymbols
wend

DB::Close
PrintTemplate("admin/grouplisttail.html")
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
