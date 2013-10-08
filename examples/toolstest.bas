import t.bas

sub PrintVar(V,level)
  local i,j

  if IsArray(V) Then
   for i=lbound(V) to ubound(V)
     print space(level)
     print i,"->"
     PrintVar V[i],level+1
   next i
   exit sub
  end if

  print V,"\n"
end sub


for i=1 to 5
for j=1 to 5
for k=1 to 5
 a[i,j,k] = i*j*k
next k
next j
next i


s = t::Array2StringMD5(a)

t::SaveString "savedarray.bin",s

s = undef

q = t::LoadString("savedarray.bin")

t::String2ArrayMD5 b,q

PrintVar b,1
