#! /usr/bin/perl
#
# This program converts a syntax definition to C program language tables.
# The tables are used by lexer.c and expression.c
#
#
$IN_FILE = shift;
$IN_FILE = "syntax.def" unless $IN_FILE;
open(F,$IN_FILE) or die "Can not open ${IN_FILE}";

$MAX_GO_CONSTANTS = 3;

$CFILE_N = "syntax.c";

@COMMANDS = ();
%COMMANDS = ();
@FUNCTIONS = ();
@BINARIES = ();
@UNARIES = ();
@NASYMBOLS = ();
@ASYMBOLS = ();
%ANASYMBOLS = ();
%NOEXEC = ();
%SYMBOL = ();
@CONSTS = ();
$SymbolCounter = 256; ## this is where the command symbols start coding
$Precedence = 1;


while( <F> ){
  chomp;
  next if /^#/ ;#skip comment lines
  next if /^\s*$/;#skip empty lines

  if( /^\s*\%\s*NUMSTART\s+(\S+)\s*$/ ){
    $SymbolCounter = $1;
    next;
    }

  if( /^\s*\%\s*CFILE\s+(\S+)\s*$/ ){
    $CFILE_N = $1;
    next;
    }

  if( /^\s*\%\s*FILE\s+(\S+)\s*$/ ){
    $CFILE_N = "$1.c";
    $HFILE_N = "$1.h";
    next;
    }

  if( /^\s*\%\s*COMMANDS\s*$/ ){
    &do_commands;
    redo;
    }

  if( /^\s*\%\s*UNARIES\s*$/ ){
    &do_unaries;
    redo;
    }

  if( /^\s*\%\s*BINARIES\s*$/ ){
    &do_binaries;
    redo;
    }

  if( /^\s*\%\s*FUNCTIONS\s*$/ ){
    &do_functions;
    redo;
    }

  }
close F;

$CFILEC = '';
&calculate_command_codes;
&collect_symbols;
&correct_commands;

&print_cfile_start;
&print_hfile_start;

&print_command_defines;
&print_binaries;
&print_unaries;
&print_function_table;
&print_command_table;
&print_nasymbols;
&print_asymbols;
&print_csymbols;
&print_externs;
&print_command_function_table;
&print_command_symbol_table;
&print_consts;

&print_cfile_end;
&print_hfile_end;

&print_notimp_table;

# get the old file and write syntax.c only if there is something changed
if( open(CFILE,"<${CFILE_N}") ){
  undef $/;
  $OCFILEC = <CFILE>;
  close CFILE;
  exit if $OCFILEC eq $CFILEC;
  }

open(CFILE,">${CFILE_N}") or die "Cannot open ${CFILE_N}";
print CFILE $CFILEC;
close CFILE;
exit;

sub do_unaries { 
  while( <F> ){
    chomp;
    return if /^\s*\%/;
    next if /^#/ ;#skip comment lines
    next if /^\s*$/;#skip empty lines
    next if &constant_line;

    my ($OperatorCodeId,$OperatorString) = /(\w+):\s*(\S+)\s*$/;
    $COMMANDS{$OperatorCodeId} = 1;
    push @UNARIES , { OPCODE => $OperatorCodeId ,
                      NAME => $OperatorString };
    }
  }

sub do_binaries {
  while( <F> ){
    chomp;
    next if /^#/ ;#skip comment lines
    next if /^\s*$/;#skip empty lines
    next if &constant_line;

    if( /^\s*%\s*PRECEDENCE\s*$/ ){
      $Precedence++;
      next;
      }
    return if /^\s*\%/;

    my ($OperatorCodeId,$OperatorString) = /(\w+):\s*(\S+)\s*$/;
    $COMMANDS{$OperatorCodeId} = 1;
    push @BINARIES , { OPCODE => $OperatorCodeId ,
                       NAME => $OperatorString ,
                       PREC => $Precedence };
    }
  }

