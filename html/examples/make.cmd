.\scriba -v 2> buildinfo.txt
perl ssplit.pl source.ssplit
perl make.pl
call build.cmd
del *.jam
del *.jim
pause
