' Adding and Inn  addinn.bas
' convert aaa-zzz to 1 to 17576
' Case insensitive
' ------------------------------------------------
function aton(a)
 wc = ucase(a)
 c1 = mid(wc,1,1)
 c2 = mid(wc,2,1)
 c3 = mid(wc,3,1)
 nu = ((asc(c1)-65) * 676) + ((asc(c2)-65) * 26) + (asc(c3)-65) + 1
end function
' ------------------------------------------------
' split keywords into arrays kw(1-n) and kd(1-n)
' words in kw[x] and data in kd[x] ki is limit of keywords
' uses filename target and filenumber 1
Sub buildkeys 
 open target for input as 1
 ki=0
moreoftarget:
 if eof(1) then goto wraptarget
 line input #1, ka
 ki=ki+1
 ka = rtrim(ka)
 if instr(ka,"=") = undef then goto addontext 
 split ka by "=" to kw[ki], kd[ki]
 kw[ki] = ucase(kw[ki]) 
 goto moreoftarget
addontext:
 kd[ki] = ka
 goto moreoftarget
wraptarget:
 close 1
End sub
' ----------------------------------------------
'  Opening a new Booking record set
'
' Input must be code aaa - zzz
'
' this test will use "aaa"
' output test file is "work.bkg"
 bbcode = "aaa"
 infile = bbcode & ".txt"
 target = infile
 call buildkeys
 stdate = day() & "/" & month() & "/" & year()
 stdate = stdate & space(5)
 stdate = left(stdate,10)
' date stored as 10 byte day/month/year
' data in memory array kw[n] and kd[n] now look for room
' -- 51 -------------
 for x = 1 to ki-1
 if kw[x] <> "ROOM" then goto nextup
makerecord:
' roomformat = record,name,beds,sleeping descr,bath,actual price,booking format 7 or 3
 kd[x] = kd[x] & ",7"
 split kd[x] by "," to r,rn,rb,rs,rf,rp,rk,rx
' decode r into a physical record number
 a = r
 call aton a
 r = nu
' r is record number 
 rn = rn & space(25)
 rn = left(rn,20)
 rb = rb & space(3)
 rb = left(rb,3)
 rs = rs & space(20)
 rs = left(rs,20)
 rf = rf & space(14)
 rf = left(rf,14)
 rp = rp & space(10)
 rp = left(rp,10)
 rk = rk & space(3)
 rk = left(rk,3)
' check for update - ie record exists r is within file length
 open "work.bkg" for random as 2 len = 1500
 lock region#2 from 0 to r for write
 lx = lof(2)
 if r < lx then goto recupdate
  if r = lx then goto recupdate
 rg = stdate
 rq = string(1420,"@")
' -- 85 --
 a = space(70) & rg & rq 
 for s = (lx+1) to r
 print #2,a
 next s
' now its all an update 
recupdate:
 seek#2,r
 a = input (1,#2)
 rg = mid(a,71,10)
 rq = mid$(a,81,1420)
 seek#2,r
     a = rn & rb & rs & rf & rp & rk & rg & rq
 print #2, a
 lock region#2 from 0 to r for release
 close 2
nextup:
 next x
' end of program
