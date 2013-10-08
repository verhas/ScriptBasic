' FILE: modinst.bas
'
' This file is not a standalone BASIC program. This file is
' included by the 'extensions/<module>/install.sb' programs to
' perform the common part of the module installation.
'
' <module> is actually the name of the actual module.

module modinst

sub PrintCopied(a,b)
  print format("%s %*s --> %s\n",a , 30-len(a),"",b)
end sub

sub install(ModuleName)

on error goto BadConfiguration
IncludeDirectory = CONF("include")
ModulesDirectory = CONF("module")
ModuleExtension  = CONF("dll")
DocuDirectory    = CONF("docu")
on error goto null

if not FileExists(IncludeDirectory) then 
  on error goto CanNotCreateIncludeDirectory
  MkDir IncludeDirectory
  on error goto null
print """The directory to store the header files
did not exist. The installer created the directory:
""",IncludeDirectory,"\n"
end if

if IsFile(IncludeDirectory) then goto IncludeDirectoryIsFile

if FileExists(ModuleName & ".bas") then
  on error goto CanNotCopyHeaderFile
  FileCopy ModuleName & ".bas" , IncludeDirectory & ModuleName & ".bas"
  on error goto null
  PrintCopied ModuleName & ".bas" , IncludeDirectory & ModuleName
else
  print "The installer does not find the module header file: ", ModuleName , ".bas\n"
end if

if not FileExists(ModulesDirectory) then 
  on error goto CanNotCreateModulesDirectory
  MkDir ModulesDirectory
  on error goto null
print """The directory to store the module files
did not exist. The installer created the directory:
""",ModulesDirectory,"\n"
end if

if IsFile(ModulesDirectory) then goto ModulesDirectoryIsFile

ModuleFile = "../../bin/mod/dll/" & ModuleName & ModuleExtension
if FileExists(ModuleFile) then goto StartCopiingModuleDll

ModuleFile = "../../bin/vc7/mod/dll/" & ModuleName & ModuleExtension
if FileExists(ModuleFile) then goto StartCopiingModuleDll

ModuleFile = "../../bin/bcc/mod/dll/" & ModuleName & ModuleExtension
if FileExists(ModuleFile) then goto StartCopiingModuleDll

print "The installer does not find the module file: ", ModuleFile , "\n"

goto ModuleDllWasCopied

StartCopiingModuleDll:

on error goto CanNotCopyModuleFile
FileCopy ModuleFile , ModulesDirectory & ModuleName & ModuleExtension
on error goto null
PrintCopied ModuleFile,ModulesDirectory & ModuleName & ModuleExtension

ModuleDllWasCopied:

if not FileExists(DocuDirectory) then 
  on error goto CanNotCreateDocuDirectory
  MkDir DocuDirectory
  on error goto null
print """The directory to store module documentation files
did not exist. The installer created the directory:
""",DocuDirectory,"\n"
end if

if FileExists("../../bin/texi/mod_" & ModuleName & ".chm") then
  on error goto CanNotCopyDocuFile
  FileCopy "../../bin/texi/mod_" & ModuleName & ".chm",DocuDirectory & "mod_" & ModuleName & ".chm"
  on error goto null
  PrintCopied "../../bin/texi/mod_" & ModuleName & ".chm",DocuDirectory & "mod_" & ModuleName & ".chm"
endif

if FileExists("../../bin/texi/mod_" & ModuleName & ".html") then
  on error goto CanNotCopyDocuFile
  FileCopy "../../bin/texi/mod_" & ModuleName & ".html",DocuDirectory & "mod_" & ModuleName & ".html"
  on error goto null
  PrintCopied "../../bin/texi/mod_" & ModuleName & ".html",DocuDirectory & "mod_" & ModuleName & ".html"
endif

on error goto CanNotOpenDocuSourceDirectory
open directory "../../bin/texi/" pattern "mod_" & ModuleName & "_*" option SbCollectFullPath and SbSortByNone as 1
on error goto null

if not FileExists(DocuDirectory & "mod_" & ModuleName ) then 
  on error goto CanNotCreateModuDocuDirectory
  MkDir DocuDirectory & "mod_" & ModuleName
  on error goto null
