#!/usr/bin/perl
#
=pod
=title Source Documentation to HTML
H<Source Documentation to HTML>

This program converts documentation embedded in the source
to HTML format. The result is a series of HTML files that are
linked together.

The source can practically be any programming language.

=toc

=section usage
=title Usage of the program
=abstract
How to start the program to generate the documentation HTML files
from the source.
=end
H<Usage of the program>

The program should be started from the command line. The only
parameter is the package documentation description file name.
This file describes some syntax, how the source should be handled. You
can also start the program from another Perl script. In that case the
variable T<$DocDescFile> should be defined before invoking the
script containing the name of the documentation description file.
This can be convenient when starting of the program is automated.
=cut


# get the Documentum description file from the command line
# unless it is already defined in case this file is executed
# using a do command from another Perl script that sets this
# variable
$DocDescFile = shift unless defined($DocDescFile);

=pod
=section syntax
=title Documentation Description File Syntax
=abstract
Commands and parameters that can be specified in the
documentation description file.
=end
H<Documentation Description File Syntax>

In the Source Documentation Definition (SDD) file you can define the strings
that 
=itemize
=item starts embedded documentation, Keyword is T<StartString> Missing definition will
lead to T<=pod> which is the usual Perl documentation start string
=item finishes a section of embedded documentation Keyword is T<EndString>.
Missing definition will
lead to T<=cut> which is the usual Perl documentation end string
=item how each line between should be transformed, e.g. chomp off leading T<C> from
FORTRAN comments, Keyword is T<PreprocessComment>. Default value is do nothing.
=item you can define a single T<PreprocessComment>, but you can have several T<PreprocessDocLine> having
the same effect.
=item directory where the source files are, Keyword is T<SourceRoot> Default is the
current directory.
=item directory where the resulting html files should be put, Keyword is T<DocRoot> Default is the
current directory.
=item directory where the resulting code files should be put. Keyword is T<CodeRoot>. Default is
not to produce code output.
=item which line to strip from the code output. Keyword is T<StripLine>. You can use this keyword more than once.
If there are more than one T<StripLine> strings defined they will be evaluated in the order they
present in the SDD file for each code line that is not an esd comment. If any of the
T<StripLine> strings results true, the rest is ignored and the actual line is stripped from the code file.
=item how to preprocess each code line that is not an esd comment line. Keyword is T<PreprocessLine>. 
You can use this keyword more than once.
If there are more than one T<PreprocessLine> strings defined they will be evaluated in the order they
present in the SDD file for each code line that is not an esd comment. The resulting string will be put into the
code file.
=item default html file name, usually T<Default> of T<index>, which you do
not want to use as a section name, Keyword is T<Default>. Default value is T<main>.
=item extension for html files, usually T<htm> or T<html>. Keyword is T<Html>.
Default value is T<HTML>
=item you can define the font face and size for the generate text. Keywords are T<FontFace> and T<FontSize>
=item you can define the font size for the titles and for the verbatim text. Keywords are T<TitleFontSize> and
T<TTFontSize>.
=noitemize

The files that belong to the package are listed between the lines
T<Files> and T<End>. Each line defines a file name with full path relative to
T<SourceRoot> and with extension. After the file and the = sign the directory 
should be specified, where the sections from the file will be put. The directory
should be given relative to T<DocRoot>.

The last thing give in the SDD file is the package main section. The lines of this
section are between the lines T<Package> and T<End>

For a good example see the T<esd2html.sdd> file.
=cut

$StartString         = undef;
$StopString          = undef;
$PreprocessComment   = undef;
$DocRoot             = undef;
$SourceRoot          = undef;
$CodeRoot            = undef;
@StripLine           = ();
@PreprocessLine      = ();
@PreprocessDocLine   = ();
$Package             = undef;
$PackageTitle        = undef;
$MainName            = undef;
$HtmlExtension       = undef;
$FontFace            = 'Times';
$FontSize            = 3;
$TTFontSize          = 3;
$TitleFontSize       = 4;
$PackageAbstract     = '';
%File = ();
@File = ();
die "Can not open $DocDescFile!" unless open(F,"<$DocDescFile");

