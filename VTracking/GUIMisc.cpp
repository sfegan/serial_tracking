//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIMisc.cpp
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
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#include<iostream>
#include<iomanip>
#include<cmath>

#include<qfont.h>
#include<qfontmetrics.h>
#include<qstyle.h>
#include<qbitmap.h>
#include<qpixmap.h>
#include<qimage.h>
#include<qpainter.h>
#include<qapplication.h>

#include"GUIMisc.h"

// ============================================================================
// MyQGroupBox
// ============================================================================

MyQGroupBox::~MyQGroupBox()
{
  // nothing to see here
}

void MyQGroupBox::setup()
{
  setFrameShape(QFrame::WinPanel);
  setFrameShadow(QFrame::Plain);
}

// ============================================================================
// MyMouseFilter
// ============================================================================

MyMouseFilter::~MyMouseFilter()
{
  // nothing to see here
}

bool MyMouseFilter::eventFilter( QObject *o, QEvent *e )
{
  if((e->type() == QEvent::MouseButtonPress)
     ||(e->type() == QEvent::MouseButtonRelease)
     ||(e->type() == QEvent::MouseButtonDblClick)
     ||(e->type() == QEvent::MouseMove)
     ||(e->type() == QEvent::ContextMenu))
    return TRUE;
  else
    return FALSE;
}

// ============================================================================
// InfoQLineEdit
// ============================================================================

InfoQLineEdit::~InfoQLineEdit()
{
  // nothing to see here
}

QSize InfoQLineEdit::sizeHint() const
{
  return QLineEdit::sizeHint();
}

QSize InfoQLineEdit::minimumSizeHint() const
{
  return m_minsizehint; //QLineEdit::minimumSizeHint();
}

void InfoQLineEdit::setup(bool edit)
{
  if(edit==false)
    {
      installEventFilter(new MyMouseFilter(this,QString(name())+" filter"));
      setReadOnly(true);
      setFocusPolicy(NoFocus);
      setFrameShape(QFrame::Box);
      setFrameShadow(QFrame::Plain);
      setLineWidth(1);
      setDragEnabled(false);
      setAlignment(Qt::AlignLeft);
      //setAlignment(Qt::AlignRight);
    }
  m_sizehint=QLineEdit::sizeHint();
  m_minsizehint=QLineEdit::minimumSizeHint();
}

void InfoQLineEdit::
setup(const QString& texttemplate, double fontsize, bool tight, bool edit)
{
  QFont f=font();
  if(fontsize != 1.0)
    { f.setPointSize(int(floor(double(f.pointSize())*fontsize))); setFont(f); }
  setup(texttemplate,f,tight,edit);
}

void InfoQLineEdit::
setup(const QString& texttemplate, QFont font, bool tight, bool edit)
{
  setup(edit);

  setFont(font);
  QFontMetrics fm(font);

  int h = fm.lineSpacing()+2;
  int w = fm.width(texttemplate)*21/20;
  int m = frameWidth()*2;
  QSize s = 
    style().sizeFromContents(QStyle::CT_LineEdit, this,
			     QSize( w + m, h + m ).
			     expandedTo(QApplication::globalStrut()));

  if(tight)
    {
      m_sizehint=s;
      m_minsizehint=s;
    }

  setMinimumWidth(s.width());
  setText(texttemplate);
}

// ============================================================================
// MyQTable
// ============================================================================

MyQTableItem::~MyQTableItem()
{
  // nothing to see here
}

int MyQTableItem::alignment() const
{
  return Qt::AlignLeft|Qt::AlignVCenter;
}

MyQTable::~MyQTable()
{
  // nothing to see here
}

void MyQTable::paintCell(QPainter* p,int row,int col,const QRect& cr, 
			 bool selected,const QColorGroup& cg)
{
  QColorGroup thecg=cg;
  //if(row%2==1)thecg.setBrush(QColorGroup::Base,QBrush(QColor(215,231,255)));
  if(row%2==1)thecg.setBrush(QColorGroup::Base,QBrush(QColor(221,255,207)));
  QTable::paintCell(p,row,col,cr,selected,thecg);
}

void MyQTable::paintFocus(QPainter* p,const QRect& cr)
{
  // nothing to see here
}

void MyQTable::setText(int row,int col,const QString &text)
{
  QTableItem *itm = item( row, col );
  if(itm) 
    {
      itm->setText( text );
      itm->updateEditor( row, col );
      updateCell( row, col );
    } 
  else 
    {
      QTableItem *i = new MyQTableItem( this, QTableItem::OnTyping,
                                        text, QPixmap() );
      setItem( row, col, i );
    }
}

void MyQTable::setup()
{
  setFocusStyle(QTable::FollowStyle);
}

MyQLabel::~MyQLabel()
{
  // nothing to see here
}

void MyQLabel::paintEvent(QPaintEvent *event)
{
  QPixmap pix(size());
  QPainter paint(&pix);

  if(parentWidget() && parentWidget()->backgroundPixmap())
    {
      paint.drawTiledPixmap(0,0,width(),height(),
			    *parentWidget()->backgroundPixmap(),x(),y());
    } 
  else 
    {
      paint.fillRect(0,0,width(),height(),
		     colorGroup().brush(QColorGroup::Background));
    }

  drawFrame( &paint );
  drawContents( &paint );
  bitBlt(this,QPoint(0,0),&pix,rect());
}