print """The directory to store module documentation files
did not exist. The installer created the directory:
""",DocuDirectory & "mod_" & ModuleName,"\n"
end if

while not eod(1)
  ffn = NextFile(1)
  i = instr(ffn,"mod_")
  fn = mid(ffn,i)
  on error goto CanNotCopyDocuFile
  FileCopy ffn,DocuDir & "mod_" & ModuleName & "/" & fn
  on error goto null
  PrintCopied ffn,DocuDir & "mod_" & ModuleName & "/" & fn
wend

close directory 1

DocuWasCopied:

exit sub

CanNotCreateModuDocuDirectory:
print """The installer tried to create the directory
""", DocuDirectory & "mod_" & ModuleName , """
but it failed to do so."""
STOP

CanNotOpenDocuSourceDirectory:
print """The installer tried to list all the html files in
the directory ../../bin/texi but it could not open the directory
for listing. Installation is aborted.
"""
STOP

CanNotCopyDocuFile:
print """The installer can not copy module documentation file. The
reason for this can be that the file or an earlier version
is already there, but is read only or the directory
permission does not allow the file to be created.

The directory is: """,DocuDirectory,"""

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""
STOP

BadConfiguration:
print """ScriptBasic is not configured properly. 
This means that the current version of ScriptBasic as
installed does not ha a proper module and/or include
directory configured.

You should edit the file scriba.conf.lsp and recompile it
using scriba with option -k and run the installer again.

This configuration failure does not only affect this installer
but also the normal working of ScriptBasic therefore it is
recommended that you configure ScriptBasic properly.

The was aborted.
"""
STOP

CanNotCreateDocuDirectory:
print """The installer tried to create the directory

""",DocuDirectory,"""

but failed. This directory is needed to store the
module documentation files. Usually this directory is created
during ScriptBasic installation. This did not happen
and therefore this installer tried to create the
directory, but failed.

The reason for the failure can be that the upper
directories' access control prevent the creation of the
new directory.

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""
STOP

CanNotCreateIncludeDirectory:
print """The installer tried to create the directory

""",IncludeDirectory,"""

but failed. This directory is needed to store the
module header files. Usually this directory is created
during ScriptBasic installation. This did not happen
and therefore this installer tried to create the
directory, but failed.

The reason for the failure can be that the upper
directories' access control prevent the creation of the
new directory.

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""
STOP

IncludeDirectoryIsFile:
print """The installer needs a directory named

""",IncludeDirectory,"""

Currently there is a plain file (not directory) with that
name and therefore the installer can not create the directory.
Please check what this file is, delet it, move it to another
location or if none of these work reconfigure ScriptBasic
to store the module header files in a different location.

The module installation was aborted.
"""
STOP

CanNotCopyHeaderFile:
print """The installer can not copy the header file. The
reason for this can be that the file or an earlier version
is already there, but is read only or the directory
permission does not allow the file to be created.

The directory is: """,IncludeDirectory,"""

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""

STOP

CanNotCreateModulesDirectory:
print """The installer tried to create the directory

""",ModulesDirectory,"""

but failed. This directory is needed to store the
module files. Usually this directory is created
during ScriptBasic installation. This did not happen
and therefore this installer tried to create the
directory, but failed.

The reason for the failure can be that the upper
directories' access control prevent the creation of the
new directory.

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""
STOP

ModulesDirectoryIsFile:
print """The installer needs a directory named

""",ModulesDirectory,"""

Currently there is a plain file (not directory) with that
name and therefore the installer can not create the directory.
Please check what this file is, delet it, move it to another
location or if none of these work reconfigure ScriptBasic
to store the module header files in a different location.

The module installation was aborted.
"""
STOP

CanNotCopyModuleFile:
print """The installer can not copy the module file. The
reason for this can be that the file or an earlier version
is already there, but is read only or the directory
permission does not allow the file to be created.

The directory is: """,ModulesDirectory,"""

Please check that the directory and the parent directories
from the root directory have the appropriate permission and
also that you try to run this installer program logged in
with sufficient privileges. This is usually root on UNIX
or Administrator under Windows NT.


The module installation was aborted.
"""

STOP

end sub

end module
