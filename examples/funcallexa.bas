a = MyFunction(1,2,MyFunction(1,1,1))
print a
printnl
function MyFunction(a,b,c)
local x

print a,b,c,x
printnl
x = a * b * c
MyFunction = a + b + c

end function
