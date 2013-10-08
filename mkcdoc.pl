#use strict;

my %SECTION;
my %COMMAND;
my %TITLE; # the title line of the command
my %SUBTITLE; # the subtitle of the command
my %DISPLAY; # the display of the command in the TOC
my @files;
my $SourceFile;

opendir(D,"commands") or die "Can not open directory commands.";
@files = readdir(D);
closedir D;

for $SourceFile ( @files ){
  &ProcessFile( $SourceFile );
  }

my %SAVE_SECTION = %SECTION;
my %SAVE_COMMAND = %COMMAND;

&CreateOutput;

%SECTION = %SAVE_SECTION;
%COMMAND = %SAVE_COMMAND;

&CreateTexiOutput;
exit;

sub ProcessFile {
  my $file = shift;
  my $line;
  my $ActualCommand = undef;
  my @sections;
  my $section;

  return unless open(F,"commands/$file");
  while( defined($line = <F>) ){

    # /**SectionName starts a new command

    if( $line =~ /^\/\*\*(.*)\s*$/ ){
      if( defined($ActualCommand) ){
        warn "The command $ActualCommand was not closed. Closing implicitly.";
        $ActualCommand = undef;
        }
      $ActualCommand = $1;
      next;
      }

    next if ! defined $ActualCommand;

    if( $line =~ /^\*\// ){
      $ActualCommand = undef;
      next;
      }

    # =section list of sections the command has to be listed in

    if( $line =~ /^=section\s+(.*)/ ){
      my @sections = split /\s+/ , $1;
      for $section ( @sections ){
        push @{$SECTION{$section}},$ActualCommand;
        }
      next;
      }

    # =title title line of the actual command

    if( $line =~ /^=title\s+(.*)/ ){
      if( defined($TITLE{$ActualCommand}) ){
        my $err;
        $err = "Title is double defined for $ActualCommand\n" .
               "   " . $TITLE{$ActualCommand} . "\n" .
               "   " . $1 . "\n" ;
        warn $err;
        }
      $TITLE{$ActualCommand} = $1;
      next;
      }

    # =subtitle title line of the actual command

    if( $line =~ /^=subtitle\s+(.*)/ ){
      if( defined($SUBTITLE{$ActualCommand}) ){
        warn "Subtitle is double defined for $ActualCommand";
        }
      $SUBTITLE{$ActualCommand} = $1;
      next;
      }

    # =display title line of the actual command

    if( $line =~ /^=display\s+(.*)/ ){
      if( defined($DISPLAY{$ActualCommand}) ){
        warn "Title is double defined for $ActualCommand";
        }
      $DISPLAY{$ActualCommand} = $1;
      next;
      }

    push @{$COMMAND{$ActualCommand}},$line;
    next;
    }
  close F;
  }

sub CreateOutput {
  my $command;
  my $lines;
  my @commands;
  my $section;
  my @sections;

  @commands = sort keys %COMMAND;
  @sections = sort keys %SECTION;

  mkdir "../html";
  open(F,">../html/commands.html") or die "Can not open output file.";
  print F <<END;
<HTML>
<HEAD>
<TITLE></TITLE>
</HEAD>
<BODY>
<H1>ScriptBasic commands and functions reference</H1>

<H2>List of Sections</H2>
END

  for $section ( @sections ){
    print F "<a href=\"#section_$section\">$section</A>\n";
    }

print F <<END;
<H2>List of Commands</H2>
<FONT SIZE="1">
END

  for $command ( @commands ){
    my $display;
    $display = $command;
    $display = $DISPLAY{$command} if defined $DISPLAY{$command};
    print F "<a href=\"#command_$command\">$display</A>\n";
    }

print F <<END;
</FONT>
<H2>List of Commands by Sections</H2>
END

  for $section ( @sections ){
    print F "<H3><a name=\"section_$section\">$section</A></H3>\n";
	print F "<FONT SIZE=\"1\">\n";
    my @scommands = sort @{$SECTION{$section}};
    my $scommand;
    for $scommand (@scommands){
      my $display;
      $display = $scommand;
      $display = $DISPLAY{$scommand} if defined $DISPLAY{$scommand};
      print F "<a href=\"#command_$scommand\">$display</A>\n";
      }
    print F "</FONT\>\n";
    }

print F <<END;
<H2>Commands</H2>
END

  for $command ( @commands ){
    my $title = $command;
    my $line;
    my $verbatim = 0;
    my $subtitle;
    my $FH;

    $title = $TITLE{$command} if defined $TITLE{$command};
    $subtitle = undef;
    $subtitle = $SUBTITLE{$command} if defined $SUBTITLE{$command};

    #
    # print lines that start a command
    #

    print F "<H3><a name=\"command_$command\">$title</A></H3>\n";
    print F "<H4>$subtitle</H4>\n" if defined $subtitle;
    print F "<BLOCKQUOTE>\n";
    $FH = 0;
    for $line ( @{$COMMAND{$command}} ){
      if( $line =~ /^\s*=details\s*$/ ){
        print F <<END;
<a href="commands/$command.html">details</A>
END
        mkdir "../html/commands",0777;
        open(DF,">../html/commands/$command.html") or die "Can not open file ../html/commands/$command.html";
        $FH = 1;
        print DF <<END;
<HTML>
<HEAD>
<TITLE>$command</TITLE>
</HEAD>
<BODY>
<H1>$title</H1>
<a href="../commands.html#command_$command">BACK</a><P>
END
        next;
        }
      if( $line =~ /^\s*=verbatim\s*$/ ){
        $verbatim++;
  	if( $FH ){
          print DF "<PRE>\n";
  	  }else{
          print F "<PRE>\n";
  	  }
        next;
        }
      if( $line =~ /^\s*=noverbatim\s*$/ ){
        $verbatim--;
  	if( $FH ){
          print DF "</PRE>\n";
  	  }else{
          print F "</PRE>\n";
  	  }
        next;
        }
      if( $verbatim ){
  	if( $FH ){
          print DF $line;
  	  }else{
          print F $line;
  	  }
        next;
        }

      if( $line =~ /^\s*=itemize\s*$/ ){
  	if( $FH ){
          print DF "<UL>\n";
  	  }else{
          print F "<UL>\n";
  	  }
        next;
        }
      if( $line =~ /^\s*=noitemize\s*$/ ){
  	if( $FH ){
          print DF "</UL>\n";
  	  }else{
          print F "</UL>\n";
  	  }
        next;
        }

      if( $line =~ /^\s*$/ ){
  	if( $FH ){
          print DF "<P>\n";
  	  }else{
          print F "<P>\n";
  	  }
        next;
        }

      my $l_line = $line;
      while( $l_line =~ /R\<(\w+?)\>/ ){
        my $d = $1;
        $d = $DISPLAY{$d} if defined $DISPLAY{$d};
        $l_line =~ s/R\<(\w+?)\>/\001a href=\"#command_$1\">$d\001\/a>/;
        }
      $l_line =~ s/T\<(.+?)\>/\001tt>$1\001\/tt>/g;
      $l_line =~ s/B\<(.+?)\>/\001B>$1\001\/B>/g;
      $l_line =~ s/I\<(.+?)\>/\001I>$1\001\/I>/g;

      $l_line =~ s/^\s*=item\s+/\001LI>/g;

      $l_line =~ tr{\001}{<};
      if( $FH ){
        print DF $l_line;
      }else{
        print F $l_line;
        }
      }
    #
    # print lines that close a command
    #
    print F "</BLOCKQUOTE>\n";
    if( $FH ){
	  print DF <<END;
<P><a href="../commands.html#command_$command">BACK</a>
</BODY>
</HTML>
END
	  close DF; # if it was opened
	  }
    }

  #
  # print lines that close the entire HTML file
  #
  print F <<END;
</BODY>
</HTML>
END
  close F;
  }

