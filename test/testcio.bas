import cio.bas

cio::SetColor FBlue or BGrey
cio::cls
i = 1
while 1

if i % 2 Then
 cio::SetColor FBlue or BGrey
Else
cio::SetColor FBlue or BIntense
End If

Xs = cio::SizeX()
Ys = cio::SizeY()

cio::gotoxy 10,23
print "The console size is : ",Xs," ",Ys,"\n"
print "The possible maximal sizes are: ",cio::PossibleMaxX()," ",cio::PossibleMaxY(),"\n"
print "The actual size is: ",cio::SizeX()," ",cio::SizeY()

cio::SetCursor 50

cio::gotoxy 1,40
for j=1 to 300
 cio::SetColor j
 print "Q"
next j

sleep 1
if cio::kbhit() then
 ch = cio::getch()
 cio::gotoxy 10,20
 print "Key ",ch," was pressed..."
 if ch = 13 then cio::nobreak
 if ch = asc("a") then cio::break
end if

cio::gotoxy 10,10
print i
i += 1
print " "
print cio::GetTitle()
cio::SetTitle i 
wend

