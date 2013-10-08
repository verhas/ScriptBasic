import "../sibawa.ini"
use dbg
CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

AcltypeId = cgi::Param("acltype")

AclTypeName = mt::GetVariable("AclTypeName:" & AclTypeId)
AclTypeDescription = mt::GetVariable("AclTypeDescription:" & AclTypeId)
if isDefined(AclTypeName) then

  cgi::SymbolName "ID",AclTypeId
  cgi::SymbolName "NAME",AclTypeName
  cgi::SymbolName "DESCRIPTION",AclTypeDescription
  for i=1 to 32
    cgi::SymbolName "BIT"&i,mt::GetVariable("AclTypeBitName:" & AcltypeId & ":" & i )
    cgi::SymbolName "DESCRIPTION"&i,mt::GetVariable("AclTypeBitDescription:" & AclTypeId & ":" & i )
  next i

  PrintTemplate("admin/editacltype.html")
else
  cgi::SymbolName "ID",AcltypeId
  PrintTemplate("admin/editnoacltype.html")
endif

end

