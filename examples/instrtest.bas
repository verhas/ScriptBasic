print InStr("almafa","maf")
print
print InStr("almafa","alma")
print
print InStr("almafa","afa")
print
print InStr("almafa","faa")
print

a = "alabama mama"
StartPosition = 1
while IsDefined(StartPosition)
  StartPosition = InStr(a,"ma",StartPosition)
  print StartPosition
  print
  StartPosition = StartPosition + 1
wend
