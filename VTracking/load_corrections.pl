#!/usr/bin/perl -w
use strict;
use FileHandle;

my $id = shift @ARGV;
my $tbl = "tblPositioner_Telescope".$id."_Corrections";
print "DELETE from ",$tbl,";\n";

my $file;
while($file = shift @ARGV)
{
  my $fh = new FileHandle($file,'r');
  my $count=0;
  my @data;
  while($count<21)
  {
    my $line = $fh->getline;
    $line = "0\n" if ( not defined $line );
    chomp $line;
    $data[$count] = $line;
    $count++;
  }
  $data[0]  = $data[0]  ? '"enabled"':'"disabled"'; 
  $data[1]  = $data[1]  ? '"enabled"':'"disabled"'; 
  $data[12] = $data[12] ? '"enabled"':'"disabled"'; 
  $file =~ /(..)(..)(..)(_vff)?[.]dat/;
  my $comment = "\"From file: ".$file.'"';
  my $date = "20".$1."-".$2."-".$3." 00:00:00";
  print "UPDATE ",$tbl," SET db_end_time=\"",$date,"\" WHERE db_end_time IS NULL;\n";
  print "INSERT INTO ",$tbl," VALUES ( \"",$date,"\", NULL, ",join(', ',@data),", ",$comment," );\n";
}
