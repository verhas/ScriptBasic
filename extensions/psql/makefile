


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/psql.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/psql.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_psql.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/psql.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/psql.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/psql.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/psql.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/interface.o -lpq

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/psql/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_psql.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_psql.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_psql.texi


