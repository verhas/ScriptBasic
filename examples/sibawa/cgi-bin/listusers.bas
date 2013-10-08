import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

UserName = cgi::Param("user")

on error goto AdminDataBaseError
DB::Connect
query = "select NAME,REALNAME,PRIV from USERS"
if IsDefined(UserName) then
  query = query & " WHERE NAME LIKE '" & UserName & "%'"
endif

PrintTemplate("admin/userlisthead.html")

DB::Query query


while DB::FetchArray(q)
  cgi::SymbolName "NAME",q[0]
  cgi::SymbolName "REALNAME",q[1]
  if len(q[2]) = 0 then q[2] = "&nbsp;"
  cgi::SymbolName "PRIV",q[2]
  PrintTemplate("admin/userlistline.html")
  cgi::ResetSymbols
wend

DB::Close
PrintTemplate("admin/userlisttail.html")
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
