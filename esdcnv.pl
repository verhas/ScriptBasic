$prefix = shift;

while( <> ){
  next if /^=H/ ;
  if( /^=section\s+(.*)/ ){
    print "=H ${prefix}_$1()\n";
    next;
    }
  s/R\<(.*?)\>/R<${prefix}_$1()>/g;
  print;
  }
