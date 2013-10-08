


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/ux.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/ux.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_ux.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/ux.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/ux.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/ux.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/ux.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/ux/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_ux.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_ux.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_ux.texi


