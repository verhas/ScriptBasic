#!/basic.exe
print " "
a=1
call f(a+0)
print a
print
call f(a)
print a
print
function f(x)
 x=x+1
end function
