import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

on error goto AdminDataBaseError
DB::Connect
query = "select * from ROLES WHERE NAME ='" & cgi::Param("name") & "'"
DB::Query(query)

if DB::FetchArray(q) then
  DB::Close
  PrintTemplate "admin/roleexists.html"
  stop
endif

query = "INSERT INTO ROLES (NAME,DESCRIPTION) VALUES('" & _
        cgi::Param("name") & "','" & _
        cgi::Param("description") & "')"

DB::Query query
DB::Close

cgi::SymbolName "NAME",cgi::Param("name")
cgi::SymbolName "DESCRIPTION",cgi::Param("description")

PrintTemplate "admin/addrole.html"
stop
AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
