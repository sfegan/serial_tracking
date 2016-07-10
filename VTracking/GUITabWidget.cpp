//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITabWidget.cpp
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
 * $Revision: 2.6 $
 * $Tag$
 *
 **/

#include<qapplication.h>
#include<qpainter.h>
#include<qlayout.h>
#include<qwidget.h>
#include<qobject.h>
#include<qobjectlist.h>

#include"GUITabWidget.h"


GUITabPaneGetUpdateData::~GUITabPaneGetUpdateData()
{
  // nothing to see here
}

GUITabPane::~GUITabPane()
{
  // nothing to see here
}

void GUITabPane::update(const GUIUpdateData& data)
{
  // nothing to see here
}

void GUITabPane::updateArray(const std::vector<GUIUpdateData>& data)
{
  // nothing to see here
}

GUITabWidget::GUITabWidget(QWidget* parent, const char* name):
  QFrame(parent,name), m_protection_level(), m_panes(), m_pane_map(),
  m_tabber(0), m_access_denied(0), m_event_pending(false), m_last_ud()
{ 
  QString basename(name);
  QGridLayout* layout = new QGridLayout(this,1,1,0,0,basename+" frame layout");
  m_tabber = new MyQTabWidget(this,basename+" tab");
  connect(m_tabber,SIGNAL(currentChanged(QWidget*)),
	  this,SLOT(catchTabChanged(QWidget*)));
  layout->addWidget(m_tabber,0,0);
  qApp->installEventFilter(this);
  m_access_denied = new GUIAccessDeniedWidget(this,"access denied");
}

GUITabWidget::~GUITabWidget()
{
  // nothing to see here
}

void GUITabWidget::addManagedPane(const QString& label, GUITabPane* pane,
				  unsigned ud_index,
				  unsigned display_level, 
				  unsigned interaction_level,
				  const GUITabPaneGetUpdateData* ud_getter)
{
  QWidget* widget=pane->widget();
  Pane* thepane = 
    new Pane(label,pane,ud_index,display_level,interaction_level,ud_getter);
  m_panes.push_back(thepane);
  m_pane_map[widget]=thepane;
  if(protectionLevel()<=display_level)m_tabber->addTab(widget,label);
  else widget->hide();
  if(protectionLevel()<=interaction_level)pane->setControlsEnable(true);
  else pane->setControlsEnable(false);
}

void GUITabWidget::update(const std::vector<GUIUpdateData>& ud)
{
  m_last_ud = ud;
  for(std::vector<Pane*>::iterator i = m_panes.begin(); 
      i != m_panes.end(); i++)
    {
      std::vector<GUIUpdateData> my_ud;
      const std::vector<GUIUpdateData>* use_ud = &m_last_ud;

      if((*i)->ud_getter)
	{
	  my_ud = (*i)->ud_getter->getUpdateData(m_last_ud);
	  use_ud = &my_ud;
	}

      (*i)->pane->updateArray(*use_ud);
      if((*i)->ud_index < use_ud->size())
	(*i)->pane->update((*use_ud)[(*i)->ud_index]);
    }
  for(std::vector<GUIUpdateData>::iterator iud = m_last_ud.begin();
      iud != m_last_ud.end(); iud++)iud->replay=true;
}

void GUITabWidget::setProtectionLevel(unsigned level)
{
  QWidget* w = currentPage();
  Pane* pane = m_pane_map[w];
  
  if(protectionLevel()==level)return;
  m_protection_level=level;

  int c = m_tabber->count();
  for(int i=0;i<c;i++)m_tabber->removePage(m_tabber->page(0));

  for(std::vector<Pane*>::iterator i = m_panes.begin(); 
      i != m_panes.end(); i++)
    {
      if(protectionLevel()<=(*i)->display_level)
	m_tabber->addTab((*i)->pane->widget(),(*i)->label);
      else (*i)->pane->widget()->hide();

      if(protectionLevel()<=(*i)->interaction_level)
      	(*i)->pane->setControlsEnable(true);
      else
      	(*i)->pane->setControlsEnable(false);
    }

  if((pane)&&(protectionLevel()<=pane->display_level))setCurrentPage(w);
  else setCurrentPage(m_tabber->page(0));
}

