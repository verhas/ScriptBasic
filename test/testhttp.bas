open "scriptbasic.com:80" for socket as 1
print#1,"GET http://scriptbasic.com/ HTTP/1.0\n\n"
while not eof(1)
 line input #1,a
 print a
wend
close 1