sub do_functions {
  while( <F> ){
    chomp;
    return if /^\s*\%/;
    next if /^#/ ;#skip comment lines
    next if /^\s*$/;#skip empty lines
    next if &constant_line;

    my ($FunctionCodeId,$FunctionName,$MinArgs,$MaxArgs) = /(\w+):\s*(.+)\s+(\d+)\s+(\d+)\s*$/;
    $COMMANDS{$FunctionCodeId} = 1;
    push @FUNCTIONS , { OPCODE => $FunctionCodeId ,
                        NAME => $FunctionName ,
                        MINARGS => $MinArgs,
                        MAXARGS => $MaxArgs };
    }
  }

sub do_commands {
  while( <F> ){
    chomp;
    return if /^\s*\%/;
    next if /^#/ ;#skip comment lines
    next if /^\s*$/;#skip empty lines
    next if &constant_line;

    my ($CommandOpCodeId,$CommandSyntax) = /^([\w\/]+):\s*(.*)$/;
    my $CommandFunction;
    ($CommandOpCodeId,$CommandFunction) = split /\// , $CommandOpCodeId;
    $CommandFunction = 'This' unless $CommandFunction;
    $COMMANDS{$CommandOpCodeId} = 1;
    my @SyntaxString = split /\s+/,$CommandSyntax;
    my @Syntax = ();
    my $noexec = 0;
    for( @SyntaxString ){
      if( /\'(\W)\'/ ){
        push @Syntax , { TYPE => 'EX_LEX_CHARACTER' , VALUE => $1 };
        next;
        }
      if( /\'(.+)\'/ ){
        $SYMBOL{$1} = $SymbolCounter++ unless defined $SYMBOL{$1};
        push @Syntax , { TYPE => 'EX_LEX_NSYMBOL' ,
                         VALUE => $SYMBOL{$1} ,
                         LEXEME => $1 };
        next;
        }
      if( /^nl$/ ){
        push @Syntax , { TYPE => 'EX_LEX_CHARACTER' , VALUE => '\n' };
        next;
        }
      if( /^tab$/ ){
        push @Syntax , { TYPE => 'EX_LEX_CHARACTER' , VALUE => '\t' };
        next;
        }
      if( /^integer$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LONG' , VALUE => 0 };
        next;
        }
      if( /^label$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LABEL' , VALUE => 0 };
        next;
        }
      if( /^arg_num$/ ){
        push @Syntax , { TYPE => 'EX_LEX_ARG_NUM' , VALUE => 0 };
        next;
        }
      if( /^label_def$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LABEL_DEFINITION' , VALUE => 0 };
        next;
        }
      if( /^float$/ ){
        push @Syntax , { TYPE => 'EX_LEX_DOUBLE' , VALUE => 0 };
        next;
        }
      if( /^string$/ ){
        push @Syntax , { TYPE => 'EX_LEX_STRING' , VALUE => 0 };
        next;
        }
      if( /^absolute_symbol$/ ){
        push @Syntax , { TYPE => 'EX_LEX_ASYMBOL' , VALUE => 0 };
        next;
        }
      if( /^pragma$/ ){
        push @Syntax , { TYPE => 'EX_LEX_PRAGMA' , VALUE => 0 };
        next;
        }
      if( /^symbol$/ ){
        push @Syntax , { TYPE => 'EX_LEX_SYMBOL' , VALUE => 0 };
        next;
        }
      if( /^name_space$/ ){
        push @Syntax , { TYPE => 'EX_LEX_SET_NAME_SPACE' , VALUE => 0 };
        next;
        }
      if( /^end_name_space$/ ){
        push @Syntax , { TYPE => 'EX_LEX_RESET_NAME_SPACE' , VALUE => 0 };
        next;
        }
      if( /^expression$/ ){
        push @Syntax , { TYPE => 'EX_LEX_EXP' , VALUE => 0 };
        next;
        }
      if( /^expression_list$/ ){
        push @Syntax , { TYPE => 'EX_LEX_EXPL' , VALUE => 0 };
        next;
        }
      if( /^lval$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LVAL' , VALUE => 0 };
        next;
        }
      if( /^lval_list$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LVALL' , VALUE => 0 };
        next;
        }
      if( /^local_start$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LOCAL_START' , VALUE => 0 };
        next;
        }
      if( /^local_end$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LOCAL_END' , VALUE => 0 };
        next;
        }
      if( /^local$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LOCAL' , VALUE => 0 };
        next;
        }
      if( /^global$/ ){
        push @Syntax , { TYPE => 'EX_LEX_GLOBAL' , VALUE => 0 };
        next;
        }
      if( /^noexec$/ ){
        push @Syntax , { TYPE => 'EX_LEX_NOEXEC' , VALUE => 0 };
        $noexec = 1;
        next;
        }
      if( /^\*$/ ){
        push @Syntax , { TYPE => 'EX_LEX_STAR' , VALUE => 0 };
        next;
        }
      if( /^local_list$/ ){
        push @Syntax , { TYPE => 'EX_LEX_LOCALL' , VALUE => 0 };
        next;
        }
      if( /^global_list$/ ){
        push @Syntax , { TYPE => 'EX_LEX_GLOBALL' , VALUE => 0 };
        next;
        }
      if( /^thisfn$/ ){
        push @Syntax , { TYPE => 'EX_LEX_THIS_FUNCTION' , VALUE => 0 };
        next;
        }
      if( /^function$/ ){
        push @Syntax , { TYPE => 'EX_LEX_FUNCTION' , VALUE => 0 };
        next;
        }

      if( /^go_back\((.*)\)$/ ){
        my @whereto = split /,/,$1;
        push @Syntax , { TYPE => 'EX_LEX_GO_BACK' , VALUE => 0 , GO => \@whereto };
        next;
        }
      if( /^go_forward\((.*)\)$/ ){
        my @whereto = split /,/,$1;
        push @Syntax , { TYPE => 'EX_LEX_GO_FORWARD' , VALUE => 0 , GO => \@whereto };
        next;
        }
      if( /^come_back\((.*)\)$/ ){
        my @whereto = split /,/,$1;
        push @Syntax , { TYPE => 'EX_LEX_COME_BACK' , VALUE => 0 , GO => \@whereto };
        next;
        }
      if( /^come_forward\((.*)\)$/ ){
        my @whereto = split /,/,$1;
        push @Syntax , { TYPE => 'EX_LEX_COME_FORWARD' , VALUE => 0 , GO => \@whereto };
        next;
        }
      if( /^cname$/ ){
        push @Syntax , { TYPE => 'EX_LEX_CONST_NAME' , VALUE => 0  };
        next;
        }
      if( /^gcname$/ ){
        push @Syntax , { TYPE => 'EX_LEX_GCONST_NAME' , VALUE => 0  };
        next;
        }
      if( /^cval$/ ){
        push @Syntax , { TYPE => 'EX_LEX_CONST_VALUE' , VALUE => 0  };
        next;
        }

      print "I have found an invalid symbol: ",$_,"\n";
      die "insy";
      }
    push @COMMANDS , { OPCODE => $CommandOpCodeId , 
                       FUNCTION => $CommandFunction,
                       SYNTAX => \@Syntax };
    $NOEXEC{$CommandOpCodeId} = $noexec ;
    }
  }

