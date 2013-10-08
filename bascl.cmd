REM @echo off
REM
REM compile a BASIC program to exe using the Visual C compiler
REM The argument has to be the basic program w/o the .bas extension
REM
scriba -n -Co %1.c %1.bas
cl /Ox /GA6s /DWIN32 /MT /nologo /W0 /c /Fo%1.obj %1.c
cl /Ox /GA6s /DWIN32 /MT /nologo /W0 /Fe%1.exe %1.obj \ScriptBasic\lib\libscriba.lib ws2_32.lib advapi32.lib