LINES:
while( <F> ){

  if( /^\s*$/ ||  #empty lines
      /^\s*\#/ ){ next } #comment lines

  if( /^\s*FontFace\s*=\s*\'(.*)\'\s*$/ ){
    $FontFace = $1;
    next;
    }
  if( /^\s*FontSize\s*=\s*(.*)\s*$/ ){
    $FontSize = $1;
    next;
    }
  if( /^\s*TTFontSize\s*=\s*(.*)\s*$/ ){
    $TTFontSize = $1;
    next;
    }
  if( /^\s*TitleFontSize\s*=\s*(.*)\s*$/ ){
    $TitleFontSize = $1;
    next;
    }
  if( /^\s*Default\s*=\s*\'(.*)\'\s*$/ ){
    die "Default double defined" if defined($MainName);
    $MainName = $1;
    next;
    }
  if( /^\s*Html\s*=\s*\'(.*)\'\s*$/ ){
    die "HtmlExtension double defined" if defined($HtmlExtension);
    $HtmlExtension = $1;
    next;
    }
  if( /^\s*StartString\s*=\s*\'(.*)\'\s*$/ ){
    die "StartString double defined" if defined($StartString);
    $StartString = $1;
    next;
    }
  if( /^\s*StopString\s*=\s*\'(.*)\'\s*$/ ){
    die "StopString double defined" if defined($StopString);
    $StopString = $1;
    next;
    }
  # PreprocessComment is the supported, but PreprocessCommand is still here for backward compatibility
  if( /^\s*PreprocessComment\s*=\s*\'(.*)\'\s*$/ || /^\s*PreprocessCommand\s*=\s*\'(.*)\'\s*$/ ){
    die "PreprocessComment double defined" if defined($PreprocessComment);
    $PreprocessComment = $1;
    next;
    }
  if( /^\s*DocRoot\s*=\s*\'(.*)\'\s*$/ ){
    die "DocRoot double defined" if defined($DocRoot);
    $DocRoot = $1;
    next;
    }
  if( /^\s*TexiOutput\s*=\s*\'(.*)\'\s*$/ ){
    die "TexiOutput double defined" if defined($TexiOutput);
    $TexiOutput = $1;
    next;
    }
  if( /^\s*SourceRoot\s*=\s*\'(.*)\'\s*$/ ){
    die "SourceRoot double defined" if defined($SourceRoot);
    $SourceRoot = $1;
    next;
    }
  if( /^\s*CodeRoot\s*=\s*\'(.*)\'\s*$/ ){
    die "CodeRoot double defined" if defined($CodeRoot);
    $CodeRoot = $1;
    next;
    }
  if( /^\s*StripLine\s*=\s*\'(.*)\'\s*$/ ){
    push @StripLine,$1;
    next;
    }
  if( /^\s*PreprocessLine\s*=\s*\'(.*)\'\s*$/ ){
    push @PreprocessLine,$1;
    next;
    }
  if( /^\s*PreprocessDocLine\s*=\s*\'(.*)\'\s*$/ ){
    push @PreprocessDocLine,$1;
    next;
    }


  if( /^\s*Package\s*$/ ){
    $Package = $1;
    while( <F> ){
      if( /^\s*End\s*$/ ){
        next LINES;
        }
      if( /^\s*=title\s*(.*)$/ ){
        $PackageTitle = $1;
        next;
        }
      if( /^\s*=H\s*(.*)$/ ){
        $PackageTitle = $1;
        $_ = "H<$1>";
        }
      $PackageAbstract .= $_;
      }
    }

  if( /^\s*Files\s*$/ ){
    while( <F> ){
      if( /^\s*$/ ||  #empty lines
          /^\s*\#/ ){ next } #comment lines
      if( /^\s*End\s*$/ ){
        next LINES;
        }
      if( /^\s*(.*)\s*=\s*(.*)\s*$/ ){
        die "File $1 double defined." if defined( $File{$1} );
        $File{$1} = $2;
        push @File,$1;
        } else {
        die "Bad format file definition:\n$_\n";
        }
      }
    }
  die "Syntax error on line:\n$_\n";
  }
close F;
$StartString         = '=pod' unless defined($StartString);
$StopString          = '=cut' unless defined($StopString);
$DocRoot             = '.'    unless defined($DocRoot);
$SourceRoot          = '.'    unless defined($SourceRoot);
#$CodeRoot           = undef  unless defined($CodeRoot); #do NOT perform comment stripping by default
$MainName            = 'main' unless defined($MainName);
$HtmlExtension       = 'html' unless defined($HtmlExtension);

if( ! -e $DocRoot ){
  mkdir $DocRoot,0777;
  }

if( defined($CodeRoot) && ! -e $CodeRoot ){
  mkdir $CodeRoot,0777;
  }

%mTitle = ();
%mAbstract = ();
=pod
=section format
=title Formatting the embedded documentation
=abstract
How the sections should be formatted, what formatting and definition
command are available.
=end
H<Formatting the embedded documentation>

You can write paragraphs into multiple lines. An empty line breaks the paragraph.
You can use line commands and in-line formatting. Line commands begin at the start
of the line with an = sign and contain a keyword. The keyword can optionally be followed
by parameters.

The line commands are:
=itemize
=item T<section> switches to a section. The default section is the T<main> section.
One section can contain parts at several location in the source file, sections can be
stopped, start a new section, then continue with a new.
=item T<notoc> tells the formatter not to include the section in the table of contents. You can have, and
should have a link to the section uning a reference using the construct T<R><section>
=item T<title> Defines the title of the section. Should only be used once for each section.
=item T<H> Defines the title of the actual section and inserts the title into the
documentation as a H<header>
=item T<abstract> Starts the abstract of the section. This abstract should describe briefly
the details of the section. This will appear in the toc.
=item T<end> End of the abstract of the section
=item T<toc> place for the table of contents, used int he main section and in the main section
of the project in the SDD file. If no such a line is present in the main section then toc will be
appended to the end of the section.
=item T<hrule> Horizontal rule
=item T<itemize> Start itemizing. You can nest as HTML can.
=item T<item> start a new item
=item T<noitemize> Stop itemizing
=item T<bold> Start lines in B<bold>.
=item T<nobold> Finish bold lines.
=item T<italic> Start lines in I<italic>.
=item T<noitalic> Finish italic lines.
=item T<verbatim> Start lines verbatim.
=item T<noverbatim> Finish verbatim lines.
=noitemize

You can also use in-line formatting. These are I<?<any character except >>I< and close with>>. The
format characters can be
=itemize
=item H for header
=item B B<bold> characters
=item I I<italic> characters
=item T T<teletype> characters
=noitemize
=cut


=pod
=section reference
=title References in the source
=abstract
How to insert a refernce to other section.
=end
H<Insert reference to other section>

Regularly you will want to reference in the text other sections of the documenttation.
You can easily do it using a syntax which is similar to sytax of in-line formatting
describen in section R<format>. The character youi should use is T<R> followed by the
section name enclosed between < and >. Therefore the reference above was typed T<R><T<format>>.

If the section is in a different source file then you have to use the format:
=verbatim
            R<file/section>
=noverbatim

T<file> is the name assigned to the source file in the list T<Files> in the SDD file.
=bold
There is no check during html generation to validate the generated links!
=nobold
In other words: if you reference a section or a file that is nonexistent the reference
will point to a dead end.

=cut

if( $TexiOutput && ! open(TEXI,">$TexiOutput") ){
  die "Can not open $TexiOutput";
  }

#
# Initialize the LaTeX file
#

$setfilename = $TexiOutput;
$setfilename =~ s/\..*$//;

if( $TexiOutput ){
  my $PA = $PackageAbstract;
  $PA =~ s/^H<.*>//g;
  $PA = &texizee($PA);
  print TEXI <<END;
\\input texinfo \@c -*-texinfo-*-
\@c \%\*\*start of header
\@setfilename $setfilename
\@settitle $PackageTitle
\@setchapternewpage odd
\@c \%\*\*end of header
\@ifinfo
\@end ifinfo

\@titlepage
\@title $PackageTitle
\@author

\@page
\@vskip 0pt plus 1filll

\@end titlepage
\@summarycontents
\@contents

\@menu
\@end menu
END
  }

while( ($file,$dir) = each %File ){
  die "Can not read $SourceRoot/$file." unless open(F,"<$SourceRoot/$file");

  $DoCommentStripping = 0;
  if( defined($CodeRoot) ){
    if( open(OUT,">$CodeRoot/$file") ){
      $DoCommentStripping = 1;
      }else{
      warn "Can not write $CodeRoot/$file.";
      }
    }

  $sw = 0;
  $section = 'main';
  %Body = ();
  %Title = ();
  %noToc = ();
  %Abstract = ();
  @section = ();
  %section = ();
  SOURCE_LINE:
  while( <F> ){
    chomp;
    for $command ( @PreprocessDocLine ){
      eval $command;
      }
    if( $sw ){
      if( $_ eq $StopString ){
        $sw = 0;
        next;
        }
      eval $PreprocessComment;
      if( /\s*\=section\s*(.*)\s*/ ){
        $section = $1;
        unless( $section{$section} ){
          $section{$section} = 1;
          push @section,$section;
          }
        next;
        }
      if( /\s*\=notoc\s*$/ ){
        $noToc{$section} = 1;
        next;
        }
      if( /\s*\=title\s*(.*)$/ ){
        die "title for section $section in file $file is double defined" if defined($Title{$section});
        $Title{$section} = $1;
        next;
        }
      if( /^\s*=H\s*(.*)$/ ){
        die "title for section $section in file $file is double defined" if defined($Title{$section});
        $Title{$section} = $1;
        $_ = "H<$1>";
        }
      if( /\s*\=abstract\s*$/ ){
        die "abstract for section $section in file $file is double defined" if defined($Abstract{$section});
        $Abstract{$section} = '';
        while( <F> ){
          chomp;
          die "abstract for section $section in file $file is not closed" if $_ eq $StopString;
          eval $PreprocessComment;
          if( /\s*\=end\s*$/ ){ next SOURCE_LINE; }
          $Abstract{$section} .= "$_\n";
          }
        die "abstract for section $section in file $file is not closed till eof" if $_ eq $StopString;
        }
      $Body{$section} .= "$_\n";
      }else{#we are outside of documentation lines
      $line = $_;
      s/\s*$//;
      if( $_ eq $StartString ){
        $sw = 1;
        }else{
        if( $DoCommentStripping ){
          $_ = $line;
          for $command ( @PreprocessLine ){
            eval $command;
            }
          my $strip = 0;
          for $command ( @StripLine ){
            last if $strip = eval $command;
            }
          print OUT "$_\n" unless $strip;
          }
        }
      }
    }
  close F;
  close OUT if $DoCommentStripping;
  $DoCommentStripping = 0;

  die "no main section is defined for file $file" unless defined($Body{'main'});
  $mTitle{$file} = $Title{'main'};
  $mAbstract{$file} = $Abstract{'main'};

  print TEXI "\@chapter ",$mTitle{$file},"\n";
  print TEXI &texizee($mAbstract{$file}),"\n\n";

  $toc = "<DL>\n";
  # create the table of contents  
  for $section ( @section ){
    next if $noToc{$section};
    $toc .= "<DT><font size=$TTFontSize><TT>$section</TT></font> ";
    $toc .= "<A HREF=\"$section.$HtmlExtension\">" . $Title{$section} . "</A><BR>\n";
    if( defined($Abstract{$section}) ){
      $toc .= '<DD><FONT SIZE=1>' .
              &htmlizee($Abstract{$section}) .
              "</FONT>\n";
      }
    $toc .= "<P>\n";
    }
  $toc .= "</DL>\n";
  # create the files
  push @section,'main';
  for $section ( @section ){
    $dir =~ s/\r$//;
    mkdir "$DocRoot/$dir",0777 unless -e "$DocRoot/$dir";
    if( $section eq 'main' ){
      $fn = $MainName;
      }else{ $fn = $section; }
    die "can not write $DocRoot/$fn.$HtmlExtension" unless open(F,">$DocRoot/$dir/$fn.$HtmlExtension");

    print F "<HTML>\n<HEAD>\n";
    print F '<TITLE>',$Title{$section},"</TITLE>\n" if defined($Title{$section});
    print F "</HEAD>\n<BODY>\n<FONT FACE=\"$FontFace\" SIZE=\"$FontSize\">\n";
    print F "<TABLE BORDER=0><TR><TD WIDTH=600 VALIGN=TOP><FONT SIZE=$FontSize>\n";
    print F "<A href=\"$MainName.$HtmlExtension\"><tt><B><FONT SIZE=+1>FILE</FONT></B></tt></A>&nbsp;...&nbsp;"
      unless $fn eq $MainName;
    print F "<A href=\"../$MainName.$HtmlExtension\"><tt><B><FONT SIZE=+1>TOC</FONT></B></tt></A>\n";
    print F &htmlizee($Body{$section});
    if( $section eq 'main' && !$TocInserted ){
      print F "<P>\n";
      print F $toc;
      }
    print F '<P>';
    print F "<A href=\"$MainName.$HtmlExtension\"><tt><B><FONT SIZE=+1>FILE</FONT></B></tt></A>&nbsp;...&nbsp;"
      unless $fn eq $MainName;
    print F "<A href=\"../$MainName.$HtmlExtension\"><tt><B><FONT SIZE=+1>TOC</FONT></B></tt></A>\n";
    print F "</TD></TR></TABLE>\n";
    print F "</FONT>\n</BODY>\n</HTML>\n";
    close F;

    if( $TexiOutput ){
      print TEXI &texizee($Body{$section});
      print TEXI "\n\n";
      }

    }
  }

if( $PackageAbstract ){
  $toc = "<DL>\n";
  # create the table of contents  
  for $File ( @File ){
    $toc .= "<DT><font size=$TTFontSize><TT>$File</TT></font> ";
    $toc .= '<A HREF="' . $File{$File} ."/$MainName.$HtmlExtension\">" . $mTitle{$File} . "</A><BR>\n";
    if( defined($mAbstract{$File}) ){
      $toc .= '<DD><FONT SIZE=-1>' .
              &htmlizee($mAbstract{$File}) .
              "</FONT>\n";
      }
    $toc .= "<P>\n";
    }
  $toc .= "</DL>\n";
  die "can not write $DocRoot/$MainName.$HtmlExtension" unless open(F,">$DocRoot/$MainName.$HtmlExtension");
  print F "<HTML>\n<HEAD>\n";
  print F '<TITLE>',$PackageTitle,"</TITLE>\n" if defined($PackageTitle);
  print F "</HEAD>\n<BODY><FONT FACE=\"$FontFace\" SIZE=\"$FontSize\">\n\n";
  print F "<TABLE BORDER=0><TR><TD WIDTH=600 VALIGN=TOP><FONT SIZE=$FontSize>\n";
  print F &htmlizee($PackageAbstract);
  if( !$TocInserted ){
    print F $toc;
    }
  print F "</TD></TR></TABLE>\n";
  print F "</FONT></BODY>\n</HTML>\n";
  close F;
  }

print TEXI "\@bye\n" if $TexiOutput;
close TEXI if $TexiOutput;

exit;

sub htmlizee {
  $TocInserted = 0;
  my $line = shift;
  $line =~ s/\</&lt;/g;

  my @line = split(/\n/,$line);
  my $verbatim = 0;
  for $line ( @line ){
    if( $line =~ /^\s*=verbatim\s*$/ ){
      die 'verbatim nested' if $verbatim;
      $verbatim = 1;
      $line = "<FONT SIZE=$TTFontSize><PRE>";
      next;
      }
    if( $line =~ /^\s*=noverbatim\s*$/ ){
      die 'noverbatim W/O verbatim' unless $verbatim;
      $verbatim = 0;
      $line = '</PRE></FONT>';
      next;
      }
    if( !$verbatim ){
      $line =~ s/H\&lt\;([^>]*)\>/<p><font size=$TitleFontSize><b>$1<\/b><\/font><p>/g;
      $line =~ s/B\&lt\;([^>]*)\>/<B>$1<\/B>/g;
      $line =~ s/I\&lt\;([^>]*)\>/<I>$1<\/I>/g;
      $line =~ s/T\&lt\;([^>]*)\>/<font size=$TTFontSize><TT>$1<\/TT><\/font>/g;
      $line =~ /R\&lt\;([^>]*)\>/;
      my $ref = $1;
      if( $ref =~ m#/# ){
        if( $ref =~ m#/\s*$# ){ $ref =~ s#/\s*$##; $ref .= "/$MainName" }
        $line =~ s/R\&lt\;([^>]*)\>/<A HREF=\"..\/$ref\.$HtmlExtension\">$1<\/A>/g;
        }else{
        $line =~ s/R\&lt\;([^>]*)\>/<A HREF=\"$1\.$HtmlExtension\">$1<\/A>/g;
        }
      }
    if( !$verbatim && $line =~ /^\s*=itemize\s*$/ ){
      $line = '<UL>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*\=item\s*(.*)/ ){
      $line = '<LI>' . $1;
      next;
      }
    if( !$verbatim && $line =~ /^\s*=noitemize\s*$/ ){
      $line = '</UL>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=hrule\s*$/ ){
      $line = '<HR>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=italic\s*$/ ){
      $line = '<I>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=noitalic\s*$/ ){
      $line = '</I>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=bold\s*$/ ){
      $line = '<B>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=nobold\s*$/ ){
      $line = '</B>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=center\s*$/ ){
      $line = '<CENTER>';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=nocenter\s*$/ ){
      $line = '</CENTER>';
      next;
      }
    if( length($line) == 0 && !$verbatim ){
      $line ='<P>';
      next;
      }
    if( $line =~ /^\s*=toc\s*$/  && !$verbatim){
      $line = $toc;
      $TocInserted = 1;
      next;
      }
    }
  $line = join("\n",@line);
  return $line;
  }

sub texizee {
  $TocInserted = 0;
  my $line = shift;

  my @line = split(/\n/,$line);
  my $verbatim = 0;
  for $line ( @line ){
    $line =~ s/\</&lt;/g;
    unless( $verbatim ){
      $line =~ s/\\/\\\\/g;
      $line =~ s{\$}{\\\$}g;
      $line =~ s/\_/\\_/g;
      $line =~ s/%/\\%/g;
      $line =~ s/#/\\#/g;
      }
    if( $line =~ /^\s*=verbatim\s*$/ ){
      die 'verbatim nested' if $verbatim;
      $verbatim = 1;
      $line = "\@example\n";
      next;
      }
    if( $line =~ /^\s*=noverbatim\s*$/ ){
      die 'noverbatim W/O verbatim' unless $verbatim;
      $verbatim = 0;
      $line = "\@end example\n";
      next;
      }
    if( !$verbatim ){
      $line =~ s/H\&lt\;([^>]*)\>/\@section $1\n/g;
      $line =~ s/B\&lt\;([^>]*)\>/\@b{$1}/g;
      $line =~ s/I\&lt\;([^>]*)\>/\@emph{$1}/g;
      $line =~ s/T\&lt\;([^>]*)\>/\@code{$1}/g;
      $line =~ s/R\&lt\;([^>]*)\>/\@xref{$1}/g;
      }
    if( !$verbatim && $line =~ /^\s*=itemize\s*$/ ){
      $line = "\@itemize\n";
      next;
      }
    if( !$verbatim && $line =~ /^\s*\=item\s*(.*)/ ){
      $line = "\@item $1\n";
      next;
      }
    if( !$verbatim && $line =~ /^\s*=noitemize\s*$/ ){
      $line = "\@end itemize\n";
      next;
      }
    if( !$verbatim && $line =~ /^\s*=hrule\s*$/ ){
      $line = "\n";
      next;
      }
    if( !$verbatim && $line =~ /^\s*=italic\s*$/ ){
      $line = '\@emph{ ';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=noitalic\s*$/ ){
      $line = '}';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=bold\s*$/ ){
      $line = '@b{';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=nobold\s*$/ ){
      $line = '}';
      next;
      }
    if( !$verbatim && $line =~ /^\s*=center\s*$/ ){
      $line = "\n";
      next;
      }
    if( !$verbatim && $line =~ /^\s*=nocenter\s*$/ ){
      $line = "\n";
      next;
      }
    if( length($line) == 0 && !$verbatim ){
      $line = "\n";
      next;
      }
    if( $line =~ /^\s*=toc\s*$/  && !$verbatim){
      $line = '';
      next;
      }
    $line =~ s/\&lt;/\</g;
    }
  $line = join("\n",@line);
  return $line;
  }

__END__
=pod
=section preprocess
=H Usual preprocess commands
=abstract
Without knowing Perl, here is the cookbook for some programming languages,
how to set T<StartString>, T<EndString> and T<PreprocessComment>
=end
You usually want normal comments in your source file that should remain unnoticed by
esd and other comments that are part of the documentation. Therefore I recommend that you
use a start string like:
=itemize
=item T<{POD> for PASCAL
=item T</*COD> for C
=item T<C FOD> for FORTRAN
=noitemize

The end string can be
=itemize
=item T<EPOD}> for PASCAL
=item T<ECOD*/> for C
=item T<C EFOD> for FORTRAN
=noitemize

PASCAL and C needs no preprocess command as the comment lines can contain the documenation
in the format required by esd. However languages like FORTRAN or some BASIC dialects require
each comment lines starting with special strings. Fortran comments should start with a
C character in the first position. This should be removed before processing embedded source
documentation. The line in the sdd file 
=verbatim
PreprocessComment = 's/^C//'
=noverbatim
should process each documentation comment line and remove the leading C before processing for
html generation. The FORTRAN files, of course remain intact.

=section code
=H Generating output code

If you use extensive commenting in an interpretive language like Perl, and you program
runs many times the intrerpretation phase slow-down caused by the comments can result
significant performance degradation.

In other situation you might have a language that does not allow comments at all.

In such situations you can use esd2html converter to generate version of your script that
is compact, does not include unneccessary formatting and comment.

If you define T<CodeRoot> in the SDD file the program will copy each script to the directory
performing several conversions during the copy.

First of all only the lines that are not processed as esd comment will be copied. All lines that
should be skipped will be skipped and each line will be converted using the commands given in the SDD file.

The string defined in the SDD file using the keyword T<PreprocessLine> will be evaluated for each line
which does not belong to the esd comments. The string is evaluated as a Perl command, while the source line
is in T<$_>. If there are more than one T<PreprocessLine> string defined each will be evaluated in the order
they are specified in the SDD file.

The keyword T<StripLine> defines Perl commands to decide whether a line should be stripped off the script
or not. You can define many T<StripLine> commands. They will be evaluated in the order they are specified.
If any of them is found true the rest of the T<StripLine> commands will be skipped together with the source
line. Note that the T<StripLine> string are evaluated the same way as T<PreprocessLine> strings and can
therefore modify the output line modifying the Perl variable T<$_>.

Here are some sample T<PreprocessLine> and T<StripLine> strings with explanations:

=verbatim
PreprocessLine = 's/^\s*//'                   delete spaces from front of each line
PreprocessLine = 's/\s*$//'                   delete trailing spaces
StripLine = '/^\s*#/'                         skip Perl comments
StripLine = '/^\s*REM\s+/i'                   skip non empty BASIC comments
StripLine = '/^\s*REM$/i'                     skip empty BASIC comments
StripLine = '/^\s*$/'                         skip empty lines
=noverbatim

 
=cut
