print """HTTP/1.0 200 OK
Content-Type: text/html

<HTML>
<BODY>
The real page was not found, howerver<P>
"""
print "Hello, this program runs\n"
for i=1 to 10
 print sin(i*pi/10),"\n"
 print "<P>\n"
next i

print "</BODY></HTML>\n"
