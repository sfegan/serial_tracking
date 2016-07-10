//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopePhysicsModel.cpp
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
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<cmath>

#include<zthread/Guard.h>

#include<Exception.h>
#include<Debug.h>

#include"RNG.h"
#include"TextMenu.h"
#include"TelescopePhysicsModel.h"

using namespace VTracking;
using namespace VMessaging;

// ============================================================================
// TelescopePhysicsModel BASE CLASS
// ============================================================================

TelescopePhysicsModel::~TelescopePhysicsModel()
{
  // nothing to see here
}

// ============================================================================
// SimpleTelescopePhysicsModel
// ============================================================================

SimpleTelescopePhysicsModel::SimpleTelescopePhysicsModel(double dt):
  TelescopePhysicsModel(dt),
  m_I_xz(100000),    // [kg m^2] = [N m s^2]
  m_I_y(100000),     // [kg m^2] = [N m s^2] 
  m_Tg_00(),         // [N m]
  m_Tg_90(),         // [N m]
  m_az_TFd(100),     // [N m]
  m_az_TFs(500),     // [N m]
  m_az_vFs(0.00001), // [deg s^-1]
  m_el_TFd(100),     // [N m]
  m_el_TFs(500),     // [N m]
  m_el_vFs(0.00001), // [deg s^-1]
  m_az_Nr(25308),    // [1]
  m_el_Nr(25650),    // [1]
  m_az_b(0),         // [N m s]
  m_el_b(0),         // [N m s]
  m_sens(100),       // Sensitivity Gain [1]
  m_L(0.30936),      // Inductance [H]
  m_R(2.56),         // Resistance [Ohm]
  m_Kt(0.47),        // Motor torque constant [N m A^-1]
  m_Ke(0.00833),     // Motor EMF constant [V deg^-1 s]
  m_i_lim(20),       // Current limit
  m_TFbrake(100),    // [N m]
  m_az_lim(false),   // Telescope has azimuthal bumpers
  m_el_lim(true),    // Telescope has elevation bumpers
  m_az_cw(272),      // Position of CW azimuthal bumper [deg] 
  m_az_cc(-272),     // Position of CC azimuthal bumper [deg] 
  m_el_up(92),       // Position of UP elevation bumper [deg] 
  m_el_dn(-2),       // Position of DN elevation bumper [deg] 
  m_bumper_elast(0.8),  // Coefficient of elasticity of bumper
  m_az_x(0),         // Az position [deg]
  m_az_v(0),         // Az velocity [deg s^-1]
  m_az_i(0),         // Az motor current [A]
  m_el_x(0),         // El position [deg]
  m_el_v(0),         // El velocity [deg s^-1]
  m_el_i(0),         // El motor current [A]
  m_T_rms(0)         // RMS noise in azimuth and elevation positions [deg]
{
  // nothing to see here
}

SimpleTelescopePhysicsModel::~SimpleTelescopePhysicsModel()
{
  // nothing to see here
}

