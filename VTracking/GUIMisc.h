//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIMisc.h
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
 * $Date: 2008/02/10 11:50:52 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIMISC_H
#define VTRACKING_GUIMISC_H

#include<qcolor.h>
#include<qgroupbox.h>
#include<qlineedit.h>
#include<qtable.h>
#include<qlabel.h>
#include<qpixmap.h>
#include<qpainter.h>
#include<qpalette.h>
#include<qcombobox.h>
#include<qpushbutton.h>
#include<qvalidator.h>
#include<qtabbar.h>
#include<qtabwidget.h>

#include<map>

#define MAKEMULT(x,y) (QString(x)+QChar(0x00D7)+QString(y))
#define MAKEDEG(x) (QString(x)+QChar(0x00B0))
#define MAKEDPS(x) (MAKEDEG(x)+QString("/s"))
#define PLUSMINUS(x) (QString(QChar(0x00B1))+QString(x))
#define XPLUSMINUSY(x,y) (QString(x)+PLUSMINUS(y))

static const QColor color_bg_attn(255,255,100);
static const QColor color_fg_attn(0,0,0);

//static const QColor color_bg_warn(200,28,31); // too dark with black writing
static const QColor color_bg_warn(225,32,35);
static const QColor color_fg_warn(0,0,0);

//static const QColor color_bg_on(60,200,60);
static const QColor color_bg_on(39,163,23);
static const QColor color_fg_on(0,0,0);

enum ScopeValuesToDisplay { SV_AUTO, SV_SKY, SV_ENCODER };

#define WARNANGLE_CW 260
#define WARNANGLE_CC -260
#define WARNANGLE_UP 90
#define WARNANGLE_DN -1

class MyQGroupBox: public QGroupBox
{
  Q_OBJECT
public:
  MyQGroupBox(QWidget* parent=0, const char* name=0):
    QGroupBox(parent,name) { setup(); }
  MyQGroupBox(const QString & title, QWidget* parent=0, const char* name=0):
    QGroupBox(title,parent,name) { setup(); }
  MyQGroupBox(int strips, Orientation orientation,
	      QWidget* parent=0, const char* name=0):
    QGroupBox(strips,orientation,parent,name) { setup(); }
  MyQGroupBox(int strips, Orientation orientation, const QString& title, 
	      QWidget* parent=0, const char* name=0):
    QGroupBox(strips,orientation,title,parent,name) { setup(); }
  virtual ~MyQGroupBox();
private:
  void setup();
};

class MyMouseFilter: public QObject
{
public:
  MyMouseFilter(QObject* parent=0, const char* name=0)
    : QObject(parent,name) { /* nothing to see here */ }
  ~MyMouseFilter();
protected:
  bool eventFilter( QObject *o, QEvent *e );
};

class InfoQLineEdit: public QLineEdit
{
  Q_OBJECT
public:
  InfoQLineEdit(QWidget* parent, const char* name=0):
    QLineEdit(parent,name), m_sizehint(), m_minsizehint() 
  { setup(false); }
  InfoQLineEdit(const QString& texttemplate, double fontsize,
		bool tight, bool edit,
		QWidget* parent, const char* name=0):
    QLineEdit(parent,name), m_sizehint(), m_minsizehint()
  { setup(texttemplate,fontsize,tight,edit); }
  InfoQLineEdit(const QString& texttemplate, QFont font, bool tight, bool edit,
		QWidget* parent, const char* name=0):
    QLineEdit(parent,name), m_sizehint(), m_minsizehint()
  { setup(texttemplate,font,tight,edit); }
  virtual ~InfoQLineEdit();
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;
private:
  void setup(bool edit);
  void setup(const QString& texttemplate, double fontsize, 
	     bool tight, bool edit);
  void setup(const QString& texttemplate, QFont font, bool tight, bool edit);

  QSize m_sizehint;
  QSize m_minsizehint;
};

class MyQTableItem: public QTableItem
{
public:
  MyQTableItem(QTable* table,EditType et): 
    QTableItem(table,et) {}
  MyQTableItem(QTable* table,EditType et,const QString& text): 
    QTableItem(table,et,text) {}
  MyQTableItem(QTable* table,EditType et,const QString& text,const QPixmap& p):
    QTableItem(table,et,text,p) {}
  virtual ~MyQTableItem();
  virtual int alignment() const;
};

