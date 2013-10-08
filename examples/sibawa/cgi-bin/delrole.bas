import "../sibawa.ini"



CheckPrivilege "admin"



cgi::Header 200,"text/html"

cgi::FinishHeader



if cgi::Param("surecheck") <> 1 then

  cgi::SymbolName "NAME",cgi::Param("name")

  PrintTemplate "admin/suroledelete.html"

  stop

endif



RoleName = cgi::Param("name")



on error goto AdminDataBaseError

DB::Connect

query = "SELECT ID FROM ROLES WHERE NAME='" & RoleName & "'"

DB::Query(query)

if DB::FetchArray(q) then

  RoleId = q[0]

  query = "DELETE FROM ROLES WHERE ID=" & RoleId

  DB::Query(query)

  DB::Close

  PrintTemplate "admin/delrole.html"

else

  cgi::SymbolName "NAME",RoleName

  PrintTemplate "admin/delnorole.html"

end if

stop

AdminDataBaseError:



DisplayError "Data base error. The query was: " & query



end
