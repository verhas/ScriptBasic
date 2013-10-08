import "../sibawa.ini"
import "../acedit.ini"

' use dbg

if HasPrivilege("admin") then
  HasReadPermission = TRUE
  HasWritePermission = TRUE
  IsPrivileged = TRUE
else
  HasReadPermission = FALSE
  HasWritePermission = FALSE
  IsPrivileged = FALSE
endif

cgi::Header 200,"text/html"
cgi::FinishHeader

AclName = cgi::Param("acl")

DB::Connect
DB::Query "select ID,NAME,ACLID,DESCRIPTION,ACLTYPE,OWNER FROM ACLS USERS WHERE NAME ='" & AclName & "'"

if not DB::FetchArray(q) then
  cgi::SymbolName "NAME",AclName
  PrintTemplate("admin/editnoacl.html")
  DB::Close
  stop
endif  

AclId        = q[0]
AclName      = q[1]
ControlAclId = q[2]
Description  = q[3]
AclTypeId    = q[4]
OwnerId      = q[5]

if not IsPrivileged then
  RoleArray = undef
  if OwnerId = sibawa::userid then
    RoleArray[1] = GetRoleIdFromName("owner")
  endif
  EffectiveAcv = GetEffectiveAcv(ControlAclId,RoleArray)
  HasReadPermission = mid$(EffectiveAcv,1,1) = "A"
  HasWritePermission = mid$(EffectiveAcv,2,1) = "A"
endif

if not HasReadPermission or not HasWritePermission then
  PrintTemplate "admin/noright.html"
  stop
endif

ControlAclName = GetAclNameFromId(ControlAclId)
AclTypeName    = GetAclTypeNameFromId(AclTypeId)
OwnerName      = GetUserNameFromId(OwnerId)

'
' Get all the ACEs that belong to this ACL
'
DB::Query "SELECT ID,GUID,ACETYPE,ACEVAL FROM ACES WHERE ACLID=" & AclId

n = 0
while DB::FetchArray(q)
  AceId[n]   = q[0]
  AceGuid[n] = q[1]
  AceType[n] = q[2]
  AceVal[n]  = q[3]
  n = n+1
wend

'
' Start to print out the head
'
cgi::SymbolName "ID",AclId
cgi::SymbolName "ACLNAME",AclName
cgi::SymbolName "ACLACLNAME",ControlAclName
cgi::SymbolName "ACLID",ControlAclId
cgi::SymbolName "DESCRIPTION",Description
cgi::SymbolName "ACLTYPE",AcltypeName
cgi::SymbolName "OWNER",OwnerName
cgi::SymbolName "N",n
PrintTemplate("admin/editaclhead.html")

'
' Print the applicable ACL permission keywords
'
for i=1 to 32
 KeyWord = GetAceKeywordByBit(AclTypeId,i)
 if isDefined(KeyWord) then
   cgi::ResetSymbols
   cgi::SymbolName "KEYWORD",KeyWord
   cgi::SymbolName "BIT",i
   cgi::SymbolName "ACLTYPEID",AclTypeId
   PrintTemplate("admin/editaclkw.html")
 endif
next i

'
' Start to print out the head and then the ACEs
'
cgi::SymbolName "ID",AclId
cgi::SymbolName "ACLNAME",AclName
cgi::SymbolName "ACLACLNAME",ControlAclName
cgi::SymbolName "ACLID",ControlAclId
cgi::SymbolName "DESCRIPTION",Description
cgi::SymbolName "ACLTYPE",AcltypeName
cgi::SymbolName "OWNER",OwnerName
cgi::SymbolName "N",n
PrintTemplate("admin/editaclthead.html")
'
' print out the already existing ACEs
'
for i=0 to n-1
  cgi::ResetSymbols
  if AceType[i] = "U" then
    query = "SELECT NAME FROM USERS WHERE ID=" & AceGuid[i]
    template = "admin/editaclseu.html"
  else if AceType[i] = "G" then
    query = "SELECT NAME FROM GROUPS WHERE ID=" & AceGuid[i]
    template = "admin/editaclseg.html"
  else if AceType[i] = "R" then
    query = "SELECT NAME FROM ROLES WHERE ID=" & AceGuid[i]
    template = "admin/editaclser.html"
  else
  endif
  DB::Query query
  if DB::FetchArray(q) then
    cgi::SymbolName "NAME",q[0]
  else
    cgi::SymbolName "NAME","?" & AceGuid[i] & "?"
  endif
  cgi::SymbolName "I",i
  cgi::SymbolName "ID",AceId[i]
  cgi::SymbolName "GUID",AceGuid[i]
  cgi::SymbolName "ACETYPE",AceType[i]
  cgi::SymbolName "HACEVAL",AceVal[i]
  cgi::SymbolName "ACEVAL",GetAceKeywordString(AclTypeId,AceVal[i])

  PrintTemplate(template)
next i
cgi::ResetSymbols
cgi::SymbolName "NAME",AclName
PrintTemplate("admin/editacltail.html")


DB::Close
end
