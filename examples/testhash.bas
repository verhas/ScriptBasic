import hash.bas
Const nl="\n"
h = hash::New()
hash::SetValue h,"evil",666
Zazu = hash::Value(h,"evil")
print Zazu,nl
Zazu = "BILL"
print hash::Value(h,"evil"),nl
hash::Release h
