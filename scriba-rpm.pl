#! /usr/bin/perl

# this file is created by setup.pl when started with the option --install
require 'config.pl';

@DIRS = (
$CONFIGDIR,
$INCLUDE,
$SOURCE,
$MODULE,
);

open(F,">/usr/src/redhat/SPECS/scriba-rpm.spec") or die "cannot write the spec file";

open(BUILD,"build.txt") or die "Can not read build number";
$BUILD = <BUILD>;
close BUILD;
chomp $BUILD;

open(VERSION,"version.txt") or die "Can not read version";
$VERSION = <VERSION>;
close VERSION;
chomp $VERSION;

print F <<END;
#
# ScriptBasic RPM spec file
#
# Use this file to build the RPM package of the actual ScriptBasic build
# The command to build the RPM file is: make rpm
#
Summary: ScriptBasic interpreter
Name: scriba
Version: ${VERSION}b${BUILD}
Release: 1
Copyright: LGPL
Group: Applications/Interpreter
Source: http://scriptbasic.com/scriba-${VERSION}b${BUILD}-source.tar.gz
URL: http://scripbasic.com
Packager: Peter Verhas <peter\@verhas.com>

%description
ScriptBasic is a BASIC interpreter implementing a scripting
version of the language BASIC. It comes with a command line
version of the interpreter as well as a standalone http daemon
with ScriptBasic built in. ScriptBasic can handle external
modules written in C and can easily embedded delivering
a well defined and featureful API for C and C++ programmers.

%prep

%setup

%build
make all

%install
make install

%files
$BIN/scriba
$BIN/sbhttpd
$LIB/libscriba.a
$ETC/sbhttpd
END

# for each directory that is relevant and listed in the directory list
# in this file above (and was created into config.pl by setup.pl)
#
for $dir ( @DIRS ){
  opendir(DIR,$dir);
  @fl = readdir(DIR);
  closedir DIR;

  for $f (@fl){
    next if $f eq '..' or $f eq '.';
    print F "$dir/$f\n";
    }
  }

close F;

# now executing the tar file making script making script
`perl ./scriba-tar.pl`;
# now executing the tar file making script
`./mktar`;
# now copy the tar file to the location needed by RPM
`cp scriba-${VERSION}b${BUILD}-source.tar.gz /usr/src/redhat/SOURCES/scriba-${VERSION}b${BUILD}-source.tar.gz`;

print <<END;
It seems that the file
/usr/src/redhat/SPECS/scriba-rpm.spec

Run now

             rpm -ba /usr/src/redhat/SPECS/scriba-rpm.spec

END

