#!/usr/bin/perl -w

use strict;
use FileHandle;

my $line;

my $cpp_fh = new FileHandle("BRUDrive_emulator_cpp.txt","w");

$cpp_fh->print("  std::string address_str = packet.substr(0,2);\n",
	       "  uint8_t address = SimpleAtomCODEC<uint8_t>::decode(address_str);\n",
	       "  uint16_t code = SimpleAtomCODEC<uint16_t>::decode(std::string(\"0\")+packet.substr(2,3));\n",
	       "  std::string payload = packet.substr(5,packet.length()-7);\n",
	       "\n",
	       "  if(address != m_address)return false;\n",
	       "\n",
	       "  switch(code)\n",
	       "    {\n");

my $first=1;

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

    $cpp_fh->print("\n") unless $first;
    $cpp_fh->print("    case 0x0",$id,":\n");

    if($member =~ /^read/)
      {
	if($cmd=~/^BRUEmptyCmd$/)
	  {
	    $cpp_fh->print("      response=std::string(\"",$id,"\");\n");
	  }
	elsif($cmd=~/^BRUAtomCmd<(.*)>$/)
	  {
	    $cpp_fh->print("      if(m_emulator_memory.find(\"",$scvariable,"\")==m_emulator_memory.end())\n",
			   "        {\n",
			   "          $1 val = sc_Cmd",$scvariable,".min();\n",
			   "          std::string val_str = SimpleAtomCODEC<",$1,">::encode(val);\n",
			   "          m_emulator_memory[\"",$scvariable,"\"]=val_str;\n",
			   "        }\n",
			   "      response=std::string(\"",$id,"\")+m_emulator_memory[\"",$scvariable,"\"];\n");
	  }
	elsif($cmd=~/^BRUAtomArrayCmd<(.*),.*>$/)
	  {
	    $cpp_fh->print("      if(m_emulator_memory.find(\"",$scvariable,"\")==m_emulator_memory.end())\n",
			   "        {\n",
			   "          std::string val_str;\n",
			   "          for(unsigned i=0;i<sc_Cmd",$scvariable,".arrayLen();i++)\n",
			   "            val_str += SimpleAtomCODEC<",$1,">::encode(sc_Cmd",$scvariable,".min()[i]);\n",
			   "          m_emulator_memory[\"",$scvariable,"\"]=val_str;\n",
			   "        }\n",
			   "      response=std::string(\"",$id,"\")+m_emulator_memory[\"",$scvariable,"\"];\n");
	  }
	elsif($cmd=~/^BRUIndex8AtomCmd<(.*)>$/)
	  {
# BOUNDS CHECK
	    $cpp_fh->print("      {\n",
                           "        std::string name = std::string(\"",$scvariable,",\")+payload.substr(0,2);\n",
			   "        if(m_emulator_memory.find(name)==m_emulator_memory.end())\n",
			   "          {\n",
			   "            $1 val = sc_Cmd",$scvariable,".min();\n",
			   "            std::string val_str = SimpleAtomCODEC<",$1,">::encode(val);\n",
			   "            m_emulator_memory[name]=val_str;\n",
			   "          }\n",
			   "        response=response=std::string(\"",$id,"\")+m_emulator_memory[name];\n",
			   "      };\n");
	  }
	elsif($cmd=~/^BRUIndex8AtomArrayCmd<(.*),.*>$/)
	  {
# BOUNDS CHECK
	    $cpp_fh->print("      {\n",
                           "        std::string name = std::string(\"",$scvariable,",\")+payload.substr(0,2);\n",
			   "        if(m_emulator_memory.find(name)==m_emulator_memory.end())\n",
			   "          {\n",
			   "            std::string val_str;\n",
			   "            for(unsigned i=0;i<sc_Cmd",$scvariable,".arrayLen();i++)\n",
			   "              val_str += SimpleAtomCODEC<",$1,">::encode(sc_Cmd",$scvariable,".min()[i]);\n",
			   "            m_emulator_memory[\"",$scvariable,"\"]=val_str;\n",
			   "          }\n",
			   "        response=std::string(\"",$id,"\")+m_emulator_memory[name];\n",
			   "      };\n");
	  }
	elsif($cmd=~/^BRUStringCmd<.*>$/)
	  {
	    $cpp_fh->print("      if(m_emulator_memory.find(\"",$scvariable,"\")==m_emulator_memory.end())\n",
			   "        m_emulator_memory[\"",$scvariable,"\"]=\"\";\n",
			   "      response=std::string(\"",$id,"\")+m_emulator_memory[\"",$scvariable,"\"];\n");
	  }
      }
    elsif($member =~ /^write/)
      {
	if($cmd=~/^BRUEmptyCmd$/)
	  {
	    $cpp_fh->print("      response=std::string(\"",$id,"\");\n");
	  }
	elsif($cmd=~/^BRUAtomCmd<(.*)>$/)
	  {
# BOUNDS CHECK
	    $cpp_fh->print("      m_emulator_memory[\"",$scvariable,"\"]=payload;\n",
			   "      response=std::string(\"",$id,"\");\n");
	  }
	elsif($cmd=~/^BRUAtomArrayCmd<(.*),.*>$/)
	  {
# BOUNDS CHECK
	    $cpp_fh->print("      m_emulator_memory[\"",$scvariable,"\"]=payload;\n",
			   "      response=std::string(\"",$id,"\");\n");
	  }
	elsif($cmd=~/^BRUIndex8AtomCmd<(.*)>$/)
	  {
# DOUBLE BOUNDS CHECK
	    $cpp_fh->print("      {\n",
                           "        std::string name = std::string(\"",$scvariable,",\")+payload.substr(0,2);\n",
			   "        m_emulator_memory[name]=payload;\n",
			   "        response=std::string(\"",$id,"\");\n",
			   "      };\n");
	  }
	elsif($cmd=~/^BRUIndex8AtomArrayCmd<(.*),.*>$/)
	  {
# DOUBLE BOUNDS CHECK
	    $cpp_fh->print("      {\n",
                           "        std::string name = std::string(\"",$scvariable,",\")+payload.substr(0,2);\n",
			   "        m_emulator_memory[name]=payload;\n",
			   "        response=std::string(\"",$id,"\");\n",
			   "      };\n");
	  }
	elsif($cmd=~/^BRUStringCmd<.*>$/)
	  {
# BOUNDS CHECK
	    $cpp_fh->print("      m_emulator_memory[\"",$scvariable,"\"]=payload;\n",
			   "      response=std::string(\"",$id,"\");\n");
	  }
      }
    else
      {
	die "Unknown function type ".$member;
      }

    $cpp_fh->print("      break;\n");
    $first=0;

    $line = <ARGV>;
  }

$cpp_fh->print("\n",
	       "    default:\n",
	       "      return false;\n",
	       "    }\n",
	       "  response=address_str+response;\n",
	       "  return true;\n");
    
