#
# PACKAGE EXAMPLE
#

# This is a Perl defined macro extension module. This file should be placed into the
# directory named 'jamal' under the Perl Module library directory. This is
# E:\Perl\lib\jamal on my Windows NT. On UNIX this is something different.
#
# You can also install this program using the program jamint.pl or using jamal.pl
# with the option '-i' if you have an up to date version of Jamal

package jamal::esd;

BEGIN {
$VERSION = '2.0'; #the current module version
&jamal::require('2.0');#require jamal version
$RVERSION = &jamal::version('1.0','2.0');
$RVERSION = $VERSION unless $RVERSION;
# define package specific macros
if( $RVERSION > 1.0 ){
#  &jamal::DefineMacro( 'cap' , ',x' , '<font size=+1>x</font>' );
  }

# this global variable will hold all the commands
%COMMAND = ();
}

#
# This sub defines the macro 'command' 
# There are two arguments. The first one is a key that orders the
# execution of the commands and that also helps to redefine commands.
# These commands defined calling this sub are 'eval'ed for each included
# source file. 
#
sub command {
  my $arg = shift;

  # there should be a key and the command
  $arg =~ /^\s+(\w+)\s+(.*)/;

  my ($name,$command) = ($1,$2);

  $COMMAND{$name} = $command;
  return '';
  }

#
# This macro can be used to clean all the commands.
#
sub nocommand {
  %COMMAND = ();
  return '';
  }

sub include {
  my $arg = shift;
  $arg =~ s/^\s+//;
  my ($FileName,$StartString,$StopString) = split /\s+/ , $arg;

  my $output = '';
  my $sw = 0;
  open(F,$FileName) or die "Can not open $FileName";
  while( <F> ){
    chomp;
    if( $sw == 0 && $_ eq $StartString ){
      $sw = 1;
      next;
      }
    if( $sw == 1 && $_ eq $StopString ){
      $sw = 0;
      next;
      }

  my $k;
  for $k (keys %COMMAND ){
    eval $COMMAND{$k};
    }

    if( $sw ){
      $output .= $_ . "\n";
      }
    }
  close F;

  return $output;
  }

sub execute {
  my $arg = shift;

#
# If ever you want to use this file under UNIX comment this line out
#
  $arg =~ s{/}{\\}g;
  return `$arg`;
  }

END {

  }
1;
