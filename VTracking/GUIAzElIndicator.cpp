//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIAzElIndicator.cpp
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
 * $Date: 2007/12/06 01:17:57 $
 * $Revision: 2.16 $
 * $Tag$
 *
 **/

#include<qpainter.h>
#include<qpixmap.h>

#include<Angle.h>
#include"GUIMisc.h"
#include"GUIPixmaps.h"
#include"GUIAzElIndicator.h"

using namespace SEphem;
using namespace VTracking;

GUIAzElIndicator::~GUIAzElIndicator()
{
  // nothing to see here
}

void GUIAzElIndicator::update(const GUIUpdateData& ud)
{
  // Remember the important stuff

  m_has_tar = (ud.tar_object!=0)&&(ud.in_limits);
  if(ud.full_update)
    {
      m_active = ud.tse.state!=TelescopeController::TS_COM_FAILURE;
      m_state = ud.tse.state;

      m_last_tel_az = m_tel_az;
      m_last_tel_el = m_tel_el;
      if(m_last_sv != SV_AUTO)m_last_sv = m_sv;
    }

  if(m_sv_demand == SV_AUTO)
    {
      if((ud.tar_object!=0)&&(!ud.tar_object->useCorrections()))
	m_sv = SV_ENCODER;
      else if(m_has_tar)m_sv = SV_SKY;
    }
  else
    {
      m_sv = m_sv_demand;
    }

  if(m_sv == SV_ENCODER)
    {
      m_tel_az = ud.tse.status.az.driveangle_deg;
      m_tel_el = ud.tse.status.el.driveangle_deg;
    }
  else
    {
      if(ud.tse.status.az.driveangle_deg < -45)
	m_tel_az = ud.tel_azel.phi().deg() - 360;
      else if(ud.tse.status.az.driveangle_deg > 45)
	m_tel_az = ud.tel_azel.phi().deg();
      else
	m_tel_az = ud.tel_azel.phi().degPM();
      m_tel_el = ud.tel_azel.latitude().degPM();
    }

  m_az_glow = ((ud.tse.status.az.driveangle_deg>=WARNANGLE_CW)||
	       (ud.tse.status.az.driveangle_deg<=WARNANGLE_CC));
  m_el_glow = ((ud.tse.status.el.driveangle_deg>=WARNANGLE_UP)||
	       (ud.tse.status.el.driveangle_deg<=WARNANGLE_DN));

  if(m_last_sv == SV_AUTO)
    {
      // Should only handle initial case and post COM_FAILURE
      m_last_sv = m_sv;
      m_last_tel_az = m_tel_az;
      m_last_tel_el = m_tel_el;
    }

  if(!m_active)m_last_sv=SV_AUTO;

  m_az_dps = ud.tse.az_driveangle_estimated_speed_dps;
  m_el_dps = ud.tse.el_driveangle_estimated_speed_dps;

  if(ud.tse.state==TelescopeController::TS_COM_FAILURE)
    {
      if(ud.tse.cf == TelescopeController::CF_SERVER)
	m_message = "NO SERVER";
      else m_message = "SCOPE COM FAIL";
      m_message_color=MC_WARN;
    }
  else if(ud.tse.status.interlock
	  || ud.tse.status.interlockAzPullCord
	  || ud.tse.status.interlockAzStowPin
	  || ud.tse.status.interlockElStowPin
	  || ud.tse.status.interlockAzDoorOpen
	  || ud.tse.status.interlockElDoorOpen
	  || ud.tse.status.interlockSafeSwitch
	  || !ud.tse.status.remoteControl)
    {
      m_message = "INTERLOCK";
      m_message_color=MC_WARN;      
    }
  else if(ud.tse.status.el.limitCwUp
	  || ud.tse.status.el.limitCcwDown
	  || ud.tse.status.az.limitCwUp
	  || ud.tse.status.az.limitCcwDown)
    {
      m_message = "LIMIT";
      m_message_color=MC_WARN;   
    }
  else if((!m_suppress_servo_fail_error 
	   && ( ud.tse.status.az.servo1Fail
		|| ud.tse.status.az.servo2Fail
		|| ud.tse.status.el.servo1Fail
		|| ud.tse.status.el.servo2Fail ) )
	  || ud.tse.status.az.positionFault
	  || ud.tse.status.el.positionFault
	  || !ud.tse.status.checksumOK
	  || ud.tse.status.msgBadFrame
	  || ud.tse.status.msgCommandInvalid
	  || ud.tse.status.msgInputOverrun
	  || ud.tse.status.msgOutputOverrun)
    {
      m_message = "ERROR";
      m_message_color=MC_WARN;      
    }
  else if(ud.tse.state==TelescopeController::TS_TRACK)
    {
      m_message = "TRACKING";
      m_message_color=MC_GOOD;
    }
  else if(ud.tse.state==TelescopeController::TS_RAMP_DOWN)
    {
      m_message = "RAMP DOWN";
      m_message_color=MC_NONE;
    }
  else if(m_has_tar)
    {
      if(ud.tse.state==TelescopeController::TS_STOP)
	m_message = "STOP";
      else if(ud.tse.state==TelescopeController::TS_SLEW)
	m_message = "SLEW";
      else if(ud.tse.state==TelescopeController::TS_RESTRICTED_MOTION)
	m_message = "MODSLEW";
      m_message += " - ETA: ";
      m_message += ud.eta_string;
      m_message_color=MC_NONE;
    }
  else
    {
      m_message = "STOPPED";
      m_message_color=MC_NONE;
    }

  if(m_sv == SV_ENCODER)
    {
      m_tar_az = Angle::toDeg(ud.tar_az_driveangle);
      m_tar_el = Angle::toDeg(ud.tar_el_driveangle);
    }
  else
    {
      double tar_az = Angle::toDeg(ud.tar_az_driveangle);
      if(tar_az < -45)
	m_tar_az = ud.tar_azel.phi().deg() - 360;
      else if(tar_az > 45)
	m_tar_az = ud.tar_azel.phi().deg();
      else
	m_tar_az = ud.tar_azel.phi().degPM();

      //m_tar_az = ud.tar_azel.phi().degPM();
      m_tar_el = ud.tar_azel.latitude().degPM();
    }

  m_x = int(floor(30*(1+sin(fmod(ud.mjd*86400/2,1)*Angle::sc_twoPi))));
  m_y++;


  // Schedule a repaint
  QWidget::update();
}

