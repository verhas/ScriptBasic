print """<HTML>
<HEAD>
</HEAD>
<BODY>
This is a sample file containing embedded ScriptBasic code.
"""
 for i=1 to 10
print """
<font size=\""""
print i
print """\">Font"""
print i
print """</font>
"""
next i
print """
</BODY>
</HTML>


"""
