/* FILE: curses.c

This file implements the curses module for ScriptBasic. This module is
an interface to the Unix curses library.

--GNU LGPL
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

the next line tells the program setup.pl that this module can be installed only
under UNIX.

UXLIBS: -lcurses

*/
#include <curses.h>
#include <malloc.h>

/**
=H the module CURSES
This module implements a ScriptBasic-curses interface (something like CIO
for Unix) that makes direct cursor control and screen drawing possible on
Unix systems.

Author: Berki Lukacs Tamas 

=section Installation
=H Installation

The distribution contains the C language source file and the ScriptBasic
include file. The C source must be compiled with the following command:

=verbatim
cc -shared -o libcurses.so curses.c -lcurses
=noverbatim

The curses library and its include files (for Debian users: libncurses5 and
libncurses5-dev packages) must be installed.

Move the library to your ScriptBasic library directory and the include file
to the ScriptBasic include file directory (or redefine include and library
directories in basic.conf).

I hope that this module will be included in the ScriptBasic distribution
RSN.

=section CIO compatibility
My original plan was to port the CIO library to Unices, but most of its
functions could not be adapted and curses supports a much wider range
of screen operations than CIO, so this extension became a curses interface.

This means that curses routines are not compatible with CIO, although
it should be very easy to make a subset of CIO routines available under
Unices.

sbkbhit is the equivalent of getch(0, 0).
Console title modification is (of course) not supported, but something
could be done with xterm's title modification escape sequences.

NoBreak is not supported. ScriptBasic programs should be terminated with
Ctrl-C (SIGINT).

SizeX and SizeY are the equivalent of getmaxx and getmaxy.

Color support is VERY different in curses.

Cls is the direct equivalent of erase.

Naturally, the ncurses console cannot be resized like the SetWindow command
does it under Windows.

=section Color handling

The only non-straightforward thing about the curses library is its
color support.

Every character on the screen has an associated color pair number. A
color pair consists of a background and a foreground color. There are a
a very finite number of available color pairs, which can be found out
using the maxcolors function.

If you want to print a character in a specified color, you initialize
a color pair to that color, and then make that the active color pair
using setcolor.

You cannot cheat (i.e. print more colors on the screen than the
number of available color pairs) by printing a character and then
changing its color pair's definition, because if a color pair is
changed, its occurrences on the screen are repainted using the new
color. This is quirk in curses (I think for the sake of being
compatible with some kind of old terminals).

=section Windows

This is VERY straightforward. Windows can be created and deleted using
newwin and delwin. Sub-window support is there in ncurses, but I chose
not to support that because the documentation of ncurses says that it
is still buggy.

*/

#include "../../basext.h"

#define MAX_WINDOWS 16
WINDOW *window_list[MAX_WINDOWS];
WINDOW *current_window;

/* This variable stores whether the module should issue a refresh() call
   after each operation that can potentially modify the screen
*/

static int auto_refresh = -1;

static int color_to_basic(int color)
{
	switch (color) {
	    case COLOR_BLACK: return 0;
	    case COLOR_RED: return 1;
	    case COLOR_GREEN: return 2;
	    case COLOR_YELLOW: return 3;
	    case COLOR_BLUE: return 4;
	    case COLOR_MAGENTA: return 5;
	    case COLOR_CYAN: return 6;
	    case COLOR_WHITE: return 7;
	    default:
		return -1;
	}
}

static int color_to_curses(int sbcolor)
{
	int colors[] = {
		COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
		COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
	};

	if (sbcolor < 0 || sbcolor > 7)
		return -1;

	return colors[sbcolor];
}

