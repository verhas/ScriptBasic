open(F,">build.cmd") or die "Can not open build.cmd\n";
opendir(D,".");
@f = grep /\.jam$/ , readdir D;
closedir D;
for( @f ){
  print F "perl jamal.pl -m macros.jim $_ ../../html/";
  s/\.jam$//;
  print F "$_\n";
  }
close F;
