#!/usr/bin/perl
# LEGAL WARNING:
#
# This software is provided on an "AS IS", basis,
# without warranty of any kind, including without
# limitation the warranties of merchantability, fitness for
# a particular purpose and non-infringement. The entire
# risk as to the quality and performance of the Software is
# borne by you. Should the Software prove defective, you
# and not the author assume the entire cost of any service
# and repair.
#
$version = '1.0';

if( $#ARGV == 0 && $ARGV[0] eq '-h' ){
  print <<END__HELP;
Source Splitter $version
Usage:

   ssplit source_file [directory]

This program splits a text file. The primary
purpose of this tool was to maintain many small HTML files as a one
somewhat larger jamal source, compile it using jamal and then split
to small html files. The source file should contain lines

\%file file_name

to start a new file. The default directory is .

For further information see on-line documentation at

            http://www.isys.hu/c/verhas/progs/perl/ssplit
END__HELP
  exit;
  }

$file = shift;
$dir = shift;
$dir = '.' unless $dir;
$subdir = '';
$output_opened = 0;
$buffer = '';
$output = '';

open(F,$file) or die "Can not open $file\n";
while( <F> ){

  if( /^\s*%\s*file\s+(.*?)\s*$/ ){
    my $new_output = $1;
    &save_buffer($output);
    $buffer = '';
    $output = "$dir/$subdir/${new_output}";
    $output =~ s{//}{/};
    next;
    }

  if( /^\s*%\s*dir\s+(.*?)\s*$/ ){
    $subdir = $1;
    next;
    }

  $buffer .= $_;
  }
&save_buffer($output);
close F;
exit;

sub save_buffer {
  my $file = shift;

  return unless $output || $buffer ;
  if( $buffer && !$output ){
    die "Is there text before the first '%file' ??";
    }

  if( open(OUT,"<$file") ){
    my $os = $/; undef $/;
    my $sbuffer = <OUT>;
    $/ = $os;
    close OUT;
    return if $buffer eq $sbuffer;
    }
  &make_dir($file);
  open(OUT,">$file") or die "Can not output $file";
  print OUT $buffer;
  close OUT;
  }

sub make_dir {
  my $dir = shift;
  my @dlist = split '/' , $dir;
  pop @dlist; # pop off file name
  return if $#dlist == -1;

  $root = '';
  for( @dlist ){
    $root .= '/' if $root;
    $root .= $_; # take the next subdirectory
    mkdir $root, 0777 unless -d $root
    }
  }