sub constant_line {
  return 0 unless /^!\s*(\w+)\s+(.*)$/;
  push @CONSTS , { NAME => $1, VALUE=> $2};
  return 1;
  }

sub print_unaries {
  $CFILEC .= "unsigned long UNARIES[] = {\n";
  for $UNA ( @UNARIES ){
    $CFILEC .= 'CMD_' . $UNA->{OPCODE} . " ,\n";
    }
  $CFILEC .= " 0 };\n";
  }

sub print_binaries {
  $CFILEC .= "unsigned long BINARIES[] = {\n";
  for $BIN ( @BINARIES ){
    $CFILEC .= '  CMD_' . $BIN->{OPCODE} . ' , ' . $BIN->{PREC} . " ,\n";
    }
  $CFILEC .= " 0\n };\n";
  }

sub print_function_table {
  $CFILEC .= "BFun INTERNALFUNCTIONS[] = {\n";
  for $FUN ( @FUNCTIONS ){
    $CFILEC .= '{ CMD_' . $FUN->{OPCODE} . ' , ' . $FUN->{MINARGS} . ' , ' . $FUN->{MAXARGS} . '}, /*' . $FUN->{NAME} . "*/\n";
    }
  $CFILEC .= "  { 0, 0, 0 }\n };\n";
  }

