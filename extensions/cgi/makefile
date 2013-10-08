


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/cgi.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/cgi.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_cgi.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/cgi.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/cgi.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/cgi.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/cgi.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o : cgi.c cgi.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/cgi.o cgi.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/interface.o : interface.c cgi.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/s_interface.o : interface.c cgi.h 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/cgi/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_cgi.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_cgi.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_cgi.texi


