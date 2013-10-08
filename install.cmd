REM
REM FILE: install.cmd
REM
REM Run this command script to install ScriptBasic after it was compiled
REM from source. The installation directory is T:\ScriptBasic
REM
REM This installation is needed by the setup program pack.cmd to build
REM the compressed binary installation package using the files put into
REM that directory.
REM
REM This script only checks that the two version of the compilers supported
REM and calls the basic program install.sb to do the work.
REM

IF EXIST bin\vc7\exe\scriba.exe GOTO VC7INSTALL
IF EXIST bin\bcc\exe\scriba.exe GOTO BCCINSTALL
GOTO ENDLABEL

:VC7INSTALL
IF "%1" == "" GOTO VC7IPRD
bin\vc7\exe\scriba install.sb %1 vc7
GOTO ENDLABEL
:VC7IPRD
bin\vc7\exe\scriba install.sb T:\ScriptBasic vc7
GOTO ENDLABEL

:BCCINSTALL
IF "%1" == "" GOTO BCCIPRD
bin\bcc\exe\scriba install.sb %1 bcc
GOTO ENDLABEL
:BCCIPRD
bin\bcc\exe\scriba install.sb T:\ScriptBasic bcc
GOTO ENDLABEL

:ENDLABEL
