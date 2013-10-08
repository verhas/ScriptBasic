import t.bas
import re.bas

on error goto FileOpenError
BuildNumber = Command()
if len(BuildNumber) > 0 then
  BuildNumber += 0
  goto BuildCalculated
endif

fn = 0
FileToOpen = "build.txt"
Doing = "read"
open FileToOpen for input as fn
line input#fn, BuildNumber
close fn
' the terminating \n does not disturb, because
' we convert it to number on the fly
BuildNumber = BuildNumber + 1

BuildCalculated:

print "The next build is: ",BuildNumber

fn = 0
FileToOpen = "build.txt"
Doing = "write"
open FileToOpen for output as fn
print#fn,BuildNumber

close fn

fn = 0
FileToOpen = "buildnum.h"
open FileToOpen for output as fn

print#fn,"""/* FILE: buildnum.h

This file was automatically created by newbuild.bas
Each time a new build is to be released this program
is ran and it increments the build number.

*/
#ifndef SCRIPTBASIC_BUILD
#define SCRIPTBASIC_BUILD """,BuildNumber,"""
#endif

"""
close fn

fn = 0
FileToOpen = "buildnum.c"
open FileToOpen for output as fn
print#fn,"""/* FILE: buildnum.c
HEADER: buildnum.h
This file was automatically created by newbuild.bas
Each time a new build is to be released this program
is ran and it increments the build number.
TO_HEADER:

#ifndef SCRIPTBASIC_BUILD
#define SCRIPTBASIC_BUILD """,BuildNumber,"""
#endif
*/
"""

close fn

fn = 0
FileToOpen = "pack.cmd"
Doing = "read"
s = t::LoadString(FileToOpen)
If re::m( s,"(.*)SET BUILD=([0-9]+)(.*)") then
  s = re::$(1) & "SET BUILD=" & BuildNumber & re::$(3)
  Doing = "write"
  t::SaveString FileToOpen,s
Else
  print "\npack.cmd does not contain build version\n"
Endif
stop

FileOpenError:
print
print "Can not open the file ",FileToOpen," for ",Doing,".\n"
print
print "Read/Write access is needed for the following files:\n"
print "build.txt\nbuildnum.h\nbuildnum.c\npack.cmd\n"
stop
