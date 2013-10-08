opendir(D,".");
@files = grep /bas/ , readdir(D);
closedir(D);

open(F,">concat.txt");

for $f (@files){
  print F "\n\@section $f\n\n[[[ExampleAndResult $f\n";
  open(FI,$f);
  @lines =<FI>;
  close FI;
  print F @lines;
  print F "\n]]]\n";
  }
close F;

