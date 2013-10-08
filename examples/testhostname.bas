 on error goto ErrorLabel
 open "16.193.50.33:80" for socket as 1
 print#1,"GET http://localhost/ HTTP/1.0\n\n"
 while not eof(1)
  line input#1,a
  print a
 wend
 close 1
 stop
ErrorLabel:
 print "The web server 16.193.50.33 on port 80 is not reachable\n"
