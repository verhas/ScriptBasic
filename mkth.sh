echo processing $1
cd html/texi
mkdir tmp
cd tmp

jamal ../$1.texi.jam $1.texi
t2h $1.texi $1
mkdir ../../../../html/texi/

cp $1.texi ../../../../html/texi/
cp $1.rtf ../../../../html/texi/

gzip $1.rtf
cp $1.rtf.gz ../../../../html/texi/

cp $1.html ../../../../html/texi/
gzip $1.html
cp $1.html.gz ../../../../html/texi/

#hhc $1.hhp
#cp $1.chm ../../../../html/texi/

tar cfz $1.html.tgz *.html
cp $1.html.tgz ../../../../html/texi/

rm -rf ../../../../html/texi/$1
mkdir ../../../../html/texi/$1
cp *.html ../../../../html/texi/$1/

tex $1.texi >/dev/null 2>/dev/null
tex $1.texi >/dev/null 2>/dev/null
dvips $1.dvi -o $1.ps
gzip $1.ps
dvipdfm $1.dvi
cp $1.ps.gz ../../../../html/texi/
cp $1.pdf ../../../../html/texi/
gzip $1.pdf
cp $1.pdf.gz ../../../../html/texi/
gzip $1.texi
cp $1.texi.gz ../../../../html/texi/

cd ..
#rm -rf tmp
cd ../..
