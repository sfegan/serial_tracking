//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIAzElIndicator.h
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
 * $Date: 2007/01/29 01:04:23 $
 * $Revision: 2.9 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIAZELINDICATOR_H
#define VTRACKING_GUIAZELINDICATOR_H

#include<qwidget.h>

#include"GUIMisc.h"
#include"GUIUpdateData.h"

class GUIAzElIndicator: public QWidget
{
  Q_OBJECT
public:
  GUIAzElIndicator(bool suppress_servo_fail_error=false,
		   QWidget* parent=0, const char* name=0, WFlags f=0):
    QWidget(parent,name,f|WNoAutoErase), 
    m_active(false), m_has_tar(false), 
    m_state(VTracking::TelescopeController::TS_COM_FAILURE), 
    m_message("COM FAIL - L"), m_message_color(MC_WARN),
    m_sv_demand(SV_AUTO), m_sv(SV_AUTO),
    m_tar_az(), m_tar_el(), m_tel_az(), m_tel_el(), m_az_glow(), m_el_glow(),
    m_az_dps(), m_el_dps(),
    m_last_tel_az(), m_last_tel_el(), m_last_sv(SV_AUTO),
    m_az_ver_on(true), m_el_ver_on(true),
    m_x(), m_y(), m_suppress_servo_fail_error(suppress_servo_fail_error)
  { }

  virtual ~GUIAzElIndicator();

  void update(const GUIUpdateData& ud);

public slots:
  void setScopeValuesToDisplay(ScopeValuesToDisplay sv);

protected:
  virtual void paintEvent(QPaintEvent* ev);

private:
  static void drawVernier(QPainter& painter,
			  const QColor& _white, const QColor& _black,
			  bool active, int x, int y, int r, double rot);

  bool                                            m_active;
  bool                                            m_has_tar;
  VTracking::TelescopeController::TrackingState   m_state;
  std::string                                     m_message;
  enum { MC_NONE, MC_WARN, MC_GOOD }              m_message_color;
  ScopeValuesToDisplay                            m_sv_demand;
  ScopeValuesToDisplay                            m_sv;
  double                                          m_tar_az;
  double                                          m_tar_el;
  double                                          m_tel_az;
  double                                          m_tel_el;
  bool                                            m_az_glow;
  bool                                            m_el_glow;
  double                                          m_az_dps;
  double                                          m_el_dps;
  double                                          m_last_tel_az;
  double                                          m_last_tel_el;
  ScopeValuesToDisplay                            m_last_sv;
  bool                                            m_az_ver_on;
  bool                                            m_el_ver_on;
  int                                             m_x;
  unsigned                                        m_y;
  bool                                            m_suppress_servo_fail_error;
}; // class GUIAzElIndicator

#endif // VTRACKING_GUIAZELINDICATOR_H