sub correct_commands {
  for $CMD ( @COMMANDS ){
    for $SYN ( @{$CMD->{SYNTAX}} ){
      if( $SYN->{TYPE} eq 'EX_LEX_NSYMBOL' ){
        $SYN->{VALUE} = 'CMD_' . $ANASYMBOLS{$SYN->{LEXEME}} if defined $ANASYMBOLS{$SYN->{LEXEME}};
        }
      if( $SYN->{TYPE} eq 'EX_LEX_CHARACTER' ){
        if( defined $ANASYMBOLS{$SYN->{VALUE}} ){
          $SYN->{TYPE} = 'EX_LEX_NSYMBOL';
          $SYN->{VALUE} = 'CMD_' . $ANASYMBOLS{$SYN->{VALUE}};
          }else{
          $SYN->{VALUE} = "'" . $SYN->{VALUE} . "'";
          }
        }
      }
    }
  }

sub print_command_table {
  $CFILEC .= "LineSyntax COMMANDS[] = {\n";
  for $CMD ( @COMMANDS ){

    $CFILEC .= '{ CMD_' . $CMD->{OPCODE} . ' , ex_IsCommand' . $CMD->{FUNCTION} . ", \n {\n";
    for $SYN ( @{$CMD->{SYNTAX}} ){
     $CFILEC .= '  { ' . $SYN->{TYPE} . ' , ' . $SYN->{VALUE};
     if( defined $SYN->{GO} ){
       for $GOTOS ( @{$SYN->{GO}} ){
         $CFILEC .= ", CMD_$GOTOS ";
         }
       }
      $CFILEC .= " },\n";
      }
    $CFILEC .= "  { 0 ,  0 }\n }\n},\n";
    }
  $CFILEC .= "  { 0 , NULL    , { { 0 , 0}}}\n";
  $CFILEC .= "};\n";
  }

sub print_notimp_table {
  open(NF,">notimp.c") or die "Can not open notimp.c for output\n";

print NF <<END;
/* FILE: notimp.c

   This file was automatically generated by the program syntaxer.pl

   If you alter this file setting any of the symbols to 1 instead of zero
   the corresponding command will not be implemented into the next compilation
   of ScriptBasic. Running a BASIC program containing an instruction not implemented
   will raise an error than can be captured by ON ERROR GOTO unless ON ERROR GOTO is
   also unimplemented.

TO_HEADER:

END

  for $CMD ( @COMMANDS ){
    print NF "#define NOTIMP_",$CMD->{OPCODE}," 0\n"
    }
  for $CMD ( @FUNCTIONS ){
    print NF "#define NOTIMP_",$CMD->{OPCODE}," 0\n"
    }

print NF "#include \"mynotimp.h\"\n*/\n";
  close NF;
  }

