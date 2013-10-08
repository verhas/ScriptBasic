' You have to get the following output:
'---------------------------------------------
'values in the module=a b undef
'values called from within the module:
'  values in TestSub=a b c
'a in sub=VARIABLE
'a in module=VARIABLE
'values outside the module=a undefundef
'values called from outside the module:
'  values in TestSub=a b c
'a in sub=VARIABLE
'a in global=undef
'---------------------------------------------
module TEST
a = "VARIABLE"
Global Const a = "a "
Const b = "b "

sub TestSub
const c = "c "
print "  values in TestSub=",a,b,c,"\n"
var a
print "a in sub=",a,"\n"
end sub

print "values in the module=",a,b,c,"\n"
print "values called from within the module:\n"
TestSub
var a
print "a in module=",a,"\n"
end module
print "values outside the module=",a,b,c,"\n"
print "values called from outside the module:\n"
TEST::TestSub
var a
print "a in global=",a,"\n"
