' This concatenation will result "65"
a = 6 & 5
' the first character of the string "65" is used
q = STRING(5,a)
' val(65) on the other hand is "A"
w = STRING(5,val(a))
print "This should print 66666AAAAA\n"
print q,w
