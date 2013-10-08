include "cgi.bas"
print "HTTP/1.0 200 OK\nContent-type: text/html\n\n"

print "<HTML><BODY><H1>Hello!!</H1>

This is a simple ScriptBasic program that does nothing else, but displays the environment variables.

<pre>
"
print "METHOD IS ",cgi::method(),"\n"
print "Parameter a is ", cgi::param("a"),"\n"
e = ENVIRON(0)
index = 0
while e <> undef
print e,"\n"
index = index+1
e = ENVIRON(index)
wend
print "
</pre>
</BODY>
</HTML>
"
