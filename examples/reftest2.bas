sub test(a,s)
 local z,i

 if s = 1 then

  for i=1 to 10
   z[i] = i
  next
 
  ref a = z

 else

  test(a,s-1)

 endif
end sub

test q,100

for i=1 to 10
 print i,". ",q[i],"\n"
next i
