#!/usr/bin/perl -w

# 
# moon.pl
#
# Convert moon corefficients from moon1.dat and moon2.dat to C code
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2006/04/24 15:47:01 $
# $Revision: 2.1 $
# $Tag$
#

use strict;
use FileHandle;

my $moon = new FileHandle("<moon1.dat");
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
    my ($D,$M,$MP,$F,$Sl,$Sr) = split /\t/,$line;
    $Sl =~ s/\s*//g;
    $Sr =~ s/\s*//g;

    if ( $Sl == 0 )
      {
	$line = $moon->getline();
	next;
      }

    print "  Sl ";
    if($Sl > 0) { print "+= "; }
    else { print "-= "; }

    if($M == 0) { }
    elsif(($M == 1)||($M == -1)) { print "E*"; }
    elsif(($M == 2)||($M == -2)) { print "E2*"; }
    elsif(($M == 3)||($M == -3)) { print "E3*"; }
    elsif(($M == 4)||($M == -4)) { print "E4*"; }
    else { die; }

    print abs($Sl*0.000001),"*sin(";

    my $first = "";

    if($D>0) { print "D",abs($D); $first = "+"; }
    elsif($D<0) { print "-D",abs($D); $first = "+"; }
    if($M>0) { print $first,"M",abs($M); $first = "+"; }
    elsif($M<0) { print "-M",abs($M); $first = "+"; }
    if($MP>0) { print $first,"MP",abs($MP); $first = "+"; }
    elsif($MP<0) { print "-MP",abs($MP); $first = "+"; }
    if($F>0) { print $first,"F",abs($F); $first = "+"; }
    elsif($F<0) { print "-F",abs($F); $first = "+"; }

    print ");";

    print "\n";
    $line = $moon->getline();
  }

$moon = new FileHandle("<moon2.dat");
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
    my ($D,$M,$MP,$F,$Sb) = split /\t/,$line;
    $Sb =~ s/\s*//g;

    if ( $Sb == 0 )
      {
	$line = $moon->getline();
	next;
      }

    print "  Sb ";
    if($Sb > 0) { print "+= "; }
    else { print "-= "; }

    if($M == 0) { }
    elsif(($M == 1)||($M == -1)) { print "E*"; }
    elsif(($M == 2)||($M == -2)) { print "E2*"; }
    elsif(($M == 3)||($M == -3)) { print "E3*"; }
    elsif(($M == 4)||($M == -4)) { print "E4*"; }
    else { die; }

    print abs($Sb*0.000001),"*sin(";

    my $first = "";

    if($D>0) { print "D",abs($D); $first = "+"; }
    elsif($D<0) { print "-D",abs($D); $first = "+"; }
    if($M>0) { print $first,"M",abs($M); $first = "+"; }
    elsif($M<0) { print "-M",abs($M); $first = "+"; }
    if($MP>0) { print $first,"MP",abs($MP); $first = "+"; }
    elsif($MP<0) { print "-MP",abs($MP); $first = "+"; }
    if($F>0) { print $first,"F",abs($F); $first = "+"; }
    elsif($F<0) { print "-F",abs($F); $first = "+"; }

    print ");";

    print "\n";
    $line = $moon->getline();
  }

exit;

$moon = new FileHandle("<moon1.dat");
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
    my ($D,$M,$MP,$F,$Sl,$Sr) = split /\t/,$line;
    $Sl =~ s/\s*//g;
    $Sr =~ s/\s*//g;

    if ( $Sr == 0 )
      {
	$line = $moon->getline();
	next;
      }

    print "  Sr ";
    if($Sr > 0) { print "+= "; }
    else { print "-= "; }

    if($M == 0) { }
    elsif(($M == 1)||($M == -1)) { print "E*"; }
    elsif(($M == 2)||($M == -2)) { print "E2*"; }
    elsif(($M == 3)||($M == -3)) { print "E3*"; }
    elsif(($M == 4)||($M == -4)) { print "E4*"; }
    else { die; }

    print abs($Sr*0.001),"*cos(";

    my $first = "";

    if($D>0) { print "D",abs($D); $first = "+"; }
    elsif($D<0) { print "-D",abs($D); $first = "+"; }
    if($M>0) { print $first,"M",abs($M); $first = "+"; }
    elsif($M<0) { print "-M",abs($M); $first = "+"; }
    if($MP>0) { print $first,"MP",abs($MP); $first = "+"; }
    elsif($MP<0) { print "-MP",abs($MP); $first = "+"; }
    if($F>0) { print $first,"F",abs($F); $first = "+"; }
    elsif($F<0) { print "-F",abs($F); $first = "+"; }

    print ");";

    print "\n";
    $line = $moon->getline();
  }
