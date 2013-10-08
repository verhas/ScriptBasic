A=1
module boo
 A= 2
 module boo::baa
 A= 3
 print A
 print boo::A
 print main::A
end module
end module