void SimpleTelescopePhysicsModel::
iterate(const double az_V, const double el_V, bool az_brake, bool el_brake)
{
  // deirdre says that you should do a poo now.  

  const double sinel = sin(m_el_x*M_PI/180.0);
  const double cosel = cos(m_el_x*M_PI/180.0);
  const double coselsinel = cosel*sinel;
  const double sin2el = sinel*sinel;
  const double cos2el = cosel*cosel;

  const double I_az = cos2el*m_I_xz + sin2el*m_I_y;
  const double I_el = m_I_xz;
  const double I_delta = m_I_y-m_I_xz;

  // =========
  // Elevation
  // =========

  const double Tgrav_el = cosel*m_Tg_00 + sinel*m_Tg_90;
  const double Trotn_el = cosel*sinel*I_delta*m_el_v*m_el_v;
  const double Tmech_el = m_Kt*m_el_i*m_el_Nr;
  const double Trand_el = m_T_rms>0 ? RNG::NRRan2::makeGasDev() * m_T_rms : 0;
  const double TnonF_el = Tgrav_el+Trotn_el+Tmech_el+Trand_el;

  const double Tdamp_el = -m_el_b*m_el_v*m_el_Nr*m_el_Nr;
  double Tfric_el = 0;
  if(el_brake)Tfric_el = m_TFbrake*m_el_Nr;
  if(fabs(m_el_v) > m_el_vFs)Tfric_el += m_el_TFd;
  else Tfric_el += m_el_TFs;
  Tfric_el += Tdamp_el;

  if(m_el_v>m_el_vFs) Tfric_el = -Tfric_el;
  else if((m_el_v>=-m_el_vFs)&&(TnonF_el > 0)) Tfric_el = -Tfric_el;

  const double T_el = TnonF_el + Tfric_el;

  const double el_dvdt = T_el/I_el;
  const double el_dxdt = m_el_v;
  const double el_didt = (-m_R*m_el_i + el_V*m_sens - m_Ke*m_el_Nr*m_el_v)/m_L;

  double el_v = m_el_v + el_dvdt * m_dt;
  double el_x = m_el_x + el_dxdt * m_dt;
  double el_i = m_el_i + el_didt * m_dt;
  if(m_L==0)el_i = (el_V*m_sens - m_Ke*m_el_Nr*m_el_v)/m_R;

  // Motion should not be reversed due to friction
  if((m_el_v>=0)&&(el_v<0)&&(TnonF_el>=0))el_v = 0;
  else if((m_el_v<=0)&&(el_v>0)&&(TnonF_el<=0))el_v = 0;

  // Check motion into the bumpers
  if(m_el_lim)
    if(el_x>m_el_up)
      { el_x=m_el_up; if(el_v>0)el_v = -el_v*m_bumper_elast; }
    else if(el_x<m_el_dn)
      { el_x=m_el_dn; if(el_v<0)el_v = -el_v*m_bumper_elast; }

  // Check current limit
  if(el_i > m_i_lim)el_i = m_i_lim;
  else if(el_i < -m_i_lim)el_i = -m_i_lim;

  // =======
  // Azimuth
  // =======

  const double Trotn_az = -2*coselsinel*I_delta*m_el_v*m_az_v;
  const double Tmech_az = m_Kt*m_az_i*m_az_Nr;
  const double Trand_az = m_T_rms>0 ? RNG::NRRan2::makeGasDev() * m_T_rms : 0;
  const double TnonF_az = Trotn_az+Tmech_az+Trand_az;

  const double Tdamp_az = -m_az_b*m_az_v*m_az_Nr*m_az_Nr;
  double Tfric_az = 0;
  if(az_brake)Tfric_az = m_TFbrake*m_az_Nr;
  if(fabs(m_az_v) > m_az_vFs)Tfric_az += m_az_TFd;
  else Tfric_az += m_az_TFs;
  Tfric_az += Tdamp_az;

  if(m_az_v>m_az_vFs) Tfric_az = -Tfric_az;
  else if((m_az_v>=-m_az_vFs)&&(TnonF_az > 0)) Tfric_az = -Tfric_az;

#if 0
  Debug::stream() 
    << "Az_V:     " << az_V << std::endl
    << "Trotn_az: " << Trotn_az << std::endl
    << "Tmech_az: " << Tmech_az << std::endl
    << "Tdamp_az: " << Tdamp_az << std::endl
    << "TnonF_az: " << TnonF_az << std::endl
    << "Tfric_az: " << Tfric_az << std::endl
    << "El_V:     " << el_V << std::endl
    << "Tgrav_el: " << Tgrav_el << std::endl
    << "Trotn_el: " << Trotn_el << std::endl
    << "Tmech_el: " << Tmech_el << std::endl
    << "Tdamp_el: " << Tdamp_el << std::endl
    << "TnonF_el: " << TnonF_el << std::endl
    << "Tfric_el: " << Tfric_el << std::endl;
#endif

  const double T_az = TnonF_az + Tfric_az;

  const double az_dvdt = T_az/I_az;
  const double az_dxdt = m_az_v;
  const double az_didt = (-m_R*m_az_i + az_V*m_sens - m_Ke*m_az_Nr*m_az_v)/m_L;

  double az_v = m_az_v + az_dvdt * m_dt;
  double az_x = m_az_x + az_dxdt * m_dt;
  double az_i = m_az_i + az_didt * m_dt;
  if(m_L==0)az_i = (az_V*m_sens - m_Ke*m_az_Nr*m_az_v)/m_R;

  // Motion should not be reversed due to friction
  if((m_az_v>=0)&&(az_v<0)&&(TnonF_az>=0))az_v = 0;
  else if((m_az_v<=0)&&(az_v>0)&&(TnonF_az<=0))az_v = 0;

  // Check motion into the bumpers
  if(m_az_lim)
    if(az_x>m_az_cw)
      { az_x=m_az_cw; if(az_v>0)az_v = -az_v*m_bumper_elast; }
    else if(az_x<m_az_cc)
      { az_x=m_az_cc; if(az_v<0)az_v = -az_v*m_bumper_elast; }
  
  // Check current limit
  if(az_i > m_i_lim)az_i = m_i_lim;
  else if(az_i < -m_i_lim)az_i = -m_i_lim;

  // Transfer new values for next iteration
  m_el_v = el_v;
  m_el_x = el_x;
  m_el_i = el_i;

  m_az_v = az_v;
  m_az_x = az_x;
  m_az_i = az_i;
}

