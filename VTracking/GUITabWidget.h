//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITabWidget.h
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
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUITABWIDGET_H
#define VTRACKING_GUITABWIDGET_H

#include<vector>
#include<map>

#include<qtabwidget.h>

#include"GUIMisc.h"
#include"GUIUpdateData.h"

class GUITabPaneGetUpdateData
{
public:
  virtual ~GUITabPaneGetUpdateData();
  virtual std::vector<GUIUpdateData> 
  getUpdateData(const std::vector<GUIUpdateData>& ud) const = 0;
};

class GUITabPane
{
public:
  GUITabPane(QWidget* widget): m_widget(widget), m_enable_controls(true) { }
  virtual ~GUITabPane();
  
  virtual void updateArray(const std::vector<GUIUpdateData>& ud);
  virtual void update(const GUIUpdateData& ud);

  void setControlsEnable(bool enable) { m_enable_controls=enable; }
  bool controlsEnabled() const { return m_enable_controls; }

  QWidget* widget() { return m_widget; }
private:
  QWidget* m_widget;
  bool     m_enable_controls;
};

class GUIAccessDeniedWidget: public QWidget
{
  Q_OBJECT
public:
  GUIAccessDeniedWidget(QWidget* parent, const char* name);
  virtual ~GUIAccessDeniedWidget();
protected:
  void paintEvent(QPaintEvent* ev);
private:
  QPixmap* m_pix;
};

class GUITabWidget: public QFrame
{
  Q_OBJECT
public:
  GUITabWidget(QWidget* parent=0, const char* name=0);
  virtual ~GUITabWidget();
  
  void addManagedPane(const QString& label, GUITabPane* pane,
		      unsigned ud_index,
		      unsigned display_level, unsigned interaction_level,
		      const GUITabPaneGetUpdateData* ud_getter = 0);

  void update(const std::vector<GUIUpdateData>& ud_vec);

  void setProtectionLevel(unsigned level);
  unsigned protectionLevel() const { return m_protection_level; }

  QWidget* currentPage() const { return m_tabber->currentPage(); }

signals:
  void paneChanged(QWidget*);

public slots:
  void setCurrentPage(QWidget* widget);
  void setHighlighting(QWidget* w, bool highlight=true, const QColor& c = red);

private slots:
  void catchTabChanged(QWidget* w);

protected:
  virtual void resizeEvent (QResizeEvent* e);
  bool eventFilter(QObject* o, QEvent* e);

private:
  class Pane
  {
  public:
    Pane():
      label(), pane(0), ud_index(0), ud_getter(0), 
      display_level(0), interaction_level(0) { }
    Pane(const QString& l, GUITabPane* p, unsigned index,
	 unsigned dl, unsigned il, const GUITabPaneGetUpdateData* getter):
      label(l), pane(p), ud_index(index), ud_getter(getter),
      display_level(dl), interaction_level(il) { }
    QString label;
    GUITabPane* pane;
    unsigned ud_index;
    const GUITabPaneGetUpdateData* ud_getter;
    unsigned display_level;
    unsigned interaction_level;
  };
  
  unsigned                        m_protection_level;
  std::vector<Pane*>              m_panes;
  std::map<QWidget*, Pane*>       m_pane_map;

  MyQTabWidget*                   m_tabber;
  GUIAccessDeniedWidget*          m_access_denied;

  bool                            m_event_pending;

  std::vector<GUIUpdateData>      m_last_ud;
};


#endif // VTRACKING_GUITABWIDGET_H
