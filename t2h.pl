#! /usr/bin/perl

#
# This is a simple texi2html converter
#
# Why I wrote it?
#
# I wanted to have a small navigational TOC in front of each section and subsection.
# I have spent more than a day hunting on information how to do it using texi2html
#
# I produced this program in 2 hours. (July 26, 2001. 8:06 to 10:03am)
#
# (Later I extended it, so that took some more time.)
#
# It is not as functional as the original texi2html, but does what I want.
#
# Peter Verhas
#
#

sub getitoc {
  my $p = shift;
  my $i,$r;
  return $ITOC{$p} if defined $ITOC{$p};
  $r = $p;
  $p = lc $p;
  $p =~ s/\W//g;
  return $ITOC{$p} if defined $ITOC{$p};

  for $i (@TOCI){
    my $k = lc $TOC{$i};
    $k =~ s/\W//;
#    if( $k eq $p ){ print "Warning: $r is sloppy. It is supposed to be $TOC{$i}\n"; }
    return $i if $k eq $p;
    }

  $p = lc $r;
  $p =~ s/\s.*$//;
  $p =~ s/\W.*$//;
  for $i (@TOCI){
    my $k = lc $TOC{$i};
    $k =~ s/\s.*$//;
    $k =~ s/\W.*$//;
#    if( $k eq $p ){ "Warning: $r is sloppy. It is supposed to be $TOC{$i}\n"; }
    return $i if $k eq $p;
    }

#  print "Warning: xref $r is not defined\n";
  return undef;
  }