void GUIAzElIndicator::setScopeValuesToDisplay(ScopeValuesToDisplay sv)
{
  m_sv_demand = sv;
}

#define MULTIPLIER    2

#define LINEW         2
#define BORDER        1 * MULTIPLIER
#define AZWIDTH       8 * MULTIPLIER
#define AZSPACE       2 * MULTIPLIER
#define AZTICKS       5 * MULTIPLIER
#define AZNEWS        2 * MULTIPLIER

#define ELSPACE       AZSPACE
#define ELWIDTH       AZWIDTH
#define ELTICKS       AZTICKS

#define WWIDGET       16 * MULTIPLIER
#define LWIDGET       8  * MULTIPLIER

#define RAZIND(x)     ( x - BORDER - AZWIDTH/2 )
#define RAZDIAL(x)    ( x - BORDER - AZWIDTH - AZSPACE )
#define RAZTICKO(x)   ( x - BORDER - AZWIDTH - AZSPACE - MULTIPLIER )
#define RAZTICKISM(x) ( x - BORDER - AZWIDTH - AZSPACE - AZTICKS )
#define RAZTICKILG(x) ( x - BORDER - AZWIDTH - AZSPACE - 2*AZTICKS )
#define RAZNEWS(x)    ( x - BORDER - AZWIDTH - AZSPACE - 2*AZTICKS - AZNEWS )
#define RAZWIDGET(x)  ( RAZDIAL(x) + WWIDGET/2 )

