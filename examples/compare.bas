' note that strings are multi line
print """This program tests different string comparisions.
The result should be a lot of 1
printed. If there is any 0 printed there 
is an error in the interpreter.

"""

if "a" = "a" then
 print 1
else
 print 0
end if
if "a" <> "a" then
 print 0
else
 print 1
end if

if "a" < "a" then
 print 0
else
 print 1
end if
if "a" < "b" then
 print 1
else
 print 0
end if
if "b" < "a" then
 print 0
else
 print 1
end if


if "a" > "a" then
 print 0
else
 print 1
end if
if "a" > "b" then
 print 0
else
 print 1
end if
if "b" > "a" then
 print 1
else
 print 0
end if

if "a" <= "a" then
 print 1
else
 print 0
end if
if "a" <= "b" then
 print 1
else
 print 0
end if
if "b" <= "a" then
 print 0
else
 print 1
end if

if "a" >= "a" then
 print 1
else
 print 0
end if
if "a" >= "b" then
 print 0
else
 print 1
end if
if "b" >= "a" then
 print 1
else
 print 0
end if

REM comparing different length strings

if "a" = "ab" then
 print 0
else
 print 1
end if
if "a" <> "ab" then
 print 1
else
 print 0
end if

if "a" < "ab" then
 print 1
else
 print 0
end if
if "ab" < "b" then
 print 1
else
 print 0
end if
if "b" < "ab" then
 print 0
else
 print 1
end if


if "a" > "ab" then
 print 0
else
 print 1
end if
if "a" > "bb" then
 print 0
else
 print 1
end if
if "b" > "ab" then
 print 1
else
 print 0
end if

if "a" <= "ab" then
 print 1
else
 print 0
end if
if "ab" <= "b" then
 print 1
else
 print 0
end if
if "b" <= "ab" then
 print 0
else
 print 1
end if

if "ab" >= "a" then
 print 1
else
 print 0
end if
if "ab" >= "b" then
 print 0
else
 print 1
end if
if "b" >= "ab" _
then
 print 1
else
 print 0
end if


print "\ntest ended\n"