void SimpleTelescopePhysicsModel::
getState(double& az_x, double& el_x, double& az_v, double& el_v,
	 double& az_i, double& el_i)
{
  az_x = m_az_x;
  el_x = m_el_x;
  az_v = m_az_v;
  el_v = m_el_v;
  az_i = m_az_i;
  el_i = m_el_i;
}

void SimpleTelescopePhysicsModel::readState(std::istream& stream)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::string what_you_talking_about_willis;

  stream >> what_you_talking_about_willis;
  assert(what_you_talking_about_willis == "PHYSICS_MODEL");
  stream >> what_you_talking_about_willis;
  assert(what_you_talking_about_willis == "SIMPLE_V1.0");

  stream
    >> what_you_talking_about_willis >> m_I_xz
    >> what_you_talking_about_willis >> m_I_y
    >> what_you_talking_about_willis >> m_Tg_00
    >> what_you_talking_about_willis >> m_Tg_90
    
    >> what_you_talking_about_willis >> m_az_TFd
    >> what_you_talking_about_willis >> m_az_TFs
    >> what_you_talking_about_willis >> m_az_vFs
    >> what_you_talking_about_willis >> m_el_TFd
    >> what_you_talking_about_willis >> m_el_TFs
    >> what_you_talking_about_willis >> m_el_vFs

    >> what_you_talking_about_willis >> m_az_Nr
    >> what_you_talking_about_willis >> m_el_Nr

    >> what_you_talking_about_willis >> m_az_b
    >> what_you_talking_about_willis >> m_el_b
    
    >> what_you_talking_about_willis >> m_sens
    >> what_you_talking_about_willis >> m_L
    >> what_you_talking_about_willis >> m_R
    >> what_you_talking_about_willis >> m_Kt
    >> what_you_talking_about_willis >> m_Ke
    >> what_you_talking_about_willis >> m_i_lim
    >> what_you_talking_about_willis >> m_TFbrake

    >> what_you_talking_about_willis >> m_az_lim
    >> what_you_talking_about_willis >> m_el_lim
    >> what_you_talking_about_willis >> m_az_cw
    >> what_you_talking_about_willis >> m_az_cc
    >> what_you_talking_about_willis >> m_el_up
    >> what_you_talking_about_willis >> m_el_dn
    >> what_you_talking_about_willis >> m_bumper_elast

    >> what_you_talking_about_willis >> m_az_x
    >> what_you_talking_about_willis >> m_az_v
    >> what_you_talking_about_willis >> m_az_i
    >> what_you_talking_about_willis >> m_el_x
    >> what_you_talking_about_willis >> m_el_v
    >> what_you_talking_about_willis >> m_el_i

    >> what_you_talking_about_willis >> m_T_rms;
}

