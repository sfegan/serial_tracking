#!/usr/bin/perl -w

# 
# nutation.pl
#
# Convert nutation corefficients from nutation.dat to C code
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2006/04/24 15:47:01 $
# $Revision: 2.1 $
# $Tag$
#

use strict;
use FileHandle;

my $moon = new FileHandle("<nutation.dat");
my $line = $moon->getline();
while ( defined $line )
  {
    chomp $line;
    $line =~ s/#.*//;
    $line =~ s/^\s*$//;
    if ( !$line )
      {
	$line = $moon->getline();
	next;
      }
    my ($D,$M,$MP,$F,$S,$dpsi1,$dpsiT,$deps1,$depsT) = split /\t+/,$line;
    $dpsi1 =~ s/\s*//g;
    $dpsiT =~ s/\s*//g;
    $deps1 =~ s/\s*//g;
    $depsT =~ s/\s*//g;

    if (( $dpsi1 == 0 )&&( $dpsiT == 0 ))
      {
	$line = $moon->getline();
	next;
      }

    print "  dpsi += ";
    print "( " if(( $dpsi1 != 0 )&&( $dpsiT != 0 ));
    print $dpsi1 if ( $dpsi1 != 0 );
    if($dpsiT > 0) { print " + ",abs($dpsiT),"*T"; }
    elsif($dpsiT < 0) { print " - ",abs($dpsiT),"*T"; }
    print " )" if(( $dpsi1 != 0 )&&( $dpsiT != 0 ));

    print "*sin(";

    my $first = "";

    if($D>0) { print "D",abs($D); $first = "+"; }
    elsif($D<0) { print "-D",abs($D); $first = "+"; }
    if($M>0) { print $first,"M",abs($M); $first = "+"; }
    elsif($M<0) { print "-M",abs($M); $first = "+"; }
    if($MP>0) { print $first,"MP",abs($MP); $first = "+"; }
    elsif($MP<0) { print "-MP",abs($MP); $first = "+"; }
    if($F>0) { print $first,"F",abs($F); $first = "+"; }
    elsif($F<0) { print "-F",abs($F); $first = "+"; }
    if($S>0) { print $first,"S",abs($S); $first = "+"; }
    elsif($S<0) { print "-S",abs($S); $first = "+"; }

    print ");";

    print "\n";
    $line = $moon->getline();
  }

$moon = new FileHandle("<nutation.dat");
$line = $moon->getline();
while ( defined $line )
  {
    chomp $line;
    $line =~ s/#.*//;
    $line =~ s/^\s*$//;
    if ( !$line )
      {
	$line = $moon->getline();
	next;
      }
    my ($D,$M,$MP,$F,$S,$dpsi1,$dpsiT,$deps1,$depsT) = split /\t+/,$line;
    $dpsi1 =~ s/\s*//g;
    $dpsiT =~ s/\s*//g;
    $deps1 =~ s/\s*//g;
    $depsT =~ s/\s*//g;

    if (( $deps1 == 0 )&&( $depsT == 0 ))
      {
	$line = $moon->getline();
	next;
      }

    print "  deps += ";
    print "( " if(( $deps1 != 0 )&&( $depsT != 0 ));
    print $deps1 if ( $deps1 != 0 );
    if($depsT > 0) { print " + ",abs($depsT),"*T"; }
    elsif($depsT < 0) { print " - ",abs($depsT),"*T"; }
    print " )" if(( $deps1 != 0 )&&( $depsT != 0 ));

    print "*cos(";

    my $first = "";

    if($D>0) { print "D",abs($D); $first = "+"; }
    elsif($D<0) { print "-D",abs($D); $first = "+"; }
    if($M>0) { print $first,"M",abs($M); $first = "+"; }
    elsif($M<0) { print "-M",abs($M); $first = "+"; }
    if($MP>0) { print $first,"MP",abs($MP); $first = "+"; }
    elsif($MP<0) { print "-MP",abs($MP); $first = "+"; }
    if($F>0) { print $first,"F",abs($F); $first = "+"; }
    elsif($F<0) { print "-F",abs($F); $first = "+"; }
    if($S>0) { print $first,"S",abs($S); $first = "+"; }
    elsif($S<0) { print "-S",abs($S); $first = "+"; }

    print ");";

    print "\n";
    $line = $moon->getline();
  }

