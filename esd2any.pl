#! /usr/bin/perl
#
# This program can be used to extract embedded information from program files
#
# We tried to embed texi format documentation. This helps to keep the source documentation
# at the same place where the code is, and it is easy to include program fragments like
# function prototypes into the documentation.
#
# This program is LGPL
#
# Peter Verhas July 27, 2001.
#
$infile = shift;
open(F,$infile) or die "Can not open $infile\n";
@config = <F>;
close F;
chomp @config;

$StartString = undef;
$EndString = undef;
$OutputFile = undef;

@files = ();
@preprocess = ();
@postprocess = ();

while( defined($line = shift @config) ){
  last if $line =~ /^\s*\$any\s+config\s*$/i;
  }

while( defined($line = shift @config) ){

  last if $line =~ /^\s*\$any\s+end\s*$/i;

  if( $line =~ /^\s*\$file\s+(.*)\s*$/i ){
    push @files,$1;
    next;
    }

  if( $line =~ /^\s*\$preprocess\s+(.*)\s*$/i ){
    push @preprocess,$1;
    next;
    }

  if( $line =~ /^\s*\$postprocess\s+(.*)\s*$/i ){
    push @postprocess,$1;
    next;
    }

  if( $line =~ /^\s*\$startstring\s+(.*)$/i ){
    $StartString = $1;
    next;
    }

  if( $line =~ /^\s*\$endstring\s+(.*)$/i ){
    $EndString = $1;
    next;
    }

  if( $line =~ /^\s*\$output\s+(.*)$/i ){
    $OutputFile = $1;
    next;
    }

  next if $line =~ /^\s*$/;
  next if $line =~ /^\s*\#/;

  die "unknown line in ANY config: $line";
  }

die "No output file is defined." unless defined $OutputFile;

die "No input file is given in file $infile" if $#files == -1;

open(OUT,">$OutputFile") or die "Can not open output file.";

for $file ( @files ){
  open(F,$file) or die "Can not open input file $file";
  $DoOutput = 0;
  while( <F> ){
    chomp;
    for $prep (@preprocess){
      eval $prep;
      }
    if( $DoOutput ){
      for $prep (@postprocess){
        eval $prep;
        }
      if( $_ eq $EndString ){
        $DoOutput = 0;
        next;
        }
      print OUT "$_\n";
      }else{
      if( $_ eq $StartString ){
        $DoOutput = 1;
        next;
        }
      }
    }
  close F;
  if( $DoOutput ){
    warn "File $file was finished with output on.";
    }
  }
close OUT;
