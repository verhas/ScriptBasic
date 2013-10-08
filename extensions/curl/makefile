


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curl.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curl.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curl.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curl.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/curl.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curl.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/curl.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/interface.o -lcurl -lssl -lcrypto -ldl -lc

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/curl/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curl.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curl.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_curl.texi