void SimpleTelescopePhysicsModel::writeState(std::ostream& stream) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  stream 
    << "PHYSICS_MODEL       SIMPLE_V1.0" << std::endl

    << "INERTIA_XZ          " << m_I_xz << std::endl
    << "INERTAL_Y           " << m_I_y << std::endl
    << "GRAVTORQUE_00       " << m_Tg_00 << std::endl
    << "GRAVTORQUE_90       " << m_Tg_90 << std::endl
    
    << "FRICDYNAMIC_AZ      " << m_az_TFd << std::endl
    << "FRICSTATIC_AZ       " << m_az_TFs << std::endl
    << "FRICSTATICSPEED_AZ  " << m_az_vFs << std::endl
    << "FRICDYNAMIC_EL      " << m_el_TFd << std::endl
    << "FRICSTATIC_EL       " << m_el_TFs << std::endl
    << "FRICSTATICSPEED_EL  " << m_el_vFs << std::endl

    << "GEARRATIO_AZ        " << m_az_Nr << std::endl
    << "GEARRATIO_EL        " << m_el_Nr << std::endl

    << "DAMPING_RATIO_AZ    " << m_az_b << std::endl
    << "DAMPING_RATIO_EL    " << m_el_b << std::endl
    
    << "MOTOR_GAIN_SENS     " << m_sens << std::endl
    << "MOTOR_INDUCTANCE    " << m_L << std::endl
    << "MOTOR_RESISTANCE    " << m_R << std::endl
    << "MOTOR_TORQUE_CONST  " << m_Kt << std::endl
    << "MOTOR_EMF_CONST     " << m_Ke << std::endl
    << "MOTOR_CURRENT_LIMIT " << m_i_lim << std::endl
    << "BRAKE_TORQUE        " << m_TFbrake << std::endl

    << "BUMPER_PRESENT_AZ   " << m_az_lim << std::endl
    << "BUMPER_PRESENT_EL   " << m_el_lim << std::endl
    << "BUMPER_POSITION_CW  " << m_az_cw << std::endl
    << "BUMPER_POSITION_CC  " << m_az_cc << std::endl
    << "BUMPER_POSITION_UP  " << m_el_up << std::endl
    << "BUMPER_POSITION_DN  " << m_el_dn << std::endl
    << "BUMPER_ELAST        " << m_bumper_elast << std::endl

    << "POSITION_AZ         " << m_az_x << std::endl
    << "VELOCITY_AZ         " << m_az_v << std::endl
    << "CURRENT_AZ          " << m_az_i << std::endl
    << "POSITION_EL         " << m_el_x << std::endl
    << "VELOCITY_EL         " << m_el_v << std::endl
    << "CURRENT_EL          " << m_el_i << std::endl

    << "NOISE_TORQUE_RMS    " << m_T_rms << std::endl;
}

bool SimpleTelescopePhysicsModel::hasPhysicsMenu() const
{
  return true;
}

