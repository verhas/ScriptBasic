import "../sibawa.ini"
import "../acedit.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

Bit = cgi::Param("bit")
AclTypeId = cgi::Param("acltypeid")

AclTypeDescription = mt::GetVariable("AclTypeDescription:" & AclTypeId)
AclTypeName = mt::GetVariable("AclTypeName:" & AclTypeId)

cgi::SymbolName "NAME",AclTypeName
cgi::SymbolName "DESCRIPTION",AclTypeDescription

cgi::SymbolName "BITNAME",mt::GetVariable("AclTypeBitName:" & AclTypeId & ":" & Bit)
cgi::SymbolName "BITDESCRIPTION",mt::GetVariable("AclTypeBitDescription:" & AclTypeId & ":"  & Bit)

PrintTemplate("admin/descaclbit.html")

stop
