open(F,">build.cmd") or die "Can not open build.cmd\n";
opendir(D,"./progs");
@f = grep /\.bas$/ , readdir D;
closedir D;
for( @f ){
  /(.*)\.bas$/;
  if( -e "progs/$1.txt" ){
    print F "scriba progs/$_  < progs/$1.txt > output/$1.txt\n";
    }else{
    print F "scriba progs/$_ > output/$1.txt\n";
    }
  }
opendir(D,".");
@f = grep /\.jam$/ , readdir D;
closedir D;
print F "mkdir ..\\..\\..\\html\\examples\n";
print F "mkdir output\n";
for( @f ){
  print F "perl jamal.pl -m macros.jim $_ ../../../html/examples/";
  s/\.jam$//;
  print F "$_\n";
  }
close F;
