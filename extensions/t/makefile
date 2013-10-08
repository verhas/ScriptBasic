


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/t.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/t.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_t.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/t.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/t.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/t.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/t.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/t/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_t.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_t.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_t.texi