class MyQTable: public QTable
{
  Q_OBJECT
public:
  MyQTable(QWidget* parent=0,const char* name=0): 
    QTable(parent,name) { setup(); }
  MyQTable(int numRows,int numCols,QWidget* parent=0,const char* name=0):
    QTable(numRows, numCols, parent, name) { setup(); }
  virtual ~MyQTable();
  virtual void setText(int row,int col,const QString &text);
  virtual void paintCell(QPainter* p,int row,int col,const QRect& cr, 
			 bool selected,const QColorGroup& cg);
  virtual void paintFocus(QPainter* p,const QRect& cr);
private:
  void setup();
};

class MyQLabel: public QLabel
{
  Q_OBJECT
public:
  MyQLabel(QWidget* parent,const char* name=0,WFlags f=0):
    QLabel(parent,name,f|WRepaintNoErase) {}
  MyQLabel(const QString& text,QWidget* parent,const char* name=0,WFlags f=0):
    QLabel(text,parent,name,f|WRepaintNoErase) {}
  MyQLabel(QWidget* buddy,const QString& text,QWidget* parent,
	   const char* name=0,WFlags f=0):
    QLabel(buddy,text,parent,name,f|WRepaintNoErase) {}
  ~MyQLabel();
protected:
  virtual void paintEvent(QPaintEvent* event);
  //virtual void drawContents(QPainter *p);
};

class MyQComboBox: public QComboBox
{
  Q_OBJECT
public:
  MyQComboBox(QWidget* parent=0, const char* name=0):
    QComboBox(parent,name) {}
  MyQComboBox(bool rw, QWidget* parent=0, const char* name=0):
    QComboBox(rw,parent,name) {}
  virtual ~MyQComboBox();
public slots:
  virtual void setActivated(int id);
};

class MyQPushButton: public QPushButton
{
  Q_OBJECT
public:
  MyQPushButton(QWidget* parent, const char* name = 0):
    QPushButton(parent,name) { setup(); }
  MyQPushButton(const QString& text, QWidget* parent, const char* name = 0):
    QPushButton(text,parent,name) { setup(); }
  MyQPushButton(const QIconSet& icon, const QString& text, QWidget* parent,
		const char* name = 0):
    QPushButton(icon,text,parent,name) { setup(); }
  virtual ~MyQPushButton();
protected:
  virtual void paintEvent(QPaintEvent* event);
  void setup();
};

class MyQTabBar: public QTabBar
{
  Q_OBJECT
public:
  MyQTabBar(QWidget* parent=0, const char* name=0):
    QTabBar(parent,name), m_hcolor() { }
  virtual ~MyQTabBar();  
  void setHighlighting(int id, bool highlight=true, const QColor& c = red);
protected:
  virtual void paint(QPainter* p, QTab* t, bool selected) const;
  virtual void paintLabel(QPainter* p, const QRect& br, QTab* t, bool has_focus) const;
private:
  std::map<int,QColor> m_hcolor;
};

class MyQTabWidget: public QTabWidget
{
  Q_OBJECT
public:
  MyQTabWidget(QWidget* parent=0, const char* name=0, WFlags f=0):
    QTabWidget(parent,name,f), m_tabbar(new MyQTabBar(this)) 
  { setTabBar(m_tabbar); }
  virtual ~MyQTabWidget();  
  void setHighlighting(QWidget* w, bool highlight=true, const QColor& c = red);
private:
  MyQTabBar* m_tabbar;
};

class TelescopeIndicator: public QObject
{
  Q_OBJECT
public:
  TelescopeIndicator(double zeroangle, double minangle, double maxangle,
		     QObject* parent=0, const char* name=0);
  virtual ~TelescopeIndicator();
  virtual QPixmap draw(double angle, const QSize& size, bool enable,
		       bool fillenable=true, const QColor& fillcolor=black, 
		       bool vernier=true);
private:
  double m_zeroangle;
  double m_minangle;
  double m_maxangle;
};

void activateLE(bool active, QLineEdit* le, const QColor& f, const QColor& b);
QRegExp raValidatorRE();
QRegExp decValidatorRE();
QRegExp epochValidatorRE();

#endif // VTRACKING_GUIMISC_H
