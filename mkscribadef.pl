open(F,"scriba.c") or die "Can not open scriba.c";
open(OUT,">scriba.def") or die "Can not output scriba.def";
print OUT <<END;
LIBRARY     scriba
DESCRIPTION "ScriptBasic Library"
EXPORTS
END

while( defined( $line = <F>) ){
  if( $line =~ /\/\*FUNCTION\*\// ){
    $line = <F>;
    $line =~ /\w+\s+(\w+)/;
    print OUT "$1\n";
    }
  }
close OUT;
close F;
