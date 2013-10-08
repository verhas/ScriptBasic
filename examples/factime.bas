print FormatDate("YEAR MON DD HH:mm:ss",FileCreateTime("kill.bas"))
print
print FormatDate("YEAR MON DD HH:mm:ss",FileModifyTime("kill.bas"))
print
print FormatDate("YEAR MON DD HH:mm:ss",FileAccessTime("kill.bas"))
print
set file "kill.bas" createtime=TimeValue(1970,10,2,14,53)
set file "kill.bas" modifytime=TimeValue(1980,10,2,14,53)
set file "kill.bas" accesstime=TimeValue(1990,10,2,14,53)
print "------------------\n"
print FormatDate("YEAR MON DD HH:mm:ss",FileCreateTime("kill.bas"))
print
print FormatDate("YEAR MON DD HH:mm:ss",FileModifyTime("kill.bas"))
print
print FormatDate("YEAR MON DD HH:mm:ss",FileAccessTime("kill.bas"))
print
