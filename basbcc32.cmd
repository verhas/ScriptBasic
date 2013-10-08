REM @echo off
REM
REM compile a BASIC program to exe using the Borland Free bcc32 compiler
REM The argument has to be the basic program w/o the .bas extension
REM
scriba -n -Co %1.c %1.bas
bcc32 -5 -c -o%1.obj %1.c
bcc32 %1.obj \ScriptBasic\bin\libscriba_omg.obj \ScriptBasic\bin\libscriba_omg.lib
