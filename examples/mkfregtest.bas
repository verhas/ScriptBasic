REM KFORREGTEST.BAS
REM """
This program is used to create another BASIC program, which can be used to test
the for loops under different conditions.

A for loop has three expressions:

1. start value for the loop
2. end value for the loop
3. step value for the loop

Each can be

1. integer positive
2. integer negative
3. real positive
4. real negative
5. string converting to integer positive
6. string converting to integer negative
7. string converting to real positive
8. string converting to real negative
9. undef

These are 8 different cases for each expression summingup to 512 different cases.
Each case can be tested with actual values that 

1. reach the end value and 
2. reach a value before the end value and the net value already steps over the end value

This summs up 1458 cases.

This program creates a BASIC program that runs through all these cases.

"""

' the 5 different values for the start and end values, the first four can be stringized

a[1] = "5"
a[2] = "5.6"
a[3] = "undef"

' different step values
b[1] = "1"
b[2] = "2"
b[3] = "1.12"
b[4] = "1.2"

TestCounter = 0

for StartValue=1 to 3

SV[1] = a[StartValue]
SV[2] = "-"&a[StartValue]
SV[3] = "\""&a[StartValue]&"\""
SV[4] = "\"-"&a[StartValue]&"\""

for EndValue=1 to 3

EV[1] = 2*a[EndValue]
EV[2] = "-"&2*a[EndValue]
EV[3] = "\""&2*a[EndValue]&"\""
EV[4] = "\"-"&2*a[EndValue]&"\""

for StepValue =1 to 4

PV[1] = b[StepValue]
PV[2] = "-"&b[StepValue]
PV[3] = "\""&b[StepValue]&"\""
PV[4] = "\"-"&b[StepValue]&"\""

for SVi=1 to 4
for EVi=1 to 4
for PVi=1 to 4

TestCounter += 1

print "print ",TestCounter,",\"\\n\"\n"
print "print \"\"\"FOR i=",SV[SVi]," TO ",EV[EVi]," STEP ",PV[PVi],"\\nprint i\\nNEXT i\\n\"\"\"\n"
print "FOR i= ",SV[SVi]," TO ",EV[EVi]," STEP ",PV[PVi],"\n"
print "print i,\" \"\n"
print "NEXT i\n"
print "print \"\\n---------------------------------------\\n\"\n\n"

next PVi
next EVi
next SVi

next StepValue

next EndValue


next StartValue