// ============================================================================
// MyQComboBox
// ============================================================================

MyQComboBox::~MyQComboBox()
{
  // nothing to see here  
}

void MyQComboBox::setActivated(int id)
{
  setCurrentItem(id);
}

// ============================================================================
// MyQComboBox
// ============================================================================

MyQPushButton::~MyQPushButton()
{
  // nothing to see here
}

void MyQPushButton::paintEvent(QPaintEvent* event)
{
  QRect cr = rect();
  QPixmap pix(cr.size());
  pix.fill(this, cr.topLeft());
  QPainter p(&pix);
  p.setBackgroundColor(backgroundColor());
  drawButton(&p);
  p.end();
  bitBlt(this,event->rect().topLeft(),&pix,event->rect(),CopyROP);
}

void MyQPushButton::setup()
{
  setWFlags(WRepaintNoErase);
}

// ============================================================================
// MyQTabBar
// ============================================================================

MyQTabBar::~MyQTabBar()
{
  // nothing to see here  
}

void MyQTabBar::setHighlighting(int id, bool highlight, const QColor& c)
{
  if(highlight)m_hcolor[id]=c;
  else if(m_hcolor.find(id) != m_hcolor.end())
    m_hcolor.erase(m_hcolor.find(id));
  update();
}

void MyQTabBar::paint(QPainter* p, QTab* t, bool selected) const
{
  QTabBar::paint(p, t, selected);
}

void MyQTabBar::
paintLabel(QPainter* p, const QRect& br, QTab* t, bool has_focus) const
{
  if(m_hcolor.find(t->identifier()) != m_hcolor.end())
    {
      //QPen op = p->pen();
      //p->setPen(QPen((*m_hcolor.find(t->identifier())).second,2));
      //QRect r = br;
      //r.addCoords(-1,1,2,2);
      //p->drawRect(r);
      //p->setPen(op);

      //QRect r = t->rect();
      //r.addCoords(2,5,-2,-3);
      QRect r = br;
      p->fillRect(r,(*m_hcolor.find(t->identifier())).second);
    }
  QTabBar::paintLabel(p, br, t, has_focus);
}

MyQTabWidget::~MyQTabWidget()
{
  // nothing to see here  
}

void MyQTabWidget::setHighlighting(QWidget* w, bool highlight, const QColor& c)
{
  int i = indexOf(w);
  if(i!=-1)m_tabbar->setHighlighting(i, highlight, c);
}

// ============================================================================
// TelescopeIndicator
// ============================================================================

TelescopeIndicator::
TelescopeIndicator(double zeroangle, double minangle, double maxangle,
		   QObject* parent, const char* name):
  QObject(parent,name), m_zeroangle(zeroangle), 
  m_minangle(minangle), m_maxangle(maxangle)
{ 
  // nothing to see here  
}

TelescopeIndicator::~TelescopeIndicator()
{
  // nothing to see here
}

