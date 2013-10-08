cd html\texi
mkdir tmp
cd tmp

perl ..\..\..\jamal.pl ..\%1.texi.jam %1.texi
perl ..\..\..\t2h.pl %1.texi %1
mkdir ..\..\..\..\html\texi\

copy %1.texi ..\..\..\..\html\texi\
gzip %1.texi
copy %1.texi.gz ..\..\..\..\html\texi\

copy %1.html ..\..\..\..\html\texi\
gzip %1.html
copy %1.html.gz ..\..\..\..\html\texi\

hhc %1.hhp
copy %1.chm ..\..\..\..\html\texi\

tar cfz %1.html.tgz *.html
copy %1.html.tgz ..\..\..\..\html\texi\

rm -rf ..\..\..\..\html\texi\%1
mkdir ..\..\..\..\html\texi\%1
copy *.html ..\..\..\..\html\texi\%1\

tex -silent %1.texi
tex -silent %1.texi
dvips %1.dvi
gzip %1.ps
dvipdfm %1.dvi
copy %1.ps.gz ..\..\..\..\html\texi\
copy %1.pdf ..\..\..\..\html\texi\
gzip %1.pdf
copy %1.pdf.gz ..\..\..\..\html\texi\

tar cfz %1.tgz *.bas
copy %1.tgz ..\..\..\..\html\texi\
cd ..
rm -rf tmp
cd ..\..
