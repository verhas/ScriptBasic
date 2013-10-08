import "../sibawa.ini"
import "../acedit.ini"

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

AclName = cgi::Param("name")

DB::Connect
query = "select ID,NAME,ACLID,DESCRIPTION,ACLTYPE,OWNER FROM ACLS USERS WHERE NAME ='" & AclName & "'"
DB::Query query

if not DB::FetchArray(q) then
  cgi::SymbolName "NAME",AclName
  PrintTemplate("admin/setnoacl.html")
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

'
' Check that all the needed parameters are present in the uploaded form data
'
n = cgi::Param("n")
if not IsDefined(n) then
  cgi::SymbolName "ACLNAME",AclName
  PrintTemplate("admin/setaclnon.html")
  stop
endif

for i=0 to n-1
  if not IsDefined(cgi::Param("haceval" & i)) then
    cgi::SymbolName "I",i
    cgi::SymbolName "ACLNAME",AclName
    PrintTemplate("admin/setaclnohacevali.html")
    stop
  endif
  if not IsDefined(cgi::Param("aceval" & i)) then
    cgi::SymbolName "I",i
    cgi::SymbolName "ACLNAME",AclName
    PrintTemplate("admin/setaclnoacevali.html")
    stop
  endif
  if not IsDefined(cgi::Param("aceid" & i)) then
    cgi::SymbolName "I",i
    cgi::SymbolName "ACLNAME",AclName
    PrintTemplate("admin/setaclnoaceid.html")
    stop
  endif
next i

if not IsDefined(cgi::Param("isuser")) then
  cgi::SymbolName "ACLNAME",AclName
  PrintTemplate("admin/setaclnoisuser.html")
  stop
endif

if trim(cgi::param("aclaclname")) <> trim(cgi::param("haclaclname")) then
  ControlAclName = trim(cgi::param("aclaclname"))
  ControlAclId = GetAclIdFromName(ControlAclName)
else
  ControlAclId = undef
endif

OwnerName = trim(cgi::Param("owner"))
OwnerId = GetUserIdFromName(OwnerName)

AclTypeName = trim(cgi::param("acltype"))
AclTypeId = GetAclTypeIdFromName(AclTypeName)

if len(cgi::Param("guname")) > 0 and len(cgi::Param("aceval")) > 0 then
  if cgi::Param("isuser") = 1 then
    query = "SELECT ID FROM USERS WHERE NAME='" & cgi::Param("guname") & "'"
    template = "admin/setaclnosuser.html"
    ACETYPE = "U"
  else if cgi::Param("isuser") = 2 then
    query = "SELECT ID FROM ROLES WHERE NAME='" & cgi::Param("guname") & "'"
    template = "admin/setaclnosrole.html"
    ACETYPE = "R"
  else
    query = "SELECT ID FROM GROUPS WHERE NAME='" & cgi::Param("guname") & "'"
    template = "admin/setaclnosgroup.html"
    ACETYPE = "G"
  endif
  DB::Query query
  if not IsDefined(DB::FetchArray(q)) then
    cgi::SymbolName "NAME",cgi::Param("guname")
    cgi::SymbolName "ACLNAME",AclName
    PrintTemplate(template)
    stop
  endif
  GUID = q[0]
endif


'
' Update the description
'
if description <> cgi::Param("description") then
  query = "UPDATE ACLS SET DESCRIPTION='" & cgi::Param("description") & "' WHERE ID=" & AclId
  DB::Query query
endif

' Update the ControlAclId
if isDefined(ControlAclId) then
  DB::Query "UPDATE ACLS SET ACLID=" & ControlAclId & " WHERE ID=" & AclId
endif

' Update owner
DB::Query "UPDATE ACLS SET OWNER=" & OwnerId & " WHERE ID=" & AclId

' Update the Acl type
DB::Query "UPDATE ACLS SET ACLTYPE=" & AclTypeId & " WHERE ID=" & AclId

'
' Update the aces that may have changed
'
for i=0 to n-1
  HiddenAceVal = cgi::Param("haceval" & i)
  NewDisAceVal = Trim(cgi::Param("aceval" & i))
  if len(NewDisAceVal) > 0 then 
    NewSetAceVal = GetAceValueFromKeywordString(AclTypeId,NewDisAceVal)
    if not isDefined(NewSetAceVal) then
      cgi::SymbolName "ACLNAME",AclName
      cgi::SymbolName "BADACEVAL",cgi::Param("aceval" & i)
      PrintTemplate("admin/badaceval.html")
      stop
    end if
    if HiddenAceVal <> NewSetAceVal then
      query = "UPDATE ACES SET ACEVAL='" & NewSetAceVal & "' WHERE ID=" & cgi::Param("aceid" & i)
    endif
  else
    query = "DELETE FROM ACES WHERE ID=" & cgi::Param("aceid" & i)
  endif
  DB::Query query
next i

'
' Insert the new ACE if there is any
'
NewDisAceVal = Trim(cgi::Param("aceval"))

if len(cgi::Param("guname")) > 0 and len(NewDisAceVal) > 0 then
  NewSetAceVal = GetAceValueFromKeywordString(AclTypeId,NewDisAceVal)
  query = "INSERT INTO ACES (GUID,ACLID,ACETYPE,ACEVAL) VALUES (" & GUID & "," & AclId & ",'" & ACETYPE & "','" & NewSetAceVal & "')"
  DB::Query query
endif

DB::Close

cgi::SymbolName "NAME",AclName
PrintTemplate "admin/setacl.html"

stop
end
