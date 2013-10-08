


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/xml.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/xml.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_xml.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/xml.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/s_interface.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/xml.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/s_interface.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/xml.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/interface.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/xml.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/interface.o -lxml2

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/interface.o : interface.c 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/s_interface.o : interface.c 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/xml/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_xml.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_xml.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_xml.texi


