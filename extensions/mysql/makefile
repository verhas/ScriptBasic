


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/mysql.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/mysql.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_mysql.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/mysql.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/mysql.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/mysql.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/mysql.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/interface.o -lmysqlclient

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/mysql/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_mysql.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_mysql.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_mysql.texi


