import curses.bas

curses::autorefresh 1

curses::border
curses::move(1,1)

curses::addstr("Color test:")
for bg=0] to 7
	for fg=0 to 7
		curses::initpair(fg+1, fg, bg)
		curses::setcolor(fg+1)
		curses::move(fg+1, 2)
		curses::addstr("A")
	next fg
	curses::getch 0
next bg

curses::erase
curses::setcolor(0)
curses::border
curses::move(1, 1)
curses::addstr("Attribute test:")
attr = 1
for sh=0 to 12
	curses::attrset(attr)
	curses::move(1, sh+2)
	curses::addstr("Attribute")
	attr = attr * 2
next sh
curses::getch 0
