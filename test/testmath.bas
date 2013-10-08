print "testing the function pow\n"
for i=-20 to 20
  print pow(i)
  print
next i
'
'
'
print "testing the function exp\n"
for i=-20 to 20
  print exp(i)
  print
next i
'
'
'
print "testing the function log\n"
for i=0 to 2 step 0.1
  print log(i)
  print
next i
'
'
'
print "testing the function log10\n"
for i=0 to 2 step 0.1
  print log10(i)
  print
next i
'
'
'
print "testing the function sin\n"
for i=-pi to +pi step pi/20.0
  print sin(i)
  print
next i

for i=-pi to +pi step pi/20.0
  print space((sin(i)+1.0)*30),"*"
  print
next i
