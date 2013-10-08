


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/bdb.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/bdb.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_bdb.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/bdb.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/bdb.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/bdb.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/bdb.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/interface.o -lbdb

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/bdb/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_bdb.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_bdb.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_bdb.texi


