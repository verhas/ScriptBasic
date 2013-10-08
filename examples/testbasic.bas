#!/home/verhas/source/basic
'
' This program tries to test several features of the ScriptBasic interpreter
'

' Testing that ScriptBasic is capable printing out constant string
print "print is OK\n"

' testing numeric operators

print "Testing numeric operators...\n"

a = 2^3
if a = 8 then
 print "Power operator seems to be OK\n"
else
 print "Power operator said that 2^3=",a,"\n"
 stop
endif

a = 2*5
if a = 10 Then
 print "multiply operator seems to be OK\n"
else
 print "multiply operator said that 2*5=",a,"\n"
 stop
endif

a = 10/2
if a = 5 Then
 print "div operator seems to be OK\n"
else
 print "div operator said that 10/2=",a,"\n"
 stop
endif

print "9/2=",9/2
printnl

print "9\\2=",9\2
printnl

print "9%4=",9%4
printnl

print "1+1=",1+1
printnl

print "6-3=",6-3
printnl

print "6-3+2=",6-3+2
printnl

print "6=6?",6=6
print
print "6=3?",6=3
print
print "3<>3?",3<>3
print
print "2<>3?",2<>3
print
print "2<3?",2<3
print
print "3<2?",3<2
print
print "3>2?",3>2
print
print "2>3?",2>3
print
print "2<=3?",2<=3
print
print "3<=2?",3<=2
print
print "3>=2?",3>=2
print
print "2>=3?",2>=3
print
print "2<=2?",2<=2
print
print "2>=2?",2>=2
print
print "3=",1 or 2
print
print "2=", 3 and 2
print
print "2=", 3 xor 1
print

print "testing string concatenation...\n"
print "a" & "a" & "a" & "a" & "a" & "a" & "a" & "a" & "a" & "a" & "a"
print

print "testing the for loop\n"
for i=1 to 10
print i
next
print

print "testing heavy memory usage via string concatenation...\n"
q = ""
for i = 1 to 1024
q = q & " "
next
print "q is now string of length ",len(q)," bytes\n"
print
print "Creating a one megabyte string concatenating 1024 times a 1K string."
print
print "Be patient...\n"
p = q
q = ""
for i = 1 to 1024
if i%100 = 0 then
  print "."
endif
q = q & p
next
print
print "q is now string of length ",len(q)," bytes\n"
print "was it ",1024*1024," bytes?\n"
if 1024*1024 = len(q) then
  print "yes it was\n"
else
  print "No it is a problem...\n"
  stop
endif

