' This is a test program demonstrating the
' different levels of error handling
' using the "command on error" in ScriptBasic
'
sub ErrorSub
'on error goto ErrorLabel
open "nofile" for input as 1

print "No error has occured in the function"

goto FinishLabel
ErrorLabel:
print "An error has occured inside the sub"
print
FinishLabel:
end sub

on error goto ErrorLabel

ErrorSub

print "No error"
goto FinishLabel

ErrorLabel:
print "An error has occured, but this is no problem.\n"

FinishLabel:
