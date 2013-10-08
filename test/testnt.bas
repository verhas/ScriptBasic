import nt.bas

print nt::RegRead("HKLM\\SOFTWARE\\SCRIPTBASIC\\CONFIG"),"\n"
print nt::RegRead("HKLM\\SOFTWARE\\SCRIPTBASIC\\DWORD"),"\n"
print nt::RegRead("HKLM\\SOFTWARE\\SCRIPTBASIC\\BIN"),"\n"

' nt::RegDel "HKLM\\SOFTWARE\\SCRIPTBASIC\\BIN"
nt::RegWrite "HKLM\\SOFTWARE\\SCRIPTBASIC\\BIN1","kakukk"
nt::RegWrite "HKLM\\SOFTWARE\\SCRIPTBASIC\\DWORD1",614

nt::RegWrite "HKLM\\SOFTWARE\\SCRIPTBASIC\\" , "default value"

' nt::RegWrite "HKLM\\SOFTWARE\\SCRIPTBASIC\\SUB\\DWORD1",614

