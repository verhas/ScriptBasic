open "locktest.txt" for output as 1
lock region#1 from 1 to 5 for write
for i=1 to 5
print #1, "A"
next i
print "5 bytes are done\n"
for i=1 to 5
print #1, "B"
next i
print "10 bytes are done first 5 bytes are locked\n"
' line input a
close 1
