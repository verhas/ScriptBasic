scriba progs/array1.bas > output/array1.txt
scriba progs/array2.bas > output/array2.txt
scriba progs/array3.bas > output/array3.txt
scriba progs/array4.bas > output/array4.txt
scriba progs/expression1.bas > output/expression1.txt
scriba progs/greet1.bas  < progs/greet1.txt > output/greet1.txt
scriba progs/greet2.bas  < progs/greet2.txt > output/greet2.txt
scriba progs/hello1.bas > output/hello1.txt
scriba progs/hello2.bas > output/hello2.txt
scriba progs/mod1.bas > output/mod1.txt
scriba progs/mod2.bas > output/mod2.txt
scriba progs/mod3.bas > output/mod3.txt
scriba progs/mod4.bas > output/mod4.txt
scriba progs/string1.bas > output/string1.txt
scriba progs/var1.bas > output/var1.txt
mkdir ..\..\..\html\examples
mkdir output
perl jamal.pl -m macros.jim array.html.jam ../../../html/examples/array.html
perl jamal.pl -m macros.jim dir.html.jam ../../../html/examples/dir.html
perl jamal.pl -m macros.jim exp.html.jam ../../../html/examples/exp.html
perl jamal.pl -m macros.jim ext.html.jam ../../../html/examples/ext.html
perl jamal.pl -m macros.jim file.html.jam ../../../html/examples/file.html
perl jamal.pl -m macros.jim fun.html.jam ../../../html/examples/fun.html
perl jamal.pl -m macros.jim greet.html.jam ../../../html/examples/greet.html
perl jamal.pl -m macros.jim hello.html.jam ../../../html/examples/hello.html
perl jamal.pl -m macros.jim if.html.jam ../../../html/examples/if.html
perl jamal.pl -m macros.jim index.html.jam ../../../html/examples/index.html
perl jamal.pl -m macros.jim loop.html.jam ../../../html/examples/loop.html
perl jamal.pl -m macros.jim math.html.jam ../../../html/examples/math.html
perl jamal.pl -m macros.jim mod.html.jam ../../../html/examples/mod.html
perl jamal.pl -m macros.jim pack.html.jam ../../../html/examples/pack.html
perl jamal.pl -m macros.jim string.html.jam ../../../html/examples/string.html
perl jamal.pl -m macros.jim sys.html.jam ../../../html/examples/sys.html
perl jamal.pl -m macros.jim var.html.jam ../../../html/examples/var.html