#define RELIND(x)     ( 29*RAZNEWS(x)/27 - ELWIDTH/2 )
#define RELDIAL(x)    ( 29*RAZNEWS(x)/27 - ELWIDTH - ELSPACE )
#define RELTICKO(x)   ( 29*RAZNEWS(x)/27 - ELWIDTH - ELSPACE - MULTIPLIER )
#define RELTICKISM(x) ( 29*RAZNEWS(x)/27 - ELWIDTH - ELSPACE - ELTICKS )
#define RELTICKILG(x) ( 29*RAZNEWS(x)/27 - ELWIDTH - ELSPACE - 2*ELTICKS )

#define XELIND(x)     ( 10*RAZNEWS(x)/27 )
#define YELIND(x)     ( 7*RAZNEWS(x)/27 )
#define RELWIDGET(x)  ( RELDIAL(x) + WWIDGET/2)

#define RVERLG(x)     ( int(round(double(x-BORDER-AZTICKS)*(sqrt(2)-1)*(sqrt(2)-1))) )
#define RVERSM(x)     ( RVERLG(x)/2 )

#define XMESSAGE(x)   ( 0 )
#define YMESSAGE(x)   ( 3*RAZNEWS(x)/5 )
#define WMESSAGE(x)   ( 5*RAZNEWS(x)/8 )
#define HMESSAGE(x)   ( 10 * MULTIPLIER )

#define WELDIGITAL(x) ( RVERLG(x)+RVERSM(x) )
#define HELDIGITAL(x) ( HMESSAGE(X) )
#define XELDIGITAL(x) ( XELIND(x) + 7*RELIND(x)/59 - WELDIGITAL(x) )
#define YELDIGITAL(x) ( YELIND(x) + 5*RELIND(x)/59 - HELDIGITAL(x) )

#define XELVERLG(x)   ( XELDIGITAL(x) + WELDIGITAL(x) - RVERLG(x) )
#define YELVERLG(x)   ( YELDIGITAL(x) - 8*HELDIGITAL(x)/4 - RVERLG(x) )
#define XELVERSM(x)   ( XELVERLG(x) - RVERLG(x) - RVERSM(x) )
#define YELVERSM(x)   ( YELDIGITAL(x) - 8*HELDIGITAL(x)/4 - RVERSM(x) )

#define WAZDIGITAL(x) ( 57*x/216-1 )
#define HAZDIGITAL(x) ( HELDIGITAL(x) )
#define XAZDIGITAL(x) ( -int(round(double(x-BORDER+HAZDIGITAL(x)+3)/sqrt(2))) )
#define YAZDIGITAL(x) ( XAZDIGITAL(x) )

#define XAZVERLG(x)   ( - x + BORDER + RVERLG(x) )
#define YAZVERLG(x)   ( x - BORDER - RVERLG(x) )
#define XAZVERSM(x)   ( XAZVERLG(x) + RVERLG(x) + RVERSM(x) )
#define YAZVERSM(x)   ( x - BORDER - RVERSM(x) )

#if 0
#define XSTATUS(x)    ( x - BORDER - 32 * MULTIPLIER )
#define YSTATUS(x)    ( x - BORDER - 20 * MULTIPLIER )
#else
#define WSTATUS(x)    ( RVERLG(x) )
#define HSTATUS(x)    ( 2*WSTATUS(x)/3 )
#define XSTATUS(x)    ( x - BORDER - WSTATUS(x) - 4 )
#define YSTATUS(x)    -( x - BORDER - HSTATUS(x) - 4 )
#endif

void GUIAzElIndicator::
drawVernier(QPainter& painter, const QColor& _white, const QColor& _black,
	    bool active, int x, int y, int r, double rot)
{
  painter.setPen(QPen(_black,1));
  painter.setBrush(QBrush(_white));
  painter.drawEllipse(x-r,y-r,2*r,2*r);
  painter.setBrush(QBrush(_black));
  int a1 = int(round(fmod(rot+0.125,1)*360*16));
  int a2 = int(round(fmod(rot+0.625,1)*360*16));
  painter.drawPie(x-r,y-r,2*r,2*r,a1,1440);
  painter.drawPie(x-r,y-r,2*r,2*r,a2,1440);
  if(active)painter.setPen(QPen(black,1));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(x-r,y-r,2*r,2*r);
}

