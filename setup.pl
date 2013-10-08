#! /usr/bin/perl

$mask = 0700; # owner has read, write and list others have nothing,
              # you usually run it as root and who else has anything
              # to do to the compilation intermediate files anyway?

#
# This script has to be executed from the ScriptBasic source directory
# if this is the directory then lmt_none.def should be there
#
if( ! -e 'lmt_none.def' ){
  print <<END;
This is the setup program of ScriptBasic. This program helps to compile
ScriptBasic modules and ScriptBasic itself. The program should run with
the current working directory being the 'source' directory of ScriptBasic.

Now this is not the case or the ScriptBasic source installation is erroneous.
'cd' into the right directory or check your download!
END
  die "\n";
  }

BEGIN {
  $OpenLogFile = 1;
  }
sub logit {
  my $msg = shift;
  my $t = time();

  if( $OpenLogFile ){
    if( $OPT{'--log-append'} ){
      open(LOGF,">>setup.log") or die "Can not open log file.";
      }else{
      open(LOGF,">setup.log") or die "Can not open log file.";
      }
    $OpenLogFile = 0;
    }
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =  localtime($t);
  print LOGF sprintf("%04s.%02s.%02s %02s:%02s:%02s %s\n",$year+1900,$mon+1,$mday,$hour,$min,$sec,$msg);
  }

END {
  close(LOGF) unless $OpenLogFile;
  }

#
# Process the command line options
#
# Read the command line first
# Check if there is any file to load from previous options.
# If there is then load options from file which are not specified on the command line
# Apply the specified command line options to overwrite the ones loaded from file.
#
sub negopt {
  my $opt = shift;
  if( $opt =~ /^\-\-no\-(.*)$/ ){ return "--$1"; }else{ $opt =~ s/^\-\-(.*)/--no-$1/; }
  $opt;
  }

%OPT = ();
while( $opt = shift ){
  ($optn,$optv) = split /=/ , $opt;
  # check the negative option
  die "Can not use option $optn and $nopt at a time" if defined $OPT{ &negopt($optn) };
  $optv = 1 if length($optv) == 0;
  $OPT{$optn} = $optv;
  $LastOptionWasHelp = $optn eq '--help';
  }

# this option is used only on command line not loaded from file
if( defined $OPT{'--help'} ){
  &help;
  exit;
  }

# check the options not to allow mistyped option and such undesired result
for( keys %OPT ){
  next if $_ eq '--help';
  next if defined $HELP{$_} || $HELP{ &negopt($_) };
  die "Invalid option: $_\n";
  }

# if --load='filename' was specified then load the file
if( $OPT{'--load'} ){
  if( $OPT{'--load'} eq '1' ){
    $OPT{'--load'} = 'configure.save';
    }
  open(F,$OPT{'--load'}) or die "Can not open configuration file " . $OPT{'--load'};
  while( <F> ){
    chomp;
    # skip comment lines
    next if /^\s*\#/;
    # skip space lines
    next if /^\s*$/;

    ($optn,$optv) = split /=/;
    # load the option only if it is not defined on the command line
    next if defined $OPT{$optn};
    next if defined $OPT{ &negopt($optn) };
    $OPT{$optn} = $optv;
    }
  close F;
  }

# determine if this is UNIX or NT or MacOS and
# also determine the source root directory
# Note: for macintosh it is assumed that the
#       development OS is OS X even though the
#       target may be OS 9 or below

use Config;

die 'Can only use one of --nt, --unix, --darwin, --macos at the same time'
     if ($OPT{'--nt'} ne '') + ($OPT{'--unix'} ne '') + ($OPT{'--darwin'} ne '') + 
        ($OPT{'--macos'} ne '') > 1;

if( $OPT{'--nt'} ){
  $cwd = `CD`;
  $ds = '\\';
  }
else{
  $cwd = `pwd`;
  $ds = '/';
  }
  
if( $OPT{'--nt'} ){
  $unix = "";
  }
elsif( $OPT{'--unix'} ){
  $unix = "unix";
  }
elsif( $OPT{'--darwin'} ){
  $unix = "darwin";
  }
elsif( $OPT{'--macos'} ){
  $unix = "macos";
  }
else{
  # if command line option does not specify the operating system
  $cwd = '';
  $cwd = eval{ `pwd`; }; # try pwd first
  if( 0 == length($cwd) ){
    # if pwd did not work then it has to be NT
    $cwd = eval { `CD`; };
    $unix = ""; # this seems to be NT
    $ds = '\\';
    die "Out of luck determining the OS, nor 'pwd' neither 'CD' worked." if 0 == length($cwd);
    }else{
    $unix = "unix";# seems to be UNIX, but let's try if this is NT with cygwin installed
    # use capital letters no to get into the home dir when under UNIX
    $ntcwd = eval{ `CD`; };
    if( substr($ntcwd,2,1) eq ':' ){
      # hoho ho you have cygwin installed, but cd returned
      $cwd = $ntcwd;
      $unix = "";
      }else{
      # OK, so we really are unix, but do we have any special flavours?
      if( $Config{osname} == 'darwin' ){ # If we are using the darwin build of perl
        if( $ENV{CWINSTALL} ){
          $unix = "macos";
          }else{
          $unix = "darwin";
          };
        }
      }
    }
  }
chomp $cwd;
$cwd .= $ds;

print "This is ", $unix ? $unix : 'NT' , " cwd=$cwd\n";
my $build = $unix ? $unix : "nt";

#
# Check that esd.pm is installed for Jamal
#
$esdlocation = undef;
for (@INC){
  next if /\./;
  if( -e "$_/jamal/esd.pm" ){
    $esdlocation = "$_/jamal/esd.pm";
    }
  }
if( ! defined $esdlocation ){
  print <<END;
The jamal module esd.pm is not installed.
You have to install it before running any
kind of ScriptBasic source compilation.
The module is needed to compile source
documentation. This is a MUST.

To install esd.pm become root on UNIX,
or just have write privilege for the
directories where the Perl modules are
(this is the 'source' directory of ScriptBasic)
and run

perl jamal.pl -i

interactively answering the program questions.
END
  exit 1;
  }

# this option is used to create the ZIP file creator cmd file
if( defined $OPT{'--zip'} ){
  my $InstallDirectory = $OPT{'--directory'};
  $InstallDirectory = "T:\\ScriptBasic\\" unless defined $InstallDirectory;
  print `perl scriba-zip.pl $InstallDirectory`;
  print <<END;
Now I have created 'mkzip.cmd' Run it to create the ZIP files.
END
  exit;
  }

# this option is used to create the ZIP file creator cmd file
if( defined $OPT{'--self-check'} ){
  &self_check;
  exit;
  }

#
# if a module is specified then configure the module
#
if( $OPT{'--module'} ){
  &configure_module($OPT{'--module'});
  &save_options;
  exit;
  }

#
# '--check' check the compilation result seeking
# for each output file and print out a brief result
# to save time and mistakes overlooking an
# error message in the [n]make output
#
if( $OPT{'--check'} ){
  &check_compilation_result;
  &save_options;
  exit;
  }

#
# '--install' starts to install the compiled
# ScriptBasic, modules as well as core (core first)
# This can only be used under UNIX.
# On Windows install.sb works separately and is started by this
# tool.
if( $OPT{'--install'} ){
  if( $unix ){
    &install_unix_scriba();
    print <<END;
ScriptBasic was not actually installed, but I created the file
'./install.sh' that you can run as root to install ScriptBasic
with the configured installation option.

I also created the text configuration file 'scriba.conf.lsp'
that reflects the installation locations and contains a
'maxlevel' value that I measured to be safe on this machine.
'install.sh' compiles this configuration for ScriptBasic and
also tries to install all modules that were compiled successfully.
END
    &save_options;
    exit;
    }else{
    my $InstallDirectory;
    if( $OPT{'--directory'} ){
      $InstallDirectory = $OPT{'--directory'};
      }else{
      $InstallDirectory = 'T:/ScriptBasic/';
      }
    if( -e 'bin/vc7/exe/scriba.exe' ){
      print `bin\\vc7\\exe\\scriba.exe install.sb $InstallDirectory vc7`;
      &save_options;
      exit;
      }
    if( -e 'bin/bcc/exe/scriba.exe' ){
      print `bin\\bcc\\exe\\scriba.exe install.sb $InstallDirectory bcc`;
      &save_options;
      exit;
      }
    print <<END;
This command is supposed to install ScriptBasic after it was compiled from
the sources using the tool 'setup'. It is not meant to install ScriptBasic
using precompiled packages. However I can not find the appropriate compiled
files created either by the VC or BCC compiler.

You have to compile the source first before trying to install
the package from compilation.
END
    &save_options;
    exit;
    }
  }

