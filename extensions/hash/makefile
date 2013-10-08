


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/hash.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/hash.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_hash.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/hash.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/hash.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/hash.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/hash.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/hash/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_hash.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_hash.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_hash.texi


