' This is a sample program demonstrating the
' function kill. Get some harmless process
' running and get the pid of it using
' ps on UNIX or the task manager under NT
' kill the process using this program
'
print "give me a pid to kill!"
line input pid
if kill(pid) then
  print "success\n"
else
  print "cannot kill\n"
endif