static int attr_to_basic(int attr)
{
	int sbattr = 0;

	if (attr & A_NORMAL)
		sbattr |= 0x0001;
	if (attr & A_STANDOUT)
		sbattr |= 0x0002;
	if (attr & A_UNDERLINE)
		sbattr |= 0x0004;
	if (attr & A_REVERSE)
		sbattr |= 0x0008;
	if (attr & A_BLINK)
		sbattr |= 0x0010;
	if (attr & A_DIM)
		sbattr |= 0x0020;
	if (attr & A_BOLD)
		sbattr |= 0x0040;
	if (attr & A_PROTECT)
		sbattr |= 0x0080;
	if (attr & A_INVIS)
		sbattr |= 0x0100;
	if (attr & A_ALTCHARSET)
		sbattr |= 0x0200;
	if (attr & A_CHARTEXT)
		sbattr |= 0x0400;

	return sbattr;
}

static attr_t attr_to_curses(int sbattr)
{
	attr_t attr = 0;

	if (sbattr & 0x0001)
		attr |= A_NORMAL;
	if (sbattr & 0x0002)
		attr |= A_STANDOUT;
	if (sbattr & 0x0004)
		attr |= A_UNDERLINE;
	if (sbattr & 0x0008)
		attr |= A_REVERSE;
	if (sbattr & 0x0010)
		attr |= A_BLINK;
	if (sbattr & 0x0020)
		attr |= A_DIM;
	if (sbattr & 0x0040)
		attr |= A_BOLD;
	if (sbattr & 0x0080)
		attr |= A_PROTECT;
	if (sbattr & 0x0100)
		attr |= A_INVIS;
	if (sbattr & 0x0200)
		attr |= A_ALTCHARSET;
	if (sbattr & 0x0400)
		attr |= A_CHARTEXT;

	return attr;
}

#define AUTO_REFRESH do { if (auto_refresh) {  refresh(); } } while (0)
#define HANDLE_ERR(cmd) \
	LONGVALUE(besRETURNVALUE) =  ((cmd) == ERR) ? 0 : -1;

/**
Console handling low level routines for Unix ncurses environment.

This module implements interface routines to the ncurses library.

*/

static int color_supported;
besSUB_SHUTDOWN
  return 0;
besEND

besSUB_FINISH
{
	endwin();
}
besEND

besSUB_START
{
	int i;
	
	initscr();
	current_window = stdscr;
	cbreak();
	noecho();
	if (start_color() == ERR)
		color_supported = FALSE;
	else
		color_supported = TRUE;

	for (i=0; i<MAX_WINDOWS; i++)
		window_list[i] = NULL;

	window_list[0] = stdscr;
	return COMMAND_ERROR_SUCCESS;
}  
besEND

/**
=section setwin
=H Set current window

=verbatim
setwin(WindowHandle)
=noverbatim

Set current window to the specified one. Returns 0 in case of error, returns
-1 otherwise. The handle of stdscr (full-screen default window) is 1.

*/

besFUNCTION(sbsetwin)
{
	int err, wn;

	besALLOC_RETURN_LONG;
	
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &wn besGETARGE;
	if (err)
		return err;
	wn--;
	
	if (wn < 0 || wn >= MAX_WINDOWS || window_list[wn] == NULL) {
		LONGVALUE(besRETURNVALUE) = 0;
	} else {
		current_window = window_list[wn];
		LONGVALUE(besRETURNVALUE) = -1;
	}
}
besEND

/**
=section beep
=H Beep.

=verbatim
border
=noverbatim

Sound the bell of the terminal.
*/
besFUNCTION(sbborder)
{
	wborder(current_window, 0, 0, 0, 0, 0, 0, 0, 0);
	AUTO_REFRESH;
}
besEND

/**
=section newwin
=H Create a new window

=verbatim
WindowHandle = newwin(X, Y, Width, Height)
=noverbatim

Creates a new window with specified dimensions and returns its handle.
Returns 0 in case of error.
Window handles are small integers starting from 1. 

*/

