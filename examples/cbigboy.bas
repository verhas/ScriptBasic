' Run this program to create the BASIC program
' BIGBOY.BAS
'
' Run bigboy with the command line:
'   scriba bigboy.bas -o bigboy.bbf
'
' Now you have both the source and the intermediate compiled version
'
' run
'        basic bigboy.bas
' and run
'        basic bigboy.bbf
' and see the difference in the speed of the startup.
' 
open "bigboy.bas" for output as 1

print#1, "a=1\n"
for i=1 to 10000
  print #1,"a = a +1\nprint a,\"\\n\"\n"
next

close 1
