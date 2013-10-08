import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

AclName = cgi::Param("group")

on error goto AdminDataBaseError
DB::Connect
query = "select ACLID,NAME,DESCRIPTION,ACLTYPE from ACLS"
if IsDefined(AclName) then
  query = query & " WHERE NAME LIKE \'" & AclName & "%'"
endif

PrintTemplate("admin/acllisthead.html")

DB::Query query

n = 0
while DB::FetchArray(q)
  aclid[n] = q[0]
  name$[n] = q[1]
  description[n] = q[2]
  acltype[n] = q[3]
  n = n+1
wend

for i=0 to n-1
  if aclid[i] = 0 or len(aclid[i])=0 then
    aclname[i] = ""
  else
    query = "SELECT NAME FROM ACLS WHERE ID=" & aclid[i]
    DB::Query query
    DB::FetchArray(q)
    aclname[i] = q[0]
  endif
  if acltype[i] = 0 or len(acltype[i])=0 then
    acltypename[i] = ""
  else
    query = "SELECT NAME FROM ACLTYPES WHERE ID=" & acltype[i]
    DB::Query query
    DB::FetchArray(q)
    acltypename[i] = q[0]
  endif

next i

for i=0 to n-1
  cgi::SymbolName "ACLNAME",aclname[i]
  cgi::SymbolName "NAME",name$[i]
  cgi::SymbolName "DESCRIPTION",description[i]
  cgi::SymbolName "ACLTYPE",acltypename[i]
  PrintTemplate("admin/acllistline.html")
  cgi::ResetSymbols
next i

DB::Close
PrintTemplate("admin/acllisttail.html")
stop

AdminDataBaseError:

DisplayError "Data base error. The query was: " & query

end
