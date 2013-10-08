open(F,">stresstest.bas") or die "cannot open test.bas";

print F "a = 1";
for( $i=0 ; $i < 100000 ; $i++ ){
  print F "+1";
  }
print F "\n";
print F "print a,\"\\n\"";
close F;