besFUNCTION(sbnewwin)
{
	int i;
	WINDOW *wnd;
	int rows, cols, y, x;
	int err;
	
	besALLOC_RETURN_LONG;
	if (besARGNR < 4)
		return COMMAND_ERROR_FEW_ARGS;
	err = besGETARGS "iiii", &x, &y, &cols, &rows besGETARGE;
	if (err)
		return err;
	
	for (i=0; i<MAX_WINDOWS; i++)
		if (window_list[i] == NULL)
			break;
	
	if (i == MAX_WINDOWS) {
		LONGVALUE(besRETURNVALUE) = 0;
		return COMMAND_ERROR_SUCCESS;
	}

	wnd = newwin(rows, cols, y, x);
	if (wnd == NULL) {
		LONGVALUE(besRETURNVALUE) = 0;
		return COMMAND_ERROR_SUCCESS;
	}
	
	window_list[i] = wnd;
	current_window = wnd;
	LONGVALUE(besRETURNVALUE) = i+1;
}
besEND

/**
=H Delete a window

=verbatim
delwin(WindowHandle)
=noverbatim

Delete the specified window. Return 0 in case of error.
*/

besFUNCTION(sbdelwin)
{
	int i, err;

	besALLOC_RETURN_LONG;
		
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &i besGETARGE;
	if (err)
		return err;
	i--;
	
	if (i < 0 || i >= MAX_WINDOWS || window_list[i] == NULL)
		return COMMAND_ERROR_BAD_CALL;

	err = delwin(window_list[i]);
	if (err == ERR) {
		LONGVALUE(besRETURNVALUE) = 0;
	} else {
		window_list[i] = NULL;
		LONGVALUE(besRETURNVALUE) = -1;
	}
}
besEND

/**
=section autorefresh
=H Set automatic refresh

=verbatim
autorefresh(1)
=noverbatim

Set automatic refresh flag. If it is true, a screen refresh occurs every
time a curses function is called that can possibly modify the contents
of the screen.

Even if autorefresh is inactive, there are some functions (mainly the
keyboard input functinos) that refresh the screen automatically. This
is a limitation of the curses library.
*/

besFUNCTION(sbautorefresh)
{
	int err;
	int v;

	besALLOC_RETURN_LONG;
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &v besGETARGE;
	if (err)
		return err;
	auto_refresh = v;
}
besEND

/**
=section beep
=H Beep.

=verbatim
beep
=noverbatim

Sound the bell of the terminal.
*/
besFUNCTION(sbbeep)
{
	beep();
}
besEND

/**
=section flash
=H Flash the screen

=verbatim
flash
=noverbatim

Flash the screen (cause a visible bell). If the terminal does not support
visible bell, the audible bell is sounded.
*/

besFUNCTION(sbflash)
{
	flash();
}
besEND

/**
=section erase
=H Erase the screen

=verbatim
erase
=noverbatim

Copy blanks to every character position on the screen, thus erasing it.

*/

besFUNCTION(sberase)
{
	erase();
	//	AUTO_REFRESH;
}
besEND

/**
=section refresh
=H Refresh the screen

=verbatim
refresh
=noverbatim

Refresh the screen.
*/

besFUNCTION(sbrefresh)
{
	wrefresh(current_window);
}
besEND


/**
=section move
=H Move the cursor

=verbatim
move(X, Y)
=noverbatim

Move the cursor to the specified coordinates. The coordinates are 0-based.
Return 0 on error, -1 otherwise.

*/

besFUNCTION(sbmove)
{
	int err;
	int x, y;

	besALLOC_RETURN_LONG;
	if (besARGNR < 2)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "ii", &x, &y besGETARGE;
	if (err)
		return err;

	HANDLE_ERR(wmove(current_window, y, x));
}
besEND

/**
=section getx
=H Get X position of cursor

=verbatim
XCoord = getx()
=noverbatim

Get the X coordinate of cursor position.

*/
besFUNCTION(sbgetx)
{
	int x, y;

	besALLOC_RETURN_LONG;

	getyx(current_window, y, x);
	LONGVALUE(besRETURNVALUE) = x;
}
besEND

/**
=section gety
=H Get Y position of cursor

=verbatim
YCoord = gety()
=noverbatim

Get the Y coordinate of cursor position.

*/
besFUNCTION(sbgety)
{
	int err;
	int x, y;

	besALLOC_RETURN_LONG;

	getyx(current_window, y, x);
	LONGVALUE(besRETURNVALUE) = y;
}
besEND

