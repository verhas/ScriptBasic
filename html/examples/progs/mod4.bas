A=1
module boo
 A= 2
 module ::baa
 A= 3
 print A
 print _::A
 print main::A
end module
end module
