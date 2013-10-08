#! /usr/bin/perl
#
# This program converts a file containing esd documentation to embedded texi
#

$infile = shift;
$outfile = shift;

$outfile = $infile unless defined $outfile;

open(F,$infile) or die "can not open $infile";
@lines = <F>;
close F;
chomp @lines;

for (@lines){

  s/=verbatim/\@example/;
  s/=noverbatim/\@end example/;
  s/=itemize/\@itemize/;
  s/=noitemize/\@end itemize/;
  s/=item/\@item/;
  s/T<.*;>/\@code\{$1\}/g;
  s/B<.*;>/\@b\{$1\}/g;
  s/I<.*;>/\@emph\{$1\}/g;
  s/R<.*;>/\@xref\{$1\}/g;
  s/=H(.*)$/\@section $1/;
  s/=section.*$//;
  }

open(F,">$outfile") or die "can not open $outfile";
print F join("\n",@lines);
close F;