/**
=section getsizex
=H Get width of current window

=verbatim
XSize = getsizex()
=noverbatim

Returns the width of current window.
*/
besFUNCTION(sbgetsizex)
{
	int err;
	int x, y;

	besALLOC_RETURN_LONG;

	getmaxyx(current_window, y, x);
	LONGVALUE(besRETURNVALUE) = x;
}
besEND

/**
=section getsizey
=H Get height of current window

=verbatim
YSize = getsizey()
=noverbatim

Returns the height of current window.
*/
besFUNCTION(sbgetsizey)
{
	int err;
	int x, y;

	besALLOC_RETURN_LONG;

	getmaxyx(current_window, y, x);
	LONGVALUE(besRETURNVALUE) = y;
}
besEND

/**
=section addstr
=H Print a string

=verbatim
addstr("Hello, World!")
=noverbatim

Print the specified string in the current window.
*/

besFUNCTION(sbaddstr)
{
	int i, err;
	char *str;
	VARIABLE arg;
	
	besALLOC_RETURN_LONG;
	
	for (i=1; i<=besARGNR; i++) {
		arg = besARGUMENT(i);
		if (arg == NULL)
			return COMMAND_ERROR_FEW_ARGS;
		besDEREFERENCE(arg);
		if (arg == NULL) 
			return COMMAND_ERROR_ARGUMENT_RANGE;
		arg = besCONVERT2STRING(arg);
		besCONVERT2ZCHAR(arg, str);
		err = waddstr(current_window, str);
		if (err == ERR)
			break;
	}

	AUTO_REFRESH;
	HANDLE_ERR(err);
}
besEND

/**
=section insstr
=H Insert a string 

=verbatim
insstr "Hello, World!"
=noverbatim

Insert the specified string under the cursor. Control characters
(tab, backspace, newline) are handled appropriately. It possible
that characters at the end of line are lost.
*/

besFUNCTION(sbinsstr)
{
	int err;
	char *str;

	besALLOC_RETURN_LONG;
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;
	
	err = besGETARGS "z", &str besGETARGE;
	if (err)
		return err;
	
	err = winsstr(current_window, str);
	AUTO_REFRESH;
	HANDLE_ERR(err);
}	
besEND

/**
=section insdelln
=H Insert or delete lines

=verbatim
insdelln(10)
=noverbatim

If the argument is positive, insert the appropriate number of lines above
the current line. If the argument is negative, delete the appropriate number
of lines starting from the one containing the cursor.
*/

besFUNCTION(sbinsdelln)
{
	int err;
	int n;

	besALLOC_RETURN_LONG;
	
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &n besGETARGE;
	if (err)
		return err;

	err = winsdelln(current_window, n);
	AUTO_REFRESH;
	HANDLE_ERR(err);
}
besEND

/**
=section delch
=H Delete character

=verbatim
=noverbatim

Delete the character under the cursor.

*/
besFUNCTION(sbdelch)
{
	wdelch(current_window);
}
besEND

/**
=section instr
=H Get a string from the current window

=verbatim
Content = instr(10)
=noverbatim

These routines return  a  string  of  characters  in  str,
extracted  starting  at the current cursor position in the
named window.

*/
besFUNCTION(sbinstr)
{
	char *buf;
	int err;
	int no;

	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;
	err = besGETARGS "i", &no besGETARGE;
	if (err)
		return err;

	buf = malloc(no+1);
	winnstr(current_window, buf, no);
	
	besALLOC_RETURN_STRING(strlen(buf));
	memcpy(STRINGVALUE(besRETURNVALUE), buf, STRLEN(besRETURNVALUE));
	free(buf);
}
besEND

/**
=section attron
=H Turn on the specified character attributes

=verbatim
attron(ABold or AReverse)
=noverbatim

Turn on the specified attributes for the current window. The argument should
be a bit mask constructed from the A* constants.
*/
besFUNCTION(sbattron)
{
	int err;
	int sbattr;
	attr_t attr;
	
	besALLOC_RETURN_LONG;
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &sbattr besGETARGE;
	if (err)
		return err;
	attr = attr_to_curses(sbattr);
	wattron(current_window, attr);
}
besEND

