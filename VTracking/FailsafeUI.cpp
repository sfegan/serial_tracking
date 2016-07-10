//-*-mode:c++; mode:font-lock;-*-

/**
 * \file FailsafeUI.cpp
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
 * $Date: 2007/02/03 19:30:57 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<sstream>
#include<fstream>
#include<iostream>
#include<iomanip>

#include<unistd.h>

#include<Exception.h>
#include<Angle.h>
#include<Astro.h>
#include"FailsafeUI.h"
#include"PositionLogger.h"
#include"TextMenu.h"
#include"TelescopeControllerLocal.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

FailsafeUI::~FailsafeUI()
{
  // nothing to see here
}

void FailsafeUI::run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TextMenu menu("Failsafe Menu");
  menu.addItem(TextMenu::Item('?',"Mount Status"));
  menu.addItem(TextMenu::Item('a',"Slew in Azimuth"));
  menu.addItem(TextMenu::Item('e',"Slew in Elevation"));
  menu.addItem(TextMenu::Item('s',"Stop (Ramp Down)"));
  menu.addItem(TextMenu::Item('S',"Standby (Brake On)"));
  menu.addItem(TextMenu::Item('p',"Point to Az, El"));
  menu.addItem(TextMenu::Item('d',"Display Drive Parameters"));
  menu.addItem(TextMenu::Item('Z',"Reset Drive Parameters to Default"));
  menu.addItem(TextMenu::Item('T',"Tune Drive Parameters"));
#ifndef MAKE_FAILSAFE_EXECUTABLE
  menu.addItem(TextMenu::Item('C',"Start Controller Thread"));
  menu.addItem(TextMenu::Item('L',"Start Controller Thread with Position Logging"));
#endif
  menu.addItem(TextMenu::Item('V',"Set verbose level"));
  menu.addItem(TextMenu::Item('x',"Exit"));
  int c=0;

  TextMenu::pressAnyKey();

  while(c!='x')
    {
      c=menu.exec();
      if(!std::cin)break;

      try
	{
	  switch(c)
	    {
	    case '?':
	      status();
	      break;
	    case 'a':
	      slewAz();
	      break;
	    case 'e':
	      slewEl();
	      break;
	    case 's':
	      rampDown();
	      break;
	    case 'S':
	      standby();
	      break;
	    case 'p':
	      point();
	      break;
	    case 'd':
	      displayParameters();
	      break;
	    case 'Z':
	      resetParameters();
	      break;
	    case 'T':
	      tuneParameters();
	      break;
#ifndef MAKE_FAILSAFE_EXECUTABLE
	    case 'C':
	      start_controller();
	      break;
	    case 'L':
	      {
		std::string filename;
		const char* filename_p="";
		std::cout << "Enter log file name: " << std::flush;
		if(std::cin >> filename)filename_p=filename.c_str();
		char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
		start_controller(filename_p);
	      }
	      break;
#endif
	    case 'V':
	      verbose();
	      break;
	    case 'x':
	      break;
	    default:
	      c=0;
	      break;
	    }
	}
      catch(const VTracking::Timeout& x)
	{
	  std::cout 
	    << "============== FailsafeUI::run() caught exception ==============="
	    << std::endl
	    << "Timeout" << std::endl
	    << "Suppressing \"Timeout\" indications until communication is restored"
	    << std::endl
	    << "================================================================="
	 << std::endl;
	}
      catch(const Exception& x)
	{
	  std::cout 
	    << "============== FailsafeUI::run() caught exception ==============="
	    << std::endl;
	  x.print(std::cout);
	  std::cout 
	    << "================================================================="
	    << std::endl;
	}
      catch(...)
	{
	  std::cout 
	    << "============== FailsafeUI::run() caught exception ==============="
	    << std::endl
	    << "An unknown exception has been caught" << std::endl
	    << "================================================================="
	    << std::endl;
	}

      if((c!=0)&&(c!='x'))TextMenu::pressAnyKey();
    }
}

void FailsafeUI::status()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus status;
  m_api->reqStat(status);
  print_status(status);
}

void FailsafeUI::print_status(const ScopeAPI::PositionerStatus& status)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // --------------------------------------------------------------------------
  // Azimuth
  // --------------------------------------------------------------------------

  std::cout 
    << "Az: " 
    << std::showpos << std::setw(9) << std::setprecision(4) << std::fixed
    << status.az.driveangle_deg << ' ';
  if(status.azTravelledCCW)std::cout << "CCW ";

  switch(status.az.driveMode)
    {
    case ScopeAPI::DM_STANDBY:     std::cout << "STANDBY "; break;
    case ScopeAPI::DM_SLEW:        std::cout << "SLEW    "; break;
    case ScopeAPI::DM_POINT:       std::cout << "POINT   "; break;
    case ScopeAPI::DM_SPIN:        std::cout << "SPIN    "; break;
    case ScopeAPI::DM_SECTOR_SCAN: std::cout << "SCAN    "; break;
    case ScopeAPI::DM_RASTER:      std::cout << "RASTER  "; break;
    case ScopeAPI::DM_CHANGING:    std::cout << "CHANGE  "; break;
    case ScopeAPI::DM_UNKNOWN:     std::cout << "UNKNOWN "; break;
    }
  
  if(status.az.limitCwUp)std::cout << "LIMIT-CW ";
  if(status.az.limitCcwDown)std::cout << "LIMIT-CCW ";
  if((status.az.servo1Fail)||(status.az.servo2Fail))std::cout << "SERVO-FAIL ";
  std::cout << std::setw(6) << std::setprecision(2) 
	    << status.azCableWrap*100.0 << '%' << std::endl;

  // --------------------------------------------------------------------------
  // Elevation
  // --------------------------------------------------------------------------

  std::cout 
    << "EL: " 
    << std::showpos << std::setw(9) << std::setprecision(4) << std::fixed
    << status.el.driveangle_deg << ' ';

  switch(status.el.driveMode)
    {
    case ScopeAPI::DM_STANDBY:     std::cout << "STANDBY "; break;
    case ScopeAPI::DM_SLEW:        std::cout << "SLEW    "; break;
    case ScopeAPI::DM_POINT:       std::cout << "POINT   "; break;
    case ScopeAPI::DM_SPIN:        std::cout << "SPIN    "; break;
    case ScopeAPI::DM_SECTOR_SCAN: std::cout << "SCAN    "; break;
    case ScopeAPI::DM_RASTER:      std::cout << "RASTER  "; break;
    case ScopeAPI::DM_CHANGING:    std::cout << "CHANGE  "; break;
    case ScopeAPI::DM_UNKNOWN:     std::cout << "UNKNOWN "; break;
    }
  
  if(status.el.limitCwUp)std::cout << "LIMIT-UP ";
  if(status.el.limitCcwDown)std::cout << "LIMIT-DOWN ";
  if((status.el.servo1Fail)||(status.el.servo2Fail))std::cout << "SERVO-FAIL ";
  std::cout << std::endl;

  // --------------------------------------------------------------------------
  // MISCELLANEOUS
  // --------------------------------------------------------------------------

  if(status.interlock)
    {
      bool s=false;
      std::cout << "INTERLOCK ";

      if((status.interlockAzPullCord)||(status.interlockAzStowPin)||
	 (status.interlockElStowPin)||(status.interlockAzDoorOpen)||
	 (status.interlockElDoorOpen)||(status.interlockSafeSwitch))
	std::cout << '[';

      if(status.interlockAzPullCord)
	{ std::cout << "AZ-PULL-CORD"; s=true; }
      if(status.interlockAzStowPin)
	{ if(s)std::cout << ' '; std::cout << "AZ-STOW-PIN"; s=true; }
      if(status.interlockElStowPin)
	{ if(s)std::cout << ' '; std::cout << "EL-STOW-PIN"; s=true; }
      if(status.interlockAzDoorOpen)
	{ if(s)std::cout << ' '; std::cout << "EL-DOOR-OPEN"; s=true; }
      if(status.interlockElDoorOpen)
	{ if(s)std::cout << ' '; std::cout << "AZ-DOOR-OPEN"; s=true; }
      if(status.interlockSafeSwitch)
	{ if(s)std::cout << ' '; std::cout << "SAFE-SWITCH"; s=true; }
      if(s)std::cout << "] ";
      std::cout << std::endl;
    }
}

void FailsafeUI::slewAz()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  double az_speed=0;
  std::cout << "Enter Az speed in deg/sec: " << std::flush;
  if(std::cin >> az_speed)m_api->cmdSlew(az_speed,0);
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::slewEl()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  double el_speed=0;
  std::cout << "Enter El speed in deg/sec: " << std::flush;
  if(std::cin >> el_speed)m_api->cmdSlew(0,el_speed);
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::rampDown()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_api->cmdSlew(0,0);
}

void FailsafeUI::point()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  double az=0;
  double el=16;
  std::cout << "Enter Az and El seperated by space: " << std::flush;
  if((std::cin >> az)&&(std::cin >> el))
    m_api->cmdPoint(Angle::makeDeg(az), 0.3, Angle::makeDeg(el), 0.3); 
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::standby()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_api->cmdStandby();
}

void FailsafeUI::resetParameters()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  PIUScopeAPI::PIDParameters az_param;
  PIUScopeAPI::PIDParameters el_param;
  Angle az_offset;
  Angle el_offset;

#if 1
  az_param.Kp = 5, el_param.Kp = 5;
  az_param.Ki = 5, el_param.Ki = 5;
  az_param.Kd = 0, el_param.Kd = 0;
  az_param.Kvff = 0, el_param.Kvff = 0;
  az_param.Ilim = 100, el_param.Ilim = 100;
  az_param.alim = 0.16, el_param.alim = 0.16;
  az_param.vlim = 0.4, el_param.vlim = 0.4;
  az_offset.setDeg(0), el_offset.setDeg(0);
#else
  az_param.Kp = 1, el_param.Kp = 11;
  az_param.Ki = 2, el_param.Ki = 12;
  az_param.Kd = 3, el_param.Kd = 13;
  az_param.Kvff = 4, el_param.Kvff = 14;
  az_param.Ilim = 5, el_param.Ilim = 15;
  az_param.alim = 1, el_param.alim = 2;
  az_param.vlim = 0.2, el_param.vlim = 0.3;
  az_offset.setDeg(15), el_offset.setDeg(180);
#endif
  
  m_api->setAzPIDParameters(az_param);
  m_api->setElPIDParameters(el_param);      
  m_api->setAzOffset(az_offset);
  m_api->setElOffset(el_offset);
}

void FailsafeUI::tuneParameters()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  PIUScopeAPI::PIDParameters az_param;
  PIUScopeAPI::PIDParameters el_param;
  Angle az_offset;
  Angle el_offset;

  int c=0;

  while(c!='x')
    {
      m_api->reqAzPIDParameters(az_param);
      m_api->reqElPIDParameters(el_param);
      m_api->reqAzOffset(az_offset);
      m_api->reqElOffset(el_offset);

      std::ostringstream AzKp;   AzKp << az_param.Kp;
      std::ostringstream AzKi;   AzKi << az_param.Ki;
      std::ostringstream AzKd;   AzKd << az_param.Kd;
      std::ostringstream AzKvf; AzKvf << az_param.Kvff;
      std::ostringstream AzIli; AzIli << az_param.Ilim;
      std::ostringstream AzAli; AzAli << az_param.alim;
      std::ostringstream AzVli; AzVli << az_param.vlim;
      std::ostringstream AzOff; AzOff << az_offset.degPMString(3);

      std::ostringstream ElKp;   ElKp << el_param.Kp;
      std::ostringstream ElKi;   ElKi << el_param.Ki;
      std::ostringstream ElKd;   ElKd << el_param.Kd;
      std::ostringstream ElKvf; ElKvf << el_param.Kvff;
      std::ostringstream ElIli; ElIli << el_param.Ilim;
      std::ostringstream ElAli; ElAli << el_param.alim;
      std::ostringstream ElVli; ElVli << el_param.vlim;
      std::ostringstream ElOff; ElOff << el_offset.degPMString(3);

      std::ostringstream AzKey; AzKey << m_key_az;

      TextMenu menu("Tune Drive Parameters Menu (Please Be Careful)");

      menu.addItem(TextMenu::Item('1',std::string("AZIMUTH   Kp   - ")+AzKp.str()));
      menu.addItem(TextMenu::Item('2',std::string("AZIMUTH   Ki   - ")+AzKi.str()));
      menu.addItem(TextMenu::Item('3',std::string("AZIMUTH   Kd   - ")+AzKd.str()));
      menu.addItem(TextMenu::Item('4',std::string("AZIMUTH   Kvff - ")+AzKvf.str()));
      menu.addItem(TextMenu::Item('5',std::string("AZIMUTH   Ilim - ")+AzIli.str()));
      menu.addItem(TextMenu::Item('6',std::string("AZIMUTH   Alim - ")+AzAli.str()));
      menu.addItem(TextMenu::Item('7',std::string("AZIMUTH   Vlim - ")+AzVli.str()));
      menu.addItem(TextMenu::Item('8',std::string("AZIMUTH   Off  - ")+AzOff.str()));

      menu.addItem(TextMenu::Item('A',std::string("ELEVATION Kp   - ")+ElKp.str()));
      menu.addItem(TextMenu::Item('B',std::string("ELEVATION Ki   - ")+ElKi.str()));
      menu.addItem(TextMenu::Item('C',std::string("ELEVATION Kd   - ")+ElKd.str()));
      menu.addItem(TextMenu::Item('D',std::string("ELEVATION Kvff - ")+ElKvf.str()));
      menu.addItem(TextMenu::Item('E',std::string("ELEVATION Ilim - ")+ElIli.str()));
      menu.addItem(TextMenu::Item('F',std::string("ELEVATION Alim - ")+ElAli.str()));
      menu.addItem(TextMenu::Item('G',std::string("ELEVATION Vlim - ")+ElVli.str()));
      menu.addItem(TextMenu::Item('H',std::string("ELEVATION Off  - ")+ElOff.str()));

      menu.addItem(TextMenu::Item('k',std::string("AZIMUTH   Key  - ")+AzKey.str()));

      menu.addItem(TextMenu::Item('x',"Exit to Main Menu"));

      c=menu.exec();
      if(!std::cin)break;
      
      double double_value;
      char junk=0;
      
      switch(c)
	{
	case '1':
	  std::cout << "Enter value for Az Kp: " << std::flush;
	  if(std::cin >> az_param.Kp)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '2':
	  std::cout << "Enter value for Az Ki: " << std::flush;
	  if(std::cin >> az_param.Ki)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '3':
	  std::cout << "Enter value for Az Kd: " << std::flush;
	  if(std::cin >> az_param.Kd)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '4':
	  std::cout << "Enter value for Az Kvff: " << std::flush;
	  if(std::cin >> az_param.Kvff)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '5':
	  std::cout << "Enter value for Az Ilim: " << std::flush;
	  if(std::cin >> az_param.Ilim)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '6':
	  std::cout << "Enter value for Az Alim [deg/s^2]: " << std::flush;
	  if(std::cin >> az_param.alim)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '7':
	  std::cout << "Enter value for Az Vlim [deg/s]: " << std::flush;
	  if(std::cin >> az_param.vlim)m_api->setAzPIDParameters(az_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case '8':
	  std::cout << "Enter value for Az Offset [deg]: " << std::flush;
	  if(std::cin >> double_value)
	    {
	      az_offset.setDeg(double_value);
	      m_api->setAzOffset(az_offset);
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'A':
	  std::cout << "Enter value for El Kp: " << std::flush;
	  if(std::cin >> el_param.Kp)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'B':
	  std::cout << "Enter value for El Ki: " << std::flush;
	  if(std::cin >> el_param.Ki)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'C':
	  std::cout << "Enter value for El Kd: " << std::flush;
	  if(std::cin >> el_param.Kd)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'D':
	  std::cout << "Enter value for El Kvff: " << std::flush;
	  if(std::cin >> el_param.Kvff)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'E':
	  std::cout << "Enter value for El Ilim: " << std::flush;
	  if(std::cin >> el_param.Ilim)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'F':
	  std::cout << "Enter value for El Alim [deg/s^2]: " << std::flush;
	  if(std::cin >> el_param.alim)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'G':
	  std::cout << "Enter value for El Vlim [deg/s]: " << std::flush;
	  if(std::cin >> el_param.vlim)m_api->setElPIDParameters(el_param);
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'H':
	  std::cout << "Enter value for El Offset [deg]: " << std::flush;
	  if(std::cin >> double_value)
	    {
	      el_offset.setDeg(double_value);
	      m_api->setElOffset(el_offset);
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	  
	case 'k':
	  std::cout << "Enter value for Az Key [deg]: " << std::flush;
	  std::cin >> m_key_az;
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}

      if((c!=0)&&(c!='x'))TextMenu::pressAnyKey();
    }
}
  
void FailsafeUI::displayParameters()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  PIUScopeAPI::PIDParameters az_param;
  PIUScopeAPI::PIDParameters el_param;
  Angle az_off;
  Angle el_off;

  m_api->reqAzPIDParameters(az_param);
  m_api->reqElPIDParameters(el_param);
  m_api->reqAzOffset(az_off);
  m_api->reqElOffset(el_off);
  
  std::cout << std::setfill(' ') << std::dec
	    << "Parameter   Azimuth     Elevation " << std::endl
	    << "----------  ----------  ----------" << std::endl
	    << "Kp          " 
	    << std::left << std::setw(10) << az_param.Kp << "  "
	    << std::left << std::setw(10) << el_param.Kp << std::endl
	    << "Ki          " 
	    << std::left << std::setw(10) << az_param.Ki << "  "
	    << std::left << std::setw(10) << el_param.Ki << std::endl
	    << "Kd          " 
	    << std::left << std::setw(10) << az_param.Kd << "  "
	    << std::left << std::setw(10) << el_param.Kd << std::endl
	    << "KVff        " 
	    << std::left << std::setw(10) << az_param.Kvff << "  "
	    << std::left << std::setw(10) << el_param.Kvff << std::endl
	    << "Int Lim     " 
	    << std::left << std::setw(10) << az_param.Ilim << "  "
	    << std::left << std::setw(10) << el_param.Ilim << std::endl
	    << "Max Acc     " 
	    << std::left << std::setw(10) << az_param.alim << "  "
	    << std::left << std::setw(10) << el_param.alim << std::endl
	    << "Max Vel     " 
	    << std::left << std::setw(10) << az_param.vlim << "  "
	    << std::left << std::setw(10) << el_param.vlim << std::endl
	    << "Offset      " 
	    << std::left << std::setw(10) << az_off.degPMString(3) 
	    << "  "
	    << std::left << std::setw(10) << el_off.degPMString(3) 
	    << std::endl;
}

void FailsafeUI::verbose()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int level=0;
  std::cout << "Enter verbosity level (0/1/2): " << std::flush;
  if(std::cin >> level)m_datastream->setLoud(level);
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
} 

#ifndef MAKE_FAILSAFE_EXECUTABLE
#include<zthread/Thread.h>
#include<SphericalCoords.h>
#include"CorrectionParameters.h"
#include"TargetObject.h"

#include<time.h>
#include<sys/time.h>

#include<qapplication.h>
#include<qstyle.h>
#include<qstylefactory.h>
#include"GUI.h"

void FailsafeUI::start_controller(const char* log)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // GPS coordinates
  SEphem::Angle lat;
  SEphem::Angle lon;
  lat.setFromDMSString("31d40m29.04s");
  lon.setFromDMSString("-110d57m10.08s");
  SEphem::SphericalCoords earthPos = 
    SEphem::SphericalCoords::makeLatLong(lat,lon);
  
  // Correction Parameters
  CorrectionParameters cp;
  //cp.load("corrections.dat");

  // Telescope Motion Limits
  KeyLimits limits(m_key_az+2.0,m_key_az-2.0);

  // PositionLogger
  PositionLogger* logger = 0;
  if((log)&&(strlen(log)!=0))logger = new FilePositionLogger(log);

  // Start the controller
  TelescopeController* controller = 
    new TelescopeControllerLocal(m_api,&limits,earthPos,250,0,cp,logger);
  
  ZThread::Thread thread(controller);
  
  TextMenu menu("Telescope Controller Menu");
  menu.addItem(TextMenu::Item('?',"Mount Status"));
  menu.addItem(TextMenu::Item('h',"Slew to stow position (Home)"));
  menu.addItem(TextMenu::Item('z',"Slew to zenith"));
  menu.addItem(TextMenu::Item('p',"Point to Az/El"));
  menu.addItem(TextMenu::Item('v',"Track in Az/El at constant velocity"));
  menu.addItem(TextMenu::Item('r',"Track RA/Dec"));
  menu.addItem(TextMenu::Item('t',"Track Target"));
  menu.addItem(TextMenu::Item('s',"Stop telescope tracking"));
  menu.addItem(TextMenu::Item('c',"Load tracking corrections"));
  menu.addItem(TextMenu::Item('C',"Clear tracking corrections"));
  menu.addItem(TextMenu::Item('G',"Start GUI"));
  menu.addItem(TextMenu::Item('x',"Exit Controller Thread"));
  int c=0;

  TextMenu::pressAnyKey();

  while(c!='x')
    {
      c=menu.exec();
      if(!std::cin)break;

      try
	{
	  switch(c)
	    {
	    case '?':
	      controller_status(controller);
	      break;
	    case 'h':
	      controller_stow(controller);
	      break;
	    case 'z':
	      controller_zenith(controller);
	      break;
	    case 'p':
	      controller_goto_azel(controller);
	      break;
	    case 'v':
	      controller_track_cv(controller);
	      break;
	    case 'r':
	      controller_track_radec(controller);
	      break;
	    case 's':
	      controller->reqStop();
	      break;
	    case 'G':
	      controller_gui(controller,earthPos);
	      break;
	    case 'x':
	      break;
	    default:
	      c=0;
	      break;
	    }
	}
      catch(const VTracking::Timeout& x)
	{
	  std::cout 
	    << "======== FailsafeUI::start_controller() caught exception ========"
	    << std::endl
	    << "Timeout" << std::endl
	    << "Suppressing \"Timeout\" indications until communication is restored"
	    << std::endl
	    << "================================================================="
	 << std::endl;
	}
      catch(const Exception& x)
	{
	  std::cout 
	    << "======== FailsafeUI::start_controller() caught exception ========"
	    << std::endl;
	  x.print(std::cout);
	  std::cout 
	    << "================================================================="
	    << std::endl;
	}
      catch(...)
	{
	  std::cout 
	    << "======== FailsafeUI::start_controller() caught exception ========"
	    << std::endl
	    << "An unknown exception has been caught" << std::endl
	    << "================================================================="
	    << std::endl;
	}

      if((c!=0)&&(c!='x'))TextMenu::pressAnyKey();
    }

  controller->terminate();
  thread.wait();
}

void FailsafeUI::controller_status(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TelescopeController::StateElements tse;
  tse=controller->tse();
  print_status(tse.status);

  char date_string[11];
  strftime(date_string,sizeof(date_string),"%Y-%m-%d",gmtime(&tse.tv.tv_sec));
  
  std::cout << "UT Date: " << date_string << "  "
	    << "Time: " << tse.timeangle.hmsString(1) << "  "
	    << "LMST: " << tse.lmst.hmsString(1) << std::endl;

  std::cout << "Requested: ";
  switch(tse.req)
    {
    case TelescopeController::REQ_STOP:  std::cout << "STOP    "; break;
    case TelescopeController::REQ_SLEW:  std::cout << "SLEW    "; break;
    case TelescopeController::REQ_TRACK: std::cout << "TRACK   "; break;
    }
  std::cout << "State: ";
  switch(tse.state)
    {
    case TelescopeController::TS_STOP: 
      std::cout << "STOP" << std::endl; break;
    case TelescopeController::TS_SLEW: 
      std::cout << "SLEW" << std::endl; break;
    case TelescopeController::TS_TRACK: 
      std::cout << "TRACK" << std::endl; break;
    case TelescopeController::TS_RESTRICTED_MOTION: 
      std::cout << "RESTRICTED" << std::endl; break;
    case TelescopeController::TS_RAMP_DOWN: 
      std::cout << "RAMP_DOWN" << std::endl; break;
    case TelescopeController::TS_COM_FAILURE: 
      std::cout << "COM_FAILURE" << std::endl; break;
    }
}

void FailsafeUI::controller_stow(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TargetObject* obj(0);
  obj = new AzElObject(SphericalCoords::makeLatLongDeg(0,0),false,true);
  controller->setTargetObject(obj);
  controller->reqSlew();
}

void FailsafeUI::controller_zenith(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TargetObject* obj(0);
  double az_deg=controller->tse().status.az.driveangle_deg;
  obj = new AzElObject(SphericalCoords::makeLatLongDeg(90,az_deg),
		       false,true);
  controller->setTargetObject(obj);
  controller->reqSlew();
}

void FailsafeUI::controller_goto_azel(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::cout << "Enter Az and El seperated by a space: " << std::flush;
  double az=0;
  double el=16;
  if((std::cin >> az)&&(std::cin >> el))
    {
      TargetObject* obj = 
	new AzElObject(SphericalCoords::makeLatLongDeg(el,az));
      controller->setTargetObject(obj);
      controller->reqSlew();
    }
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::controller_track_cv(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::cout << "Enter Az, El speeds in deg/s, seperated by a space: " 
	    << std::flush;
  double az_speed;
  double el_speed;
  if((std::cin >> az_speed)&&(std::cin >> el_speed))
    {
      TelescopeController::StateElements tse;
      tse=controller->tse();
      SphericalCoords ic = 
	SphericalCoords::makeLatLongDeg(tse.status.el.driveangle_deg,
					tse.status.az.driveangle_deg);
      TargetObject* obj = new CVObject(ic,az_speed,el_speed,tse.mjd);
      controller->setTargetObject(obj);
      controller->reqTrack();
    }
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::controller_track_radec(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::cout << "Enter RA, Dec and Epoch seperated by a space: " << std::flush;
  std::string ra_str;
  std::string dec_str;
  double epoch_j=2000;
  if((std::cin >> ra_str)&&(std::cin >> dec_str)&&(std::cin >> epoch_j))
    {
      Angle ra;
      ra.setFromHMSString(ra_str);
      Angle dec;
      dec.setFromDMSString(dec_str);
      double epoch=Astro::julianEpochToMJD(epoch);
      TargetObject* obj = 
	new RaDecObject(SphericalCoords::makeLatLong(dec,ra),epoch);
      controller->setTargetObject(obj);
      controller->reqTrack();
    }
  char junk=0; while((std::cin)&&(junk!='\n'))std::cin.get(junk);
}

void FailsafeUI::controller_track_target(TelescopeController* controller)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

}

void FailsafeUI::controller_gui(TelescopeController* controller,
				const SEphem::SphericalCoords& earthPos)
{
  static bool qapp_init=false;
  static QApplication* a;
  if(qapp_init==false)
    {
      char* argv[]={"hello",0};
      int argc=1;
      a=new QApplication(argc,argv);
      a->connect(a,SIGNAL(lastWindowClosed()),a,SLOT(quit()));
      QStyle* style = QStyleFactory::create("cde");
      if(!style)style = QStyleFactory::create("windows");
      if(style)QApplication::setStyle(style);
      QColor color(207,209,229);
      QApplication::setPalette(QPalette(color,color));
      qapp_init=true;
    }
  
  StowObjectVector stow_pos;
  stow_pos.push_back(StowObject("Stow",
				SphericalCoords::makeLatLongDeg(0,0)));
  GUIWidget* gui = new GUIWidget(m_scope_num,
				 controller,earthPos,stow_pos,0,true);
  
  gui->show();
  a->exec();
}

#endif

