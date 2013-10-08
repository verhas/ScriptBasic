print """HTTP/1.0 200 OK
Content-Type: text/html

<HTML>
<BODY>
<FORM ACTION="cgi.bas" METHOD="POST">
<INPUT TYPE="TEXT" VALUE="" NAME="KAKUKK">
<INPUT TYPE="SUBMIT" NAME="BIKANYAK" VALUE="PRESSSSS">
</FORM>
"""
print "Hello, this program runs\n"
for i=1 to 10
 print sin(i*pi/10),"\n"
 print "<P>\n"
next i

print "</BODY></HTML>\n"