sub print_command_defines {
  my $cmd,$val;
  my $startval,$endval;
  
  while( ($cmd,$val) = each %COMMANDS ){
    $startval = $val if ! defined($startval) || $val < $startval;
    $endval   = $val if ! defined($endval) || $val > $endval;
    }

  $CFILEC .= "/*\nTO_HEADER:\n";
  $CFILEC .= "#define START_CMD $startval\n";
  $CFILEC .= "#define END_CMD $endval\n";
  $CFILEC .= '#define NUM_CMD ' . ($endval-$startval+1) . "\n";
$CFILEC .= <<END;

#include "memory.h"
#include "builder.h"
#include "execute.h"

END
  while( ($cmd,$val) = each %COMMANDS ){
    $CFILEC .= "#define CMD_$cmd $val\n";
    $CFILEC .= "void COMMAND_$cmd(pExecuteObject);\n" unless $NOEXEC{$cmd};
    }
  $CFILEC .= "*/\n";
  }

sub print_command_function_table {
  my $pval = undef;
  $CFILEC .= "CommandFunctionType CommandFunction[]={\n";
  while( ($cmd,$val) = each %COMMANDS ){
    if( ! $NOEXEC{$cmd} ){
      $CFILEC .= "  COMMAND_$cmd, /* $val */\n";
      die "failed hash order: $pval $val\n" if defined($pval) && $pval +1 != $val;
      $pval = $val;
      }
    }
  $CFILEC .= "  NULL\n  };\n";
  $pval++;
  $CFILEC .= "/*\nTO_HEADER:\n#define END_EXEC $pval\n*/\n";
  }

sub print_command_symbol_table {
  my $pval = undef;
  $CFILEC .= "char *COMMANDSYMBOLS[]={\n";
  while( ($cmd,$val) = each %COMMANDS ){
    if( ! $NOEXEC{$cmd} ){
      $CFILEC .= "         \"$cmd\", /* $val */\n";
      die "failed hash order: $pval $val\n" if defined($pval) && $pval +1 != $val;
      $pval = $val;
      }
    }
  $CFILEC .= "  NULL\n  };\n";
  }


sub print_cfile_end {
  # do nothing
  }

sub print_hfile_end {

  }
sub print_cfile_start{
  $CFILEC .= <<CFILE_START_END;
/*
FILE: $CFILE_N
HEADER: $HFILE_N

This file was automatically generated from syntax.def by syntaxer.pl
You better do not edit this file. Modify syntax.def and run syntaxer.pl again.

*/
#include <stdlib.h>
#include <stdio.h>
#include "report.h"
#include "lexer.h"
#include "sym.h"
#include "expression.h"
#include "syntax.h"

CFILE_START_END

  }
sub print_hfile_start{
  $Precedence++;
  $CFILEC .= <<HFILE_START_END;
/*
TO_HEADER:
#define MAX_BINARY_OPERATOR_PRECEDENCE $Precedence
*/
HFILE_START_END

  }

sub calculate_command_codes {
  while( ($cmd,$val) = each %COMMANDS ){
    $COMMANDS{$cmd} = $SymbolCounter++ unless $NOEXEC{$cmd};
    }
  while( ($cmd,$val) = each %COMMANDS ){
    $COMMANDS{$cmd} = $SymbolCounter++ if $NOEXEC{$cmd};
    }
  }

sub is_symbol_alpha {
  my $symbol = shift;
  return $symbol =~ /^[\w\d\_\$\:]+$/;
  }

sub collect_symbols {
  for $BIN ( @BINARIES ){
    if( &is_symbol_alpha($BIN->{NAME}) ){
      push @ASYMBOLS ,$BIN->{NAME}
      }else{
      push @NASYMBOLS , $BIN->{NAME};
      }
    $ANASYMBOLS{$BIN->{NAME}} = $BIN->{OPCODE};
    }
  for $BIN ( @UNARIES ){
    if( &is_symbol_alpha($BIN->{NAME}) ){
      push @ASYMBOLS ,$BIN->{NAME}
      }else{
      push @NASYMBOLS , $BIN->{NAME};
      }
    $ANASYMBOLS{$BIN->{NAME}} = $BIN->{OPCODE};
    }
  for $BIN ( @FUNCTIONS ){
    if( &is_symbol_alpha($BIN->{NAME}) ){
      push @ASYMBOLS ,$BIN->{NAME}
      }else{
      push @NASYMBOLS , $BIN->{NAME};
      }
    $ANASYMBOLS{$BIN->{NAME}} = $BIN->{OPCODE};
    }

  # the unnamed symbols, that have string value but may not have symbolic reference
  while( ($name,$value) = each %SYMBOL ){
    if( &is_symbol_alpha($name) ){
      push @ASYMBOLS ,$name;
      }else{
      push @NASYMBOLS , $name;
      }
    }
  @NASYMBOLS = sort { length($b) cmp length($a) } @NASYMBOLS;
  @ASYMBOLS = sort { length($b) cmp length($a) } @ASYMBOLS;
  }