/**
=section attroff
=H Turn off the specified character attributes

=verbatim
attroff(ABold or AReverse)
=noverbatim

Turn off the specified attributes for the current window. The argument should
be a bit mask constructed from the A* constants.

*/
besFUNCTION(sbattroff)
{
	int err;
	int sbattr;
	attr_t attr;
	
	besALLOC_RETURN_LONG;
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &sbattr besGETARGE;
	if (err)
		return err;
	attr = attr_to_curses(sbattr);
	wattroff(current_window, attr);
}
besEND

/**
=section attrset
=H Set current character attributes.

=verbatim
attrset(ABold or AReverse)
=noverbatim

Set current character attributes to the specified value. The argument should
be a bit mask constructed from the A* constants. 

*/
besFUNCTION(sbattrset)
{
	int err;
	int sbattr;
	attr_t attr;
	
	besALLOC_RETURN_LONG;
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &sbattr besGETARGE;
	if (err)
		return err;
	attr = attr_to_curses(sbattr);
	wattrset(current_window, attr);
}
besEND

/**
=section chgat
=H Change attributes on screen

=verbatim
chgat(5, 1, ABold)
=noverbatim

Change the attributes of the characters on the screen and their color. In
the examples, the next five characters from the cursor will be changed
to the first color pair and bold face.
*/

besFUNCTION(sbchgat)
{
	int err;
	int no, col, attr;

	if (besARGNR < 3)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "iii", &no, &col, &attr besGETARGE;
	if (err)
		return err;
	
	attr = attr_to_curses(attr);
	wchgat(current_window, no, attr, col, NULL);
}
besEND

/**
=section maxcolors
=H Return number of available color pairs

=verbatim
Colors = maxcolors()
=noverbatim

Returns the number available color pairs on this system.
*/
besFUNCTION(sbmaxcolors)
{
	besALLOC_RETURN_LONG;
	LONGVALUE(besRETURNVALUE) = COLOR_PAIRS - 1;
}
besEND

/**
=section initpair
=H Initialize color pair

=verbatim
initpair(Color, Foreground, Background)
=noverbatim

Set a color pair (foreground and background).
Returns 0 on error and -1 otherwise.

*/
besFUNCTION(sbinitpair)
{
	int err;
	int c, f, b;

	besALLOC_RETURN_LONG;
	if (besARGNR < 3)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "iii", &c, &f, &b besGETARGE;
	if (err)
		return err;

	f = color_to_curses(f);
	b = color_to_curses(b);

	HANDLE_ERR(init_pair(c, f, b));
}
besEND

/**
=section setcolor
=H Set current color

=verbatim
setcolor(ColorPair)
=noverbatim

Set current color to the specified color pair. Returns 0 on error and
-1 otherwise.

*/
besFUNCTION(sbsetcolor)
{
	int err;
	int c;

	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;

	err = besGETARGS "i", &c besGETARGE;
	if (err)
		return err;

	/* disable all color attributes and enable color wished for */
	attroff(~attr_to_curses(0x07ff)); /* this is perhaps unnecessary */
	attron(COLOR_PAIR(c));
}
besEND

/**
=section setbackground
=H Set background color and attributes

=verbatim
setbackground(ColorPair, Attributes, Update)
=noverbatim

Set background color and attributes for the current window. The attribute
is logically OR'ed with all non-blank characters that are written on the
window. If Update is true, change all characters on the screen to the
new background.

*/
besFUNCTION(sbsetbackground)
{
	int err;
	int set, col, attr;

	besALLOC_RETURN_LONG;
	
	switch (besARGNR) {
	    case 2:
		err = besGETARGS "ii", &col, &attr besGETARGE;
		if (err)
			return err;
		set = 0;
		break;
	    case 3:
		err = besGETARGS "iii", &col, &attr, &set besGETARGE;
		if (err)
			return err;
		break;
	    default:
		return COMMAND_ERROR_FEW_ARGS;
	}

	attr = attr_to_curses(attr) | COLOR_PAIR(col);
	if (set)
		err = wbkgd(current_window,attr);
	else { 
		wbkgdset(current_window, attr);
		err = OK;
	}

	HANDLE_ERR(err);
}
besEND

