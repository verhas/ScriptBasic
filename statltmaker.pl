#!/usr/bin/perl

#
# This program reads a module interface file and creates a
# C program syntax table that contains the module function
# search table.
#
# Do not use this simple script as an automation tool for
# code generation. Rather this is a helpful little quick and
# dirty solution to create the module function search table
# first version for already existing modules. Later maintain
# the table with great care. Edit the table into the interface
# file at the end.
#

$ModuleFile = shift;
open(F,$ModuleFile) or die "Cannot open $ModuleFile";

$ModuleFile =~ s/\.c$//;
$ModuleFile = uc $ModuleFile;

print <<END;
/* Module function search table.
*/

SLFST ${ModuleFile}_SLFST\[\] ={

END

while( <F> ){

  if( /besVERSION_NEGOTIATE/ ){
    print "{ \"versmodu\" , versmodu },\n";
    next;
    }
  if( /besSUB_START/ ){
    print "{ \"bootmodu\" , bootmodu },\n";
    next;
    }
  if( /besSUB_FINISH/ ){
    print "{ \"finimodu\" , finimodu },\n";
    next;
    }
  if( /besSUB_ERRMSG/ ){
    print "{ \"emsgmodu\" , emsgmodu },\n";
    next;
    }
  if( /besSUB_PROCESS_START/ ){
    print "{ \"_init\" , _init },\n";
    next;
    }
  if( /besSUB_PROCESS_FINISH/ ){
    print "{ \"_fini\" , _fini },\n";
    next;
    }
  if( /besSUB_KEEP/ ){
    print "{ \"keepmodu\" , keepmodu },\n";
    next;
    }
  if( /besSUB_SHUTDOWN/ ){
    print "{ \"shutmodu\" , shutmodu },\n";
    next;
    }
  if( /besSUB_AUTO/ ){
    print "{ \"automodu\" , automodu },\n";
    next;
    }

  if( /besFUNCTION\((\w+)\)/ ){
    print "{ \"$1\" , $1 },\n";
    next;
    }
  if( /besCOMMAND\((\w+)\)/ ){
    print "{ \"$1\" , $1 },\n";
    next;
    }

  }

print "{ NULL , NULL }\n";
print "  };\n";
close F;
