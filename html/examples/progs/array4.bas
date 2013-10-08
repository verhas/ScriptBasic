a[1] = undef
print lbound(a)," ",ubound(a)
print
a[2,-3] = "whoops"
print lbound(a)," ",ubound(a)
print
print lbound(a[2])," ",ubound(a[2])
print
