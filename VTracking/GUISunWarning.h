//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUISunWarning.h
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
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUISUNWARNING_H
#define VTRACKING_GUISUNWARNING_H

#include<qwidget.h>

#include"TargetObject.h"

class GUISunWarning: public QWidget
{
  Q_OBJECT
public:
  GUISunWarning(const SEphem::SphericalCoords& earth_pos, 
		QWidget* parent, const char* name);
  virtual ~GUISunWarning();
  void update(const SEphem::SphericalCoords& scope_pos,
	      const double& mjd,  const SEphem::Angle& lmst);
  void update(const std::vector<std::pair<unsigned, SEphem::SphericalCoords> >& scope_pos,
	      const double& mjd,  const SEphem::Angle& lmst);
protected slots:
  void paintEvent(QPaintEvent* ev);
private:
  void doUpdate(const std::string& ang_text,
		const SEphem::SphericalCoords& scope_pos,
		const double& mjd,  const SEphem::Angle& lmst);
  SEphem::SphericalCoords m_earth_position;
  VTracking::SunObject    m_sun;
  VTracking::MoonObject   m_moon;
  QPixmap*                m_pix;
  unsigned                m_update_no;
};

#endif // VTRACKING_GUISUNWARNING_H
