


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/dbg.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/dbg.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_dbg.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/dbg.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/dbg.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/dbg.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/dbg.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o : dbg_con.c dbg.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/dbg_con.o dbg_con.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/interface.o : interface.c dbg_comm.h dbg.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/s_interface.o : interface.c dbg_comm.h dbg.h 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/dbg/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_dbg.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_dbg.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_dbg.texi


