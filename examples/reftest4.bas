z = "eggdrop"
a[1] = 1
a[2] = 1.1
a[3] = "1111111"
a[4,1] = 4
a[4,2] = 4.4
a[4,3] = "4444444444444444"
ref a[4,4,5,4,5,5,6] = z
b = a
print b[1] ,"\n"
print b[2] ,"\n"
print b[3] ,"\n"
print b[4,1] ,"\n"
print b[4,2] ,"\n"
print b[4,3] ,"\n"
print b[4,4,5,4,5,5,6] ,"\n"
z = "chicken"
print b[4,4,5,4,5,5,6] ,"\n"

