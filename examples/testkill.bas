print "give me a pid to kill!"
line input pid
if kill(pid) then
  print "success\n"
else
  print "cannot kill\n"
endif
