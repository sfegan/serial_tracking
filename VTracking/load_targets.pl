#!/usr/bin/perl -w
use strict;
use POSIX;
use FileHandle;

my $filename = shift @ARGV;
my @collections = @ARGV;

my $fh = new FileHandle($filename,"r");

my $line;
while ( $line = getline $fh )
  {
    chomp $line;

    my ($most,$desc) = $line =~ /(.*)\s*#\s*(.*)?/;
    $most = $line if ( not defined $desc );
    $desc = "" if ( not defined $desc );

    my @bits = split('\s+',$most);

    my $id = shift @bits;
    pop @bits;
    pop @bits;
    pop @bits;
    my $epoch = pop @bits;
    my $dec_dms = pop @bits;
    my $ra_hms = pop @bits;
    my $name = join(' ',@bits);

    my $ra = `angle -s hms $ra_hms | cut -d' ' -f4`;
    chomp $ra;
    my $dec = `angle -s dms $dec_dms | cut -d' ' -f5`;
    chomp $dec;

    printf("DELETE FROM tblObserving_Sources WHERE source_id='%s';\n",$name);

    printf("DELETE FROM tblObserving_Collection WHERE source_id='%s';\n",$name);
    print("INSERT IGNORE INTO tblObserving_Sources VALUES ( '",
	  $name,"', ",$ra,", ",$dec,", ",$epoch,", '",
	  $desc,"' );\n");

    my $coll;
    foreach $coll ( @collections )
      {
	print("INSERT IGNORE INTO tblObserving_Collection VALUES ( '",
	      $name,"', '",$coll,"' );\n");
      }
  }