void GUIAzElIndicator::paintEvent(QPaintEvent* ev)
{
  bool _disabled = (!isEnabled())||(!m_active);

  QColor c_transparent = palette().active().background();
  QColor c_inactive    = palette().disabled().foreground();
  QColor c_back        = _disabled ? c_transparent : white;
  QColor c_back_grey   = _disabled ? c_transparent : QColor(220,220,220);
  QColor c_back_yellow = _disabled ? c_transparent : yellow;
  QColor c_fore        = _disabled ? c_inactive    : black;
  QColor c_good        = _disabled ? c_inactive    : blue;
  QColor c_warn        = _disabled ? c_inactive    : QColor(255-m_x,m_x,m_x);
  QColor c_widget      = red;
  QColor c_vernier_overspeed = QColor(220,220,220);

  int width = rect().width();
  int height = rect().height();
  int radius = (height>width?width:height)/2;

  QPixmap pix(width,height);
  QPainter painter(&pix);

  painter.fillRect(0,0,width,height,
		   colorGroup().brush(QColorGroup::Background));
  painter.translate(width/2, height/2);
  painter.scale(1.0/MULTIPLIER,1.0/MULTIPLIER);
  QFont f = painter.font();
  f.setPointSize(f.pointSize()*MULTIPLIER);
  painter.setFont(f);
  radius *= MULTIPLIER;

  // -----------------
  // AZIMUTH DIAL FACE
  // -----------------

  painter.setPen(QPen(c_fore,2));
  painter.setBrush(QBrush(c_back_grey));
  painter.drawEllipse(-RAZDIAL(radius),-RAZDIAL(radius),
		      2*RAZDIAL(radius),2*RAZDIAL(radius));

  painter.setPen(QPen(c_fore,0));
  for(unsigned itheta=0;itheta<360;itheta+=5)
    {
      if(itheta%15 == 0)painter.setPen(QPen(c_fore,2));
#if 0
      int x1 = int(round(RAZTICKO(radius)*cos(M_PI*double(itheta)/180.0)));
      int y1 = int(round(RAZTICKO(radius)*sin(M_PI*double(itheta)/180.0)));
      int x2 = int(round(RAZTICKI(radius)*cos(M_PI*double(itheta)/180.0)));
      int y2 = int(round(RAZTICKI(radius)*sin(M_PI*double(itheta)/180.0)));
      painter.drawLine(x1,y1,x2,y2);
#else
      painter.save();
      painter.rotate(double(itheta));
      if(itheta%15 == 0)
	painter.drawLine(0,RAZTICKILG(radius),0,RAZTICKO(radius));
      else
	painter.drawLine(0,RAZTICKISM(radius),0,RAZTICKO(radius));
      painter.restore();
#endif
      if(itheta%15 == 0)painter.setPen(QPen(c_fore,0));
    }

  QString N("N");
  QString S("S");
  QString E("E");
  QString W("W");

  if(m_sv == SV_ENCODER)
    {
      static const QString e000(MAKEDEG("0"));
      static const QString e180(MAKEDEG("180"));
      static const QString e090(MAKEDEG("90"));
      static const QString e270(MAKEDEG("270"));
      N = e000;
      S = e180;
      E = e090;
      W = e270;
    }

  painter.drawText(0, -RAZNEWS(radius), 0, 0,
		   Qt::AlignTop|Qt::AlignHCenter|Qt::DontClip, N);
  painter.drawText(0, RAZNEWS(radius), 0, 0,
		   Qt::AlignBottom|Qt::AlignHCenter|Qt::DontClip, S);
  painter.drawText(RAZNEWS(radius), 0, 0, 0,
		   Qt::AlignRight|Qt::AlignVCenter|Qt::DontClip, E);
  painter.drawText(-RAZNEWS(radius), 0, 0, 0,
		   Qt::AlignLeft|Qt::AlignVCenter|Qt::DontClip, W);

  // -----------------
  // AZIMUTH INDICATOR
  // -----------------

  if(m_active)
    {
#define AZLINETYPE SolidLine
      //#define AZLINETYPE DashLine
      if(m_az_glow)
	painter.setPen(QPen(c_warn,AZWIDTH/MULTIPLIER,AZLINETYPE));
      else 
	painter.setPen(QPen(c_good,AZWIDTH/MULTIPLIER,AZLINETYPE));
      int alen = int(round(m_tel_az*16));
      painter.drawArc(-RAZIND(radius),-RAZIND(radius),
		      2*RAZIND(radius),2*RAZIND(radius), 90*16-alen,alen);
    }

  // --------------
  // TARGET AZIMUTH
  // --------------

  if((m_active)&&(m_has_tar))
    {
      painter.save();

      if(fabs(m_tel_az - m_tar_az)>5)
	{
	  painter.setPen(QPen(c_widget,4));

	  const double tick = 10.0;
	  double az1 = m_tel_az;
	  double az2 = m_tar_az;
	  double dx = fmod(double(m_y),tick);
	  if(m_tar_az < m_tel_az)az2 = m_tel_az, az1 = m_tar_az, dx=tick-dx;

	  for(double az = floor(az1/tick - 1.0)*tick+dx; az<az2; az+=tick)
	    {
	      int a1 = int(round((az<az1?az1:az)*16));
	      int a2 = int(round(((az+0.5*tick)>az2?az2:(az+0.5*tick))*16));
	      if(a2>a1)
		painter.drawArc(-RAZDIAL(radius),-RAZDIAL(radius),
				2*RAZDIAL(radius),2*RAZDIAL(radius), 
				90*16-a1,-(a2-a1));
	    }
	}

      painter.setPen(QPen(c_widget,2));
      painter.rotate(m_tar_az-90);
      painter.translate(RAZWIDGET(radius),0);
      painter.drawLine(0,0,-WWIDGET,-LWIDGET);
      painter.drawLine(0,0,-WWIDGET,+LWIDGET);
      painter.drawLine(-WWIDGET,-LWIDGET,-WWIDGET,+LWIDGET);
      painter.restore();
    }

  // -------------------
  // ELEVATION DIAL FACE
  // -------------------

  painter.save();
  painter.translate(XELIND(radius),YELIND(radius));

  painter.setPen(QPen(c_fore,2));
  painter.drawArc(-RELDIAL(radius),-RELDIAL(radius),
		  2*RELDIAL(radius),2*RELDIAL(radius),85*16,100*16);

  painter.setPen(QPen(c_fore,0));
  for(int itheta=-5;itheta<=95;itheta+=5)
    {
      if(itheta%15 == 0)painter.setPen(QPen(c_fore,2));
      painter.save();
      painter.rotate(double(180+itheta));
      if(itheta%15 == 0)
	painter.drawLine(RELTICKILG(radius),0,RELTICKO(radius),0);
      else
	painter.drawLine(RELTICKISM(radius),0,RELTICKO(radius),0);
      painter.restore();
      if(itheta%15 == 0)painter.setPen(QPen(c_fore,0));
    }

  for(int itheta=0;itheta<=90;itheta+=30)
    {
      double xd = -(RELTICKILG(radius)-2)*cos(double(itheta)/180.0*M_PI);
      double yd = -(RELTICKILG(radius)-2)*sin(double(itheta)/180.0*M_PI);
      int flags = Qt::AlignLeft|Qt::AlignTop;
      if(itheta==0)flags = Qt::AlignLeft|Qt::AlignVCenter;
      else if(itheta==90)flags = Qt::AlignTop|Qt::AlignHCenter;
      painter.drawText(int(round(xd)), int(round(yd)), 0, 0,
		       flags|Qt::DontClip, MAKEDEG(QString::number(itheta)));
    }
  
  // -------------------
  // ELEVATION INDICATOR
  // -------------------

  if(m_active)
    {
#define ELLINETYPE SolidLine
      //#define ELLINETYPE DashLine
      if(m_el_glow)
	painter.setPen(QPen(c_warn,ELWIDTH/MULTIPLIER,ELLINETYPE));
      else 
	painter.setPen(QPen(c_good,ELWIDTH/MULTIPLIER,ELLINETYPE));
      int alen = -int(round(m_tel_el*16));
      painter.drawArc(-RELIND(radius),-RELIND(radius),
		      2*RELIND(radius),2*RELIND(radius), 180*16, alen);
    }

  // ----------------
  // TARGET ELEVATION
  // ----------------

  if((m_active)&&(m_has_tar))
    {
      painter.save();
      if(m_tar_el > -5)
	{
	  painter.rotate(180+m_tar_el);
	  painter.setPen(QPen(c_widget,2));
	}
      else
	{
	  painter.rotate(175);
	  painter.setPen(QPen(c_warn,2));
	}
      painter.translate(RELWIDGET(radius),0);
      painter.drawLine(0,0,-WWIDGET,-LWIDGET);
      painter.drawLine(0,0,-WWIDGET,+LWIDGET);
      painter.drawLine(-WWIDGET,-LWIDGET,-WWIDGET,+LWIDGET);
      painter.restore();
    }

  painter.restore();

  // -------------------
  // ENCODER COORDINATES
  // -------------------

  if((m_active)&&(m_sv==SV_ENCODER))
    {
      painter.save();
      double xd = -(double(RAZDIAL(radius))*0.725*cos(45.0/180.0*M_PI));
      double yd = -(double(RAZDIAL(radius))*0.725*sin(45.0/180.0*M_PI));
      painter.setPen(QPen(QColor(0,0,160)/* QColor(124,125,76)*/ ));
      painter.translate(int(round(xd)), int(round(yd)));
      painter.rotate(-45);
      painter.drawText(0,0,0,0,Qt::AlignHCenter|Qt::AlignVCenter|Qt::DontClip,
		       "ENCODER\nCOORDINATES");
      painter.restore();
    }


  // ----------------
  // AZIMUTH VERNIERS
  // ----------------

  painter.setPen(QPen(c_fore,0));
  drawVernier(painter, c_back, c_fore, m_active,
	      XAZVERLG(radius), YAZVERLG(radius), RVERLG(radius),
	      -m_tel_az/2);

  double az_ver_change = fabs(m_tel_az-m_last_tel_az)*40;
  if((m_active)&&(m_last_sv==m_sv))
    {
      if(az_ver_change>0.25)m_az_ver_on = false;
      else if(az_ver_change<=0.20)m_az_ver_on = true;
    }
  else m_az_ver_on = true;

  if(!m_az_ver_on)
    drawVernier(painter, c_vernier_overspeed, c_vernier_overspeed, m_active,
		XAZVERSM(radius), YAZVERSM(radius), RVERSM(radius),
		-m_tel_az*40);
  else
    drawVernier(painter, c_back, c_fore, m_active,
		XAZVERSM(radius), YAZVERSM(radius), RVERSM(radius),
		-m_tel_az*40);

  // ------------------
  // ELEVATION VERNIERS
  // ------------------

  painter.setPen(QPen(c_fore,0));
  drawVernier(painter, c_back, c_fore, m_active,
	      XELVERLG(radius), YELVERLG(radius), RVERLG(radius),
	      -m_tel_el/2);

  double el_ver_change = fabs(m_tel_el-m_last_tel_el)*40;
  if((m_active)&&(m_last_sv==m_sv))
    {
      if(el_ver_change>0.25)m_el_ver_on = false;
      else if(el_ver_change<=0.20)m_el_ver_on = true;
    }
  else m_el_ver_on = true;

  if(!m_el_ver_on)
    drawVernier(painter, c_vernier_overspeed, c_vernier_overspeed, m_active,
		XELVERSM(radius), YELVERSM(radius), RVERSM(radius),
		-m_tel_el*40);
  else
    drawVernier(painter, c_back, c_fore, m_active,
		XELVERSM(radius), YELVERSM(radius), RVERSM(radius),
		-m_tel_el*40);

  // -------------------------------------
  // ELEVATION AND AZIMUTH DIGITAL READOUT
  // -------------------------------------

  painter.setPen(QPen(c_fore,2));
  if(m_sv == SV_ENCODER)painter.setBrush(QBrush(c_back_yellow));
  else painter.setBrush(QBrush(c_back));

  painter.drawRect(XELDIGITAL(radius)-WELDIGITAL(radius),
		   YELDIGITAL(radius)-HELDIGITAL(radius),
		   2*WELDIGITAL(radius),2*HELDIGITAL(radius));

  if(m_active)
    {
      QString eltxt = MAKEDEG(QString::number(m_tel_el,'f',2));
      painter.drawText(XELDIGITAL(radius)-WELDIGITAL(radius),
		       YELDIGITAL(radius)-HELDIGITAL(radius),
		       2*WELDIGITAL(radius),2*HELDIGITAL(radius),
		       Qt::AlignVCenter|Qt::AlignHCenter, eltxt);
    }

  painter.save();
  painter.translate(XAZDIGITAL(radius), YAZDIGITAL(radius));
  painter.rotate(-45);

  painter.drawRect(-WAZDIGITAL(radius),-HAZDIGITAL(radius),
		   2*WAZDIGITAL(radius),2*HAZDIGITAL(radius));

  if(m_active)
    {
      Angle az = Angle::frDeg(m_tel_az);
      QString aztxt = MAKEDEG(QString::number(az.deg(),'f',2));
      painter.drawText(-WAZDIGITAL(radius),-HAZDIGITAL(radius),
		       2*WAZDIGITAL(radius),2*HAZDIGITAL(radius),
		       Qt::AlignVCenter|Qt::AlignHCenter, aztxt);
    }

  painter.restore();

  // ----------------
  // STATUS INDICATOR
  // ----------------

  painter.save();
  painter.translate(XSTATUS(radius), YSTATUS(radius));

#if 0
  switch(m_state)
    {
    case TelescopeController::TS_COM_FAILURE:
      painter.drawPixmap(0,0,*GUIPixmaps::instance()->ind_cf_pixmaps(m_y%16));
      break;
    case TelescopeController::TS_STOP:
      painter.drawPixmap(0,0,*GUIPixmaps::instance()->ind_stop_pixmaps(0));
      break;      
    case TelescopeController::TS_SLEW:
    case TelescopeController::TS_TRACK:
    case TelescopeController::TS_RESTRICTED_MOTION:
    case TelescopeController::TS_RAMP_DOWN:
      painter.drawPixmap(0,0,*GUIPixmaps::instance()->ind_go_pixmaps(m_y%36));
      break;
    }
#else

#define WHEEL_R(x) (HSTATUS(radius)-9)
#define BRAKE_R(x) (WSTATUS(radius)-9)

#define CROSS_Y(x) ( HSTATUS(radius)-7 )
#define CROSS_X(x) ( CROSS_Y(x)/2 )
#define LINE_X1(x) ( WSTATUS(radius)-7 )
#define LINE_X2(x) ( CROSS_X(radius) )
#define LINE_Y(x)  ( 2*HSTATUS(radius)/7 )

  switch(m_state)
    {
    case TelescopeController::TS_COM_FAILURE:
      {
	painter.setPen(QPen(black,2));
	painter.setBrush(Qt::NoBrush);      
	painter.drawRect(-WSTATUS(radius),-HSTATUS(radius),
			 2*WSTATUS(radius),2*HSTATUS(radius));
	painter.drawLine(-CROSS_X(radius),CROSS_Y(radius),
			 CROSS_X(radius),-CROSS_Y(radius));
	painter.drawLine(CROSS_X(radius),CROSS_Y(radius),
			 -CROSS_X(radius),-CROSS_Y(radius));
	painter.drawLine(-LINE_X1(radius),LINE_Y(radius),
			 -LINE_X2(radius),LINE_Y(radius));
	painter.drawLine(LINE_X1(radius),LINE_Y(radius),
			 LINE_X2(radius),LINE_Y(radius));
	painter.drawLine(-LINE_X1(radius),-LINE_Y(radius),
			 -LINE_X2(radius),-LINE_Y(radius));
	painter.drawLine(LINE_X1(radius),-LINE_Y(radius),
			 LINE_X2(radius),-LINE_Y(radius));
	unsigned nanim  = (LINE_X1(radius)-LINE_X2(radius))/3;
	unsigned nframe = 2*(nanim+2);
	unsigned iframe = m_y%nframe;
	unsigned iset   = iframe/(nanim+2);
	unsigned ianim  = iframe%(nanim+2);
	if(ianim<nanim)
	  {
	    painter.setPen(QPen(color_bg_warn,0));
	    painter.setBrush(QBrush(color_bg_warn));
	    int xc = LINE_X1(radius)-3*ianim;
	    int yc = LINE_Y(radius);
	    if(iset==1)xc=-xc,yc=-yc;
	    painter.drawEllipse(xc-5,yc-5,10,10);
	  }
      }
      break;
    case TelescopeController::TS_STOP:
      {
	painter.setPen(QPen(color_bg_warn,2));
	painter.setBrush(Qt::NoBrush);      
	//painter.setPen(QPen(color_bg_warn,1));
	//painter.setBrush(QBrush(color_bg_warn));
	painter.drawRect(-WSTATUS(radius),-HSTATUS(radius),
			 2*WSTATUS(radius),2*HSTATUS(radius));
	//painter.setPen(QPen(palette().active().background(),3));
	//painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(color_bg_warn,3));
	painter.drawEllipse(-WHEEL_R(radius),-WHEEL_R(radius),
			    2*(WHEEL_R(radius)),2*(WHEEL_R(radius)));
	double theta = 
	  atan(double(HSTATUS(radius)-7)/double(BRAKE_R(radius)));
	int a = int(round(theta/M_PI*180.0*16.0));
	painter.drawArc(-BRAKE_R(radius),-BRAKE_R(radius),
			2*(BRAKE_R(radius)),2*(BRAKE_R(radius)),
			-a,2*a);
	painter.drawArc(-BRAKE_R(radius),-BRAKE_R(radius),
			2*(BRAKE_R(radius)),2*(BRAKE_R(radius)),
			180*16-a,2*a);
      }
      break;      
    case TelescopeController::TS_SLEW:
    case TelescopeController::TS_TRACK:
    case TelescopeController::TS_RESTRICTED_MOTION:
    case TelescopeController::TS_RAMP_DOWN:
      {
	painter.setPen(QPen(color_bg_on,2));
	painter.setBrush(QBrush(color_bg_on));
	painter.drawRect(-WSTATUS(radius),-HSTATUS(radius),
			 2*WSTATUS(radius),2*HSTATUS(radius));
	painter.setPen(QPen(palette().active().background(),3));
	painter.setBrush(Qt::NoBrush);      
	int a = (m_y%36)*160;
	painter.drawArc(-WHEEL_R(radius),-WHEEL_R(radius),
			2*(WHEEL_R(radius)),2*(WHEEL_R(radius)),
			-a,140*16);
	painter.drawArc(-WHEEL_R(radius),-WHEEL_R(radius),
			2*(WHEEL_R(radius)),2*(WHEEL_R(radius)),
			180*16-a,140*16);
      }
      break;
    }

#endif
  painter.restore();

  // -----------
  // MESSAGE BOX
  // -----------

  painter.setPen(QPen(black,2));
  switch(m_message_color)
    {
    case MC_NONE:
      //painter.setBrush(Qt::NoBrush);
      painter.setBrush(QBrush(c_back));
      break;
    case MC_WARN:
      painter.setBrush(QBrush(color_bg_warn));
      break;
    case MC_GOOD:
      painter.setBrush(QBrush(color_bg_on));
      break;
    }

  painter.drawRect(XMESSAGE(radius)-WMESSAGE(radius),
		   YMESSAGE(radius)-HMESSAGE(radius),
		   2*WMESSAGE(radius),2*HMESSAGE(radius));

  if(!m_message.empty())
    painter.drawText(XMESSAGE(radius)-WMESSAGE(radius),
		     YMESSAGE(radius)-HMESSAGE(radius),
		     2*WMESSAGE(radius),2*HMESSAGE(radius),
		     Qt::AlignVCenter|Qt::AlignHCenter, m_message);

  // -----------------------------------------------
  // WE'RE DONE, NOW COPY THE PIXMAP ONTO THE WIDGET
  // -----------------------------------------------

  bitBlt(this,ev->rect().topLeft(),&pix,ev->rect());
}
