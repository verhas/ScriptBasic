Call MySub(1,2,3)
Call MySub 2,3,4
Call MySub(3),4,5
Call MySub 4,(5),6
Call MySub
Call MySub()
Call MySub(3)

sub MySub(a,b,c)
ByVal a,b,c
if not isdefined(a) then a = "-"
if not isdefined(b) then b = "-"
if not isdefined(c) then c = "-"
print a," ",b," ",c
print "\n"

end sub

MySub(1,2,3)
MySub 2,3,4
MySub(3),4,5
MySub 4,(5),6
MySub
MySub()
MySub(3)
Call MySub(1,2,3)
Call MySub 2,3,4
Call MySub(3),4,5
Call MySub 4,(5),6
Call MySub
Call MySub()
Call MySub(3)
