REM
REM this bacth file compiles a TEXI documenation
REM the argument has to be the file to be compiled
REM w/o the texi.jam extension
REM
REM This is the light version of the batch file mkth
REM because this one does not compile with the TeX
REM
REM This version can be used during development to
REM generate a fast HTML and chm proofread version of
REM the altered document
REM
cd html\texi
mkdir tmp
cd tmp

perl ..\..\jamal.pl ..\%1.texi.jam %1.texi
perl ..\..\..\t2h.pl %1.texi %1
mkdir ..\..\..\..\html\texi\

copy %1.html ..\..\..\..\html\texi\
del %1.html

rm -rf ..\..\..\..\html\texi\%1
mkdir ..\..\..\..\html\texi\%1
copy *.html ..\..\..\..\html\texi\%1\

hhc %1.hhp
copy *.chm ..\..\..\..\html\texi\

cd ..
rem rm -rf tmp
cd ..\..
