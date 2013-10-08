#!/usr/bin/scriba
' FILE: cbbfc.bas
'
' Convert Basic Binary Format Command
'
' This program reads a ScriptBasic binary format file
' and replaces the leading interpreter specification
' by the one specified by the user.
'
' For example a program was developed on a machine having
' scriba in the directory /usr/bin. The first line of the
' bbf file is then #!/usr/bin/scriba
' To run this bbf file without having the original source file
' on a machine having scriba on /usr/local/bin this first line
' has to be replaced. This is not a simple task, because
' the bbf file is binary

cmdlin = command()

split cmdlin by " " to FileName,Interpreter

on error goto usage

open FileName for input as 1
binmode 1
File$ = input(lof(1),1)
close 1

if left(File$,1) = "#" then
  i = 1
  while i < len(File$) and mid(File$,i,1) <> "\n"
    i = i+1
  wend
  if mid(File$,i,1) = "\n" then
    File$ = "#!" & Interpreter & mid(File$,i,len(File$))
  end if
  open FileName for output as 1
  binmode 1
  print#1,File$
  close 1
end if
stop
usage:
print """Usage:

cbbfc.bas program.bbf /usr/bin/scriba
"""
