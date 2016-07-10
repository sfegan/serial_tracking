//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TextMenu.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#include<sstream>
#include<fstream>
#include<iostream>
#include<iomanip>
#include<unistd.h>

#include"TextMenu.h"

using namespace VTracking;

// ----------------------------------------------------------------------------
// TextMenu
// ----------------------------------------------------------------------------

int TextMenu::exec() const
{
  std::cout << "\33[H\33[2J"
	    << "===============================================================================" << std::endl
	    << m_title << std::endl << std::endl;
  for(std::vector<TextMenu::Item>::const_iterator i=m_items.begin(); 
      i!=m_items.end(); i++)
    {
      if(i->key()<26)std::cout << '^' << char(i->key()+'a');
      else std::cout << ' ' << char(i->key());
      std::cout << " - " << i->text() << std::endl;
    }
  
  std::cout << "===============================================================================" << std::endl
	    << std::endl;
  
  char c;
  do
    {
      std::cout << "Enter menu option: " << std::flush;
      std::cin.get(c);
      char junk=0;while((std::cin)&&(c!='\n')&&(junk!='\n'))std::cin.get(junk);
    }
  while((std::cin)&&(m_map.find(int(c))==m_map.end()));
  std::cout << "\33[H\33[2J" << std::flush;
  return int(c);
}

void TextMenu::pressAnyKey()
{
  char c;
  std::cout << "Press Enter: " << std::flush;
  std::cin.get(c);
  while((std::cin)&&(c!='\n'))std::cin.get(c);
}