#
# find out what file make_common.jim includes
#
$configure_jim = '';
open(F,"make_common.jim") or die "Can not open 'make_common.jim'";
while( <F> ){
  if( /{#include\s+(.*)}\s*$/ ){
    $configure_jim = $1;
    last;
    }
  }
close F;
print "No file is included by make_common.jim. Is this setup correct?" if $configure_jim eq '';

#
# create the file $configure_jim
# that holds the jamal macro that defines the source root directory
# and other directories
#
if( $configure_jim ){
  print "creating $configure_jim\n";
  open(F,">$configure_jim") or die "Can not create the file $configure_jim";
  print F "{\#define SourceRoot=$cwd}\n";
  if( ! $unix ){
    if( $OPT{'--no-search-bcc'} ){# default is to search the Borland C compiler installation
      #
      # Do not search the Borland C compiler, but rather take it from the configuration
      # given on the command line or from the saved config if defined
      #
      print F "{#define BorlandIncludeDirectory=",$OPT{'--BorlandIncludeDirectory'},"}\n"
        if length($OPT{'--BorlandIncludeDirectory'}) > 0;
      print F "{#define BorlandLibDirectory=",$OPT{'--BorlandLibDirectory'},"}\n"
        if length($OPT{'--BorlandLibDirectory'}) > 0;
      print F "{#define bcc32=",$OPT{'--bcc32'},"}\n"
        if length($OPT{'--bcc32'}) > 0;
      }else{
      #
      # search the Borland C compiler along the PATH
      #
      print "Trying to find out where the Borland compiler is installed (if installed)\n";
      for $dir ( split(/;/, $ENV{'PATH'}) ){
        if( -e "$dir\\bcc32.exe" ){
          print "Borland C compiler is installed in $dir\n";
          # chop off the 'bin' where Borland puts the executable
          $dir =~ s/\\bin$//i;
          print F "{#define BorlandIncludeDirectory=\"$dir\\Include\"}\n";
          print F "{#define BorlandLibDirectory=\"$dir\\Lib\"}\n";
          # altough this is in the PATH let's define it here
          print F "{#define bcc32=\"$dir\\bin\\bcc32.exe\"}\n";
          if( $OPT{'--save-bcc-config'} ){
            $OPT{'--bcc32'} ="$dir\\bin\\bcc32.exe";
            $OPT{'--BorlandIncludeDirectory'} = "$dir\\Include";
            $OPT{'--BorlandLibDirectory'} = "$dir\\Lib";
            # if there is a need to save the BCC config then obviously there is a need to save the config file
            $OPT{'--save'} = 1 unless defined $OPT{'--save'};
            }
          last;
          }
        }
      }
    }
  close F;
  }
# create the directories that are needed
# during compilation

if( ! $OPT{'--no-mkdir'} ){# default is to create the subdirs

  print "creating subdirectories for compilation output files\n";

    mkdir 'bin'             ,$mask;

  # the TEXI directory is independent of the C compiler
    mkdir 'bin/texi'        ,$mask;


  if( $unix ){

    # under UNIX we do not generate C compiler specific subdir
    # use only one compiler under UNIX, that is gcc
    mkdir 'bin/mod'         ,$mask;
    mkdir 'bin/mod/obj'     ,$mask;
    mkdir 'bin/mod/lib'     ,$mask;
    mkdir 'bin/mod/dll'     ,$mask;
    mkdir 'bin/var'         ,$mask;
    mkdir 'bin/include'     ,$mask;
    mkdir 'bin/obj'         ,$mask;
    mkdir 'bin/lib'         ,$mask;
    mkdir 'bin/make'        ,$mask;
    mkdir 'bin/cmd'         ,$mask;
    mkdir 'bin/exe'         ,$mask;

    }else{

    # directories to be used by the Borland C compiler
    mkdir 'bin/bcc'         ,$mask;
    mkdir 'bin/bcc/mod'     ,$mask;
    mkdir 'bin/bcc/mod/obj' ,$mask;
    mkdir 'bin/bcc/mod/lib' ,$mask;
    mkdir 'bin/bcc/mod/dll' ,$mask;
    mkdir 'bin/bcc/var'     ,$mask;
    mkdir 'bin/bcc/include' ,$mask;
    mkdir 'bin/bcc/obj'     ,$mask;
    mkdir 'bin/bcc/lib'     ,$mask;
    mkdir 'bin/bcc/make'    ,$mask;
    mkdir 'bin/bcc/cmd'     ,$mask;
    mkdir 'bin/bcc/exe'     ,$mask;

    # directories used by the Visual C++ v7.0 compiler
    mkdir 'bin/vc7'         ,$mask;
    mkdir 'bin/vc7/mod'     ,$mask;
    mkdir 'bin/vc7/mod/obj' ,$mask;
    mkdir 'bin/vc7/mod/lib' ,$mask;
    mkdir 'bin/vc7/mod/dll' ,$mask;
    mkdir 'bin/vc7/var'     ,$mask;
    mkdir 'bin/vc7/include' ,$mask;
    mkdir 'bin/vc7/obj'     ,$mask;
    mkdir 'bin/vc7/lib'     ,$mask;
    mkdir 'bin/vc7/make'    ,$mask;
    mkdir 'bin/vc7/cmd'     ,$mask;
    mkdir 'bin/vc7/exe'     ,$mask;
    }
  }

if( ! $OPT{'--no-makefile'} ){# default is to create the makefiles
  print "compiling Makefile using the Jamal preprocessor\n";
  if( $unix eq "unix" ){
    &logit("perl jamal.pl -m make_gcc.jim makefile.jam bin/make/Makefile");
    `perl jamal.pl -m make_gcc.jim makefile.jam bin/make/Makefile`;
    }
  elsif( $unix eq "darwin" ){
    &logit("perl jamal.pl -m make_darwin.jim makefile.jam bin/make/Makefile");
    `perl jamal.pl -m make_darwin.jim makefile.jam bin/make/Makefile`;
    }
  elsif( $unix eq "macos" ){
    &logit("perl jamal.pl -m make_macos.jim makefile.jam bin/make/Makefile");
    `perl jamal.pl -m make_macos.jim makefile.jam bin/make/Makefile`;
    }else{
    # compile the makefiles from source (note that directory.jim is ready already)
    &logit("perl jamal.pl -m make_vc7.jim makefile.jam bin\\vc7\\make\\makefile");
    `perl jamal.pl -m make_vc7.jim makefile.jam bin\\vc7\\make\\makefile`;
    &logit("perl jamal.pl -m make_bcc.jim makefile.jam bin\\bcc\\make\\makefile");
    `perl jamal.pl -m make_bcc.jim makefile.jam bin\\bcc\\make\\makefile`;
    # compile the packing cmd file. It needs Jamal to have the version and build correctly
    #&logit("perl jamal.pl pack.cmd.jam pack.cmd");
    #`perl jamal.pl pack.cmd.jam pack.cmd`
    }
  }

if( ! $OPT{'--no-syntaxer'} ){# default is to run the syntaxer.pl
  # run pre-compile tasks
  print "running syntaxer.pl to generate the syntax defintion C language tables from syntax.def\n";
  &logit("perl syntaxer.pl");
  `perl syntaxer.pl`;
  }

if( ! $OPT{'--no-generrh'} ){# default is to run the generrh.pl
  print "running generrh.pl to generate the error messages from errors.def\n";
  `perl generrh.pl`;
  }

if( ! $OPT{'--no-lmt_make'} ){#default is to run the lmt_make.pl for all lmt_*.def files
  print "running lmt_make.pl for all lmt*.def files\n";
  opendir(DIR,".") or die "Can not open the current directory.";
  my @lmtfiles = grep /^lmt_.*\.def$/ , readdir(DIR);
  closedir DIR;
  for( @lmtfiles ){
    `perl lmt_make.pl $_`;
    }
  }

if( ! $OPT{'--no-headerer'} ){# default is to run headerer for all .c files recursively
  #
  # gather all xxx.c files recursively
  #
  print "scanning all subdirectories to find all C source files\n";
  opendir(D,'.');
  @files = readdir(D);
  closedir(D);
  @dirs  = grep( (-d "$_") && /^[^.]/ ,@files);

  while( $#dirs > -1 ){
    $cdir = pop @dirs;

    opendir(D,"$cdir");
    @f = readdir(D);
    closedir(D);

    @d  = grep( (-d "$cdir/$_") && /^[^.]/,@f);
    for( @d ){
     push @dirs, "$cdir/$_";
     }
    for( @f ){
     push @files, "$cdir/$_";
     }
    }
  print "there are ",$#files+1," files in the source tree\n";

  @files = grep /\.c$/i , @files;
  print "there are ",$#files+1," C source files in the source tree\n";

  #
  # run headerer.pl for all xxx.c files
  #
  for( @files ){
    `perl headerer.pl $_`;
    }
  }

@modules_set_up = ();
if( ! $OPT{'--no-modules'} ){# configure all modules that are in the system
  opendir(DIR,"extensions");
  @extensions = readdir(DIR);
  closedir DIR;

  for $extension ( @extensions ){
    if( -d "extensions/$extension" && 
        $extension ne 'CVS'        &&
        $extension ne '.'          &&
        $extension ne '..'            ){
      print "configuring module $extension\n";
      my $nt = "--$unix";
      $nt = '--nt' unless $unix;
      my $bcc = '--compile=vc7';
      $bcc = '--compile=bcc' if $OPT{'--compile'} eq 'bcc';
      print `perl setup.pl --module=$extension $nt $bcc --log-append`;
      if( ( ( $unix eq 'unix')   && ! -e "extensions/$extension/notunix.txt"   ) ||
          ( ( $unix eq 'darwin') && ! -e "extensions/$extension/notdarwin.txt" ) ||
          ( ( $unix eq 'macos')  && ! -e "extensions/$extension/notmacos.txt"  ) ||
          ( ( !$unix )           && ! -e "extensions/$extension/notnt.txt"     ) ){
        push @modules_set_up , $extension;
        }
      }
    }
  }

print "making compile command file(s)\n";
if( $unix ){
  open(F,">compile") or die "Can not write file 'compile'";
  print F "#! /bin/sh\n";
  print F "# this shell file was automatically generated by setup.pl\n";
  print F "make -f bin/make/Makefile\n";
  print F "cd extensions\n";
  for ( @modules_set_up ){
    print F "cd $_\n";
    print F "make -f makefile\n";
    print F "cd ..\n";
    }
  print F "cd ..\n";
  close F;
  chmod(0700,"compile");
  if( ! $OPT{'--no-compile'} ){
    print "starting compilation\n";
    `./compile`;
    }
  }else{
  print "Creating compile_vc7.cmd\n";
  open(F,">compile_vc7.cmd") or die "Can not write compile_vc7.cmd";
  print F "REM this command file was automatically generated by setup.pl\n";
  print F "nmake -f bin\\vc7\\make\\makefile\n";
  print F "cd extensions\n";
  for ( @modules_set_up ){
    print F "cd $_\n";
    print F "nmake -f makefile.vc7\n";
    print F "cd ..\n";
    }
  print F "cd ..\n";
  close F;

  print "Creating compile_bcc.cmd\n";
  open(F,">compile_bcc.cmd") or die "Can not write compile_bcc.cmd";
  print F "REM this command file was automatically generated by setup.pl\n";
  print F "make -f bin\\bcc\\make\\makefile\n";
  print F "cd extensions\n";
  for ( @modules_set_up ){
    print F "cd $_\n";
    print F "make -f makefile.bcc\n";
    print F "cd ..\n";
    }
  print F "cd ..\n";
  close F;
  # the default is to start the compilation and the default compiler is vc7
  if( ! $OPT{'--no-compile'} ){
    print "starting compilation\n";
    if( $OPT{'--compile'} eq 'bcc' ){
      print `cmd /c compile_bcc.cmd`;
      }else{
      print `cmd /c compile_vc7.cmd`;
      }
    }
  }

&save_options;

if( $OPT{'--no-compile'} ){
  if( $unix ){
    print "Setup was finished. Now you can run './compile'\n";
    }else{
    print "Setup was finished. Now you can run 'compile_vc7' or 'compile_bcc'\n";
    }

print <<END;

The core ScriptBasic module should compile without any problem on
Windows NT and Linux, and no issue is likely under other UNIX
operating systems.

Please note that some of the modules need extra libraries and
header files installed on your system. These may not compile
seamlessly if the libraries or header files are missing.
Nevertheless such an error should not prevent the compilation
to compile the other modules.

The modules that need extra libraries:

END

  opendir(DIR,"extensions") or die "Can not open the directory 'extensions'";
  @extensions = readdir(DIR);
  closedir DIR;

  for $extension (@extensions){
    if( -d "extensions/$extension" && $extension ne '.' && $extension ne '..' && $extension ne 'CVS' ){
      if( -e "extensions/$extension/libraries.jim" ){
        print "Module $extension", ' 'x(8-length($extension))," needs ";
        open(F,"extensions/$extension/libraries.jim") or die "Can not read 'extensions/$extension/libraries.jim'";
        my $q = <F>;
        chomp $q;
        $q =~ s/^.*\=(.*)\}/$1/;
        if( 0 == length($q) ){
          $q = <F>;
          chomp $q;
          $q =~ s/^.*\=(.*)\}/$1/;
          }
        close F;
        print "$q\n";
        }
      }
    }
  }else{# if --compile was done
  &check_compilation_result;
  }

exit 0;

