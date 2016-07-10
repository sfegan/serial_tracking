//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TextMenu.h
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

#ifndef VTRACKING_TEXTMENU_H
#define VTRACKING_TEXTMENU_H

#include<sstream>
#include<string>
#include<vector>
#include<map>

namespace VTracking 
{  

  class TextMenu
  {
  public:

    class Item
    {
    public:
      Item(): m_key('?'), m_text("Huh?") { }
      Item(int k, const std::string& t): m_key(k), m_text(t) { }
      template<typename T> Item(int k, const std::string& t, const T& val):
	m_key(k), m_text()
      {
	std::ostringstream stream; 
	stream << val;
	m_text = t + stream.str();
      }

      int key() const { return m_key; }
      const std::string& text() const { return m_text; }
    private:
      int m_key;
      std::string m_text;
    };
    
    
    TextMenu(std::string title): m_items(), m_map(), m_title(title) {}
    void addItem(const Item& item) 
    { m_items.push_back(item); m_map[item.key()]=item; }
    int exec() const;
    static void pressAnyKey();
  private:
    std::vector<Item> m_items;
    std::map<int, Item> m_map;
    std::string m_title;
  };

}; // namespace VTracking

#endif // defined VTRACKING_TEXTMENU_H

