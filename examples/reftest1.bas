sub test(a)
 local z,i
 for i=1 to 10
  z[i] = i
 next

 ref a = z
end sub

sub testi(h)
 test h
end sub


testi q

for i=1 to 10
 print i,". ",q[i],"\n"
next i
