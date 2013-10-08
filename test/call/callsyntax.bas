' This program test that the syntax analyzer checks the parentheses
' around the CALL subroutine call arguments correct
'
'
' This file should compile and run w/o error producing no output
'
sub test_sub

end sub

Call test_sub
Call test_sub a
Call test_sub(a)
Call test_sub a,b
Call test_sub(a,b)
Call test_sub (a),b
Call test_sub ((a),b)
Call test_sub (left(a,2))+3
Call test_sub left(a,2)+3
Call test_sub (left(a,2)+3,3)
Call test_sub (i+2)/3 , (j+2)/4