sub htmlizee {
  my $line = shift;
  my $filext = shift;

  $filext = 'html' unless defined $filext;

  $line =~ s[\@\@][SAVEALLDOUBLESOBAKA]g;

  $line =~ s[\&][\&amp;]g;
  $line =~ s[\<][\&lt;]g;
  $line =~ s[\>][\&gt;]g;

  $line =~ s[\@file\{(.*?)\}][\`<font size=\"3\"><tt>$1</tt></font>\']g;
  $line =~ s[\@code\{(.*?)\}][<font size=\"3\"><tt>$1</tt></font>]g;
  $line =~ s[\@var\{(.*?)\}][<font size=\"3\"><tt>$1</tt></font>]g;
  $line =~ s[\@command\{(.*?)\}][<font size=\"3\"><tt>$1</tt></font>]g;
  $line =~ s[\@acronym\{(.*?)\}][<font size=\"3\"><tt>$1</tt></font>]g;
  $line =~ s[\@b\{(.*?)\}][<B>$1</B>]g;
  $line =~ s[\@strong\{(.*?)\}][<I>$1</I>]g;
  $line =~ s[\@emph\{(.*?)\}][<I>$1</I>]g;
  $line =~ s[\@option\{(.*?)\}][\`<font size=\"3\"><tt>$1</tt></font>\']g;
  $line =~ s[\@itemize][<UL>]g;
  $line =~ s[\@end\s+itemize][</UL>]g;
  $line =~ s[\@item][<LI>]g;
  $line =~ s[\@example][<FONT SIZE="3" COLOR="BLUE"><PRE>]g;
  $line =~ s[\@end\s+example][</PRE></FONT>]g;
  $line =~ s[\@\{][\{]g;
  $line =~ s[\@\}][\}]g;
  $line =~ s[\@\$][\$]g;

  if( $filext eq '#' ){
    while( $line =~ m[\@xref\{(.*?)\}] ){
      my $itoc = &getitoc($1);
      $line =~ s[\@xref\{(.*?)\}][<a href="#$itoc">$1</A>];
      }
    }else{
    while( $line =~ m[\@xref\{(.*?)\}] ){
      my $itoc = &getitoc($1);
      $line =~ s[\@xref\{(.*?)\}][<a href="${infile}_$itoc${filext}">$1</A>];
      }
    }
  $line =~ s[\@uref\{(.*?),(.*?)\}][<a href="$1">$2</A>]g;
  $line =~ s[\@uref\{(.*?)\}][<a href="$1">$1</A>]g;
  $line =~ s[\@email\{(.*?)\}][<a href="mailto:$1">$1</A>]g;

  $line =~ s[SAVEALLDOUBLESOBAKA][\@]g;
  $line =~ s/\n\n/\n\<P\>\n/g;

  $line;
  }

$infile = shift;

# convert all windows \ to /
$infile =~ s{\\}{/}g;

$outfile = shift;

open(F,$infile) or die "Can not open input file $infile";
@lines = <F>;
close F;

$BODYSTART = <<END;
<BODY LANG="" BGCOLOR="#C7C1A7" TEXT="#000000" LINK="#0000FF" VLINK="#800080" ALINK="#FF0000">
<FONT FACE="Verdana" Size="2">
END

# for the chtm file
$HBODYSTART = <<END;
<BODY>
<FONT FACE="Verdana" Size="2">
END

$BODYEND = <<END;
</FONT>
</BODY>
END

%TOC = ();
%TOCh = ();
%ITOC = ();
@TOCI = ();
%BODY = ();
%CINDEX = ();
$level = 0;
$chapter = 0;
$section = 0;
$subsection = 0;
$subsubsection = 0;
$bodindex = undef;

for $line (@lines){

# lines that are ignored 
  if( $line =~ /^\@title\s+(.*)$/ ){
    $TITLE = &htmlizee($1);
    next;
    }

  if( $line =~ /^\@author\s+(.*)$/ ){
    $AUTHOR = &htmlizee($1);
    next;
    }

  if( $line =~ /^\@chapter\s+(.*)$/ ){
    $chapter++;
    $section = 0;
    $subsection = 0;
    $subsubsection = 0;
    $bodindex = $chapter . '.';
    $TOC{$bodindex} = $1;
    $ITOC{$1} = $bodindex;
    push @TOCI,$bodindex;
    next;
    }

  if( $line =~ /^\@section\s+(.*)$/ ){
    $section++;
    $subsection = 0;
    $subsubsection = 0;
    $bodindex = $chapter . '.' . $section . '.' ;
    $TOC{$bodindex} = $1;
    $ITOC{$1} = $bodindex;
    push @TOCI,$bodindex;
    next;
    }

  if( $line =~ /^\@subsection\s+(.*)$/ ){
    $subsection++;
    $subsubsection = 0;
    $bodindex = $chapter . '.' . $section . '.' . $subsection . '.' ;
    $TOC{$bodindex} = $1;
    $ITOC{$1} = $bodindex;
    push @TOCI,$bodindex;
    next;
    }

  if( $line =~ /^\@subsubsection\s+(.*)$/ ){
    $subsubsection++;
    $bodindex = $chapter . '.' . $section . '.' . $subsection . '.' . $subsubsection . '.';
    $TOC{$bodindex} = $1;
    $ITOC{$1} = $bodindex;
    push @TOCI,$bodindex;
    next;
    }

  if( $line =~ /^\@[cfvkt]index\s+(.*)$/ ){
    if( defined($CINDEX{$1}) ){
      $CINDEX{$1} .= "|$bodindex";
      }else{
      $CINDEX{$1} = $bodindex;
      }
    next;
    }

  $BODY{$bodindex} .= $line;
  }

if( defined( $outfile ) ){
  $infile = $outfile;
  }else{
  #cut off extension
  $infile =~ s/\..*$//;
  }

# split infile to $inpath containing the path and file name
if( $infile =~ /(.*\/)(.*)/ ){
  $inpath = $1;
  $infile = $2;
  }else{
  $inpath = './';
  }

open(FULL,">${inpath}${infile}.html") or die "Cannot open ${inpath}${infile}.html";
print FULL <<END;
<HEAD>
<TITLE>$TITLE</TITLE>

<META NAME="description" CONTENT="$TITLE: Table of Contents">
<META NAME="keywords" CONTENT="$TITLE: Table of Contents">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">
<META NAME="Generator" CONTENT="t2h.pl">

</HEAD>

$BODYSTART
<H1>$TITLE</H1>
<H3>by $AUTHOR</H3>
<A NAME="contents"><H2>Table of Contents</H2></A>
<UL>
END


$outputfile = "${inpath}${infile}_toc.html";
open(F,">$outputfile") or die "Can not open output file $outputfile";
print F <<END;
<HEAD>
<TITLE>$TITLE: Table of Contents</TITLE>

<META NAME="description" CONTENT="$TITLE: Table of Contents">
<META NAME="keywords" CONTENT="$TITLE: Table of Contents">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">
<META NAME="Generator" CONTENT="t2h.pl">

</HEAD>

$BODYSTART
<H1>$TITLE</H1>
<H3>by $AUTHOR</H3>
<H2>Table of Contents</H2>
<UL>
END
$plevel = 0;
for $toc ( @TOCI ){
  if( $toc =~ /\.1\.$/ ){
    print F "<UL>\n";
    print FULL "<UL>\n";
    }
  $level = $toc;
  $level =~ s/\d//g;
  $level = length($level);
  if( $level < $plevel ){
    print F "</UL>\n" x ($plevel - $level);
    print FULL "</UL>\n" x ($plevel - $level);
    }
  $plevel = $level;
  $tochtml = &htmlizee($TOC{$toc});
  print F <<END;
<A HREF="${infile}_${toc}html">$toc $tochtml</A><BR>
END
  print FULL <<END;
<A HREF="#$toc">$toc   $tochtml</A><BR>
END
  }
print FULL "</UL>\n" x $plevel;
print F "</UL>\n" x $plevel;
print F <<END;
$BODYEND
</HTML>
END
close F;

#
# Create title page for the compiled help file
#
$outputfile = "${inpath}${infile}_title.htm";
open(F,">$outputfile") or die "Can not open output file $outputfile";
print F <<END;
<HEAD>
<TITLE>$TITLE</TITLE>

<META NAME="description" CONTENT="$TITLE">
<META NAME="keywords" CONTENT="$TITLE">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">
<META NAME="Generator" CONTENT="t2h.pl">

</HEAD>

$HBODYSTART
<H1>$TITLE</H1>
<H3>by $AUTHOR</H3>
</UL>
$BODYEND
</HTML>
END
close F;

#
# Create contents file for the compiled help file
#
$outputfile = "${inpath}${infile}.hhc";
open(F,">$outputfile") or die "Can not open output file $outputfile";
print F <<END;
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="t2h.pl texi to html converter">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
	<param name="ImageType" value="Folder">
</OBJECT>
<UL>
END
$plevel = 0;
for $toc ( @TOCI ){
  if( $toc =~ /\.1\.$/ ){
    print F "<UL>\n";
    }
  $level = $toc;
  $level =~ s/\d//g;
  $level = length($level);
  if( $level < $plevel ){
    print F "</UL>\n" x ($plevel - $level);
    }
  $plevel = $level;
  $tochtml = &htmlizee($TOC{$toc});
  print F <<END;
<LI> <OBJECT type="text/sitemap">
<param name="Name" value="$tochtml">
<param name="Local" value="${infile}_${toc}htm">
</OBJECT>
END
  }
print F <<END;
</UL>
$BODYEND
</HTML>
END
close F;

#
# Create index file file for the compiled help file
#
$outputfile = "${inpath}${infile}.hhk";
open(F,">$outputfile") or die "Can not open output file $outputfile";
print F <<END;
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="t2h.pl texi to html converter">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
	<param name="ImageType" value="Folder">
</OBJECT>
<UL>
END
for $toc ( @TOCI ){
  $tochtml = &htmlizee($TOC{$toc});
  print F <<END;
<LI> <OBJECT type="text/sitemap">
<param name="Name" value="$tochtml">
<param name="Local" value="${infile}_${toc}htm">
</OBJECT>
END
  }

for $cindex ( keys %CINDEX ){
  $tochtml = &htmlizee($cindex);
  $tocs = $CINDEX{$cindex};
  @TOCS = split /\|/ , $tocs;
  for $toc ( @TOCS ){
    print F <<END
<LI> <OBJECT type="text/sitemap">
<param name="Name" value="$tochtml">
<param name="Local" value="${infile}_${toc}htm">
</OBJECT>
END
    }
  }

print F <<END;
</UL>
$BODYEND
</HTML>
END
close F;
#
# Create project file for the compiled chtm file
#
$outputfile = "${inpath}${infile}.hhp";
open(F,">$outputfile") or die "Can not open output file $outputfile";
print F <<END;
[OPTIONS]
Compatibility=1.1 or later
Compiled file=${infile}.chm
Contents file=${infile}.hhc
Default topic=${infile}_title.htm
Display compile progress=No
Full-text search=Yes
Index file=${infile}.hhk
Language=0x409 English (United States)
Title=$TITLE

[FILES]
${infile}_title.htm
END
$plevel = 0;
for $toc ( @TOCI ){
  $level = $toc;
  $level =~ s/\d//g;
  $level = length($level);
  $plevel = $level;
  print F <<END;
${infile}_${toc}htm
END
  }
print F <<END;
[INFOTYPES]
END
close F;

#
# Create each html and htm files
#
for( $i=0 ; $i <= $#TOCI ; $i++ ){
  $toc = $TOCI[$i];
  $outputfile = "${inpath}${infile}_${toc}html";
  open(F,">$outputfile") or die "Can not open output file $outputfile";
  $tochtml = &htmlizee($TOC{$toc});
print F <<END;
<HEAD>
<TITLE>$TITLE: $toc $tochtml</TITLE>

<META NAME="description" CONTENT="$TITLE: $tochtml">
<META NAME="keywords" CONTENT="$TITLE: $tochtml">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">
<META NAME="Generator" CONTENT="t2h.pl">

</HEAD>

$BODYSTART
END

#
# Print the navigational links from this level upward
#
$ulc = 1;
print FULL '<P><a href="#contents">[Contents]</A><BR>' ,"\n";
print F '<UL><a href="' , $infile , '_toc.html">[Contents]</A><BR>' ,"\n";
if( $toc =~ /^(\d+\.)/ ){
  $section = $1;
  if( $toc ne $section ){
    $ulc ++;
    $tochtml = &htmlizee($TOC{$section});
    print F '<UL><a href="',$infile, '_' , $section , 'html">',
            $section,' ',$tochtml,"</A><BR>\n";
    }
  }

if( $toc =~ /^(\d+\.\d+\.)/ ){
  $section = $1;
  if( $toc ne $section ){
    $ulc ++;
    $tochtml = &htmlizee($TOC{$section});
    print F '<UL><a href="',$infile, '_' , $section , 'html">',
            $section,' ',$tochtml,"</A><BR>\n";
    }
  }

if( $toc =~ /^(\d+\.\d+\.\d+\.)/ ){
  $section = $1;
  if( $toc ne $section ){
    $ulc ++;
    $tochtml = &htmlizee($TOC{$section});
    print F '<UL><a href="',$infile, '_' , $section , 'html">',
            $section,' ',$tochtml,"</A><BR>\n";
    }
  }

if( $toc =~ /^(\d+\.\d+\.\d+\.\d+\.)/ ){
  $section = $1;
  if( $toc ne $section ){
    $ulc ++;
    $tochtml = &htmlizee($TOC{$section});
    print F '<UL><a href="',$infile, '_' , $section , 'html">',
            $section,' ',$tochtml,"</A><BR>\n";
    }
  }

while( $ulc-- ){ print F "</UL>\n"; }

print F "<P>\n";
print FULL "<P>\n";

$tochtml = &htmlizee($TOC{$toc});
print F "<H1>$toc $tochtml</H1>\n";
$dotcounter = $toc;
$n = $dotcounter =~ s/\.//g;
$tochtml = &htmlizee($TOC{$toc});
print FULL "<A name=\"$toc\"><H$n>$toc $tochtml</H$n></A>\n";

if( $i > 0 ){
  $STEPLINE = '<A HREF="' . $infile . '_' . $TOCI[$i-1] . 'html">[&lt;&lt;&lt;]</A> ';
  }else{
  $STEPLINE = '[&lt;&lt;&lt;] ';
  }

if( $i < $#TOCI ){
  $STEPLINE .= '<A HREF="' . $infile . '_' . $TOCI[$i+1] . 'html">[&gt;&gt;&gt;]</A>';
  }else{
  $STEPLINE .= '[&gt;&gt;&gt;]';
  }
print F "$STEPLINE\n";

#
# Print the navigational links from this level down
#

$ulc = 1;
$plevel = $toc;
$plevel =~ s/\d//g;
$plevel = length($plevel);
$toclen = $toc;
$toclen =~ s/\d//g;
$toclen=length($toclen);

for( $j = $i+1; $j <= $#TOCI ; $j++ ){

  $section = $TOCI[$j];
  $seclen = $TOCI[$j];
  $seclen =~ s/\d//g;
  $seclen = length($seclen);
  last if $toclen >= $seclen;

  $level = $section;
  $level =~ s/\d//g;
  $level = length($level);
  if( $level > $plevel ){
    print F "<UL>\n" x ($level - $plevel);
    $ulc += $plevel - $level;
    $ulc = 0 if $ulc < 0;
   print "Warning: daingling section \"$section $TOC{$section}\"\n" if $level > $plevel+1;
   }
  if( $level < $plevel ){
    print F "</UL>\n" x ($plevel - $level);
    $ulc -= $plevel - $level;
    $ulc = 0 if $ulc < 0;
    }
  $plevel = $level;
  $tochtml = &htmlizee($TOC{$section});
  print F <<END;
<A HREF="${infile}_${section}html">$section $tochtml</A><BR>
END
  }

while( $ulc-- ){ print F "</UL>\n"; }

print F "<HR>\n";
print F "<P>\n";

#
# print the body of that part
#

@lns = split /\n/ , $BODY{$toc};
@plns = ();
while( $#lns >= 0 ){
  $line = shift @lns;

  last if $line =~ /^\s*\@bye\s+/ ;
  last if $line =~ /^\s*\@bye$/ ;
  next if $line =~ /^\s*\@c\s+/;
  next if $line =~ /^\s*\@node\s+/;
  next if $line =~ /^\s*\@c$/;
  next if $line =~ /^\s*\@cindex\s+/;
  next if $line =~ /^\s*\@cindex$/;
  next if $line =~ /^\s*\@opindex\s+/;
  next if $line =~ /^\s*\@opindex$/;
  next if $line =~ /^\s*\@vindex\s+/;
  next if $line =~ /^\s*\@vindex$/;

  if( $line =~ /^\s*\@menu\s*$/ ){
    while( $#lns >= 0 && $line !~ /^\s*\@end\s+menu\s*$/ ){ $line = shift @lns; }
    next;
    }

  push @plns , $line;
  }
$line = &htmlizee( join( "\n", @plns));

print F  $line;
$line = &htmlizee( join( "\n", @plns),'#');
print FULL $line;

  print F <<END;
<HR>
$STEPLINE
$BODYEND
</HTML>
END
close F;

#
# Create the htm file
#
  $outputfile = "${inpath}${infile}_${toc}htm";
  open(F,">$outputfile") or die "Can not open output file $outputfile";

$tochtml = &htmlizee($TOC{$toc});
print F <<END;
<HEAD>
<TITLE>$TITLE: $tochtml</TITLE>

<META NAME="description" CONTENT="$TITLE: $tochtml">
<META NAME="keywords" CONTENT="$TITLE: $tochtml">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">
<META NAME="Generator" CONTENT="t2h.pl">

</HEAD>

$HBODYSTART
END

print F "<H1>$tochtml</H1>\n";

if( $i > 0 ){
  $STEPLINE = '<A HREF="' . $infile . '_' . $TOCI[$i-1] . 'htm">[&lt;&lt;&lt;]</A> ';
  }else{
  $STEPLINE = '[&lt;&lt;&lt;] ';
  }

if( $i < $#TOCI ){
  $STEPLINE .= '<A HREF="' . $infile . '_' . $TOCI[$i+1] . 'htm">[&gt;&gt;&gt;]</A>';
  }else{
  $STEPLINE .= '[&gt;&gt;&gt;]';
  }
print F "$STEPLINE\n";

print F "<P>\n";
#
# print the body of that part
#

@lns = split /\n/ , $BODY{$toc};
@plns = ();
while( $#lns >= 0 ){
  $line = shift @lns;

  last if $line =~ /^\s*\@bye\s+/ ;
  last if $line =~ /^\s*\@bye$/ ;
  next if $line =~ /^\s*\@c\s+/;
  next if $line =~ /^\s*\@node\s+/;
  next if $line =~ /^\s*\@c$/;
  next if $line =~ /^\s*\@cindex\s+/;
  next if $line =~ /^\s*\@cindex$/;
  next if $line =~ /^\s*\@opindex\s+/;
  next if $line =~ /^\s*\@opindex$/;
  next if $line =~ /^\s*\@vindex\s+/;
  next if $line =~ /^\s*\@vindex$/;

  if( $line =~ /^\s*\@menu\s*$/ ){
    while( $#lns >= 0 && $line !~ /^\s*\@end\s+menu\s*$/ ){ $line = shift @lns; }
    next;
    }

  push @plns , $line;
  }
$line = &htmlizee( join( "\n", @plns) , 'htm');

print F  $line;

  print F <<END;
<HR>
$STEPLINE
$BODYEND
</HTML>
END
close F;

  }
close FULL;
