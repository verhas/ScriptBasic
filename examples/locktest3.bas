open "locktest.txt" for random as 1
seek#1,5
for i=1 to 5
print #1, "D"
next i
print "10 bytes are done first 5 bytes are locked\n"
' line input a
close 1
