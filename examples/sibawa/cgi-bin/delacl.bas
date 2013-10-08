import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

if cgi::Param("surecheck") <> 1 then
  cgi::SymbolName "NAME",cgi::Param("name")
  PrintTemplate "admin/saredelete.html"
  stop
endif

AclName = cgi::Param("name")

on error goto AdminDataBaseError
DB::Connect
query = "SELECT ID FROM ACLS WHERE NAME='" & AclName & "'"
DB::Query(query)
if not IsDefined(DB::FetchArray(q)) then
  cgi::SymbolName "NAME",UserName
  PrintTemplate "admin/delnoacl.html"
endif

Id = q[0]
' delete the ACL from the ACLS table
query = "DELETE FROM ACLS WHERE ID=" & Id
DB::Query query

' delete all ACEs that belonged to this ACL
query = "DELETE FROM ACES WHERE ACLID=" & Id
DB::Query query

DB::Close
cgi::SymbolName "NAME",AclName
PrintTemplate "admin/delacl.html"

stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
