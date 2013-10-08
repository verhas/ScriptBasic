import nt.bas

nt::ListProcesses PS

for i=lbound(PS) to ubound(PS)

  for j=0 to 8
    print PS[i,j]," "
  next j
  print "\n"

next i