sub check_compilation_result {
  my ($dll,$lib,$bas);

  if( $unix ){
    print "scriba executable ", -e "bin/exe/scriba" ? "OK  " : "FAIL","\n";
    print "sbhttpd executable ", -e "bin/exe/sbhttpd" ? "OK  " : "FAIL" , "\n" unless $unix eq 'macos';
    print "libscriba library ", -e "bin/lib/libscriba.a" ? "OK  " : "FAIL", "\n";

    #
    # checking the modules
    #
    opendir(DIR,'extensions') or die "Can not list directory 'extensions'";
    @modules = readdir(DIR);
    close DIR;
    open(OUT,">modinst.sh") or die "Can not write modinst.sh"; # create a command file that will install the modules
    while( $module = shift @modules ){
      next if ! -d "extensions/$module";
      next if $module eq '.';
      next if $module eq '..';
      next if $module eq 'CVS';
      if( $unix eq "darwin" ){
        next if -e "extensions/$module/notdarwin.txt";
        print "MODULE $module: ", ' 'x(8-length($module));
        print "dll ", ($dll = -e "bin/mod/dll/$module.dylib") ? "OK   " : "FAIL ";
        }
      elsif( $unix eq "macos" ){
        next if -e "extensions/$module/notmacos.txt";
        print "MODULE $module: ", ' 'x(8-length($module));
        print "dll ", ($dll = -e "bin/mod/dll/$module") ? "OK   " : "FAIL ";
        }else{
        next if -e "extensions/$module/notunix.txt";
        print "MODULE $module: ", ' 'x(8-length($module));
        print "dll ", ($dll = -e "bin/mod/dll/$module.so") ? "OK   " : "FAIL ";
        }
      print "lib ", ($lib = -e "bin/mod/lib/$module.a") ? "OK   " : "FAIL ";
      print "bas ", ($bas = -e "extensions/$module/$module.bas")  ? "OK  " : "FAIL";
      print "\n";      
      print OUT "cd $module\nscriba -n install.sb\ncd ..\n" if $dll && $lib && $bas;
      }
    print OUT "cd ..\n";
    close OUT;
    chmod 0700,"modinst.sh";

    }else{

    # WINDOWS NT --check

    my $compiler = $OPT{'--compile'} eq 'bcc' ? 'bcc' : 'vc7' ;

    print "scriba.exe ", -e "bin/$compiler/exe/scriba.exe" ? "OK  " : "FAIL" , "\n";
    print "sbhttpd.exe ", -e "bin/$compiler/exe/sbhttpd.exe" ? "OK  " : "FAIL" , "\n";
    print "libscriba.lib ", -e "bin/$compiler/lib/libscriba.lib" ? "OK  " : "FAIL", "\n";

    #
    # checking the modules
    #
    opendir(DIR,'extensions') or die "Can not list directory 'extensions'";
    @modules = readdir(DIR);
    close DIR;
    open(OUT,">modinst.cmd") or die "Can not write modinst.cmd"; # create a command file that will install the modules
    print OUT "cd extensions\n";
    my $errorFlag = 0;
    while( $module = shift @modules ){
      next if ! -d "extensions/$module";
      next if $module eq '.';
      next if $module eq '..';
      next if $module eq 'CVS';
      # do not check a module, which is not to be compiled under win
      next if -e "extensions/$module/notnt.txt";
      print "MODULE $module: ", ' 'x(8-length($module));
      print "dll ", ($dll = -e "bin/$compiler/mod/dll/$module.dll") ? "OK   " : "FAIL ";
      print "lib ", ($lib = -e "bin/$compiler/mod/lib/$module.lib") ? "OK   " : "FAIL ";
      print "bas ", ($bas = -e "extensions/$module/$module.bas")  ? "OK  " : "FAIL";
      print "\n";
      print OUT "cd $module\nscriba -n install.sb\ncd ..\n" if $dll && $lib && $bas;
      $errorFlag = 1 if ! ($dll && $lib && $bas);
      }
    print OUT "cd ..\n";
    close OUT;
    print "\n";
    print "Some of the modules were not compiled. They may depend on extra\n";
    print "libraries that have to be compiled and installed before the module.\n";
    print "Check the libraries that the modules need!\n";
    }

  }

sub save_options {
  if( $OPT{'--save'} ){
    if( $OPT{'--save'} eq '1' ){
      $OPT{'--save'} = 'configure.save';
      }
    print "Saving configuration info to ",$OPT{'--save'},"\n";
    open(F,'>'.$OPT{'--save'}) or die "Can not save configuration to file ".$OPT{'--save'};
    for( sort keys %OPT ){
      next if $_ eq '--save'; # this is obviously not saved
      next if $_ eq '--load'; # this is obviously not saved
      print F $_,'=',$OPT{$_},"\n";
      }
    close F;
    }
  }

#
# CONFIGURE A MODULE OR CREATE THE SKELETON FOR A NEW MODULE
#
sub configure_module {
  my $module = shift;


  # create the directory if it does not exists
  mkdir "extensions", $mask;
  mkdir "extensions/$module",$mask if ! -e "extensions/$module";

  # create the module files that are nonexistent
  # this is also useful when starting a new module
  # if some of the module files already exist then
  # it does NOT change the already existing file
  &create_module_skeleton($module);

  # create the file libraries.jim
  # if it is needed
  my $libopt = '';
  open(F,"extensions/$module/interface.c") or die "Can not open extensions/$module/interface.c";
  while( <F> ){
    chomp;
    
    if( $unix eq 'unix' ){
      if( /^UXLIBS:\s*(.*)$/ ){
        $libopt = $1 || ' ';
        last;
        }
      }
    elsif( $unix eq 'darwin' ){
      if( /^DWLIBS:\s*(.*)$/ ){
        $libopt = $1 || ' ';
        last;
        }
      }
    elsif( $unix eq 'macos' ){
      if( /^MCLIBS:\s*(.*)$/ ){
        $libopt = $1 || ' ';
        last;
        }
      }
    else {
      if( /^NTLIBS:\s*(.*)$/ ){
        $libopt = $1 || ' ';
        last;
        }
      }
    }
    
  if( $libopt ){
    open(OUT,">extensions/$module/libraries.jim") or die "there is a need, but I can not create extensions/$module/libraries.jim";
    my $NTlib = $libopt;
    $NTlib = '' if $unix;
    print OUT "{#define LdNeededLibraryFiles=$libopt}\n";
    print OUT "{#define LibNeededLibraryFiles=$NTlib}\n";
    close OUT;
    }else{
    open(OUT,">extensions/$module/not$build.txt") or die "Can not write the module file 'not$build.txt'";
    print OUT "This file helps setup.pl to know that this module cannot be compiled under $build.\n";
    close OUT;
    print "The module $module is not for $build.\n";
  }
  
  close F;

  opendir(DIR,"extensions/$module") or die "Can not open module directory extensions/$module";
  my @cfiles = grep /\.c$/ , readdir(DIR);
  closedir DIR;

  print "executing headerer for the C files\n";
  for my $f ( @cfiles ){
    print " extracting header from $f\n";
    `perl headerer.pl extensions/$module/$f`;
    }

  print "creating the module object directory\n";
  if( $unix ){
    mkdir "bin" , $mask;
    mkdir "bin/mod", $mask;
    mkdir "bin/mod/obj", $mask;
    mkdir "bin/mod/obj/$module", $mask;
    }else{
    mkdir "bin", $mask;
    mkdir "bin/vc7", $mask;
    mkdir "bin/vc7/mod", $mask;
    mkdir "bin/vc7/mod/obj", $mask;
    mkdir "bin/vc7/mod/obj/$module", $mask;
    mkdir "bin/bcc", $mask;
    mkdir "bin/bcc/mod", $mask;
    mkdir "bin/bcc/mod/obj", $mask;
    mkdir "bin/bcc/mod/obj/$module", $mask;
    }

  # if there is no makefile.jam for the module then
  # create a typical one
  # this is a bit more complex than just copying a skeleton file
  if( ! -e "extensions/$module/makefile.jam" ){
    print "Creating typical makefile.jam for the module\n";
    &create_makefile_jam($module);
    }

  # run the Jamal preprocessor to generate the make files for the module
  # if the notXXX.txt files are there then there is
  # no reason to run the preprocessing
  if( $unix eq "unix" ){
    print "Processing jamal files creating makefile\n";
    `perl jamal.pl -m make_gcc.jim extensions/$module/makefile.jam extensions/$module/makefile`
       unless -e "extensions/$module/notunix.txt";
    }
  elsif( $unix eq "darwin" ){
    print "Processing jamal files creating makefile\n";
    `perl jamal.pl -m make_darwin.jim extensions/$module/makefile.jam extensions/$module/makefile`
       unless -e "extensions/$module/notdarwin.txt";
    }
  elsif( $unix eq "macos" ){
    print "Processing jamal files creating makefile\n";
    `perl jamal.pl -m make_macos.jim extensions/$module/makefile.jam extensions/$module/makefile`
       unless -e "extensions/$module/notmacos.txt";
    }else{
    print "Processing jamal files creating makefile.vc7\n";
    `perl jamal.pl -m make_vc7.jim extensions/$module/makefile.jam extensions/$module/makefile.vc7`
       unless -e "extensions/$module/notnt.txt";
    print "Processing jamal files creating makefile.bcc\n";
    `perl jamal.pl -m make_bcc.jim extensions/$module/makefile.jam extensions/$module/makefile.bcc`
       unless -e "extensions/$module/notnt.txt";
    }

  # start the compilation of the module
  if( ! $OPT{'--no-compile'} ){# default is to compile the module
    if( $unix ){
      open(OUT,">modmake.sh") or die "Can not write 'modmake.sh'";
      print OUT "cd extensions/$module\nmake\ncd ../..\n";
      close OUT;
      if( ( $unix eq 'darwin' && ! -e "extensions/$module/notdarwin.txt") ||
          ( $unix eq 'macos'  && ! -e "extensions/$module/notmacos.txt") ||
          ( $unix eq 'unix'  && ! -e "extensions/$module/notunix.txt") ){
        `/bin/sh modmake.sh`;
        my $ModLibFile = "${cwd}bin/mod/lib/$module.a";
        my $ModDllFile = "${cwd}bin/mod/dll/$module.so";
        if( $unix eq 'darwin' ){
          $ModDllFile = "${cwd}bin/mod/dll/$module.dylib";
          }
        elsif( $unix eq 'macos' ){
          $ModDllFile = "${cwd}bin/mod/dll/$module";
          }
        if( -e $ModLibFile &&
            -e $ModDllFile ){
          print "Module was compiled fine\n";
          }else{
          if( (!-e $ModLibFile) && (!-e $ModDllFile) ){
            print "ERROR: The module did not compile\n";
            }else{
            print "ERROR: Static library was not created for some reason.\n" unless -e $ModLibFile;
            print "ERROR: SO was not created for some reason.\n" unless -e $ModDllFile;
            }
          }
        }
      }else{
      open(OUT,">modmake.cmd") or die "Can not write modmake.cmd";
      if( $OPT{'--compile'} eq 'bcc' ){
        print OUT "cd extensions\\$module\nmake -f makefile.bcc\ncd ..\\..\n";
        }else{
        print OUT "cd extensions\\$module\nnmake -f makefile.vc7\ncd ..\\..\n";
        }
      close OUT;
      if( -e "extensions/$module/notnt.txt" ){
        print "The module is for UNIX operating system only, can not be compiled under NT\n";
        }else{
        print "Compiling the module executing modmake.cmd\n";
        `cmd /c modmake.cmd`;
        if( $OPT{'--compile'} eq 'bcc' ){
          my $ModLibFile = "${cwd}bin\\bcc\\mod\\lib\\$module.lib";
          my $ModDllFile = "${cwd}bin\\bcc\\mod\\dll\\$module.dll";
          if( -e $ModLibFile &&
              -e $ModDllFile ){
            print "Module was compiled fine\n";
            }else{
            if( (!-e $ModLibFile) && (!-e $ModDllFile) ){
              print "ERROR: The module did not compile\n";
              }else{
              print "ERROR: Static library was not created for some reason.\n" unless -e $ModLibFile;
              print "ERROR: DLL was not created for some reason.\n" unless -e $ModDllFile;
              }
            }
          }else{
          my $ModLibFile = "${cwd}bin\\vc7\\mod\\lib\\$module.lib";
          my $ModDllFile = "${cwd}bin\\vc7\\mod\\dll\\$module.dll";
          if( -e $ModLibFile &&
              -e $ModDllFile ){
            print "Module was compiled fine\n";
            }else{
            if( (!-e $ModLibFile) && (!-e $ModDllFile) ){
              print "ERROR: The module did not compile\n";
              }else{
              print "ERROR: Static library was not created for some reason.\n" unless -e $ModLibFile;
              print "ERROR: DLL was not created for some reason.\n" unless -e $ModDllFile;
              }
            }
          }
        }
      }
    }
  }

