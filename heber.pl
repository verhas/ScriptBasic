# Sample ScriptBasic external preprocessor
#
# This preprocessor converts heb (Html Embedded Basic) files to BASIC
#
# <% basic program %>
# <%= basic expression %>
#
$inputfile = shift;
$outputfile = shift;

# we want to read the whole file into memory
$/ = undef;
# open the file or exit 0 telling ScriptBasic that it has failed
open(F,"<$inputfile") or exit(0);
$input = <F>;
close F;
$output = '';

while( length($input) > 0 ){
  $end = index($input,'<%');
  last if $end < 0 ;
  $append = substr($input,0,$end);
  $input = substr($input,$end+2);
  if( length($append) > 0 ){
    $append =~ s/\"/\\\"/g;
    $output .= 'print """' . $append . '"""' . "\n";
    }
  $end = index($input,'%>');
  exit 0 if $end == -1;
  $append = substr($input,0,$end);
  $input = substr($input,$end+2);
  if( length($append) > 0 ){
    if( substr($append,0,1) eq '=' ){
      $append = 'print ' . substr($append,1);
      }
    $output .= $append . "\n";
    }
  }

if( length($input) > 0 ){
  $input =~ s/\"/\\\"/g;
  $output .= 'print """' . $input . '"""' . "\n";
  }
# open the output file or exit 0 telling ScriptBasic that it has failed
open(F,">$outputfile") or exit(0);
print F $output;
close F;
exit(0);