void GUITabWidget::catchTabChanged(QWidget* w)
{
  Pane* pane = m_pane_map[w];
#if 0
  for(std::vector<Pane*>::iterator i = m_panes.begin();
      i != m_panes.end(); i++)
    Debug::stream()
      << (*i)->pane << ' ' << (*i)->pane->widget() << ' ' 
      << (*i)->display_level << ' ' << (*i)->interaction_level << ' ' 
      << (*i)->label << std::endl;
  Debug::stream() << "Widget: " << w << ", Pane: " << pane << std::endl;
#endif

  if(protectionLevel()<=pane->display_level)
    {
      std::vector<GUIUpdateData> my_ud;
      const std::vector<GUIUpdateData>* use_ud = &m_last_ud;

      if(pane->ud_getter)
	{
	  my_ud = pane->ud_getter->getUpdateData(m_last_ud);
	  use_ud = &my_ud;
	}

      pane->pane->updateArray(*use_ud);
      if(pane->ud_index < use_ud->size())
	pane->pane->update((*use_ud)[pane->ud_index]);
    }

  if(protectionLevel()<=pane->interaction_level)m_access_denied->hide();
  else 
    {
      QPoint p = w->rect().center()-m_access_denied->rect().center();

#if 0
      QWidget* parent = w;
      while(parent->parentWidget())
	{
	  p=parent->mapToParent(p);
	  parent = parent->parentWidget();
	}
#else
      p=w->mapTo(this,p);
#endif

      m_access_denied->move(p);
      m_access_denied->show();
      m_tabber->setFocus();
    }

  emit paneChanged(w);
}

void GUITabWidget::setCurrentPage(QWidget* widget)
{ 
  m_tabber->showPage(widget); 
}

void GUITabWidget::setHighlighting(QWidget* w, bool highlight, const QColor& c)
{
  m_tabber->setHighlighting(w, highlight, c);
}

void GUITabWidget::resizeEvent (QResizeEvent* e)
{
  QFrame::resizeEvent(e);
  QWidget* w = currentPage();
  Pane* pane = m_pane_map[w];
  if(protectionLevel()>pane->interaction_level)
    {
      QPoint p = w->rect().center()-m_access_denied->rect().center();
      p=w->mapTo(this,p);
      m_access_denied->move(p);
    }
}

bool GUITabWidget::eventFilter(QObject* o, QEvent* e)
{
  if((e->type() == QEvent::MouseButtonPress)||
     (e->type() == QEvent::MouseButtonRelease)||
     (e->type() == QEvent::MouseButtonDblClick)||
     (e->type() == QEvent::MouseMove)||
     (e->type() == QEvent::KeyPress)||
     (e->type() == QEvent::KeyRelease)||
     (e->type() == QEvent::IMStart)||
     (e->type() == QEvent::IMCompose)||
     (e->type() == QEvent::IMEnd)||
     (e->type() == QEvent::FocusIn)||
     (e->type() == QEvent::FocusOut)||
     (e->type() == QEvent::Enter)||
     (e->type() == QEvent::Leave)||
     (e->type() == QEvent::Accel)||
     (e->type() == QEvent::Wheel)||
     (e->type() == QEvent::ContextMenu)||
     (e->type() == QEvent::AccelOverride)||
     (e->type() == QEvent::AccelAvailable)||
     (e->type() == QEvent::DragEnter)||
     (e->type() == QEvent::DragMove)||
     (e->type() == QEvent::DragLeave)||
     (e->type() == QEvent::Drop)||
     (e->type() == QEvent::DragResponse))
    {
      QWidget* w = currentPage();
      if(m_pane_map.find(w) == m_pane_map.end())return false;
      Pane* pane = m_pane_map[w];
      if(protectionLevel()<=pane->interaction_level)return false;
      
      QObject* oo = o;
      while(oo)
	{
	  if(oo==w)return true;
	  oo=oo->parent();
	}
    } 
  return false;
}

GUIAccessDeniedWidget::
GUIAccessDeniedWidget(QWidget* parent, const char* name):
  QWidget(parent,name,
	  WStyle_StaysOnTop|WStyle_Customize|WStyle_NoBorder|
	  WStyle_Tool|WX11BypassWM),
  m_pix(0)
{
  m_pix = new QPixmap(290,60);
  QPainter paint(m_pix);
  paint.fillRect(m_pix->rect(),
		 colorGroup().brush(QColorGroup::Background));
  paint.setPen(QPen(red,3));
  paint.drawRect(m_pix->rect().left()+3,m_pix->rect().top()+3,
		 m_pix->rect().right()-6,m_pix->rect().bottom()-6);

  QFont f;
  f.setPointSize(24);
  paint.setFont(f);
  paint.drawText(0,0,m_pix->size().width(),m_pix->size().height(),
		 Qt::AlignHCenter|Qt::AlignVCenter,"Access Restricted");
  resize(m_pix->size());
}

GUIAccessDeniedWidget::~GUIAccessDeniedWidget()
{
  delete m_pix;
}

void GUIAccessDeniedWidget::paintEvent(QPaintEvent* ev)
{
  bitBlt(this,ev->rect().topLeft(),m_pix,ev->rect());
}
