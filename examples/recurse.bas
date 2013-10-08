'
' This function calculates the number 10
' recursively
'
function Calculate10(x)

 print "Starting calculate 10 for the number ",x,"\n"

  if x = 10 then
   Calculate10 = x
  else
   Calculate10 = Calculate10(x+1)
  endif

 print "Ending calculate 10 for the number ",x,"\n"

end function

Calculate10 1
