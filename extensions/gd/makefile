


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/gd.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/gd.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_gd.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/gd.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/gd.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/gd.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/gd.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/interface.o -lc -lpng -lz -lgd

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/interface.o : interface.c gdfontl.h gd.h gdfonts.h gdfontt.h gdfontg.h gdfontmb.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/s_interface.o : interface.c gdfontl.h gd.h gdfonts.h gdfontt.h gdfontg.h gdfontmb.h 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/gd/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_gd.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_gd.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_gd.texi


