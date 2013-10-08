open "lock.bas" for binary as 1
lock #1, write
print "Locked now:"
' line input a
print
lock #1, release
print "unlocked, but still open"
' line input a
print
close 1
print "closed"
print
