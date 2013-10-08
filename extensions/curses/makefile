


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curses.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curses.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curses.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curses.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curses.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curses.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curses.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/interface.o -lcurses

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curses/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curses.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curses.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curses.texi


