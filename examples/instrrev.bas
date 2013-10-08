print InStrRev("almafa","maf")
print
print InStrRev("almafa","alma")
print
print InStrRev("almafa","afa")
print
print InStrRev("almafa","faa")
print

a = "alabama mama"
StartPosition = 5000
while IsDefined(StartPosition)
  StartPosition = InStrRev(a,"ma",StartPosition)
  print StartPosition
  print
  StartPosition = StartPosition - 1
wend
