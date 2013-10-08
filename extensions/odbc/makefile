


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/odbc.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/odbc.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_odbc.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/odbc.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/odbc.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/odbc.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/odbc.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/interface.o -lodbc

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/odbc/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_odbc.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_odbc.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_odbc.texi


