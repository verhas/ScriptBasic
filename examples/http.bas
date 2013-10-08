' This is a sample showing how easy to download a 
' web page using ScriptBasic
'
open "scriptbasic.com:80" for socket as 1
print#1,"GET / HTTP/1.0\nHost: scriptbasic.com\n\n"
while not eof(1)
 line input #1,a
 print a
wend
close 1