QPixmap TelescopeIndicator::draw(double angle, const QSize& size, bool enable,
				 bool fillenable, const QColor& fillcolor, bool vernier)
{
  int pixwidth = size.width();
  int pixheight = size.height();

  QPixmap pix(pixwidth,pixheight);
  QBitmap mask(pixwidth,pixheight);

  pix.fill();
  mask.fill(Qt::color0);

  int smalldim = pixwidth<pixheight ? pixwidth:pixheight;
  int penwidth=smalldim/50+1;

  QColor mask_fill_color = Qt::color0;
  if(enable)mask_fill_color = Qt::color1;

  // Draw the pixmap twice -- once for the pixmap, then for the mask
  for(unsigned i=0; i<2; i++)
    {
# if 0
      double x = sin(m_minangle/180.0*M_PI);
      double y = cos(m_minangle/180.0*M_PI);
      
      double left =   x;
      double right =  x;
      double top =    y;
      double bottom = y;
      
      x = sin(m_maxangle/180.0*M_PI);
      y = cos(m_maxangle/180.0*M_PI);
      
      if(x<left)left=x;     if(x>right)right=x;
      if(y<bottom)bottom=y; if(y>top)top=y;
      
      if((m_minangle<90)&&(m_maxangle>90))right=1;
      if((m_minangle<180)&&(m_maxangle>180))bottom=-1;
      if((m_minangle<270)&&(m_maxangle>270))left=-1;
      if((m_minangle<360)&&(m_maxangle>0))top=1;
      
      int x0 = int(round(-double(pixwidth)/(right-left)*left));
      int y0 = int(round(-double(pixheight)/(top-bottom)*bottom));
      
      int w = int(round(double(pixwidth)/(right-left)*2.0));
      int h = int(round(double(pixheight)/(top-bottom)*2.0));
      
      int a = int(round(fmod(90-m_minangle+360,360)*16));
      int alen = int(round(m_maxangle-m_minangle)*16);
      if(alen<0)alen+=360*16;
      alen=-alen;
      
      Debug::stream()
	<< left << ' ' << right << ' ' << bottom << ' ' << top 
	<< std::endl
	<< x0 << ' ' << y0 << ' ' << w << ' ' << h << std::endl
	<< a << ' ' << alen << std::endl;
      
      pix.fill();
      QPainter p(pix);
      p.setPen(black);
      p.drawArc(x0-w/2,y0-h/2,w,h,a,alen);
      
      p.flush();
      p.end();
#endif

      int x0 = pixwidth/2;
      int y0 = pixheight/2;
      int r = smalldim*19/40;
      int a;
      int alen;
      
      QPainter p;
      if(i==0)p.begin(&pix);
      else p.begin(&mask);

      if(i==0)
	{
	  p.setPen(QPen(Qt::black,penwidth));
	  p.setBrush(QBrush(Qt::white));
	}
      else 
	{
	  p.setPen(QPen(Qt::color1,penwidth));
	  p.setBrush(QBrush(mask_fill_color));
	}
      
//      p.drawEllipse(x0-r,y0-r,2*r,2*r);
      p.drawEllipse(x0-r,y0-r,2*r,2*r);
      
      if(fillenable)
	if(i==0)p.setBrush(fillcolor);
	else p.setBrush(Qt::color1);
      
      a = int(round(fmod(90-m_zeroangle-angle+720,360)*16));
      alen = int(round(angle*16));
      
      p.drawPie(x0-r,y0-r,2*r,2*r,a,alen);
      p.setBrush(QBrush());
      p.drawEllipse(x0-r,y0-r,2*r,2*r);
      
      if(vernier)
	{
	  int rs=smalldim*19/160;
	  double pielen=80;
	  a = int(round(fmod(90-pielen/2.0-fmod(angle,1.0)*360+720,360)*16));
	  alen = int(round(pielen*16));
	  x0+=r-rs;
	  y0+=r-rs;
	  r=rs;

	  if(i==0)p.setBrush(Qt::white);
	  else p.setBrush(mask_fill_color);
	  p.drawEllipse(x0-r,y0-r,2*r,2*r);
	  if(i==0)p.setBrush(Qt::black);
	  else p.setBrush(Qt::color1);
	  p.drawPie(x0-r,y0-r,2*r,2*r,a,alen);
	  p.drawPie(x0-r,y0-r,2*r,2*r,(a+2880)%5760,alen);
	}
      
      p.flush();
      p.end();
    }

#if 0
  QImage img = pix.convertToImage();

  QImage m(pixwidth, pixheight, 1, 2, QImage::LittleEndian);
  m.setColor( 0, 0xffffff );
  m.setColor( 1, 0 );
  m.fill(0xff);

  QRgb bc=Qt::white.rgb();

  for(int i=0; i<pixheight; i++)
    {
      QRgb* il = reinterpret_cast<QRgb*>(img.scanLine(i));
      uchar* ml = m.scanLine(i);
      for(int j=0; j<pixwidth; j++)
	if(il[j] == bc)ml[j>>3] &= ~(0x01 << (j&7));
    }
  
  QBitmap mask;
  mask.convertFromImage(m);
#endif

  pix.setMask(mask);

#if 0
  QImage img = pix.convertToImage();
  img=img.smoothScale(size,QImage::ScaleMin);
  pix.convertFromImage(img);
#endif
  
  return pix;
}

// ============================================================================
// Global Functions
// ============================================================================

void activateLE(bool active, QLineEdit* le, const QColor& f, const QColor& b)
{
  if(active)
    {
      le->setPaletteBackgroundColor(b);
      le->setPaletteForegroundColor(f);
      le->setEnabled(true);
    }
  else
    {
      le->unsetPalette();
      le->setEnabled(false);
    }
}

QRegExp raValidatorRE()
{
  QRegExp
    re("(0?[0-9]?|1[0-9]|2[0-3]):[0-5]?[0-9]?(:[0-5]?[0-9]?([.][0-9]+)?)?"
       "|(0?[0-9]?|1[0-9]|2[0-3])h[0-5]?[0-9]?m([0-5]?[0-9]?([.][0-9]+)?s)?"
       "|(0?[0-9]?|1[0-9]|2[0-3]) [0-5]?[0-9]? ([0-5]?[0-9]?([.][0-9]+)?)?");
  return re;
}

QRegExp decValidatorRE()
{
  QRegExp
    re("[+-]?([0-8]?[0-9]?:[0-5]?[0-9]?(:[0-5]?[0-9]?([.][0-9]+)?)?"
       "|[0-8]?[0-9]?d[0-5]?[0-9]?m([0-5]?[0-9]?([.][0-9]+)?s)?"
       "|[0-8]?[0-9]? [0-5]?[0-9]? ([0-5]?[0-9]?([.][0-9]+)?)?"
       "|90d0?0m(0?0(.0)s)?"
       "|90 0?0 (0?0(.0))?"
       "|90:0?0(:0?0(.0)?)?)");
  return re;
}

QRegExp epochValidatorRE()
{
  QRegExp re("(19|20|21)[0-9]{2}([.][0-9]+)?|2200([.]0+)?");
  return re;
}
