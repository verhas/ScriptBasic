print "hello\n"
a = 3

sub MyFunction(t)
local a,b,c

b = t + 2
print b
print " ",t
if t > 1 then MyFunction t-1
print " *"
end sub

MyFunction a

a = a + 2

MyFunction a
