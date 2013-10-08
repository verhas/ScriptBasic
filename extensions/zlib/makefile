


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/zlib.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/zlib.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_zlib.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/zlib.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/zlib.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/zlib.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/zlib.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/interface.o -lc -lz

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/interface.o : interface.c zlib.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/s_interface.o : interface.c zlib.h 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/zlib/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_zlib.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_zlib.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_zlib.texi