#################################################

sub CreateTexiOutput {
  my $command;
  my $lines;
  my @commands;
  my $section;
  my @sections;

  @commands = sort keys %COMMAND;
  @sections = sort keys %SECTION;

  open(F,">commands.texi") or die "Can not open commands.texi output file.";

  print F "\@chapter Command reference\n";

  for $command ( @commands ){
    my $title = $command;
    my $line;
    my $verbatim = 0;
    my $subtitle;
    my $FH;

    $title = $TITLE{$command} if defined $TITLE{$command};
    $subtitle = undef;
    $subtitle = $SUBTITLE{$command} if defined $SUBTITLE{$command};

    #
    # print lines that start a command
    #

    print F "\n\@section $title\n\n";
    print F "\@b{$subtitle}\n" if defined $subtitle;
    for $line ( @{$COMMAND{$command}} ){
      if( $line =~ /^\s*=details\s*$/ ){
        print F "\@subsection $command Details\n";
        next;
        }
      if( $line =~ /^\s*=verbatim\s*$/ ){
        $verbatim++;
        print F "\@example\n";
        next;
        }
      if( $line =~ /^\s*=noverbatim\s*$/ ){
        $verbatim--;
        print F "\@end example\n";
        next;
        }
      if( $verbatim ){
        print F $line;
        next;
        }

      if( $line =~ /^\s*=itemize\s*$/ ){
        print F "\@itemize\n";
        next;
        }
      if( $line =~ /^\s*=noitemize\s*$/ ){
        print F "\@end itemize\n";
        next;
        }

      if( $line =~ /^\s*$/ ){
        print F "\n\n";
        next;
        }

      my $l_line = $line;
      $l_line =~ s[\<tt\>][\@code\{]gi;
      $l_line =~ s[\<\/tt\>][\}]gi;
      $l_line =~ s[<UL>][\@itemize ]gi;
      $l_line =~ s[</UL>][\@end itemize ]gi;
      $l_line =~ s[<LI>][\@item ]gi;

      $l_line =~ s/\@/\@\@/g;
      $l_line =~ s/\$/\@\$/g;
      $l_line =~ s/\{/\@\{/g;
      $l_line =~ s/\}/\@\}/g;
      $l_line =~ s[<a href=".*?">(.*?)</a>][\@xref{$1}]gi;
      $l_line =~ s/R\<(.+?)\>/\@xref\{$1\}/g;
      $l_line =~ s/T\<(.+?)\>/\@code\{$1\}/g;
      $l_line =~ s/B\<(.+?)\>/\@b\{$1\}>/g;
      $l_line =~ s/I\<(.+?)\>/\@emph\{$1\}>/g;

      $l_line =~ s/^\s*=item\s+/\@item /g;

      print F $l_line;
      }
    }

  close F;
  }