void SimpleTelescopePhysicsModel::doPhysicsMenu(ZThread::RecursiveMutex* mutex)
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TextMenu menu("Simple Telescope Physics Model Menu");
  menu.addItem(I('?',"Display Model Status"));
  menu.addItem(I('s',"Change Model Status Variables"));
  menu.addItem(I('t',"Change Telescope Mechanical Parameters"));
  menu.addItem(I('d',"Change Drive Parameters"));
  menu.addItem(I('l',"Change Model Mechanical Limits"));
  menu.addItem(I('x',"Exit"));
  int c=0;

  while(c!='x')
    {
      c=menu.exec();
      if(!std::cin)break;

      switch(c)
	{
	case '?':
	  status(mutex);
	  break;
	case 's':
	  menuModelStatus(mutex);
	  break;
	case 't':
	  menuTelescopeParameters(mutex);
	  break;
	case 'd':
	  menuDriveParameters(mutex);
	  break;
	case 'l':
	  menuMechanicalLimits(mutex);
	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

void SimpleTelescopePhysicsModel::
status(ZThread::RecursiveMutex* mutex)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::cout
    << "POSITION_AZ  " << m_az_x << std::endl
    << "VELOCITY_AZ  " << m_az_v << std::endl
    << "CURRENT_AZ   " << m_az_i << std::endl
    << "POSITION_EL  " << m_el_x << std::endl
    << "VELOCITY_EL  " << m_el_v << std::endl
    << "CURRENT_EL   " << m_el_i << std::endl;
  TextMenu::pressAnyKey();
}

void SimpleTelescopePhysicsModel::
menuModelStatus(ZThread::RecursiveMutex* mutex)
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int c=0;

  while(c!='x')
    {
      TextMenu menu("Change Model Status Variables");

      menu.addItem(I('v',"Az Velocity - ", m_az_v));
      menu.addItem(I('p',"Az Position - ", m_az_x));
      menu.addItem(I('i',"Az Current  - ", m_az_i));

      menu.addItem(I('V',"El Velocity - ", m_el_v));
      menu.addItem(I('P',"El Position - ", m_el_x));
      menu.addItem(I('I',"El Current  - ", m_el_i));

      menu.addItem(I('z',"Zero Velocity and Currents"));

      menu.addItem(I('x',"Exit to Physics Menu"));

      c=menu.exec();
      if(!std::cin)break;
      
      char junk=0;
      double dval;

      switch(c)
	{
	case 'v':
	  std::cout << "Enter value for Az Velocity: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_az_v = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'p':
	  std::cout << "Enter value for Az Position: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_az_x = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'i':
	  std::cout << "Enter value for Az Current: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_az_i = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'V':
	  std::cout << "Enter value for El Velocity: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_el_v = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'P':
	  std::cout << "Enter value for El Position: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_el_x = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'I':
	  std::cout << "Enter value for El Current: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_el_i = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'z':
	  if(mutex)mutex->acquire();
	  m_az_v = 0;
	  m_az_i = 0;
	  m_el_v = 0;
	  m_el_i = 0;
	  if(mutex)mutex->release();
	  break;

	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

void SimpleTelescopePhysicsModel::
menuTelescopeParameters(ZThread::RecursiveMutex* mutex)
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int c=0;

  while(c!='x')
    {
      TextMenu menu("Change Telescope Parameters");

      menu.addItem(I('i', "Moment of Interia X/Z  - ", m_I_xz));
      menu.addItem(I('I', "Moment of Interia Y    - ", m_I_y));
      menu.addItem(I('g', "Grav Torque at El=0    - ", m_Tg_00));
      menu.addItem(I('G', "Grav Torque at El=90   - ", m_Tg_90));
		   
      menu.addItem(I('d', "Az Dynamic Friction    - ", m_az_TFd));
      menu.addItem(I('s', "Az Static Friction     - ", m_az_TFs));
      menu.addItem(I('v', "Az Max Static Speed    - ", m_az_vFs));

      menu.addItem(I('D', "El Dynamic Friction    - ", m_el_TFd));
      menu.addItem(I('S', "El Static Friction     - ", m_el_TFs));
      menu.addItem(I('V', "El Max Ststic Speed    - ", m_el_vFs));
      
      menu.addItem(I('N', "RMS Noise Torque Level - ", m_T_rms));
      
      menu.addItem(I('x', "Exit to Physics Menu"));

      c=menu.exec();
      if(!std::cin)break;
      
      char junk=0;
      double dval=0;

      switch(c)
	{
	case 'i':
	  std::cout << "Enter value for Moment of Interia X/Z [N m s^2]: " 
		    << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_I_xz=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'I':
	  std::cout << "Enter value for Moment of Interia Y [N m s^2]: " 
		    << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_I_y=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'g':
	  std::cout << "Enter value for Gravitational Torque at El=0 [n M]: "
		    << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_Tg_00 = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'G':
	  std::cout << "Enter value for Gravitational Torque at El=90 [n M]: "
		    << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_Tg_90 = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'd':
	  std::cout << "Enter value for Az Dynamic Friction [n M]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_az_TFd=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 's':
	  std::cout << "Enter value for Az Static Friction [n M]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_az_TFs=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'v':
	  std::cout << "Enter value for "
		    << "Az Maximum Speed Of Static Friction [deg s^-1]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_az_vFs=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'D':
	  std::cout << "Enter value for El Dynamic Friction [n M]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_el_TFd=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'S':
	  std::cout << "Enter value for El Static Friction [n M]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_el_TFs=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'V':
	  std::cout << "Enter value for "
		    << "El Maximum Speed Of Static Friction [deg s^-1]: "
		    << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_el_vFs=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'N':
	  std::cout << "RMS Noise Torque Level [n M]: " << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_T_rms=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

void SimpleTelescopePhysicsModel::
menuDriveParameters(ZThread::RecursiveMutex* mutex)
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int c=0;

  while(c!='x')
    {
      TextMenu menu("Change Drive Parameters");

      menu.addItem(I('g', "Az Gear Ratio          - ", m_az_Nr));
      menu.addItem(I('G', "El Gear Ratio          - ", m_el_Nr));
      menu.addItem(I('b', "Az Damping Ratio       - ", m_az_b));
      menu.addItem(I('B', "El Damping Ratio       - ", m_el_b));

      menu.addItem(I('s', "Motor Sensitivity      - ", m_sens));
      menu.addItem(I('l', "Motor Inductance       - ", m_L));
      menu.addItem(I('r', "Motor Resistance       - ", m_R));
      menu.addItem(I('k', "Motor Torque Constant  - ", m_Kt));
      menu.addItem(I('K', "Motor EMF Constant     - ", m_Ke));
      menu.addItem(I('a', "Motor Current Limit    - ", m_i_lim));

      menu.addItem(I('f', "Brake Friction         - ", m_TFbrake));

      menu.addItem(I('x', "Exit to Physics Menu"));

      c=menu.exec();
      if(!std::cin)break;
      
      char junk=0;
      double dval=0;

      switch(c)
	{
	case 'g':
	  std::cout << "Az Gear Ratio: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_az_Nr=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'G':
	  std::cout << "El Gear Ratio: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_el_Nr=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'b':
	  std::cout << "Az Damping Ratio [N m deg^-1 s]: " << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_az_b=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'B':
	  std::cout << "El Damping Ratio [N m deg^-1 s]: " << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_el_b=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 's':
	  std::cout << "Motor Sensitivity (Gain): " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_sens=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'l':
	  std::cout << "Motor Inductance [H]: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_L=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'r':
	  std::cout << "Motor Resistance [Ohm]: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_R=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'k':
	  std::cout << "Motor Torque Constant [N m A^-1]: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_Kt=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'K':
	  std::cout << "Motor EMF Constant [V deg^-1 s]: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_Ke=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'a':
	  std::cout << "Motor Current Limit [A]: " << std::flush;
	  if((std::cin >> dval)&&(dval>0))
	    {
	      if(mutex)mutex->acquire();
	      m_i_lim=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'f':
	  std::cout << "Brake Friction [n M]: " << std::flush;
	  if((std::cin >> dval)&&(dval>=0))
	    {
	      if(mutex)mutex->acquire();
	      m_TFbrake=dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

void SimpleTelescopePhysicsModel::
menuMechanicalLimits(ZThread::RecursiveMutex* mutex)
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int c=0;

  while(c!='x')
    {
      std::string az_lim; if(m_az_lim)az_lim="Enabled"; else az_lim="Disabled";
      std::string el_lim; if(m_el_lim)el_lim="Enabled"; else el_lim="Disabled";

      TextMenu menu("Change Mechanical Limits");

      menu.addItem(I('l', "Toggle Az Limits  - ", az_lim));
      menu.addItem(I('p', "Az CW Limit       - ", m_az_cw));
      menu.addItem(I('n', "Az CCW Limit      - ", m_az_cc));

      menu.addItem(I('L', "Toggle El Limits  - ", el_lim));
      menu.addItem(I('P', "El UP Limit       - ", m_el_up));
      menu.addItem(I('N', "El DOWN Limit     - ", m_el_dn));

      menu.addItem(I('e', "Bumper Elasticity - ", m_bumper_elast));
      
      menu.addItem(I('x',"Exit to Physics Menu"));

      c=menu.exec();
      if(!std::cin)break;
      
      char junk=0;
      double dval = 0;
      
      switch(c)
	{
	case 'l':
	  if(mutex)mutex->acquire();
	  m_az_lim = !m_az_lim;
	  if(mutex)mutex->release();
	  break;
	case 'p':
	  std::cout << "Enter value for Az CW Limit: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_az_cw = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'n':
	  std::cout << "Enter value for Az CCW Limit: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_az_cc = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'L':
	  if(mutex)mutex->acquire();
	  m_el_lim = !m_el_lim;
	  if(mutex)mutex->release();
	  break;
	case 'P':
	  std::cout << "Enter value for El UP Limit: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_el_up = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;
	case 'N':
	  std::cout << "Enter value for El DOWN Limit: " << std::flush;
	  if(std::cin >> dval)
	    {
	      if(mutex)mutex->acquire();
	      m_el_dn = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'e':
	  std::cout << "Enter value for bumper elasticity: " << std::flush;
	  if((std::cin >> dval)&&(dval>=0)&&(dval<=1))
	    {
	      if(mutex)mutex->acquire();
	      m_bumper_elast = dval;
	      if(mutex)mutex->release();
	    }
	  while((std::cin)&&(junk!='\n'))std::cin.get(junk);
	  break;

	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}
