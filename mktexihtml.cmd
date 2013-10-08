cd html\texi
perl ..\..\jamal.pl devguide.texi.jam devguide.texi
perl ..\..\jamal.pl ug.texi.jam ug.texi
perl ..\..\jamal.pl auxfiles.texi.jam auxfiles.texi
perl ..\..\jamal.pl source.texi.jam source.texi

mkdir ..\..\..\html\texi
cd ..\..\..\html\texi

perl ..\..\source\t2h.pl ..\..\source\html\texi\devguide.texi devguide
perl ..\..\source\t2h.pl ..\..\source\html\texi\ug.texi ug
perl ..\..\source\t2h.pl ..\..\source\html\texi\auxfiles.texi auxfiles
perl ..\..\source\t2h.pl ..\..\source\html\texi\source.texi source

cd ..\..\source\html\texi

del devguide.texi
del ug.texi
del auxfiles.texi
del source.texi

cd ..\..
