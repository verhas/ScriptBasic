In case you want to compile this module on Windows NT or Windows 2000 you 
have to install MySQL in development version. This requires to select
custom setup during setup and select the client libraries to be installed.

However the MySQL setup installs the include files and the lib and dll files
into the MySQL directories. This is nothing to blame on MySQL setup as it may
not know what development system you use.

For ScriptBasic currently we use Visual C++ v6.0. To be able to compile the
file mysqlinterf.c you have to manually make some copies.

copy c:\mysql\include\*.* "c:\Program Files\Microsoft Visual Studio\VC98\Include"
copy C:\mysql\lib\opt\*.lib "c:\Program Files\Microsoft Visual Studio\VC98\Lib"
copy C:\mysql\lib\opt\*.dll c:\WINNT\system32

The actual directories may be different if you have different location for
the Visual C++ development environment or in case you have installed the
NT operating system in a different directory.

Note that the libMYSQL.dll should be copied to the system32 directory even if
you just want to use the module but not compile it from source.

I have tested this setup on Windows 2000 Professional.

December 23, 2000.
Peter Verhas
