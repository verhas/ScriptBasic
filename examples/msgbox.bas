import nt.bas

nt::MsgBox """This program test the various text boxes.

In the coming message boxes press the various keys as requested by the text
and check on the console window that the program reported the correct
button you pressed. (Only initials are printed.)
""","Initial MsgBox"

splita " |ARI|O|RC|YN|YNC" BY "|" TO buttons
splita " |W|I|Q|S" BY "|" TO marks

for j=lbound(marks) to ubound(marks)

for i=lbound(buttons) to ubound(buttons)

for d=1 to len(buttons[i])
print i," ",buttons[i]," ",j," ",marks[j]," ",d,"\n"
R = nt::MsgBox(buttons[i],marks[j],buttons[i],marks[j],d)
print R
print

next d
next i
next j
