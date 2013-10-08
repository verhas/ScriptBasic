


all : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/re.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/re.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_re.texi




/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/re.a : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/s_interface.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o 
	ar -r /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/lib/re.a /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/s_interface.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/re.so : /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/interface.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o 
	ld -shared -Bstatic -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/dll/re.so /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/interface.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o  

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/interface.o : interface.c regex.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/s_interface.o : interface.c regex.h 
	gcc -w -DSTATIC_LINK=1 -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/s_interface.o interface.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o : regcomp.c cclass.h regex2.h regex.h utils.h cname.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regcomp.o regcomp.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o : regerror.c regex.h utils.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regerror.o regerror.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o : regex_test.c regex.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regex_test.o regex_test.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o : regexec.c regex2.h regex.h utils.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regexec.o regexec.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o : regfree.c regex2.h regex.h utils.h 
	gcc -w -c -o /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/mod/obj/re/regfree.o regfree.c

/Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_re.texi : manual.texi.jam
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/jamal.pl manual.texi.jam /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_re.texi
	perl /Users/verhasp/github/ScriptBasic/ScriptBasic/t2h.pl /Users/verhasp/github/ScriptBasic/ScriptBasic/bin/texi/mod_re.texi


