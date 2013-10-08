# read a C file and create a header file from it
#
# also create the .bas header file from the ScriptBasic module interface file
#
#  Lines processed: HEADER: headerfilename
#                   FILE:   name of the file (used to create header file name unless HEADER is defined)
#                   TO_HEADER:
#                      lines until a single */ on a line are put to the header file
#
#                   /*FUNCTION*/
#                   lines are put to the header file until a single { is found on a line
#                   ; is added after these lines
#
#                   GLOBAL declaration
#                   is put to the header file replcing 'GLOBAL' to 'extern'
#                   define GLOBAL to nothing in the source file
#

START_HERE:
$file = shift;
exit unless defined $file;

$input_file_name    = '';

$header_file_name   = '';
$header_file_opened = 0;
$header_file        = undef;

$bas_file_name      = '';
$bas_file_opened    = 0;
$bas_file           = undef;

$podon = 0;

open(F,"<$file") or die "Can not open the input file $file";
$header_content = '';
$bas_content    = '';
@bas_subs = ();
@bas_coms = ();

while( defined( $_ = <F>) ){
  chomp;

  if( /^\s*FILE\s*:\s*(\S+)/ ){
    $input_file_name = $1;
    next;
    }

  if( /^\s*HEADER\s*:\s*(\S+)/ ){
    $header_file_name = $1;
    next;
    }

  if( /^\s*BAS\s*:\s*(\S+)/ ){
    $bas_file_name = $1;
    $module_name = lc $bas_file_name;
    $module_name =~ s/\..+$//;
    next;
    }

  if( /^\s*TO_HEADER\s*:/ ){
    &open_header_file;
    $podon = 0;               # no =POD is on by default
    while( <F> ){
      chomp;
      if( m{^\s*=POD\s*$} ){ # start POD
        $podon = 1;
        next;
        }
      if( m{^\s*=CUT\s*$} ){ # finish POD
        $podon = 0;
        next;
        }
      last if m{^\s*\*/\s*$}; # finish copiing when the */ is reached
      if( m{^(.*?)//(.*)} ){   # chop off // comments
        $_ = $1;
        }
      s/\\\s*$/\\/;           # delete trailing space after \
      $header_content .= "$_\n" unless $podon;
      }
    next;
    }

  if( /^\s*TO_BAS\s*:/ ){
    &open_bas_file;
    $podon = 0;               # no =POD is on by default
    while( <F> ){
      chomp;
      if( m{^\s*=POD\s*$} ){ # start POD
        $podon = 1;
        next;
        }
      if( m{^\s*=CUT\s*$} ){ # finish POD
        $podon = 0;
        next;
        }
      last if m{^\s*\*/\s*$}; # finish copiing when the */ is reached
      if( m{^(.*?)//(.*)} ){   # chop off // comments
        $_ = $1;
        }
      s/\\\s*$/\\/;           # delete trailing space after \
      $bas_content .= "$_\n" unless $podon;
      }
    next;
    }
  if( m{^\s*=section\s+(\w+)\s*$} ){
    $basic_function_name = $1;
    next;
    }

  if( m{^\s*\.function\s+(\w+)\s*$} ){
    $basic_function_name = $1;
    next;
    }

  if( m{^\s*besFUNCTION\((\w+)\)\s*$} ){
    $module_function_name = $1;
    &open_bas_file;
    push @bas_subs , [ "$basic_function_name" , "$module_function_name" ];
    next;
    }

  if( m{^\s*besCOMMAND\((\w+)\)\s*$} ){
    $module_function_name = $1;
    &open_bas_file;
    push @bas_coms , [ "$basic_function_name" , "$module_function_name" ];
    next;
    }

  if( m{^\s*/\*FUNCTION\*/\s*$} ){
    &open_header_file;
    # insert /*FUNDEF*/before the function prototype to allow other tools to
    # easily parse the generated header file and find the function definitions
    $header_content .= "/*FUNDEF*/\n";
    while( <F> ){
      chomp;
      last if m{^\s*\)\s*\{\s*$};
      $header_content .= "\n$_";
      }
    $header_content .= ");\n";
    # insert /*FEDNUF*/ after the function prototype to allow other tools to
    # easily parse the generated header file and find the function definitions
    $header_content .= "/*FEDNUF*/\n";
    next;
    }
  }

$header_content .= <<END;
#ifdef __cplusplus
}
#endif
#endif
END

close F;


# create the BASIC function declarations that were colected

$max_bas_function_name_len = 0;
$max_c_function_name_len   = 0;

for $sub ( @bas_subs ){
  $max_bas_function_name_len = length( $sub->[0] ) if length( $sub->[0] ) > $max_bas_function_name_len;
  $max_c_function_name_len   = length( $sub->[1] ) if length( $sub->[1] ) > $max_c_function_name_len;
  }

for $sub ( @bas_coms ){
  $max_bas_function_name_len = length( $sub->[0] ) if length( $sub->[0] ) > $max_bas_function_name_len;
  $max_c_function_name_len   = length( $sub->[1] ) if length( $sub->[1] ) > $max_c_function_name_len;
  }

$bas_declares = '';
$bas_declares .= "' FUNCTION DECLARATIONS \n" if $#bas_subs > -1;

for $sub ( @bas_subs ){
  #                 declare command xxx
  $bas_declares .= "declare sub     ::" .
                         $sub->[0] .       ' ' x ($max_bas_function_name_len - length($sub->[0])) .
                   " alias " .
                   '"' . $sub->[1] . '"' . ' ' x ($max_c_function_name_len   - length($sub->[1])) .
                   " lib \"$module_name\"\n";
  }

$bas_declares .= "\n' COMMAND DECLARATIONS \n" if $#bas_coms > -1;

for $sub ( @bas_coms ){
  $bas_declares .= "declare command ::" .
                         $sub->[0] .       ' ' x ($max_bas_function_name_len - length($sub->[0])) .
                   " alias " .
                   '"' . $sub->[1] . '"' . ' ' x ($max_c_function_name_len   - length($sub->[1])) .
                   " lib \"$module_name\"\n";
  }

$bas_content .= $bas_declares;

$bas_content .= "\nend module\n";

if( $header_file_opened ){
  if( open(H,"<$open_header_file_name") ){
    # check if the file is identical
    my $oldsep = $/; undef $/;
    $q = <H>;
    close H;
    $/ = $oldsep;
    }
  if( $q ne $header_content ){
    open(H,">$open_header_file_name") or die "Can not open header file $open_header_file_name";
    print H $header_content;
    close H;
    }
  }

if( $bas_file_opened ){
  if( open(H,"<$open_bas_file_name") ){
    # check if the file is identical
    my $oldsep = $/; undef $/;
    $q = <H>;
    close H;
    $/ = $oldsep;
    }
  if( $q ne $bas_content && $open_bas_file_name ){
    open(H,">$open_bas_file_name") or die "Can not open bas file $open_bas_file_name";
    print H $bas_content;
    close H;
    }
  }

goto START_HERE;

sub open_header_file {

  return if $header_file_opened;

  $header_file_opened = 1;

  if( ! $header_file_name && ! $input_file_name ){
    $header_file_name = $file;
    $header_file_name =~ s/\.\w+$/.h/;
    }
  if( ! $header_file_name ){
    $header_file_name = $input_file_name;
    $header_file_name =~ s/\.\w+$/.h/;
    }
  if( ! $header_file_name ){
    die "No header file name.";
    }

  # modify the header name so that it is created in the same directory as the source
  my $dir = $file;
  $dir =~ s/\\/\//g; # leaning toothpicks effect :-) (convert \ to / for Win32 users)
  if( $dir =~ s/\/[^\/]+$// ){
    $open_header_file_name = "$dir/$header_file_name";
    }else{
    $open_header_file_name = $header_file_name;
    }
  $header_symbol = uc $header_file_name;
  $header_symbol =~ s{^.*/}{};
  $header_symbol = '__'.$header_symbol.'__';
  $header_symbol =~ s/\./_/g;
  $header_content .= <<END;
/*
$header_file_name
*/
#ifndef $header_symbol
#define $header_symbol 1
#ifdef  __cplusplus
extern "C" {
#endif
END
  }

sub open_bas_file {

  return if $bas_file_opened;

  $bas_file_opened = 1;

  return if ! $bas_file_name ;

  # modify the bas name so that it is created in the same directory as the source
  my $dir = $file;
  $dir =~ s/\\/\//g; # leaning toothpicks effect :-) (convert \ to / for Win32 users)
  if( $dir =~ s/\/[^\/]+$// ){
    $open_bas_file_name = "$dir/$bas_file_name";
    }else{
    $open_bas_file_name = $bas_file_name;
    }

  $bas_content .= <<END;
' """
FILE: $bas_file_name

This is the BASIC import file for the module $module_name.

This file was generated by headerer.pl from the file $input_file_name
Do not edit this file, rather edit the file $input_file_name and use
headerer.pl to regenerate this file.
"""

module $module_name

END
  }
