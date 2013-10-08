on error goto ErrCapture
a = 3
ref a = a

print "No error? That is bad\n"
stop

ErrCapture:

print "a is ",a," unchanged\n"

