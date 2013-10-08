import "../sibawa.ini"
' use dbg
cgi::Header 200,"text/html"
cgi::FinishHeader

OldPass = cgi::Param("oldpassword")
NewPass = cgi::Param("newpassword")
VerPass = cgi::Param("verpassword")

if not IsDefined(OldPass) OR _
   not IsDefined(NewPass) OR _
   not IsDefined(VerPass) then
  PrintTemplate "chpass/requestpage.html"
  stop
end if

'
' check the old password
'
' Note that here we do NOT rely on the session variables, but rather on the database
'
on error goto chpassDataBaseError
DB::Connect
query = "select PASSWORD from USERS WHERE name='" & cgi::RemoteUser() & "'"
DB::Query query
DB::FetchArray q
DB::Close

if q[0] <> OldPass then
  PrintTemplate "chpass/badoldpw.html"
  stop
endif

'
' check that the new password and verify match
'
if NewPass <> VerPass then
  PrintTemplate "chpass/mismatch.html"
  stop
endif

'
' OK here we have checked all things, lets alter the password
'
on error goto chpassUpdateError
DB::Connect
query = "UPDATE USERS SET PASSWORD='" & NewPass & "' WHERE name='" & cgi::RemoteUser() & "'"
DB::Query query
DB::Close

'
' also update the session variables
'
sibawa::password = NewPass
mt::SetSessionVariable "password",sibawa::password

PrintTemplate "chpass/success.html"

end

chpassDataBaseError:
  PrintTemplate "chpass/dberror.html"
  stop

chpassUpdateError:
  PrintTemplate "chpass/updaterror.html"
  stop
