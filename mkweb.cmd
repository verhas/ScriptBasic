REM
REM This batch file compiles the ScriptBasic web
REM Should be started w/o argument
REM
cd html
perl ..\ssplit.pl source.txt
perl ..\mkweb.pl
del *.jam
del *.jim
cd ..
