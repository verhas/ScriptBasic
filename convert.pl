#!/usr/bin/perl

#
# convert.pl
# This small utility opens all files in the current directory and below recirsively and
# converts the files to contain UNIX line-feeds. This is needed when the files are
# copied from a DOS development station to to UNIX via SMB.
#
# The conversion does not harm files that are already UNIX LF.
#

@BINARY = (
  'o','a','dll','lib','exe','out','so','conf','conf_old','doc','deb','gif','sln','ico','rc','old',
  );
@ASCII = (
  'c','h','txt','h_bas','bat','lsp','heb','bas','pl','html','def','bas',
  'mak','inc','ini','tpl','sh','sdd','jam','jim','cygwin','OSF','solaris','cmd','d-sbhttpd',
  'fn','log','nt','cc','ssplit','pm','sb','tmpl','TXT','cpp','ziplist','exziplist',
  );

%BADS = [];

#
# decide if a file is binary or ascii based on the file name extension
# if the file has no extension that it opens the file and reads the first line
# if the file starts with #!/ then this is text othervise binary file
#
sub isbinary {
  my $fn = shift;
  my $i;

  if(  $fn !~ /\./ ){
    #if there is no extension in the file name at all
    return 0 if $fn =~ /^Makefile$/;
    open(F,$fn) or die "cannot open \n$fn\n for reading to decide file type";
    my $l = <F>;
    close F;
    chomp $l;
    if( $l =~ /^\#\!\s*\// ){
      return 0;
      }
    return 1;
    }

  for $i ( @BINARY ){
    return 1 if substr($fn,length($fn)-length($i)-1) eq ".$i";
    }
  for $i ( @ASCII ){
    return 0 if substr($fn,length($fn)-length($i)-1) eq ".$i";
    }
  $fn =~ /\.(.*)$/;
  $BADS{$1}++;
  return -1 if $BADS{$1} == 1;
  return 0;
  }

opendir(D,'.');
@f = readdir(D);
closedir(D);
@DirsLeft = ();
$qdir = '';
while(1){
  $qdir = $qdir . '/' if $qdir;
  for( @f ){
    if( -d "$qdir$_" ){
      push @DirsLeft , "$qdir$_" unless $_ eq '.' || $_ eq '..';
      }else{
      push @filist , "$qdir$_";
      }
    }
  last if $#DirsLeft == -1;
  $qdir = pop @DirsLeft;
  opendir(D,"$qdir");
  @f = readdir(D);
  closedir(D);
  }

$badcount = 0;
for $i ( @filist ){
  $t = isbinary($i);
  if( $t == -1 ){
    $badcount++;
    $i =~ /\.(.*)$/;
    print "$1 is unknown type\n";
    }
  }

for $i ( @filist ){
  if( ! isbinary($i) ){
    # get the file time (mtime)
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($i);
    open(F,$i) or die "cannot read $i for converting\n";
    $ofile = '';
    $file = '';
    while( <F> ){
      $ofile .= $_;
      while( /\r$/ || /\n$/ ){chop;}
      $file .= "$_\n";
      }
    if( $ofile ne $file ){
      open(F,">$i") or die "cannot write the file $i for converting\n";
      print F $file;
      close F;
      utime $atime,$mtime,$i;
      chmod 0400,$i;
      }
    }
  }
my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat('../convert.pl');
utime $atime,$mtime,'convert.pl';
chmod 0500,'setup';
exit 0;
