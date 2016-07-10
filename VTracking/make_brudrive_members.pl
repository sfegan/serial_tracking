#!/usr/bin/perl -w

use strict;
use FileHandle;

my $line;

my $cpp_fh = new FileHandle("BRUDrive_members_cpp.txt","w");
my $hpp_fh = new FileHandle("BRUDrive_members_hpp.txt","w");

$line = <ARGV>;
while($line)
  {
    chomp $line;
    my ($id, $cmd, $name) = split(/\s+/,$line,3);

    my $member = lcfirst $name;
    $member =~ s/\s//g;

    my $scvariable = $name;
    $scvariable =~ s/^.*?\s+//;
    $scvariable =~ s/\s+//g;

    if($member =~ /^read/)
      {
	my $return_type = $cmd;
	my $args = "";
	my $call_args = "";

	$return_type = "void" if($cmd=~/^BRUEmptyCmd$/);
	$return_type = $1 if($cmd=~/^BRUAtomCmd<(.*)>$/);
	$return_type = "std::vector<".$1.">"  if($cmd=~/^BRUAtomArrayCmd<(.*),.*>$/);
	$return_type = $1 if($cmd=~/^BRUIndex8AtomCmd<(.*)>$/);
	$return_type = "std::vector<".$1.">"  if($cmd=~/^BRUIndex8AtomArrayCmd<(.*),.*>$/);
	$return_type = "std::string"  if($cmd=~/^BRUStringCmd<.*>$/);

	$args = "uint8_t index" if ($cmd=~/^BRUIndex8/);
	$call_args = ",index" if ($cmd=~/^BRUIndex8/);

	$hpp_fh->print("    ",$return_type," ",$member,"(",$args,") const;\n");
	
	$cpp_fh->print($return_type," BRUDrive::",$member,"(",$args,") const\n",
		       "{\n",
		       "  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);\n",
		       "  return sc_Cmd",$scvariable,".read(m_datastream,m_address",$call_args,");\n",
		       "}\n\n");
      }
    elsif($member =~ /^write/)
      {
	my $args = "";
	my $call_args = ",val";

	$args = "uint8_t index," if ($cmd=~/^BRUIndex8/);
	$call_args = ",index,val" if ($cmd=~/^BRUIndex8/);

	$args .= $1." val" if($cmd=~/^BRUAtomCmd<(.*)>$/);
	$args .= "const std::vector<".$1.">& val"  if($cmd=~/^BRUAtomArrayCmd<(.*),.*>$/);
	$args .= $1." val" if($cmd=~/^BRUIndex8AtomCmd<(.*)>$/);
	$args .= "const std::vector<".$1.">& val"  if($cmd=~/^BRUIndex8AtomArrayCmd<(.*),.*>$/);
	$args .= "const std::string& val"  if($cmd=~/^BRUStringCmd<.*>$/);
	
	$call_args =~ s/,val// if ($cmd=~/^BRUEmptyCmd$/);

	$args =~ s/,$//;
	$call_args =~ s/,$//;

	$hpp_fh->print("    void ",$member,"(",$args,") const;\n");
	
	$cpp_fh->print("void BRUDrive::",$member,"(",$args,") const\n",
		       "{\n",
		       "  RegisterThisFunction fnguard(\"BRUDrive::",$member,"\");\n",
		       "  sc_Cmd",$scvariable,".write(m_datastream,m_address",$call_args,");\n",
		       "}\n\n");
	
      }
    else
      {
	die "Unknown function type ".$member;
      }

    $line = <ARGV>;
  }
