//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopePhysicsModel.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TELESCOPEPHYSICSMODEL_H
#define VTRACKING_TELESCOPEPHYSICSMODEL_H

#include<iostream>

#include<zthread/RecursiveMutex.h>

namespace VTracking
{

  class TelescopePhysicsModel
  {
  public:
    TelescopePhysicsModel(double dt): m_dt(dt) { /* nothing to see here */ }
    virtual ~TelescopePhysicsModel();
    virtual bool hasPhysicsMenu() const = 0;
    virtual void doPhysicsMenu(ZThread::RecursiveMutex* mutex=0) = 0;
    virtual void iterate(const double az_V, const double el_V,
			 bool az_brake, bool el_brake) = 0;
    virtual void getState(double& az_x, double& el_x,
			  double& az_v, double& el_v,
			  double& az_i, double& el_i) = 0;
    virtual void readState(std::istream& stream) = 0;
    virtual void writeState(std::ostream& stream) const = 0;
  protected:
    double m_dt;
  };

  class SimpleTelescopePhysicsModel: public TelescopePhysicsModel
  {
  public:
    SimpleTelescopePhysicsModel(double dt);
    virtual ~SimpleTelescopePhysicsModel();
    virtual bool hasPhysicsMenu() const;
    virtual void doPhysicsMenu(ZThread::RecursiveMutex* mutex=0);
    virtual void iterate(const double az_V, const double el_V,
			 bool az_brake, bool el_brake);
    virtual void getState(double& az_x, double& el_x,
			  double& az_v, double& el_v,
			  double& az_i, double& el_i);
    virtual void readState(std::istream& stream);
    virtual void writeState(std::ostream& stream) const;

  private:
    void status(ZThread::RecursiveMutex* mutex);
    void menuModelStatus(ZThread::RecursiveMutex* mutex);
    void menuTelescopeParameters(ZThread::RecursiveMutex* mutex);
    void menuDriveParameters(ZThread::RecursiveMutex* mutex);
    void menuMechanicalLimits(ZThread::RecursiveMutex* mutex);

    // Telescope moments of inertia and gravitational torques
    // expressed on telescope side of gear box
    double m_I_xz;    // [kg m^2] = [N m s^2]
    double m_I_y;     // [kg m^2] = [N m s^2]
    double m_Tg_00;   // [N m]
    double m_Tg_90;   // [N m]

    // Frictional torques, dynamic and static and speed below which static
    // friction "kicks in" expressed on reflector side of gear box
    double m_az_TFd;  // [N m]
    double m_az_TFs;  // [N m]
    double m_az_vFs;  // [deg s^-1]
    double m_el_TFd;  // [N m]
    double m_el_TFs;  // [N m]
    double m_el_vFs;  // [deg s^-1]

    // Gear ratios (input speed/output speed)
    double m_az_Nr;
    double m_el_Nr;

    // Damping ratio of mechanical system expressed on motor side of
    // the gear box
    double m_az_b;    // [N m s]
    double m_el_b;    // [N m s]
    
    // Commom "motor" parameters
    double m_sens;    // Input Sensivity Gain [1]
    double m_L;       // Inductance [H]
    double m_R;       // Resistance [Ohm]
    double m_Kt;      // Motor armature constant [N m A^-1]
    double m_Ke;      // Motor EMF constant [V deg^-1 s]
    double m_i_lim;   // Motor current limit

    // Frictional torque of brakes (only one for simplicity) expressed on
    double m_TFbrake; // [N m]

    // Limits etc
    bool m_az_lim;    // Telescope has azimuthal bumpers
    bool m_el_lim;    // Telescope has elevation bumpers
    double m_az_cw;   // Position of CW azimuthal bumper [deg] 
    double m_az_cc;   // Position of CC azimuthal bumper [deg] 
    double m_el_up;   // Position of UP elevation bumper [deg] 
    double m_el_dn;   // Position of DN elevation bumper [deg] 
    double m_bumper_elast;  // Coefficient of elasticity of AZ bumper

    // State parameters on each axis expressed on reflector side of
    // gear box
    double m_az_x;    // Az position [deg]
    double m_az_v;    // Az velocity [deg s^-1]
    double m_az_i;    // Az motor current [A]
    double m_el_x;    // El position [deg]
    double m_el_v;    // El velocity [deg s^-1]
    double m_el_i;    // El motor current [A]

    // Noise
    double m_T_rms;   // RMS of noise in azimuth and elevation positions
  };

}

#endif // VTRACKING_TELESCOPEPHYSICSMODEL_H

