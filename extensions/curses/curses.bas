'
' Module curses
'
' This module lets you use the Unix curses screen-handling library
'
' The general error-handling protocols is as follows:
' When not documented otherwise, every function returns and integer that
' is TRUE when there was no error and FALSE (0) otherwise.
'
' Some general guidelines for writing curses-aware programs in ScripBasic:
' 
' Due to a limitation in the curses library, number of foreground-background
' combinations (color pairs) on the screen at one time is limited. See
' the documentation of maxcolors function for details.

' Determine if changes to the screen should take effect immediately,
' or wait for the "refresh" command.
declare sub ::autorefresh	alias "sbautorefresh"	lib "curses"

' Sould the bell of the terminal (if there is one).
declare sub ::beep		alias "sbbeep"		lib "curses"

' Flash the screen of the terminal.
declare sub ::flash		alias "sbflash"		lib "curses"

' Clear the screen.
declare sub ::erase		alias "sberase"		lib "curses"

' Refresh the screen.
declare sub ::refresh		alias "sbrefresh"	lib "curses"

' Move cursor to the specified (X,Y) coordinates
declare sub ::move		alias "sbmove"		lib "curses"

' Return current X coordinate
declare sub ::getx		alias "sbgetx"		lib "curses"

' Return current Y coordinate
declare sub ::gety		alias "sbgety"		lib "curses"

' Return width of current window
declare sub ::getsizex		alias "sbgetsizex"	lib "curses"

' Return height of current window
declare sub ::getsizey		alias "sbgetsizey"	lib "curses"

' Print string to screen
declare sub ::addstr		alias "sbaddstr"	lib "curses"

' Print string to screen, shift characters under and following the cursor
' right
declare sub ::insstr		alias "sbinsstr"	lib "curses"

' To be called with one integer argument. Is that argument is positive, 
' insert the appropriate number of lines above the current line. If
' the argument is negative, delete abs(n) lines starting from the one
' containing the cursor. Lines under the cursor are shifted up or down.
declare sub ::insdelln		alias "sbinsdelln"	lib "curses"

' Delete the character under the cursor. Characters following the
' cursor are shifted left.
declare sub ::delch		alias "sbdelch"		lib "curses"

' Turn on specified attributes. Expects one integer argument, which 
' should be a bit mask constructed using the A* constants below.
declare sub ::attron		alias "sbattron"	lib "curses"

' Turn off specified attributes. To be called the same way as attron.
declare sub ::attroff		alias "sbattroff"	lib "curses"

' Set exactly the attributes specified. 
declare sub ::attrset		alias "sbattrset"	lib "curses"

' Change the attributes of characters starting from the cursor. The 
' first argument is the number of characters, the second is the color
' to be set for those characters and the third argument is an attribute
' mask.
declare sub ::chgat		alias "sbchgat"		lib "curses"

' Return the maximum number of color pairs on this system. Every color
' pair argument to curses functions should be an integer between
' 1 and maxcolors-1.
declare sub ::maxcolors		alias "sbmaxcolors"	lib "curses"

' Initialize a color pair to specified foreground and background. The first
' argument is the number of the color pair (a small integer starting from 1),
' The second argument is the foreground color, the third is the background.
' When a color pair's definition is changed, every occurrence on the 
' screen is repainted with the new definition (you cannot cheat that way :()
declare sub ::initpair		alias "sbinitpair"	lib "curses"

' Set active color pair to the specified one (expects an integer argument).
declare sub ::setcolor		alias "sbsetcolor"	lib "curses"

' Set the background color and attributes for the current window. The 
' background means that every characters's background color is combined
' with the window's background color and every non-blank character's 
' attribute is combined with the attributes specified. The first argument
' is the background color pair, the second is the required background 
' attributes, the third argument is a flag, which, if not equals to 0
' means that the screen should be repainted so that the new background
' specification immediately comes to effect for the whole screen.
declare sub ::setbackground	alias "sbsetbackground"	lib "curses"

' Read a character input from the user. Can be given two optional
' arguments: the first one is if you would like to echo the character
' read on the screen or not, the second one determines whether to wait
' for a keystroke if there is none pending or not. If no keystroke 
' could be returned, returns -1, otherwise returns the character.
declare sub ::getch		alias "sbgetch"		lib "curses"

' Read a string from the screen. Expects one integer argument, which is
' the maximum length of the string to be read. Returns the string read.
' Error during reading the string is currently not reported.
declare sub ::getstr		alias "sbgetstr"	lib "curses"

' Returns (as a string) the name of the key given in the argument.
declare sub ::keyname		alias "sbkeyname"	lib "curses"

' Create a new window and return its handle (which is a small integer).
' Expects four integer arguments, the coordinates of the upper-left
' corner of the window and its width and height.
declare sub ::newwin		alias "sbnewwin"	lib "curses"

' Delete the window specified.
declare sub ::delwin		alias "sbdelwin"	lib "curses"

' Set current window to the one in the argument (which must be a window 
' handle). The root window (which always exists) has handle number 1.
declare sub ::setwin		alias "sbsetwin"	lib "curses"

' Draw a border using line-drawing characters around the current window.
declare sub ::border		alias "sbborder"	lib "curses"

global Const AEmpty		= 0x0000
global Const ANormal		= 0x0001
global Const AStandout		= 0x0002
global Const AUnderline		= 0x0004
global Const AReverse		= 0x0008
global Const ABlink		= 0x0010
global Const ADim		= 0x0020
global Const ABold		= 0x0040
global Const AProtect		= 0x0080
global Const AInvis		= 0x0100
global Const AAltcharset	= 0x0200
global Const AChartext		= 0x0400

global Const CBlack		= 0
global Const CRed		= 1
global Const CGreen		= 2
global Const CYellow		= 3
global Const CBlue		= 4
global Const CMagenta		= 5
global Const CCyan		= 6
global Const CWhite		= 7

'border
'move(1,1)
'initpair 1, CGreen, CYellow
'setcolor 1
'attron ABold
'addstr "Alma"
'addstr(maxcolors())
'getch 
