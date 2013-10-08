REM This is a sample program to demonstrate local variables

'A is a global variable
A=13
Call MySUB()
Print A,"\n",B

Sub MySUB
Local A
A=9
B=55
End Sub
