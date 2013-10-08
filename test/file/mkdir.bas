' To run this test open two terminal windows. Start the program in one
' until you get the first message.
' Test in the second window that the directories hoho and hoh/haha were
' created.
' Press enter and after the program has finished check that the directory
' disappeared.
mkdir "./hoho/haha"
print "Now you have dir hoho/haha\n"
print "Press enter..."
line input q
deltree "hoho"
print "Now you do not have dir hoho\n"