#
# CREATE A SKELETONFILE FOR A MODULE
#
sub create_skeleton_file {
  my $module = shift;
  my $file = shift;

  $file_content = $SKELETONS{$file};
  my $MODULE = uc $module;
  $file_content =~ s/\$module/$module/g;
  $file_content =~ s/\$MODULE/$MODULE/g;
  $file_content =~ s/\$date/$date/g;
  
  my $openfile = "extensions/$module/$file";
  # if the file exists do NOT overwrite
  if( ! -e $openfile ){
    open(F,">$openfile") or die "Can not create $openfile";
    print F $file_content;
    close F;
    }
  }

#
# CREATE THE DIRECTORY FOR THE NEW MODULE AND CREATE ALL SKELETON FILES
#
sub create_module_skeleton {
  my $module = shift;

  for my $file ( keys %SKELETONS ){
    create_skeleton_file($module,$file);
    }
  }


#
#
# CREATE A TYPICAL MAKEFILE FOR A MODULE THAT DOES NOT HAVE
#
#    THIS IS JUST TO COMPILE ALL .c FILES AND THE DOCUMENTATION
#
sub create_makefile_jam {
  my $module = shift;
  my $libsflag;

    open(F,">extensions/$module/makefile.jam") or die "Can not create extensions/$module/makefile.jam";
    print F <<END;
{\@comment
Convert this makefile.jam into makefile.vc7, makefile.bcc and to makefile 
using the setup.pl script configure --module=$module
}
{#define MODULE=$module}

all : {CreLib $module} {CreDll $module} {CreTexiDocument}

END

    #
    # determine if there is a need to include libraries.jim
    #
    $libsflag = 1;
    if( open(CFILE,"extensions/$module/interface.c") ){
      print "Searching for libraries\n";
      while( <CFILE> ){
        chomp;
        if( m{^UXLIBS} or m{^NTLIBS} ){
          print F "{#include libraries.jim}\n";
          $libsflag = 0;
          last
          }
        }
      close CFILE;
      }

    print F "{#define LibNeededLibraryFiles=}{#define LdNeededLibraryFiles=}\n" if $libsflag;

    opendir(DIR,"extensions/$module") or die "Can not read the directory extensions/$module";
    @cfiles = grep /\.c$/ , readdir(DIR);
    closedir DIR;

    # chop off the .c extension from the file names
    for( @cfiles ){
      print "file: $_\n";
      s/\.c$//;
      }

    #
    # print the target line for the module library
    print F "{CreLib $module} : ";
    for( @cfiles ){
      if( $_ eq 'interface' ){
        print F "{CreSObj $_} ";
        }else{
        print F "{CreObj $_} ";
        }
      }
    print F "\n";
    # print the LIB compilation line
    print F "\t{lib} {LibOptOutput {CreLib $module}} ";
    for( @cfiles ){
      if( $_ eq 'interface' ){
        print F "{LibOptInput {CreSObj $_}} ";
        }else{
        print F "{LibOptInput {CreObj $_}} ";
        }
      }
    print F " {LibNeededLibraryFiles}\n\n";

    #
    # print the target line for the module library
    print F "{CreDll $module} : ";
    for( @cfiles ){
      print F "{CreObj $_} ";
      }
    print F "\n";
    # print the DLL compilation line
    print F "\t{ld} {LdOptOutput {CreDll $module}} ";
    for( @cfiles ){
      print F "{LdOptInput {CreObj $_}} ";
      }
    print F "{LdNeededLibraryFiles}\n\n";


    #
    # print the .c -> .obj compilation lines
    for $cfile ( @cfiles ){
      my %depheaders = ();
      # try to determine the local header files that this .c file depends on
      if( open(CFILE,"extensions/$module/$cfile.c") ){
        print "determining dependency from extensions/$module/$cfile.c\n";
        while( <CFILE> ){
          chomp;
          if( /^\s*\#\s*include\s+\"([\w\/]+)\.h\"\s*$/ ){
            $depheaders{$1} = 1;
            }
          }
        close CFILE;
        }
      print F "{CreObj $cfile} : $cfile.c ";
      for( keys %depheaders ){
        print F "$_.h ";
        }
      print F "\n";

      print F "\t{cc} {CcOptCompile} {CcOptOutput {CreObj $cfile}} {CcOptInput $cfile.c}";
      print F "\n\n";
      # the interface file has to be compiled twice: one for the DLL and one for the LIB
      if( $cfile eq "interface" ){
        print F "{CreSObj interface} : interface.c ";
        for( keys %depheaders ){
          print F "$_.h ";
          }
        print F "\n";

        print F "\t{cc} {CcOptDefine STATIC_LINK=1} {CcOptCompile} {CcOptOutput {CreSObj interface}} {CcOptInput interface.c}";
        print F "\n\n";
        }
      }
    print F "{CompileTexiDoc}\n\n";
    close F;
  }

sub ius_CreateDirectory {
  my $dir = shift;
  my $permission = shift;
  $dir = $OPT{$dir};

  return <<END;
#
# creating the directory $dir
#
if [ -e $dir ] ; then
  echo "$dir already exists"
else
  mkdir -p $dir 2> /dev/null
  if [ \$? -ne 0 ] ; then
    echo "###ERROR creating directory $dir"
    let ERRCOUNT= \$ERRCOUNT+1
  fi
  chmod $permission $dir 2> /dev/null
  if [ \$? -ne 0 ] ; then
    echo "###ERROR setting the permission of $dir to $permission"
    let ERRCOUNT= \$ERRCOUNT+1
  fi
fi
#--------------------------------------------------------------

END
  }

sub ius_CopyFile {
  my $file       = shift;
  my $to         = shift;
  my $owner      = shift;
  my $permission = shift;

  return <<END;
# 
# Copy the file $file to $to
# 
chmod 777 $to 2>/dev/null                  
cp $file $to 2>/dev/null
if [ \$? -ne 0 ] ;then
  echo "###ERROR copying the file $file to $to"
  let ERRCOUNT = \$ERRCOUNT+1
fi         

chown $owner:$owner $to 2>/dev/null
if [ \$? -ne 0 ] ; then
  echo "###ERROR setting the owner of the file $to to $owner"
  let ERRCOUNT = \$ERRCOUNT+1
fi

chmod $permission $to 2>/dev/null
if [ \$? -ne 0 ] ; then
  echo "###ERROR changing the permission of $to to $permission"
  let ERRCOUNT = \$ERRCOUNT+1
fi
#--------------------------------------------------------------

END
  }

sub ius_CopyFiles{
  my $file        = shift;
  my $destination = shift;
  my $tofils      = shift;
  my $owner       = shift;
  my $permission  = shift;

  return <<END;
#
# Copy files $file to $destination
#
chmod 777 $tofils 2>/dev/null
cp $file $destination 2>/dev/null
if [ \$? -ne 0 ] ;then
  echo "###ERROR copying the file $file to $destination"
  let ERRCOUNT = \$ERRCOUNT+1
fi  

chown $owner:$owner $tofils 2>/dev/null
if [ \$? -ne 0 ] ; then
  echo "###ERROR setting the owner of the file $tofils to $owner"
  let ERRCOUNT = \$ERRCOUNT+1
fi

chmod $permission $tofils 2>/dev/null
if [ \$? -ne 0 ] ; then
  echo "###ERROR changing the permission of $tofils to $permission"
  let ERRCOUNT = \$ERRCOUNT+1
fi
#--------------------------------------------------------------

END
  }

#
# Create the script 'install.sh'
#
sub install_unix_scriba {

  -e './bin/exe/scriba' or die "./bin/exe/scriba does not exists. Was ScriptBasic compiled?";
  # start it safe
  chmod 0700,'install.sh';
  unlink install.sh;
  for my $installkey ( keys %HELP ){
    if( $installkey =~ /^--install-/ && $installkey ne '--install-interactive' ){
      if( ! defined $OPT{$installkey} ){
        if( $OPT{'--no-install-interactive'} ){
          $OPT{$installkey} = $INSTALL_DEFAULT{$installkey};
          }else{
          print $HELP{$installkey};
          print "please specify the value:";
          my $answer = <>;
          chomp $answer;
          $answer =~ s/^\s*//;
          $answer =~ s/\s*$//;
          $OPT{$installkey} = $INSTALL_DEFAULT{$installkey};
          $OPT{$installkey} = $answer if $answer;
          }
        }
      }
    }
  open(F,">install.sh") or die "Can not write 'install.sh'";
  print F "#! /bin/sh\n";
  print F <<ENDSHELL;
#
# This script is 'install.sh' will install ScriptBasic from the
# compiled binary to the final destination.
#
# This script was gerenared by the program setup.pl started with
# the option --install
#
# This is not a source file, do not edit!
#

ENDSHELL
  print F "ERRCOUNT=0\n";

  print F ius_CreateDirectory('--install-log','666');
  print F ius_CreateDirectory('--install-configdir','555');
  print F ius_CreateDirectory('--install-include','555');
  print F ius_CreateDirectory('--install-source','555');
  print F ius_CreateDirectory('--install-module','555');
  print F ius_CreateDirectory('--install-lib','655');

  $CONFIGDIR= $OPT{'--install-configdir'};
  $INCLUDE  = $OPT{'--install-include'};
  $SOURCE   = $OPT{'--install-source'};
  $MODULE   = $OPT{'--install-module'};
  $DOCU     = $OPT{'--install-docu'};
  $LIB      = $OPT{'--install-lib'};
  $CACHE    = $OPT{'--install-cache'};
  $HEBTEMP  = $OPT{'--install-hebtemp'};
  $BIN      = $OPT{'--install-bin'};
  $LOG      = $OPT{'--install-log'};
  $ETC      = $OPT{'--install-etc'};

  print F <<ENDSHELL;
#
# clean old cache files that may
# have been created by previous version
#
if [ -e $CACHE ] ; then
 echo "purging old cache files from $CACHE"
 rm -rf $CACHE/*
else
 echo "creating cache directory $CACHE"
 mkdir -p $CACHE
fi
echo "setting permission 777 to $CACHE"
chmod 777 $CACHE 2>/dev/null
if [ \$? -ne 0 ] ; then
 echo "###ERROR setting the permission of the directory $CACHE to 777"
 let ERRCOUNT = \$ERRCOUNT+1
fi

if [ -e $HEBTEMP ] ; then
 rm -rf $HEBTEMP/*
fi
mkdir -p $HEBTEMP
chmod a+rw $HEBTEMP

cp ./bin/exe/scriba $BIN/scriba
chown root:root $BIN/scriba
chmod 555 $BIN/scriba


echo "Now I stop the Eszter SB Engine."
if [ -e $ETC/sbhttpd ] ; then
  $ETC/sbhttpd stop >/dev/null 2>/dev/null
fi
killall sbhttpd 2> /dev/null

echo "copying the file sbhttpd binary executable to $BIN"
ENDSHELL
  print F ius_CopyFile('./bin/exe/sbhttpd', "$BIN/sbhttpd", 'root', '555');
  print F "echo \"copying the file sbhttpd shell script to $ETC\"\n";
  print F ius_CopyFile('./etc-init.d-sbhttpd',"$ETC/sbhttpd", 'root', '555');
  print F <<ENDSHELL;
cat <<'END'
************************************************************
The Eszter SB Application Engine (if it was running) was
stopped to allow upgrade. It may happen that it was not
running at all though. No problem.

Due to security reasons the installation process does not
start or in case it was already running restart the server.

If you need Eszter SB Application Engine running then please
start it saying:

$ETC/sbhttpd start
************************************************************"
END
# this is commented out because we do not want to install
# and start a http daemon on any system so that the system
# manager may not be aware of it
# $ETC/sbhttpd start
# get the file if it is not belonging to root
chown root:root $CONFIGDIR/basic.conf
# make it so that we can write it
chmod u+rw $CONFIGDIR/basic.conf
# recompile the file 
$BIN/scriba -k -f $CONFIGDIR/basic.conf scriba.conf.unix.lsp
if [ \$? -ne 0 ] ; then
  echo "###ERROR creating the default configuration file"
  let ERRCOUNT = \$ERRCOUNT+1
fi
# make it readable for all, but no writes it
chmod 444 $CONFIGDIR/basic.conf
cat <<END
************************************************************
The ScriptBasic configuration file was updated using the
default sample configuration file. In case you have already
a configuration file that you have used reinstall it
using the configuration compiler program saying:

$BIN/scriba -f $CONFIGDIR/basic.conf -k your_old_config_file

If you did not have a configuration from an older installation
then start using the default configuration and save the text
version of the default config file at a location you wish.
The text version of the default configuration is stored in
the file scriba.conf.unix.lsp
************************************************************
END
ENDSHELL

  print F "echo \"copying the standard header files\"\n";
  print F ius_CopyFiles('include/*.bas',$INCLUDE,"$INCLUDE/*.bas",'root',444);

  print F "echo \"installing all modules that were successfully compiled\"\n";

  #
  # starting ScriptBasic BASIC installation scripts for each module
  #
  opendir(DIR,'extensions') or die "Can not list directory 'extensions'";
  @modules = readdir(DIR);
  close DIR;
  while( $module = shift @modules ){
    next if ! -d "extensions/$module";
    next if $module eq '.';
    next if $module eq '..';
    next if $module eq 'CVS';
    next if -e "extensions/$module/notunix.txt";
    if( -e "bin/mod/dll/$module.so" &&
        -e "bin/mod/lib/$module.a"  &&
        -e "extensions/$module/$module.bas" &&
        -e "extensions/$module/install.sb" ){
      print F "cd extensions/$module\n";
      print F "$BIN/scriba -n install.sb\n";
      print F "cd ../..\n";
      }
    }

  print F "echo \"copy the scriba library file\"\n";
  print F ius_CopyFile('./bin/lib/libscriba.a', "$LIB/libscriba.a", 'root', '444');
  print F "echo \"copy the example preprocessor to the source directory\"\n";
  print F ius_CopyFile('./heber.bas', "$SOURCE/heber.bas", 'root', '444');

  print F <<ENDSHELL;
if [ \$ERRCOUNT -eq 0 ] ; then
 echo "It seems that the installation was sucessful"
else
 if [ \$ERRCOUNT -eq 1 ] ; then
  echo "There was one error during installation."
 else
  echo "There were \$ERRCOUNT errors during installation."
 fi
fi
echo
echo "DID YOU READ ALL THE MESSAGES ABOVE? THEY ARE IMPORTANT!"

ENDSHELL

  close F;
  chmod 0500,"install.sh";

  # create etc-init.d-sbhttpd
  open(F,">etc-init.d-sbhttpd") or die "Can not write etc-init.d-sbhttpd";
  print F <<ENDSHELL;
#!/bin/sh
#
# Start/stops the ScriptBasic httpd daemon (/usr/bin/sbhttpd)
#

PIDFILE=$LOG/pid.txt
DAEMONIMAGE=$BIN/sbhttpd
CONFIG=$CONFIGDIR/basic.conf

# Sanity check: see if ScriptBasic has been configured on this system.
if [ ! -f \$CONFIG ]; then
	echo "The file \$CONFIG does not exist! There is something wrong"
	echo "with the installation of ScriptBasic on this system. Please re-install"
	echo "ScriptBasic. I can't continue!!!"
	exit 1
fi

if [ ! -f \$DAEMONIMAGE ]; then
	echo "The file \$DAEMONIMAGE does not exist! There is something wrong"
	echo "with the installation of Eszter SB Application Engine on this system."
	echo "Please re-install ScriptBasic. I can't continue!!!"
	exit 1
fi

if [ -f \$PIDFILE ] ; then
	PID=`cat \$PIDFILE`
	if [ "x\$PID" != "x" ] && kill -0 \$PID 2>/dev/null ; then
		RUNNING=1
	else
		RUNNING=0
	fi
else
	RUNNING=0
fi

for ARG in \$\@ \$ARGS
do

case "\$1" in
	start)
		echo "Starting Eszter SB Application Engine"
		if [ \$RUNNING -eq 1 ]; then
			echo "\$0 \$ARG: sbhttpd (pid \$PID) already running"
			continue
		fi
		if /usr/bin/sbhttpd -start ; then
		    echo "\$0 \$ARG: sbhttpd started"
		else
		    echo "\$0 \$ARG: sbhttpd could not be started"
		    ERROR=3
			fi
		;;		
	stop)
		echo "Stopping Eszter SB Application Engine"
                # first try to ask it to stop gracefully
		if [ \$RUNNING -eq 0 ]; then
			echo "\$0 \$ARG: sbhttpd is not running."
			continue
		fi
                rm -f \$PIDFILE
		for w in 1 2 3 4 5 6 7 8 9 0 A B C D E F G H I J K L M N O P Q R S
		do
		if [ "x\$PID" != "x" ] && kill -0 \$PID 2>/dev/null ; then
			echo -n "."
			sleep 1
		else
			break
                fi
		done
		if [ "x\$PID" == "x" ] || ! kill -0 \$PID 2>/dev/null ; then
			echo
			echo "\$0: \$ARG: sbhttpd stopped gracefully"
			continue
                fi
		kill \$PID
		for w in 1 2 3 4 5 6 7 8 9 0
		do
		if [ "x\$PID" != "x" ] && kill -0 \$PID 2>/dev/null ; then
			echo -n "."
			sleep 1
		else
			break
                fi
		done
		if [ "x\$PID" == "x" ] || ! kill -0 \$PID 2>/dev/null ; then
			echo
			echo "\$0: \$ARG: sbhttpd stopped forcefully (gyk: killed)"
			continue
                fi
		kill -9 \$PID
		if [ "x\$PID" != "x" ] && kill -0 \$PID 2>/dev/null ; then
			echo
			echo "\$0 \$ARG: Can't stop sbhttpd (pid=\$PID)"
		else
			echo
			echo "\$0: \$ARG: sbhttpd stopped brutally (gyk: kill -9)"
			continue
                fi
		;;
	restart)
		$ETC/sbhttpd stop
		$ETC/sbhttpd start
		echo "\$0 \$ARG: sbhttpd restarted"
		;;
	*)
		echo "Usage: $ETC/sbhttpd {start|stop|restart}"
		exit 1
		;;
esac
done

exit 0

ENDSHELL
  close F;

  # create config.pl
  open(F,">config.pl") or die "Can not write config.pl";
  print F <<END;
# This file contains the configuration specific 
# directory definitions. This file was created by
# the program setup.pl and is included by the script
# scriba-rpm.pl
\$CONFIGDIR = "$CONFIGDIR";
\$INCLUDE   = "$INCLUDE";
\$SOURCE    = "$SOURCE";
\$MODULE    = "$MODULE";
\$LIB       = "$LIB";
\$CACHE     = "$CACHE";
\$HEBTEMP   = "$HEBTEMP";
\$BIN       = "$BIN";
\$LOG       = "$LOG";
\$ETC       = "$ETC";
1;
END
  close F;

  # now create mkdeb.sh
  open(F,">mkdeb.sh") or die "Can not write mkdeb.sh";

  print F <<ENDSHELL;
#! /bin/sh
#
# This file will create the debian package after
# ScriptBasic was compiled and configured
#
# This file is not a source file, do not edit!
#
# This file was generated by the program setup.pl
# when started with the option --install
#
CONFIGDIR=deb/scriba$CONFIGDIR
INCLUDE=deb/scriba$INCLUDE
SOURCE=deb/scriba$SOURCE
MODULE=deb/scriba$MODULE
LIB=deb/scriba$LIB
CACHE=deb/scriba$CACHE
HEBTEMP=deb/scriba$HEBTEMP
BIN=deb/scriba$BIN
LOG=deb/scriba$LOG
ETC=deb/scriba$ETC

#
# clean old version of the package
#
mkdir -p deb/scriba
rm -rf deb/scriba/* 2>/dev/null
rm deb/*.deb 2>/dev/null

#
# insert the current build number into the control file
#
perl preparedeb.pl

#
# copy the control files to the place where they have to be
# to build the debian package
#
mkdir -p deb/scriba
cp -R deb/DEBIAN deb/scriba
rm -rf deb/scriba/DEBIAN/CVS
chown root:root deb/scriba/DEBIAN
chown root:root deb/scriba/DEBIAN/*
chmod 075 deb/scriba/DEBIAN
chmod 075 deb/scriba/DEBIAN/*

mkdir -p \$CONFIGDIR
mkdir -p \$BIN
mkdir -p \$INCLUDE
mkdir -p \$SOURCE
mkdir -p \$MODULE
mkdir -p \$LIB
mkdir -p \$CACHE
mkdir -p \$HEBTEMP
mkdir -p \$LOG
mkdir -p \$ETC

cp ./bin/exe/scriba \$BIN
cp ./bin/exe/sbhttpd \$BIN
cp ./etc-init.d-sbhttpd \$ETC/sbhttpd
cp ./bin/mod/dll/*.so \$MODULE
cp ./bin/lib/*.a \$LIB
cp ./bin/mod/lib/*.a \$LIB
cp ./include/* \$INCLUDE
./bin/exe/scriba  -f \$CONFIGDIR/basic.conf -k scriba.conf.unix.lsp
cp ./heber.bas \$SOURCE

chown -R root:root deb/scriba
chmod -R 0755 deb
chmod -R 0777 \$CACHE
chmod -R 0777 \$HEBTEMP
chmod -R 0777 \$LOG
cd deb
dpkg --build scriba
mv scriba.deb scriba-v`cat ../version.txt`b`cat ../build.txt`-1_i386.deb
cd ..
ENDSHELL

  close F;
  chmod 0500,"mkdeb.sh";

    # now create mkrpm.sh
  open(F,">mkrpm.sh") or die "Can not write mkrpm.sh";

  print F <<ENDSHELL;
#! /bin/sh

echo "Deleting all intermediate files."

rm -f *.h syntax.c errcodes.c
rm -rf deb/scriba/*
rm -f deb/*.deb
rm -rf bin/*
rm -f `find . -name \*.lib -print`
rm -f `find . -name \*.dll -print`
rm -f `find . -name \*.exe -print`
rm -f `find . -name \*.zip -print`
rm -f `find . -name \*.gz  -print`
rm -f `find . -name \*.tar -print`
rm -f `find . -name \*.aux -print`
rm -f `find . -name \*.dvi -print`
rm -f `find . -name \*.ps  -print`
rm -f `find . -name \*.tex -print`
rm -f `find . -name \*.toc -print`
rm -f `find . -name \*.exp -print`
rm -f `find . -name \*.bbf -print`
rm -f `find . -name \*.bkg -print`
rm -f `find . -name \*.obj -print`
rm -f `find . -name \*.ps -print`
rm -f `find . -name \*.jar -print`
rm -f `find . -name \*.tex -print`
rm -f `find . -name \*.h_bas -print`
rm -f `find . -name \*.pbt -print`
rm -f `find . -name \*.log -print`
rm -f `find . -name \*.c_ -print`
rm -f `find . -name \*~ -print`
rm -f `find . -name \*.thtml -print`
rm -rf `find . -name \*Debug\* -print`
rm -rf `find . -name \*imDebug\* -print`
rm -rf `find . -name \*Release\* -print`
rm -rf `find . -name \*imRelease\* -print`
rm -f *.conf
rm -rf gif
rm -rf html
rm -rf filesdoc
rm -rf extensions/japi/reference
rm -rf extensions/japi/c-examples

echo "copiing source files into RPM temporary directory"

# delete the old directory if it existed
rm -rf scriba-`cat version.txt`b`cat build.txt` | grep -v CVS 2> /dev/null

# create the directory
mkdir scriba-`cat version.txt`b`cat build.txt`

# get all the files in the current directory except the one that we just have created
ls -1|grep -v scriba-`cat version.txt`b`cat build.txt` >tmp

# copy all the files into the temporary directory
cp -R `cat tmp` scriba-`cat version.txt`b`cat build.txt`

# remove the file list
rm tmp

# pack the source files into tar.gz file
tar czf scriba-v`cat version.txt`b`cat build.txt`-source.tar.gz scriba-`cat version.txt`b`cat build.txt`

# remove the temporary directory
rm -rf scriba-`cat version.txt`b`cat build.txt`

# copy the tar.gz file into the dir where RPM expects it
cp scriba-v`cat version.txt`b`cat build.txt`-source.tar.gz /usr/src/redhat/SOURCES

# this script creates the RPM descriptor file
perl scriba-rpm.pl

# build the RPM packages
rpm -ba /usr/src/redhat/SPECS/scriba-rpm.spec
ENDSHELL
  close F;
  chmod 0500,"mkrpm.sh";


  print "Now I will try to determine the largest safe value for maxlevel.\n";
  print "This may take a while, stay tuned...\n";
  # determine maxlevel running infinite loop in scriba
  open(F,">tmpconf.lsp") or die "Can not write tmpconf.lsp";
  print F "maxlevel 0\n";
  close F;
  # now compile the temporary configuration file
  `./bin/exe/scriba -f tmpconf.conf -k tmpconf.lsp 2> /dev/null`;
  # now run the infinite loop
  `./bin/exe/scriba -n -f tmpconf.conf infrec.sb`;
  open(F,"depth.txt") or die "there was not depth.txt created";
  $maxlevel = <F>;
  close F;
  $MAXLEVEL = length($maxlevel);
  print "Maxlevel is $MAXLEVEL\n";
  unlink 'tmpconf.conf';
  unlink 'tmpconf.lsp';
  unlink 'depth.txt';

  die "Call stack is too low in the compiled scriba executable" if $MAXLEVEL < 100;
  $MAXLEVEL -= 100; # to be safe

  &get_config_file;

  open(F,">scriba.conf.unix.lsp") or die "Cannot write scriba.conf.unix.lsp";
  print F $SCRIBA_CONF_UNIX_LSP;
  close F;

  }

sub help{
  my $counter;
  $counter = 0;
  if( ! $LastOptionWasHelp ){
    for( keys %OPT ){
      next if $_ eq '--help';
      if( defined $HELP{$_} ){
        $helptext = $HELP{$_};
        }elsif( defined $HELP{ &negopt($_) } ){
        $helptext = $HELP{ &negopt($_) };
        }else{
        $helptext = <<END;
$_

This option is invalid. WHen you use an option not known to setup the
program stops without processing. This is to have safer behaviour.

You most probably mistyped an option name. Try to type:

setup --help

to get the list of available options.
###
END
        }
      $helptext =~ s/\#\#\#.*$//;
      print "$helptext\n";
      $counter ++;
      }
    }
  if( $counter == 0 ){
    print <<END;
Usage: ./setup [options]
    run this program to configure ScriptBasic
    source installation
Options:
END
    for( sort keys %HELP ){
      print "    $_\n"
      }
    print <<END;
Any option can be used as --no-option to get negative effect.

Type: 'setup --help --option' to get detailed help on specific option

END
    }
  }

BEGIN {
#
# UNIX installation default configuration values
#
$INSTALL_DEFAULT_CONFIGDIR='/etc/scriba';
$INSTALL_DEFAULT_INCLUDE  ='/usr/share/scriba/include';
$INSTALL_DEFAULT_SOURCE   ='/usr/share/scriba/source';
$INSTALL_DEFAULT_MODULE   ='/usr/local/lib/scriba';
$INSTALL_DEFAULT_DOCU     ='/usr/share/scriba/source';
$INSTALL_DEFAULT_LIB      ='/usr/local/lib';
$INSTALL_DEFAULT_CACHE    ='/var/cache/scriba/cache';
$INSTALL_DEFAULT_HEBTEMP  ='/var/cache/scriba/hebtemp';
$INSTALL_DEFAULT_BIN      ='/usr/bin';
$INSTALL_DEFAULT_LOG      ='/var/log/scriba';
$INSTALL_DEFAULT_ETC      ='/etc/init.d';

%INSTALL_DEFAULT = (
'--install-configdir' => $INSTALL_DEFAULT_CONFIGDIR,
'--install-include'   => $INSTALL_DEFAULT_INCLUDE,
'--install-source'    => $INSTALL_DEFAULT_SOURCE,
'--install-module'    => $INSTALL_DEFAULT_MODULE,
'--install-docu'      => $INSTALL_DEFAULT_DOCU,
'--install-lib'       => $INSTALL_DEFAULT_LIB,
'--install-cache'     => $INSTALL_DEFAULT_CACHE,
'--install-hebtemp'   => $INSTALL_DEFAULT_HEBTEMP,
'--install-bin'       => $INSTALL_DEFAULT_BIN,
'--install-log'       => $INSTALL_DEFAULT_LOG,
'--install-etc'       => $INSTALL_DEFAULT_ETC,
  );

%HELP = (

'--log-append' => <<END,
--log_append

Use this option to instruct setup.pl to open its log file
'setup.log' in append mode. This is usually not needed by
the user, but is used by setup.pl when in a subprocess it
starts child processes to run setup.pl for sub tasks.
END

'--module' => <<END,
--module=module_name

Use this option to configure a module.

###
END

'--modules' => <<END,
--modules or --no-modules

Use this option to tell setup to run itself as subprocess
for each module that exists in the directory 'extensions'.

The default behaviour is to setup the modules automatically thus
you will rather use '--no--modules' than '--modules'.

NOTE that there is an extra 's' at the end of the option and the
option '--module' without the trailing 's' is used to specify
a specific module to be configured.

###
END

'--nt' => <<END,
--nt

You can use this option to tell setup that the operating system is NT or
some other Win32 operating system and not UNIX. This may be needed on some
installation where cygwin is installed and makes setup.pl think that it is
executing in UNIX environment.

To be specific: setup.pl determines the current working directory executing
the command pwd. If it fails it assumes that this is WIN32 environment. If it
succedes the it believes this is UNIX.

If you did not specified this option, but still you see this help text, dont
worry. You started setup using the command file 'setup.cmd', which can only
be executed under Win and thus invokes the Perl script setup.pl already
specifying the option '--nt'
###
END

'--unix' => <<END,
--unix

You can use this option to tell setup that the operating system is UNIX. It may
happen that the setup.pl script erroneously believes that the operating system
is NT or some other WIN32 flavour. In this situation you can use this option to
tell the script that the OS is UNIX.
###
END

'--darwin' => <<END,
--darwin

You can use this option to tell setup that the operating system is Darwin
(that is, Mac OS X using the Apple Developer Tools and gcc). It may happen that
the setup.pl script erroneously believes that the operating system is NT or some
other WIN32 flavour. In this situation you can use this option to tell the
script that the OS is Darwin.
END

'--macos' => <<END,
--macos

You can use this option to tell setup that the operating system is MacOS
(that is, Mac OS 8 to 10 using Metrowerks CodeWarrior and Carbon libs for
development). It may happen that the setup.pl script erroneously believes that
the operating system is NT or some other WIN32 flavour. In this situation you
can use this option to tell the script that the OS is MacOS.
END

'--save' => <<END,
--save or --save=file_name

Use this option to save the command line options into a
file. You can specify a file to save the configuration into,
like

configure --save=configure.save

If you do not specify any file then the default file name
'configure.save' will be used. To load the options from a
file use the option '--load'. Note that when the options are
saved into a file the option '--save' is not saved.

The default behavior is not to save the command line options
thus '--no-save' is useless.
###
END

'--load' => <<END,
--load or --load=file_name

Use this option to load command line options from a file
where it was saved during a previous run. You can specify
the file name to read the command line options from,
like

configure --load=configure.save

If you do not specify any file then the default file name
'configure.save' will be used.

The default behaviour is not to load any 
END

'--search-bcc' => <<END,
--search-bcc or --no-search-bcc

Use --no-search-bcc to force setup not to search the Windows NT
path to find the Borland C compiler (BCC32.EXE). This can be useful if you
do not have the Borland C compiler installed on the system anyway.

The default is to search the PATH for the Borland C compiler and set the
Jamal macros in the file 'configure.jim' to specify the binary, include and
lib path for the compiler.
END

'--BorlandIncludeDirectory' => <<END,
--BorlandIncludeDirectory=value

Use this option to specify the value where the Borland C compiler should
search the C header files. (Command line option -I for the bcc32 compiler.)

You can save the result of a search using the option '--save-bcc-config'
and loading next time using the option '--load'.
END

'--BorlandLibDirectory' => <<END,
--BorlandLibDirectory=value

Use this option to specify the value where the Borland C compiler should
search the library files files. (Command line option -L for the bcc32 compiler.)

You can save the result of a search using the option '--save-bcc-config'
and loading next time using the option '--load'.
END

'--bcc32' => <<END,
--bcc32=value

Use this option to specify the command line that the utiliy 'make' should
use to invoke the Borland C compiler.

You can save the result of a search using the option '--save-bcc-config'
and loading next time using the option '--load'.
END

'--save-bcc-config' => <<END,
--save-bcc-config

Use this option to force setup to save the values specific
to the Borland C compiler to the command line configuration file
(configure.save or as defined by the option '--save').

Using this option the options

'--BorlandIncludeDirectory'
'--BorlandLibDirectory'
'--bcc32'

will get the value that the PATH search results and are saved into the
file. If you use this option there is no need to specify the option
'--save'.
END

'--mkdir' => <<END,
--mkdir or --no-mkdir

Use this option to tell setup to create the directories that
are needed by the various compilers to store the intermeadiate and
final compiled files (objs, libs and exes).

The default is to generate the directories, thus you usually will
use '--no-mkdir' and you will use '--mkdir' only to override the
option loaded from a file.
END

'--makefile' => <<END,
--makefile or --no-makefile

Use this option to tell setup to create the makefiles for
the various compilers using the Jamal macro processor.

The default is to create the makefiles, thus you usually will
use "--no-makefile' and you will use '--makefile' only to override the
option loaded from a file.
END

'--syntaxer' => <<END,
--syntaxer or --no-syntaxer

Use this option to tell setup to create the file 'syntax.c'
from the syntax defintion 'syntax.def' running the program 'syntaxer.pl'

The default is to recreate the file 'syntax.c', thus you usually will
use "--no-syntaxer' and you will use '--syntaxer' only to override the
option loaded from a file.
END

'--lmt_make' => <<END,
--lmt_make or --no-lmt_make

Use this option to generate the C source files from the lmt_*.def files.
LMT stands for Linked Modules Table. The lmt_*.def files list all the
modules that are to be linked with certain variants. To statically link
modules there is a need for a module table in C source code that list the
modules and the lookup functions of the modules. When a module is linked
dynamically such lookup is provided by the OS. To ease maintenance lmt_*.def
files list only the names of the modules, and this option creates the
appropriate C source files.

The default is to generate the lmt_*.c files, thus you usually will
use "--no-lmt_make" and you will use "--lmt_make" only override the
option loaded from file.
END

'--zip' => <<END,
--zip

Use this option to create the command file mkzip.cmd and the file list
file mkzip.lst. After this run mkzip.cmd to create the ZIP file containing
the Windows source package.

To run mkzip.cmd you need WinZIP command line extension wzzip properly installed
and licesed.
END

'--headerer' => <<END,
--headerer or --no-headerer

Use this optiont to tell setup to create the header files
running the program 'headerer.pl' for each C source file that is in
the source distribution.

setup builds up a file list scanning all directories in the
source distribution recursively that may really take some time.

The default is to create the header files, thus you usually will
use "--no-headerer' and you will use '--headerer' only to override the
option loaded from a file.

END

'--generrh' => <<END,
--generrh or --no-generrh

Use this option to tell setup to create the file 'errcodes.c'
from the syntax defintion 'errors.def' running the program 'generrh.pl'

The default is to recreate the file 'errcodes.c', thus you usually will
use "--no-generrh' and you will use '--generrh' only to override the
option loaded from a file.
END

'--directory' => <<END,
--directory=InstallDirectory

Use this option under Windows NT together with the option '--install'.
Under UNIX this option is ignored.

This option specifies where the option '--install' should copy the
compiled files. If this option is not specified then the default
installation directory 'T:/ScriptBasic' is used. This happens to be the
directory on the machine of the developer.
END

'--install' => <<END,
--install or --no-install

Use this option under UNIX to ask setup.pl to create install.sh, mkdeb.sh,
config.pl, etc-init.d-sbhttpd that can be used to install the compiled code.

This option under Windows of any type starts install.sb with the appropriate
arguments. See also the option '--directory'. You can also use the command
file 'install.cmd'.

There are several sub options that are taken into account only when the
installation takes place, and some of which are mandatory and are asked
interactively unless specified on the command line or in a saved
command line option file. All these sub options start with '--install-'
and continue with the installation parameter keyword.

Note that you have to run './setup --install --save' to successfully run
mkrpm.sh because that scripts need the saved information when it executes
'./setup --install --load' during make.
END

'--install-interactive' => <<END,
--install-interactive or --no-install-interactive

Use this option to ask setup to install ScriptBasic according to
command line options and ask for values not specified. This is the
default behaviour. You have to use '--no-install-interactive' to 
alter this.

If you use '--no-install-interactive' setup will use the command line
option values or the default value for installation parameters.

END

'--install-configdir' => <<END,
--install-configdir=directory

Use this option to specify the configuration directory where
the compiled configuration file is saved.

The default value is

${INSTALL_DEFAULT_CONFIGDIR}
END

'--install-include' => <<END,
--install-include=directory

Use this option to define where the ScriptBasic module
header files are to be stored. The default value is

$INSTALL_DEFAULT_INCLUDE
END

'--install-docu' => <<END,
--install-docu=directory

Use this option where ScriptBasic modules should install their
documentation files when they are compiled from source and
are installed calling their 'install.sb' program file.

The default value is
$INSTALL_DEFAULT_DOCU
END

'--install-source' => <<END,
--install-source=directory

Use this option to specify where ScriptBasic should store the
sample and preprocessor source files. The default value is

$INSTALL_DEFAULT_SOURCE
END

'--install-module' => <<END,
--install-module=directory

Use this option to specify where ScriptBasic should store the
module shared object files. The default value is

$INSTALL_DEFAULT_MODULE
END

'--install-lib' => <<END,
--install-lib=directory

Use this option to specify where ScriptBasic installation should put
the ScriptBasic run-time library files. This is needed only when you
want to compile BASIC to executable via C code or compile
your own variation. The default location is

$INSTALL_DEFAULT_LIB
END

'--compile' => <<END,
--compile=bcc or --compile=vc7 or --no-compile

You can use this option to specify the compiler under Windows NT or
you can tell the setup program not to compile the project.

SETUP by default uses the Visual C++ compiler command line compiler
'cl'. Thus --compile=vc7 is the default behavior. If --compile=bcc32
is specified the project is compiled using the Borland C++ compiler.

If --no-compile is used then the SETUP script will not compile the
project. In this case the project can be compiled using the generated
script (UNIX: 'compile'; WIN: 'compile_bcc.cmd' or 'compile_bcc.cmd').

END

'--install-cache' => <<END,
--install-cache=directory

Use this option to specify where ScriptBasic has to store the
compilation cache files. The default value is

$INSTALL_DEFAULT_CACHE
END

'--install-hebtemp' => <<END,
--install-hebtemp=directory

Use this option to specify where ScriptBasic external preprocessor
HEB should store the temporary BASIC files that it generates. The
default location is


$INSTALL_DEFAULT_HEBTEMP
END

'--check' => <<END,
--check

Use this option to check the result of the compilation of ScriptBasic. Using
this option setup will check the files that has to be created by the
compilation and will print a short report on the screen.

Note that in case some of the modules are missing it may be because the
libraries that the module depends on may not be installed on your system.
This may or may not be an error depending on your desires.
END

'--self-check' => <<END,
--self-check

This option is to check the source code of setup.pl if there is any option
defined used and processed in it, but has no help text. This is a helpful
option for those, who develop setup.pl itself, because an option is accepted
only if there is a help text for it.
END

'--install-bin' => <<END,
--install-bin=directory

Use this option to specify where ScriptBasic installation should
put the binary executable of the interpreter. The default value
is

$INSTALL_DEFAULT_BIN

Install ScriptBasic into a different directory in exceptional
cases only, because most of the BASIC code will start referring
to '#! /usr/bin/scriba'

END

'--install-log' => <<END,
--install-log=directory

Use this option to specify where ScriptBasic Eszter SB Application
Engine variation (the web server version) has to write the log files.
The default value is

$INSTALL_DEFAULT_LOG
END

'--install-etc' => <<END,
--install-etc=directory


Use this option to specify where installation should put the
Eszter SB Application Engine start/stop script. The default
location is

$INSTALL_DEFAULT_ETC
END
  );
%SKELETONS = (

'install.sb' => <<'END',
#! /usr/bin/scriba
' """
FILE: install.sb

This file should be executed to install the module
$module after it has been compiled.

Each module should have a similar installation 
BASIC program.
"""

' Include this file!
' This contains the function modinst::install() that
' will install the header file ModulName.bas
' and the module file ModulName.dll or ModuleName.so
'
include modinst.bas

'
' Call the common module installer function.
' The argument is the name of the module. This is
' the name of the subdirectory where this BASIC
' program is, the name of the module dll and the
' module header BAS file.
'
MODINST::INSTALL("$module")

'
' Insert any code after this comment that the 
' module needs to finalize its installation.
'
' Note that here you can use module functions as
' the module is already installed when the code
' gets here. However the functions you use should
' be either declared here or the header file has
' to be included from the original location.
' In other words you have to
'
' include "module.bas"
'
' with the quotes to include the file from the actual
' directory. This is because the 'include' is processed
' when the interpreter reads the program before it 
' starts and by that time the header file is not installed
' yet.


' It is nice to print a message like that.
' If there is any operation that the installing
' person has to manually perform then print the message
' informing him or her here.
'
if MODINST::ERROR > 0 then
  print "There were errors during the installation of the module $module\n"
  print "See the error messages that were printed and try to correct the error.\n"
else
  print "The module $module was successfully installed.\n"
endif

STOP
END

'interface.c' => <<'END',
/*
READ THIS FILE AND CHANGE THE SOURCE WHEREVER YOU SEE COMMENTS STARTING
WITH THE WORD *TODO*

WHEN YOU ARE FINISHED YOU CAN 

  FILE   : interface.c
  HEADER : interface.h
  BAS    : $module.bas
  AUTHOR : *TODO*

  DATE: $date

  CONTENT:
  This is the interface.c file for the ScriptBasic module $module
----------------------------------------------------------------------------

Remove the two characters // from following line if this module is supposed to
be compiled under Windows. If there is a need for some library to successfully
compile the module under Windows specify the names of the libraries on the line
as it is listed for the linker application. This is usually something like

libname1.lib libname2.lib ... libnameX.lib

If there are no libraries, but still the module is to be compiled under Windows
do remove the // characters so that the program setup.pl will know that the
module is meant for Windows.
//NTLIBS:
----------------------------------------------------------------------------

Remove the two characters // from following line if this module is supposed to
be compiled under UNIX. If there is a need for some library to successfully
compile the module under UNIX specify the names of the libraries on the line
as it is listed for the linker application. This is usually something like

-lm -ldl -la

If there are no libraries, but still the module is to be compiled under UNIX
do remove the // characters so that the program setup.pl will know that the
module is meant for UNIX.

//UXLIBS:
----------------------------------------------------------------------------

Remove the two characters // from following line if this module is supposed to
be compiled under MacOS. If there is a need for some library to successfully
compile the module under MacOS specify the names of the libraries on the line
as it is listed for the linker application.
If there are no libraries, but still the module is to be compiled under MacOS
do remove the // characters so that the program setup.pl will know that the
module is meant for MacOS.

//MCLIBS:
----------------------------------------------------------------------------

Remove the two characters // from following line if this module is supposed to
be compiled under Darwin. If there is a need for some library to successfully
compile the module under Darwin specify the names of the libraries on the line
as it is listed for the linker application.
If there are no libraries, but still the module is to be compiled under Darwin
do remove the // characters so that the program setup.pl will know that the
module is meant for Darwin.

//DWLIBS:

*/

/*
*TODO*
INCLUDE HERE THE SYSTEM HEADER FILES THAT ARE NEEDED TO COMPILE THIS MODULE
*/

#ifdef WIN32
/*
*TODO*
INCLUDE HERE THE WIN32 SPECIFIC HEADER FILES THAT ARE NEEDED TO COMPILE THIS MODULE
*/
#else
/*
*TODO*
INCLUDE HERE THE UNIX SPECIFIC HEADER FILES THAT ARE NEEDED TO COMPILE THIS MODULE
*/
#endif

/*
*TODO*
INCLUDE HERE THE LOCAL HEADER FILES THAT ARE NEEDED TO COMPILE THIS MODULE
*/
#include <stdio.h>
#include "../../basext.h"

/*
*TODO*
INSERT THE BASIC CODE THAT WILL GET INTO THE FILE $module.BAS
AFTER THE LINE 'TO_BAS:' AND BEFORE THE LINE END OF THE COMMENT

NOTE THAT SUB AND COMMAND DECLARATIONS ARE CREATED AUTOMATICALLY
FROM THE FUNCTION DEFINTIONS WHEN THE MODULE IS CONFIGURED BEFORE
COMPILATION

TO_BAS:
*/

/*
*TODO*
DECLARE HERE THE MODULE OBJECT TYPE. THIS STRUCTURE SHOULD HOLD THE
DATA AVAILABLE FOR EACH INTERPRETER THREAD. USE THIS STRUCTURE TO
STORE GLOBAL VALUES INSTEAD OF USING GLOBAL VARIABLES.
*/
typedef struct _ModuleObject {
  char a; /* You may delete this. It is here to make the initial interface.c compilable. */
  }ModuleObject,*pModuleObject;


/*
*TODO*
ALTER THE VERSION NEGOTIATION CODE IF YOU NEED
*/
besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND

/*
*TODO*
ALTER THE ERROR MESSAGE FUNCTION
*/
besSUB_ERRMSG

  switch( iError ){
    case 0x00080000: return "ERROR HAS HAPPENED";
    }
  return "Unknown $module module error.";
besEND

/*
*TODO*
ALTER THE MODULE INITIALIZATION CODE
*/
besSUB_START
  pModuleObject p;

  besMODULEPOINTER = besALLOC(sizeof(ModuleObject));
  if( besMODULEPOINTER == NULL )return 0;

/*
*TODO*
INSERT HERE ANY CODE THAT YOU NEED TO INITIALIZE THE MODULE FOR THE
ACTUAL INTERPRETER THREAD
*/

besEND

/*
*TODO*
ALTER THE MODULE FINISH CODE IF NEEDED
*/
besSUB_FINISH
  pModuleObject p;

  /*
    YOU CERTAINLY NEED THIS POINTER TO FREE ALL RESOURCES THAT YOU ALLOCATED
    YOU NEED NOT CALL besFREE TO FREE THE MEMORY ALLOCATED USING besALLOC
    CLOSE ANY FILE THAT REMAINED OPEN, RELEASE DATABASE HANDLES AND ALL
    OTHER RESOURCES INCLUDING MEMORY *NOT* ALLOCATED CALLING besALLOC
  */
  p = (pModuleObject)besMODULEPOINTER;
  if( p == NULL )return 0;

  return 0;
besEND


/*
*TODO*
WRITE YOUR MODULE INTERFACE FUNCTIONS FOLLOWING THIS SKELETON

NOTE THAT THIS IS A SAMPLE FUNCTION, YOU CAN ALSO DELETE
LINES FROM IT IF NEEDED
*/
/**
=section functionname
=H title that goes into the BASIC documentation for this function

detail here what the function does so that the BASIC programmer
can understand how he/she can use it
*/
besFUNCTION(functionname)
  pModuleObject p;
  VARIABLE Argument;

  p = (pModuleObject)besMODULEPOINTER;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);

  /* if argument is undef then return undef */
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  return COMMAND_ERROR_SUCCESS;
besEND

/*
*TODO*
INSERT HERE THE NAME OF THE FUNCTION AND THE FUNCTION INTO THE
TABLE. THIS TABLE IS USED TO FIND THE FUNCTIONS WHEN THE MODULE
INTERFACE FILE IS COMPILED TO BE LINKED STATIC INTO A VARIATION
OF THE INTERPRETER.
*/

SLFST $MODULE_SLFST[] ={

{ "versmodu" , versmodu },
{ "bootmodu" , bootmodu },
{ "finimodu" , finimodu },
{ "emsgmodu" , emsgmodu },
{ "functionname" , functionname },
{ NULL , NULL }
  };
END

'manual.texi.jam' => <<'END',
{#sep/[[[/]]]}

[[[#define MODULE=$module]]]
[[[#define FILE=interface.c]]]
[[[#include ../../html/texi/skeleton.jim]]]
END

);

}

sub get_config_file {
$SCRIBA_CONF_UNIX_LSP = <<END,
; scriba.conf.unix.lsp
; ScriptBasic sample configuration file
;
; This is the sample configuration file for ScriptBasic
; when it is installed on UNIX. The format of this file is
;
;                 key value
;
; where key is a case sensitive symbol and value is the value
; associated to it. The value can be string, integer, real or
; sub configuration. In this latter case the value is enclosed
; between ( and )
;
; subconfigurations are like the whole configuration file contain
; one or more 'key value' pairs.
;
; The configuration file should not be empty and no sub configuration
; is allowed to be empty.
;
; After the configuration file is altered it has to be compiled. This
; can be accomplished using ScriptBasic with the command line option -k
;
;    scriba -k scriba.conf.unix.lsp
;
; will compile the configuration and save it into the binary file
; which is the configuration file for ScriptBasic. This is usually
; /etc/scriba/basic.conf under UNIX
;
; Having the configuration in binary mode fastens up CGI scripts and
; gives some light security, but do not depent on that too much.


; this is the extension of the dynamic load libraries on this system
dll ".so"

; Specify here the directory where the module shared objects are.
; You can specify more than one directory. ScriptBasic will search these
; directories in the order as they are specified here.
; The configuration valéue SHOULD contain the trailing /
module "$MODULE/"

; Specify here the directory where the module header include files are.
; You can specify more than one directory. ScriptBasic will search these
; directories in the order as they are specified here.
; The configuration valéue SHOULD contain the trailing /
include "$INCLUDE/"

; specify where documentation is to be stored under UNIX
docu "$DOCU/"

;
; define preprocessors
;
preproc (

  ; -- define internal preprocessors --
  internal (
    ; the key is the name of the preprocessor that the user uses on the command line,
    ; for example 'scriba -i dbg'
    ; the value should be the full path to the shared object implementing the
    ; preprocessor.
    ;
    ; This is the sample preprocesor that implements the command line
    ; version fo the debugger.
    dbg "$MODULE/dbg.so"
    )

  ; -- define external preprocessors --

  ; external preprocessors are invoked in separate process based on
  ; command line option but also based on file extension

  ; define the extension for which there is a preprocessor to be started
  extensions (
     ; the key is the extension and the value is the symbolic name of the external preprocessor
     heb "heb"
     )

  ; define the external preprocessors
  external (
    ; here the key is the symbolic name of the preprocessor,
    ; which was specified in the string (e.g. "heb") above
    ; associated to the extension
    heb (
      ; define the command line of the preprocessor
      ; this command line will be appended with two arguments:
      ;  - the first argument is the source file to process
      ;  - the second argument is the preprocessed file that the
      ;    preprocessor should create (full path will be passed to
      ;    the preprocessor)
      executable "$BIN/scriba $SOURCE/heber.bas"
      ; This is the directory where the preprocessor has to put the
      ; preprocessed file. The interpreter uses this parameter
      ; to create the second argument for the preprocessor.
      directory "$HEBTEMP"
      )
    )
  )

;
; LIMIT VALUES TO STOP INIFINITE LOOP
;

; the maximal number of steps allowed for a program to run
; comment it out or set to zero to have no limit
maxstep 0

; the maximal number of steps allowed for a program to run
; inside a function.
; comment it out or set to zero to have no limit
maxlocalstep 0

; the maximal number of recursive function call deepness
; essentially this is the "stack" size
; When the sample configuration file is created by setup.pl
; this value is calculated. You may alter it to be smaller,
; but on this platform the safe value is $MAXLEVEL
; If you increase it $MAXLEVEL+100 you surely get into trouble.
; (do not) comment it out or set to zero to have no limit
maxlevel $MAXLEVEL


; the maximal memory in bytes that a basic program 
; is allowed to use for its variables
; comment it out or set to zero to have no limit
maxmem 0

; ScriptBasic loads a module when the first function
; implemented in the module is used by the BASIC program.
; Some modules (for example noprint sample module) has
; to be loaded before the BASIC program starts. Use this
; key zero or more times to specify the modules that has
; to be loaded before any BASIC program on your installation
; runs.
;
; The sample module 'noprint' prevents the BASIC programs to use
; the command 'print', which is not a wise choice, but it is just
; a demo code anyway. You can develop security modules that you can
; force the interpreter to preload.
;preload "ext_trial"

; This is the directory where we store the compiled code
; to automatically avoid recompilation
; You need only one of this directory, do not specify more than one.
; You can comment out this directive to prevent cache usage or
; point it to a directory that does not exist or can not be written
; by the BASIC programs. Using the cache gives some performance
; advance but also implies some security considerations.
cache "$CACHE"

;
; berkeley db config
;
bdb (

 ; directories where to store the 
 dir (
   home "$CACHE/berkeleydb/" ; the home directory of operation of the Berkerley DB
   data "db"  ; database files
   log  "log" ; log files
   temp "tmp" ; temporary files
   )

;  limits (
;    lg_max "1024000"
;    mp_mmapsize "0000"
;    mp_size "000"
;    tx_max "000"
;    lk_max "000"
;    )

  ; what lock strategy to use
  ; it can be any of the followings
  ; default oldest random youngest
  lockstrategy default

  flags (
    ; set the value to yes or no
    lock_default no
    init_lock yes
    init_log yes
    init_mpool yes
    init_txn yes
    create yes
    )
  )
;
; MySQL configuration
;
mysql (
  connections (
    test (        ; the name of the connection
	  host "127.0.0.1" ; the host for the connection
	  db "test"   ; database for the connection
	  user "root" ; user for the connection
	  password "" ; password for the connection
	  port 0      ; the port to use
	  socket ""   ; the name of the socket or ""
	  flag 0      ; the client flag
	  clients 10  ; how many clients to serve before really closing the connections
	  )
    auth  (
	  host "127.0.0.1" ; the host for the connection
	  db "auth"   ; database for the connection
	  user "root" ; user for the connection
	  password "" ; password for the connection
	  port 0      ; the port to use
	  socket ""   ; the name of the socket or ""
	  flag 0      ; the client flag
	  clients 10  ; how many clients to serve before really closing the connections
          )
    )
  )

;
; Configure the sample ScriptBasic httpd daemon
;
servers (
  server (
    port 8889
    ip "10.22.2.94"
    protocol http
    )
  server (
    port 21
    ip "127.0.0.1"
    protocol ftp
    salute """220 Eszter SB Application Engine salute text"""
    codebase "$SOURCE\\ftp\\"
    programs (
      PASS "$SOURCE\\ftp\\pass.bas"
      ACCT "$SOURCE\\ftp\\acct.bas"
      )
    )
  threads 20
  listenbacklog 30
  home "$SOURCE/"
  proxyip 0 ; set it true if you use Eszter engine behind Apache rewrite and proxy modules

  pid (
    file "$LOG/pid.txt"
    delay 10 ; number of seconds to sleep between the examination of the pid file if that still exists
    wait ( 
      period 10 ; number of times to wait for the threads to stop before forcefully exiting
      length 1  ; number of seconds to wait for the threads to stop (total wait= period*length)
      )
    )

  vdirs (
    dir "/cgi-bin/:$SOURCE/"
    )
  client (
;    allowed "127.0.0.1/255.255.255.255"
    allowed "0.0.0.0/0.0.0.0"

;    denied "127.0.0.1/0.0.0.0"
;    denied "16.192.0.0/255.255.0.0"
    )
  errmsgdest 3
  nolog 0 ; set this true not to use logs or ignore erroneouslog configuration
;  run (
;     start   "start.bas"
;     restart "restart.bas"
;      )
  log (
    panic ( file "$LOG/panic.log" )
    app   ( file "$LOG/app.log" )
    err   ( file "$LOG/err.log" )
    hit   ( file "$LOG/hit.log" )
    stat  ( file "$LOG/stat.log" )
    )
  ; the error page when a page is not found
msg404 """
<HTML>
<HEAD>
<TITLE>Error 404 page not found</TITLE>
</HEAD>
<BODY>
<FONT FACE="Verdana" SIZE="2">
<H1>Page not found</H1>
We regretfully inform you that the page you have requested can not be found on this server.
<p>
In case you are sure that this is a server configuration error, please contact
<FONT SIZE="3"><TT>root\@localhost</TT></FONT>
</FONT>
</BODY>
</HTML>
 """
  code404 "200 OK" ; the http error code for page not found. The default is 404
  ; the program to run when a page is not found
  ;run404 "$SOURCE/run404.bas"
  )
END

}
sub self_check {
  open(F,"<setup.pl") or die "Can not open setup.pl";
  undef $/;
  my $prog = <F>;
  close F;
  my $counter = 0;
  while(  $prog =~ /\$OPT\{\'(.*?)\'/g ){
    my $option = $1;
    next if $option eq '--help';
    if( ! defined( $HELP{ $option } ) &&
        ! defined( $HELP{ &negopt( $option) } ) ){
      $counter ++;
      print "Option '$option' is used, but no help: this is an internal error\n";
      }
    }
  if( $counter == 0 ){
    print "setup.pl seems to be OK, at least for the option help text.\n";
    }
  }
  
