import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

AcltypeName = cgi::Param("acltype")

on error goto AdminDataBaseError
DB::Connect
query = "select ID,NAME,DESCRIPTION from ACLTYPES"
if IsDefined(RoleName) then
  query = query & " WHERE NAME LIKE '" & AcltypeName & "%'"
endif

PrintTemplate("admin/acltypelisthead.html")

DB::Query query


while DB::FetchArray(q)
  cgi::SymbolName "ID",q[0]
  cgi::SymbolName "NAME",q[1]
  cgi::SymbolName "DESCRIPTION",q[2]
  PrintTemplate("admin/acltypelistline.html")
  cgi::ResetSymbols
wend

DB::Close
PrintTemplate("admin/acltypelisttail.html")
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end