sub print_nasymbols {
  my %PRINTED = ();

  $CFILEC .= "LexNASymbol NASYMBOLS[] = {\n";
  for $SYM ( @NASYMBOLS ){
    next if defined $PRINTED{$SYM};
    $PRINTED{$SYM} = 1;
    $pSYM = $SYM;
    $pSYM =~ s{\\}{\\\\}g;
    $CFILEC .= '{ "' . $pSYM . '" , ';
    if( ! defined $ANASYMBOLS{$SYM} ){
      $CFILEC .= $SYMBOL{$SYM};
      }else{
      $CFILEC .= 'CMD_' . $ANASYMBOLS{$SYM};
      }
    $CFILEC .= " } ,\n";
    }
  $CFILEC .= "{ NULL, 0 }\n  };\n";

  
  }

sub print_asymbols {
  my %PRINTED = ();

  $CFILEC .= "LexNASymbol ASYMBOLS[] = {\n";
  for $SYM ( @ASYMBOLS ){
    next if defined $PRINTED{$SYM};
    $PRINTED{$SYM} = 1;
    my $kw = uc $SYM;
    $kw =~ s/\$/S/g;
    $CFILEC .= "/*\nTO_HEADER:\n#define KEYWORDCODE_$kw " .
                 ( defined $ANASYMBOLS{$SYM}  ? 'CMD_' . $ANASYMBOLS{$SYM} : $SYMBOL{$SYM} ) .
                "\n*/\n";

    $CFILEC .= '{ "' . $SYM . '" , ';
    if( ! defined $ANASYMBOLS{$SYM} ){
      $CFILEC .= $SYMBOL{$SYM};
      }else{
      $CFILEC .= 'CMD_' . $ANASYMBOLS{$SYM};
      }
    $CFILEC .= " } ,\n";
    }
  $CFILEC .= "{ NULL, 0 }\n  };\n";

  }

sub print_csymbols {
  my %PRINTED = ();

  $CFILEC .= "LexNASymbol CSYMBOLS[] = {\n";
  while( ($cmd,$val) = each %COMMANDS ){
    $CFILEC .= "{ \"$cmd\" , CMD_$cmd } , /* $val */\n";
    }
  $CFILEC .= "{ NULL, 0 }\n  };\n";
  }

sub print_consts {
  $CFILEC .= "PredLConst PREDLCONSTS[]={\n";
  while( $qw = pop @CONSTS ){
    $CFILEC .= "  { \"" . lc($qw->{'NAME'}) . "\"," . $qw->{'VALUE'} . " },\n";
    }
  $CFILEC .= "  { NULL , 0L }\n};";
  }

sub print_externs {
  $CFILEC .=<<EXTERNS_END;
/*
TO_HEADER:
extern PredLConst PREDLCONSTS[];
extern unsigned long BINARIES[];
extern unsigned long UNARIES[];
extern BFun INTERNALFUNCTIONS[];
extern LineSyntax COMMANDS[];
extern LexNASymbol NASYMBOLS[];
extern LexNASymbol ASYMBOLS[];
extern LexNASymbol CSYMBOLS[];
extern char *COMMANDSYMBOLS[];
extern CommandFunctionType CommandFunction[];
*/
EXTERNS_END

  }
