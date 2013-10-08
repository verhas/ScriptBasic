Const nl = "\n"
for i=1 to 5
 print i,".\n"
 print "The local         time is: ",Year,".",Month,".",Day," ",Hour,":",Minute,":",Sec,nl
 print "The Greenich mean time is: ",FormatDate("YEAR.MM.DD hh:mm:ss",GmTime()),nl
 a = now
 b = TimeValue(year,month,day,hour,minute,sec)
 print "Local time is sec from Now():",a,nl, _
       "Local time in sec from TimeValue(...)",b,nl, _
       "The difference should be zero: ",a-b,nl
 print "GMT:",GmTime,"=",LocalTimeToGmTime(Time),nl
 print "LCT:",Time,"=",GmTimeToLocalTime(GmTime),nl
 sleep 1
next i

