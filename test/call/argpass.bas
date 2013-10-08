'
'
'

sub test_sub(a,b,c)
local lv,d
d = 1
print "1 a b c>",a," ",b," ",c,"\n"
a += 1
b = c + d
lv = 66
gv = 55
print "2 a b c>",a," ",b," ",c,"\n"
end sub

lv = 77
gv = 100

a = 1
b = 2
c = 3

test_sub a,b,c
print "3 a b c>",a," ",b," ",c,"\n"

print "4 lv gv>",lv," ",gv,"\n"

test_sub ByVal a, b, c
print "5 a b c>",a," ",b," ",c,"\n"

