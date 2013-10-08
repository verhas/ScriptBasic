print Replace("alabama mama", "a","x",3,5)
print
OriginalString = "a-a-a-a-"
StringToReplace = "a"
ReplaceString ="KURTA"
for i=-1 to 5
  print Replace(OriginalString,StringToReplace,ReplaceString,i)
  print
next i
print "---------\n"
print Replace(OriginalString,StringToReplace,ReplaceString)
print
for i=-1 to 5
  print Replace(OriginalString,StringToReplace,ReplaceString,i,3)
  print
next i
print "---------\n"
print Replace(OriginalString,StringToReplace,ReplaceString,undef,3)
print