/**
=section getch
=H Get character

=verbatim
Char = getch(Echo, Delay)
=noverbatim

Waits for a keypress and returns its curses key code. If Echo is true,
echo the character on the screen. If Delay is false, return -1 if
there is no available keystroke (and do not wait for one). Both
arguments are optional.

*/
besFUNCTION(sbgetch)
{
	int err, doecho = 0, dodelay = 1;
	int ch;

	besALLOC_RETURN_LONG;

	switch (besARGNR) {
	    case 1:
		err = besGETARGS "i", &doecho besGETARGE;
		if (err)
			return err;
	    case 0:
		break;
	    default:
		err = besGETARGS "ii", &doecho, &dodelay besGETARGE;
		break;
		
	}

	if (doecho)
		echo();
	if (!dodelay) {
		nodelay(current_window, 1);
	}
	ch = wgetch(current_window);
	noecho();
	nodelay(current_window, 0);

	if (ch == ERR)
		LONGVALUE(besRETURNVALUE) = -1;
	else
		LONGVALUE(besRETURNVALUE) = ch;
}
besEND

/**
=section getstr
=H Get string

=verbatim
Str = getstr(Length)
=noverbatim

Get string input from user. Length specifies the maximum possible length
for the string. Line-editing characters are handled appropriately.

*/
besFUNCTION(sbgetstr)
{
	char *buf;
	int err;
	int no;

	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;
	err = besGETARGS "i", &no besGETARGE;
	if (err)
		return err;

	buf = malloc(no+1);
	echo();
	wgetnstr(current_window, buf, no);
	noecho();
	
	besALLOC_RETURN_STRING(strlen(buf));
	memcpy(STRINGVALUE(besRETURNVALUE), buf, STRLEN(besRETURNVALUE));
	free(buf);
}
besEND

/**
=section keyname
=H Return name of a key

=verbatim
Name = keyname(Key)
=noverbatim

Return the name of a keystroke (probably returned by getch) as a string.

*/
besFUNCTION(sbkeyname)
{
	int err, key;
	const char *kn;
	
	if (besARGNR < 1)
		return COMMAND_ERROR_FEW_ARGS;
	err = besGETARGS "i", &key besGETARGE;
	if (err)
		return err;

	kn = keyname(key);
	
	besALLOC_RETURN_STRING(strlen(kn));
	memcpy(STRINGVALUE(besRETURNVALUE), kn, STRLEN(besRETURNVALUE));
}
besEND

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

SLFST CURSES_SLFST[] ={

{ "shutmodu" , shutmodu },
{ "bootmodu" , bootmodu },
{ "versmodu" , versmodu },
{ "finimodu" , finimodu },
	
{ "beep" , sbbeep },
{ "flash", sbflash },
{ "erase", sberase },
{ "refresh", sbrefresh },
{ "autorefresh", sbautorefresh },
	
{ "move", sbmove },
{ "getx", sbgetx },
{ "gety", sbgety },
{ "getsizex", sbgetsizex },
{ "getsizey", sbgetsizey },

{ "addstr", sbaddstr },
{ "insstr", sbinsstr },
{ "insdelln", sbinsdelln },
{ "delch", sbdelch },
{ "instr", sbinstr },
	
{ "attron", sbattron },
{ "attroff", sbattroff },
{ "attrset", sbattrset },
{ "chgat", sbchgat },
	
{ "maxcolors", sbmaxcolors },
{ "initpair", sbinitpair },
{ "setcolor", sbsetcolor },
{ "setbackground", sbsetbackground },
	
{ "getch", sbgetch },
{ "getstr", sbgetstr },

{ "keyname", sbkeyname },

{ "setwin", sbsetwin },
{ "newwin", sbnewwin },
{ "delwin", sbdelwin },
{ "border", sbborder },
{ NULL , NULL }
};
