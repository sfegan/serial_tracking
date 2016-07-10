//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUISunWarning.cpp
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
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#include<iostream> 

#include<VSDataConverter.hpp>
#include<SphericalCoords.h>
#include<Astro.h>

#include"GUIMisc.h"
#include"GUISunWarning.h"

#define UPDATE_DIV 7
#define SUNMOON_DIV 7
#define SCOPE_DIV (7*5)

using namespace VERITAS;
using namespace SEphem;

GUISunWarning::
GUISunWarning(const SEphem::SphericalCoords& earth_pos, 
	      QWidget* parent, const char* name)
  :   QWidget(parent,name,
	      WStyle_StaysOnTop|WStyle_Customize|WStyle_NoBorder|
	      WStyle_Tool|WX11BypassWM),
      m_earth_position(earth_pos), m_sun(), m_moon(), m_pix(0), m_update_no(0)
{
  QFont f;
  f.setPointSize(10);

  QFontMetrics m(f);
  int width = m.width(MAKEDEG("Ang: 288.8"));
  int height = m.height();
  height *= 2; // two lines
  height += 2; // line seperation
  width += height + 2; // size of the sun icon plus spacing
  width += 2*(2+2+2); //box outer space, line width and inner space
  height += 2*(2+2+2); // ditto
  
  m_pix = new QPixmap(width,height);

  QPainter p(m_pix);
  p.fillRect(m_pix->rect(),
	     colorGroup().brush(QColorGroup::Background));

  resize(m_pix->size());
}

GUISunWarning::~GUISunWarning()
{
  delete m_pix;
}

#define SUN_ONLY -0.5
#define SUN_WARN -12.0
#define MOON_WARN 0.0
#define SUN_SEP 60.0

void GUISunWarning::
update(const std::vector<std::pair<unsigned, SEphem::SphericalCoords> >& 
       scope_pos,
       const double& mjd,  const SEphem::Angle& lmst)
{
  if(scope_pos.size())
    {
      unsigned iscope = (m_update_no/SCOPE_DIV)%scope_pos.size();
      std::string ang_text("T");
      ang_text += VSDataConverter::toString(scope_pos[iscope].first+1);
      ang_text += ": ";
      doUpdate(ang_text, scope_pos[iscope].second, mjd, lmst);  
    }
  else
    doUpdate("Stow:", SphericalCoords::makeLatLongDeg(0,0), mjd, lmst);  
}

void GUISunWarning::update(const SEphem::SphericalCoords& scope_pos,
			   const double& mjd,  const SEphem::Angle& lmst)
{
  doUpdate("Ang: ", scope_pos, mjd, lmst);
}


void GUISunWarning::doUpdate(const std::string& ang_text,
			     const SEphem::SphericalCoords& scope_pos,
			     const double& mjd,  const SEphem::Angle& lmst)
{
  if(m_update_no % UPDATE_DIV != 0)
    {
      m_update_no++;
      return;
    }

  bool draw_moon = false;

  SEphem::SphericalCoords sun;
  SEphem::SphericalCoords moon;
  double sun_el_deg;
  double sun_ang_deg;
  double moon_el_deg;
  double moon_ang_deg;

  bool flash = false;

  // SUN
  sun = m_sun.getAzEl(mjd, lmst, m_earth_position);
  sun_el_deg = sun.latitudeDeg();
  sun_ang_deg = sun.separation(scope_pos).deg();

  //  std::cerr << m_earth_position.latitudeDeg() << ' ' << m_earth_position.longitudeDeg() << ' ' << mjd << ' ' << lmst.hmsString(2) << ' ' << sun_el_deg << '\n';

  // Decide what to show...
  if(sun_el_deg > SUN_ONLY)
    {
      if(sun_ang_deg <= SUN_SEP)flash=true;
    }
  else
    {
      // MOON
      moon = m_moon.getAzEl(mjd, lmst, m_earth_position);
      moon_el_deg = moon.latitudeDeg();
      moon_ang_deg = moon.separation(scope_pos).deg();

      if((moon_el_deg > MOON_WARN)&&(sun_el_deg > SUN_WARN))
	{
	  draw_moon = (m_update_no/(UPDATE_DIV*SUNMOON_DIV))%2==1;
	}
      else if(moon_el_deg > MOON_WARN)
	{
	  draw_moon = true;
	}
      else if(sun_el_deg <= SUN_WARN)
	{
	  hide();
	  return;
	}
    }

  bool inverted = flash && ((m_update_no/UPDATE_DIV)%2==1);
    
  QPainter p(m_pix);

  if(inverted)
    {
      p.setPen(QPen(white,2));
      p.fillRect(m_pix->rect(),QBrush(red));
    }
  else
    {
      //p.setPen(QPen(black,2));
      p.setPen(QPen(red,2));
      p.fillRect(m_pix->rect(),
		 colorGroup().brush(QColorGroup::Background));
    }
  
  int width = m_pix->width();
  int height = m_pix->height();

  p.drawRect(3,3,width-5,height-5);

  width -= 2*(2+2+2);
  height -= 2*(2+2+2);

  
  double el_deg;
  double ang_deg;

  if(draw_moon)
    {
      double moonphase = Astro::moonPhase(mjd);
      QString mp_str = QString::number(moonphase*100,'f',0);

      el_deg = moon_el_deg;
      ang_deg = moon_ang_deg;

      QFont f;
      f.setPointSize(8);
      p.setFont(f);
  
      QWMatrix mtx = p.worldMatrix();
      p.translate(6+height/2, 6+height/2);
      p.drawArc(-height/2,-height/2,height,height,90*16,180*16);
      p.drawArc(-height/4,-height/2,2*height/4,height,100*16,160*16);
      p.setPen(QPen(p.pen().color(),0,Qt::DotLine));
      p.drawArc(-height/2,-height/2,height,height,270*16,180*16);
      p.drawText(-height/4,-height/2,3*height/4,height,
		 Qt::AlignHCenter|Qt::AlignVCenter,mp_str);
      p.setWorldMatrix(mtx);
    }
  else
    {
      el_deg = sun_el_deg;
      ang_deg = sun_ang_deg;

      int dia=height/2;

      p.drawEllipse(6+dia/2,6+dia/2,height-(dia/2)*2,height-(dia/2*2));
      for(unsigned i=0;i<8;i++)
	{
	  double th = double(i)/4.0*M_PI;
	  int x1 = 6+int(round(dia*(1.0+cos(th))));
	  int y1 = 6+int(round(dia*(1.0+sin(th))));
	  int x2 = 6+int(round(dia*(1.0+0.5*cos(th))));
	  int y2 = 6+int(round(dia*(1.0+0.5*sin(th))));
	  p.drawLine(x1,y1,x2,y2);
	}
    }

  QFont f;
  f.setPointSize(10);
  p.setFont(f);
  
  QString el_str = 
    QString("El: ")+MAKEDEG(QString::number(el_deg,'f',1));
  QString ang_str = 
    QString(ang_text)+MAKEDEG(QString::number(ang_deg,'f',1));

  int x = 6 + height + 2;
  int y = 6;
  int w = width - height - 2;
  int h = (height - 2)/2;
  p.drawText(x,y,w,h,Qt::AlignLeft|Qt::AlignTop,el_str);
  y+=h+2;
  p.drawText(x,y,w,h,Qt::AlignLeft|Qt::AlignTop,ang_str);

  m_update_no++;
  show();
  QWidget::update();
  return;
}

void GUISunWarning::paintEvent(QPaintEvent* ev)
{
  bitBlt(this,ev->rect().topLeft(),m_pix,ev->rect());
}
