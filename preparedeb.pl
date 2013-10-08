#!/usr/bin/perl

open(F,"deb/DEBIAN/control") or die;
undef $/;
$control_file = <F>;
close F;
open(F,"build.txt") or die;
$build = <F>;
close F;
$build = $build +0;
$control_file =~ s/(Version\:\s+\d+\.\d+\.)\d+(\-\d+)/$1$build$2/;
open(F,">deb/DEBIAN/control") or die;
print F $control_file;
close F;
