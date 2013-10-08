import mt.bas

chdir "\\"

mt::ListSessions SessionArray

for i= lbound(SessionArray) to ubound(SessionArray)
  if len(SessionArray[i]) > 3 then
    print "Deleting the session ",SessionArray[i],"\n"
    mt::DeleteSession SessionArray[i]
  end if
next i
sleep 30
