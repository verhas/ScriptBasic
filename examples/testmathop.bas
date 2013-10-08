print "testing command MULT\n"
for i=-9 to 9
print i*i
print " "
next
print
print 3.3 * 2
print
print 3.3 * 3.3
print
print sqr(27) * sqr(27)
print
print sqr(27) * (sqr(27)-0.000001)
print

print "TESTING COMPARISION\n"
for i=1 to 10
for j=1 to 10
 print abs(i < j)
next
print
next

print
for i=1 to 10
for j=1 to 10
 print abs(i <= j)
next
print
next

print
for i=1 to 10
for j=1 to 10
 print abs(i > j)
next
print
next

print
for i=1 to 10
for j=1 to 10
 print abs(i >= j)
next
print
next

print
for i=1 to 10
for j=1 to 10
 print abs(i = j)
next
print
next

print
for i=1 to 10
for j=1 to 10
 print abs(i <> j)
next
print
next

for i=1.1 to 10
for j=1 to 10
 print abs(i < j)
next
print
next

print
for i=1.1 to 10
for j=1 to 10
 print abs(i <= j)
next
print
next

print
for i=1.1 to 10
for j=1 to 10
 print abs(i > j)
next
print
next

print
for i=1.1 to 10
for j=1 to 10
 print abs(i >= j)
next
print
next

print
for i=1.1 to 10
for j=1.1 to 10
 print abs(i = j)
next
print
next

print
for i=1.1 to 10
for j=1.1 to 10
 print abs(i <> j)
next
print
next
print "TESTING string comparision\n"
print "apple" < "birch"
print
print "apple" < "apple"
print
print "apple" < "apple "
print
print "apple " < "apple"
print
print "apple" <= "birch"
print
print "apple" <= "apple"
print
print "apple" <= "apple "
print
print "apple " <= "apple"
print
print "apple" > "birch"
print
print "apple" > "apple"
print
print "apple" > "apple "
print
print "apple " > "apple"
print
print "apple" >= "birch"
print
print "apple" >= "apple"
print
print "apple" >= "apple "
print
print "apple " >= "apple"
print
print "apple" = "birch"
print
print "apple" = "apple"
print
print "apple" = "apple "
print
print "apple " = "apple"
print
print "apple" <> "birch"
print
print "apple" <> "apple"
print
print "apple" <> "apple "
print
print "apple " <> "apple"
print

print "TESTING and or xor\n"
for i=0 to 15
for j=0 to 15
print i and j, " " , i or j , " ", i xor j
print
next
next

print "Testing %\n"
for i=1 to 10
print i,":"
for j=1 to 10
print i%j," "
next
print
next

print

print 2/3
print
print pi
print
print pi
print

print int(3.3)," ",int(-3.3)
print
print frac(3.3)," ",frac(-3.3)
print
print odd(3.3)
print

print "testing error handling\n"
on error goto ErrorHasHappened
option RaiseMathError sbMathErrDiv or sbMathErrUndef or sbMathErrUndefCompare

if undef=undef then
 print "undef=undef is true\n"
else
 print "undef=undef is false\n"
endif

if undef=13 then
 print "undef=13 is true\n"
 print "probably resume next is not really consistent here\n"
else
 print "undef=13 is false\n"
endif

print "asin(2)=",asin(2)
print
print "log(-100)=",log(-100)
print

if undef then
  print "undef is true\n"
else
  print "undef is false\n"
endif

stop

ErrorHasHappened:
print "Error captured: ",error(),"\n"
print "Press any key to continue...\n"
line input key
on error goto ErrorHasHappened
resume next

