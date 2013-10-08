' FILE: createtestif.bas
'
' This basic program creates another BASIC program to test the IF/ELSEIF/ELSE
' construction executions. This program creates a program that has
' several IF ELSEIF ELSE ENDIF constructs with all possible true/false
' conditions orders. The generated program should NOT write out any "FAILED"
' and should print several OKs
' Finally the the two numbers should match the constants printed counting the printed
' OKs ensuring that all OKs were printed and counting the hopefully zero FAILED prints.
'

'
' You can change this, but increasing it 
' exponentially increases the size of the
' generated program
CONST LIMIT = 8

open "testif.bas" for output as 1
print#1,"""' This program was created by createtestif.bas
' for more information on this program read the comments of that program
"""
print#1,"c=0\nu=0\nfor i=1 to 2"
c=0
for i=1 to LIMIT
  for j=0 to 2^i
    print#1, "IF ",ABS(odd(j))," THEN\n"
    if odd(j) then
      print#1, "print ",c,",\" \",c,\" OK\\n\"\nc+=1\n"
      c+=1
      OKwas = true
    else
      print#1, """print "FAILED\\n"\nu+=1\n"""
      OKwas = 0
    endif

    BITS = j
    for k=1 to i
      BITS \= 2
      if not OKwas and odd(BITS) then
        print#1, "ELSE IF 1 THEN\n"
        print#1, "print ",c,",\" \",c,\" OK\\n\"\nc+=1\n"
        c+=1
        OKwas = true
      else
          print#1, "ELSE IF ",abs(odd(BITS))," THEN\nprint \"FAILED\\n\"\nu+=1\n"
      endif
    next k
    print#1, "ELSE\n"
    if OKwas then
      print#1, """print "FAILED\\n"\nu+=1\n"""
    else
      print#1, "print ",c,",\" \",c,\" OK\\n\"\nc+=1\n"
      c+=1
    endif
    print#1, "END IF\n\n"
  next j
print#1, "' ------\n"
next i
print#1, "next\n"
print#1,"print c,\" = ",2*c,"\"\nprint\n"
print#1,"print u,\" = ",0,"\"\nprint\n"
close 1
print "Program testif.bas was created\n"
