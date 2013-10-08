d = 1.23456789e0
mult = 1.0e0

for i = 0 to 10
	for j = 0 to 10
		print "Round(", d*mult, ",", j, ") = ", round(d*mult,j),"\n"
		print "Round(", -1.0e0*d*mult, ",", j, ") = ",round(-1.0e0*d*mult,j), "\n"
	next
	mult *= 10.0e0
next
