opendir(D,".");
@f = grep /\.jam$/ , readdir D;
closedir D;
for( @f ){
  $src = $_;
  $res = $_;
  $res =~ s/\.jam$//;
  print "Processing $src ...\n";
  `perl ..\\jamal.pl -m macros.jim $src ../../html/$res`;
  }
