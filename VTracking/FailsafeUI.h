//-*-mode:c++; mode:font-lock;-*-

/**
 * \file FailsafeUI.h
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

#ifndef VTRACKING_FAILSAFEUI_H
#define VTRACKING_FAILSAFEUI_H

#include<string>
#include<vector>
#include<map>

#include"DataStream.h"
#include"ScopeAPI.h"
#include"PIUScopeAPI.h"
#include"EIA422.h"

#ifndef MAKE_FAILSAFE_EXECUTABLE
#include"TelescopeController.h"
#endif

namespace VTracking 
{  
  class FailsafeUI
  {
  public:
    FailsafeUI(int scope_num,
	       DataStream* datastream, ScopeAPI* api, 
	       const SEphem::SphericalCoords& earth_pos,
	       const SEphem::SphericalCoords& stow_pos,
	       double key_az=0):
      m_scope_num(scope_num), m_datastream(datastream), m_api(api),
      m_earth_pos(earth_pos), m_stow_pos(stow_pos), m_key_az(key_az) { }
    
    virtual ~FailsafeUI();
    void run();

  private:
    int                      m_scope_num;
    DataStream*              m_datastream;
    ScopeAPI*                m_api;
    SEphem::SphericalCoords  m_earth_pos;
    SEphem::SphericalCoords  m_stow_pos;
    double                   m_key_az;

    void print_status(const ScopeAPI::PositionerStatus& status);
    void status();
    void slewEl();
    void slewAz();
    void rampDown();
    void point();
    void standby();
    void resetParameters();
    void tuneParameters();
    void displayParameters();
    void verbose();

#ifndef MAKE_FAILSAFE_EXECUTABLE
    void start_controller(const char* log="");
    void controller_status(TelescopeController* controller);
    void controller_stow(TelescopeController* controller);
    void controller_zenith(TelescopeController* controller);
    void controller_goto_azel(TelescopeController* controller);
    void controller_track_cv(TelescopeController* controller);
    void controller_track_radec(TelescopeController* controller);
    void controller_track_target(TelescopeController* controller);
    void controller_gui(TelescopeController* controller,
			const SEphem::SphericalCoords& earthPos);
#endif

  }; // class FailsafeUI

}; // namespace VTracking

#endif // defined VTRACKING_FAILSAFEUI_H

