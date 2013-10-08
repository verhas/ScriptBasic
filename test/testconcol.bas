'
' This program lists all the possible console character
' colors on a windows console. This program can not
' be executed under UNIX
'
import cio.bas

cio::SetColor FWhite
cio::cls
cio::SetTitle "Testing console colors"
for i=1 to 255
  cio::gotoxy +(i \ 16) * 4 , +(i % 16) * 2
  cio::gotoxy( (i \ 16) * 4 , +(i % 16) * 2 )
  cio::gotoxy (i \ 16) * 4 , +(i % 16) * 2
  cio::SetColor (i)
  j = i
  if i < 100 then j = "0" & j
  print j
next i
cio::SetColor FWhite
cio::SetCursor 0
i = cio::getch()
print "\nDONEZ\n"
