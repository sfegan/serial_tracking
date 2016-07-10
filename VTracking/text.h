//-*-mode:c++; mode:font-lock;-*-

/**
 * \file text.h
 *
 * Text for help options and tool tips (for ease of spell checking)
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.25 $
 * $Tag$
 *
 **/

// ****************************************************************************
// HELP TEXT FOR MAIN
// ****************************************************************************

#define OPT_MAIN_VERBOSE_1			\
  "Set verbosity level 1."

#define OPT_MAIN_VERBOSE_2			\
  "Set verbosity level 2."

#define OPT_MAIN_CONTROLLER				\
  "Operate as single telescope controller [default]."

#define OPT_MAIN_ARRAY				\
  "Operate as an array GUI."

#define OPT_MAIN_EMULATOR			\
  "Operate as single telescope emulator."

#define OPT_MAIN_FAILSAFE				\
  "Operate as single telescope fail-safe controller."

#define OPT_MAIN_TERMINATE				\
  "Send terminate command to the remote CORBA server."

#define OPT_MAIN_SCOPE							\
  "Set telescope number, counting from zero. The default protocol "	\
  "and communication stream will be selected appropriately. To "	\
  "select the Whipple telescope, set the telescope number to 10."

#define OPT_MAIN_PROTO							\
  "Set protocol to use to talk to the telescope. Valid choices "	\
  "are: (1) \"PIU\": use the standard Ethernet protocol, (2) "		\
  "\"PIU_PROTO\": use the Ethernet protocol appropriate for the "	\
  "VERITAS prototype system, (3) \"EIA\": use the older serial "	\
  "EIA-422 protocol or (4) \"10M\": use the Whipple 10m "		\
  "pseudo-protocol. This option can be used to override the "		\
  "default selected by \"-scope\"."

#define OPT_MAIN_DATASTREAM						\
  "Set the data-stream to communicate to the telescope. Examples "	\
  "are \"udp:192.168.1.100/5000\" (UDP packets to IP address "		\
  "192.188.1.100 port 5000), \"serial:/dev/ttyS0\" (use serial "	\
  "port 0), \"unix:/tmp/socket0\" (UNIX socket)."

#define OPT_MAIN_QT_ARGS				\
  "Comma separated list of options to send to Qt."

#define OPT_MAIN_NO_QT				\
  "Do not start Qt interface."

#define OPT_MAIN_CORBA_ARGS				\
  "Comma separated list of options to send to omniORB."

#define OPT_MAIN_CORBA_PORT						\
  "Port number to use when creating endpoint for CORBA servants."

#define OPT_MAIN_CORBA_NAMESERVER					\
  "IOR of CORBA name-server, probably should be in the form "		\
  "\"corbaname::hostname\" or \"corbaloc::hostname:2809/NameService\"."

#define OPT_MAIN_NO_CORBA			\
  "Do not start CORBA servants."

#define OPT_MAIN_CORBA_REMOTE_IOR					\
  "Specify the IOR of the remote controller. If an empty value is "	\
  "given, the CORBA name-server is queried for the IOR."

// ****************************************************************************
// HELP TEXT FOR MAINCONTROLLER
// ****************************************************************************

#define OPT_MAIN_CONTROLLER_DN_LIMIT				\
  "Set default downward limit on motion outside of the key."

#define OPT_MAIN_CONTROLLER_DAYTIME					\
  "Set limits on motion which are appropriate for daytime use. "	\
  "El<45, -45<Az<45. This option is not meant as a replacement for "	\
  "your common sense. You can do serious damage moving the telescope "	\
  "during the daytime, always pay close attention. NEVER LEAVE THE "	\
  "TELESCOPE UNATTENDED."

#define OPT_MAIN_CONTROLLER_OVERRIDE_KEY				\
  "Override key thereby allowing for motion below the downward "	\
  "limit at all azimuths."

#define OPT_MAIN_CONTROLLER_OVERRIDE_SECURITY				\
  "Override security enabling access to calibration targets and "	\
  "correction parameters."

#define OPT_MAIN_CONTROLLER_RECORD					\
  "Write position information to a data file from the control "		\
  "loop. If the filename is empty then no information is written "	\
  "[default]."

#define OPT_MAIN_CONTROLLER_LOGGER_TIMEOUT				\
  "Set the timeout, in seconds, after which the logger will stop "	\
  "recording position information in the database if no commands "	\
  "have been received and the telescope is motionless. If set to "	\
  "zero, timeout is disabled. If the \"record\" option is given "	\
  "the default timeout is set to zero, but can be changed with "	\
  "this option."

#define OPT_MAIN_CONTROLLER_NO_DB					\
  "Do not write position and control information to the database."

#define OPT_MAIN_CONTROLLER_DB_LOG_PERIOD				\
  "Set DB logging period in multiples of the controller iteration "	\
  "period. A value of zero disables DB logging."

#define OPT_MAIN_CONTROLLER_DB_CACHE_SIZE				\
  "Set the number of DB records to cache and write simultaneously."

#define OPT_MAIN_CONTROLLER_CONTROLLER					\
  "Specify the controller to use. Valid options are (1) \"local\": "	\
  "this process is the primary telescope controller, communicate "	\
  "directly with the telescope and (2) \"remote\": this process is "	\
  "a slave and should communicate with the primary controller using "	\
  "CORBA."

#define OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_1				\
  "Set the iteration period of the controller in ms. A value of 0 "	\
  "selects the default period, "
#define OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_2 \
  "ms for a local controller, "
#define OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_3 \
  "ms for a remote (CORBA) controller."

#define OPT_MAIN_CONTROLLER_CORBA_READONLY				\
  "When operating in as a REMOTE (CORBA CLIENT) controller, do not "	\
  "send any commands to the telescope controller, except for "		\
  "PANIC! requests to stop. This option allows the GUI be used to "	\
  "monitor tracking without interference. When operating as a "		\
  "CORBA SERVER, do not accept commands the client, excepting PANIC! "	\
  "requests to stop. Requests for status information are accepted."

#define OPT_MAIN_CONTROLLER_CORBA_SIMPLE_INTERFACE_READONLY		\
  "When operating as a CORBA SERVER, do not accept commands from "	\
  "the client over the simple interface. Requests for status "		\
  "information are accepted."

#define OPT_MAIN_CONTROLLER_CORBA_NO_AUTO_STOPPING			\
  "When operating in as a REMOTE (CORBA) controller, do not "		\
  "send stop commands when the program starts, when a connection "	\
  "to the telescope is lost and reestablished or when the program "	\
  "exits. This option allows multiple copies of the GUI to "		\
  "co-exists since starting and stopping the program will not "		\
  "command the telescopes to stop. Just make sure that you remember "	\
  "to stop the yourself when you no longer need them!"

#define OPT_MAIN_NO_GRB				\
  "Do not run the GRB monitor."

#define OPT_MAIN_CONTROLLER_DAEMON		\
  "Run in background as a daemon."

#define OPT_MAIN_CONTROLLER_LOG_FILE					\
  "Set name of log file. If an empty value is given then a "		\
  "telescope/date specific log file will be opened when in daemon "	\
  "mode."

#define OPT_MAIN_ARRAY_THEME						\
  "Set the array GUI theme. Valid options are \"default\", "		\
  "\"a-team\", \"hobbit\", \"fellowship\", \"yellow_submarine\", "	\
  "\"musketeers\" and \"johns\"."

#define OPT_MAIN_ARRAY_SUPPRESS_SERVO_FAILURE				\
  "Suppress the servo failure error for any telescope(s) on the "	\
  "main array page. Specify as a comma separated list of telescope "	\
  "numbers (counting from zero)."

// ****************************************************************************
// TOOLTIPS FOR STATUS BAR
// ****************************************************************************

#define TT_SB_AZ				\
  "Azimuth drive angle"

#define TT_SB_EL				\
  "Elevation drive angle"

#define TT_SB_REQ					\
  "Requested tracking state, either\n"			\
  "STOP, SLEW or TRACK"

#define TT_SB_STAT							\
  "State of telescope controller, one of the following:\n"		\
  "STOP: Stopped with brakes energized.\n"				\
  "SLEW: Slewing to target position.\n"					\
  "TRACK: Tracking target to within 0.01 degrees.\n"			\
  "RESTRICTED: Cannot slew directly to target from current\n"		\
  "   position. Motion is modified to avoid restricted regions\n"	\
  "   outside of limits.\n"						\
  "RAMP_DOWN: Slowing down, prior to applying brakes.\n"		\
  "NO SERVER: Communication failure between local program and\n"	\
  "   the server. Indicates CORBA or network error.\n"			\
  "SCOPE COM FAIL: Remote controller indicates a failure\n"		\
  "   communicating to the telescope. Communication with the CORBA\n"	\
  "   control server is OK."

// ****************************************************************************
// TOOLTIPS FOR SUMMARY AND DETAILS PAGE
// ****************************************************************************

#define TT_SUMDET_DATE				\
  "Date in UTC"

#define TT_SUMDET_UTC				\
  "Coordinated Universal Time"

#define TT_SUMDET_LMST				\
  "Local Mean Sidereal Time"

#define TT_SUMDET_MJD				\
  "Modified Julian Date"

#define TT_SUMDET_TARGET			\
  "Name and details of selected target"

#define TT_SUMDET_TEL_AZ						\
  "Telescope azimuth. Prefix of \"Enc:\" indicates that position is\n"	\
  "given in encoder coordinates. Otherwise, it is given in sky\n"	\
  "(corrected) coordinates."

#define TT_SUMDET_TEL_RA				\
  "Apparent telescope right ascension in current epoch"

#define TT_SUMDET_TEL_GLON			\
  "Approximate telescope galactic longitude"

#define TT_SUMDET_TEL_EL						\
  "Telescope elevation. Prefix of \"Enc:\" indicates that position\n"	\
  "is given in encoder coordinates. Otherwise, it is given in sky\n"	\
  "(corrected) coordinates."

#define TT_SUMDET_TEL_DEC				\
  "Apparent telescope declination in current epoch"

#define TT_SUMDET_TEL_GLAT			\
  "Approximate telescope galactic latitude"

#define TT_SUMDET_SRC_AZ \
  "Target Azimuth. Background color indicates various conditions...\n"	\
  "SOLID RED: Target is outside of allowed telescope limits.\n"		\
  "FLASHING RED: Target is moving too fast in azimuth to be tracked.\n"	\
  "FLASHING YELLOW: Over the next 30 minutes, target will be:\n"	\
  "   (1) outside of limits, /OR/\n"					\
  "   (2) will move too fast to be tracked /OR/\n"			\
  "   (3) will require a 360 degree CW->CCW rotation."

#define TT_SUMDET_SRC_AZ_DIR \
  TT_SUMDET_SRC_AZ "\n"							\
  "In addition, the preferred rotation direction, if one is defined,\n" \
  "is indicated as a flashing suffix to the azimuth angle:\n"		\
  "(CW):   Preferred direction is clockwise.\n"				\
  "(CCW):  Preferred direction is counter-clockwise\n"			\
  "(FREE): Individual telescopes in array are free to slew to\n"	\
  "        target by the shortest route.\n"				\
  "No suffix indicates that the telescopes in the array will rotate\n" \
  "in the same direction to the target."

#define TT_SUMDET_SRC_RA				\
  "Apparent target right ascension in current epoch"

#define TT_SUMDET_SRC_GLON			\
  "Approximate target galactic longitude"

#define TT_SUMDET_SRC_EL						\
  "Target Elevation. Background color indicates various conditions...\n" \
  "SOLID RED: Target is outside of allowed telescope limits.\n"		\
  "FLASHING YELLOW: Target will be outside of limits in 30 minutes\n"

#define TT_SUMDET_SRC_DEC			\
  "Apparent target declination in current epoch"

#define TT_SUMDET_SRC_GLAT			\
  "Approximate target galactic latitude"

#define TT_SUMDET_ERR_TOT			\
  "Angle between telescope and target"

#define TT_SUMDET_ERR_AZ				\
  "Difference in azimuth between\n"			\
  "telescope and target"

#define TT_SUMDET_ERR_EL				\
  "Difference in elevation between\n"			\
  "telescope and target"

#define TT_SUMDET_ERR_ETA					\
  "Estimated slew time to current target location (MM:SS)"

#define TT_SUMDET_AZ_SPEED						\
  "Azimuth drive velocity in degrees per\n"				\
  "second smoothed over short integration time"

#define TT_SUMDET_AZ_LIMIT						\
  "Telescope position exceeds azimuth\n"				\
  "clockwise or counter-clockwise limit"

#define TT_SUMDET_AZ_MODE			\
  "Azimuth servo mode (STANDBY, SLEW or POINT)"

#define TT_SUMDET_AZ_ANG			\
  "Raw azimuth drive angle\nor cable wrap"

#define TT_SUMDET_EL_SPEED						\
  "Elevation drive velocity in degrees per\n"				\
  "second smoothed over short integration time"

#define TT_SUMDET_EL_LIMIT						\
  "Telescope position exceeds elevation\n"				\
  "downward or upward limit"

#define TT_SUMDET_EL_MODE			\
  "Elevation servo mode (STANDBY, SLEW or POINT)"

#define TT_SUMDET_EL_ANG			\
  "Raw elevation drive angle"

#define TT_SUMDET_INTERLOCK						\
  "Interlock set. Commands are not accepted by the\n"			\
  "telescope, brakes are set and motion is disabled"

#define TT_SUMDET_ERROR				\
  "Drive servo failure or command receipt error"

// ****************************************************************************
// TOOLTIPS FOR OBJECT SELECTOR WIDGET
// ****************************************************************************

#define TT_OBJSEL_AZ				\
  "Target azimuth (+DDD.DDDD)"

#define TT_OBJSEL_EL				\
  "Target elevation (+/-DD.DDDD)"

#define TT_OBJSEL_NO_CORR						\
  "Do not apply tracking model to requested target position.\n"		\
  "This option allows a fixed target to be reproduced despite\n"	\
  "changes to the tracking correction model. It may be useful\n"	\
  "when slewing to the alignment point"

#define TT_OBJSEL_NO_STOP						\
  "Do not enable the brakes when the target position is reached.\n"	\
  "Instead, keep station at the target using the servo system"

#define TT_OBJSEL_RA				\
  "Target right ascension (HH:MM:SS.S)"

#define TT_OBJSEL_DEC				\
  "Target declination (+/-DD:MM:SS.S)"

#define TT_OBJSEL_RADEC_MODE			\
  "Tracking mode (On/Off, Wobble, Orbit, or Up/Down/Sideways)"

#define TT_OBJSEL_EPOCH				\
  "Epoch for target RA/Dec (EEEE.E)"

#define TT_OBJSEL_TARGET			\
  "List of targets"

#define TT_OBJSEL_TARGET_LOAD			\
  "Load targets from file"

#define TT_OBJSEL_TARGET_MODE				\
  "Tracking mode (On/Off, Wobble, Orbit, Up/Down/Sideways, or Pointing star)"

#define TT_OBJSEL_ONOFF				\
  "Offset from target position (On/Off)"

#define TT_OBJSEL_WOB_DIR			\
  "Direction to wobble off-source"

#define TT_OBJSEL_WOB_OFF			\
  "Size of wobble offset"

#define TT_OBJSEL_EL_OFF			\
  "Up/Down (Elevation) offset in degrees"

#define TT_OBJSEL_AZ_OFF			\
  "Sideways (Azimuth) offset in degrees"

#define TT_OBJSEL_ORB_PER			\
  "Period of orbit in minutes. CW means the orbit\n"	\
  "goes in the sense of North->East (as defined by\n"	\
  "wobble mode) while CCW means the orbit is in the\n"	\
  "sense of North->West." 

#define TT_OBJSEL_ORB_DIR						\
  "Initial direction of orbit. Setting this to \"Free\" means\n"	\
  "that the path of the orbit is defined purely by the UTC time\n"	\
  "(in MJD) with the wobble angle evolving as:\n\n"			\
  "  theta = 2*PI*MJD/(period/24/60)\n\n"				\
  "If an explicit initial direction is supplied the angle is a\n"	\
  "function of this value and the start time of the orbit:\n\n"		\
  "  theta = theta_0 + 2*PI*(MJD-MJD_0)/(period/24/60)"

#define TT_OBJSEL_ORB_OFF			\
  "Size of orbit wobble offset"

#define TT_OBJSEL_GRB				\
  "List of available GRBs"

#define TT_OBJSEL_MISC				\
  "Miscellaneous useful target positions"

#define TT_OBJSEL_GO				\
  "Go"

#define TT_OBJSEL_STOP				\
  "Stop"

// ****************************************************************************
// TOOLTIPS FOR POSITIONER STATUS WIDGET
// ****************************************************************************

#define TT_PSW_UP_LIMIT				\
  "Telescope elevation exceeds upward limit"

#define TT_PSW_DN_LIMIT				\
  "Telescope elevation exceeds downward limit"

#define TT_PSW_CW_LIMIT				\
  "Telescope azimuth exceeds clockwise limit"

#define TT_PSW_CC_LIMIT					\
  "Telescope azimuth exceeds counter-clockwise limit"

#define TT_PSW_MODE				\
  "Servo mode - Standby, Slew or Point"

#define TT_PSW_SERVO_ON				\
  "Servo amplifier power on"

#define TT_PSW_SERVO_FAULT			\
  "Servo amplifier / motor failure"

#define TT_PSW_BRAKES				\
  "Brake On"

#define TT_PSW_AZ_ANG				\
  "Raw azimuth drive angle\nor cable wrap"

#define TT_PSW_AZ_SPEED							\
  "Azimuth drive velocity smoothed\n"					\
  "by integration over short window"

#define TT_PSW_EL_ANG				\
  "Raw elevation drive angle"

#define TT_PSW_EL_SPEED							\
  "Elevation drive velocity smoothed\n"					\
  "by integration over short window"

#define TT_PSW_INTERLOCK						\
  "Interlock set. Commands are not accepted by the\n"			\
  "telescope, brakes are set and motion is disabled"

#define TT_PSW_ADC1				\
  "User ADC 1 value"

#define TT_PSW_ADC2				\
  "User ADC 2 value"

#define TT_PSW_WRAP				\
  "Cable wrap indicator fraction (-1 to +1)"

#define TT_PSW_CCW				\
  "Telescope has traveled CCW"

#define TT_PSW_AZ_PULL				\
  "Azimuth pull cord interlock set" 

#define TT_PSW_SAFE				\
  "Safety switch interlock set"

#define TT_PSW_AZ_STOW				\
  "Azimuth stow pin interlock set" 

#define TT_PSW_ElSTOW				\
  "Elevation stow pin interlock set"

#define TT_PSW_AZ_DOOR				\
  "Azimuth door interlock set"

#define TT_PSW_EL_DOOR				\
  "Elevation door interlock set"

#define TT_PSW_BAD_FRAME			\
  "Bad UDP Frame received by controller"

#define TT_PSW_BAD_CMD					\
  "Last command received by controller was invalid"

#define TT_PSW_IP_OVERRUN					\
  "Previous command was not yet processed by the controller"

#define TT_PSW_OP_OVERRUN				\
  "Controller still sending previous command response"
 
#define TT_PSW_RELAY1				\
  "User Relay 1"

#define TT_PSW_RELAY2				\
  "User Relay 2"

#define TT_PSW_CHECKSUM				\
  "Flash memory check-sum is incorrect"

#define TT_PSW_HAND_PADDLE						\
  "The hand paddle is connected and the\n"				\
  "positioner cannot be controlled remotely"

// ****************************************************************************
// TOOLTIPS FOR TRACKING CORRECTIONS PANE
// ****************************************************************************

#define TT_TCP_TAR_AZ				\
  "Target Azimuth"

#define TT_TCP_TAR_EL				\
  "Target Elevation"

#define TT_TCP_AZ_OFF							\
  "Angle (in the azimuth drive plane) between the elevation drive\n"	\
  "axis and the East-West direction when the azimuth encoder\n"		\
  "reads as zero. Measured in the usual azimuth sense (CW => +).\n"	\
  "Defined this way due to possible collimation error (below)"

#define TT_TCP_EL_OFF							\
  "Angle between the azimuth drive plane and the plane defined by\n"	\
  "the elevation axis and focus point when the elevation encoder\n"	\
  "reads as zero. Positive means the El zero position points upwards"

#define TT_TCP_AZ_NS							\
  "Angle in the North-Zenith-South plane of the azimuth drive\n"	\
  "plane with respect to the real horizontal. Positive means the\n"	\
  "upward normal to the azimuth drive plane points to the south"

#define TT_TCP_EL_UDEW							\
  "Inclination of the elevation drive axis to the azimuth\n"		\
  "drive plane. Positive means the eastward normal to the\n"		\
  "elevation axis points upwards (in the stow position)"

#define TT_TCP_AZ_EW \
  "Angle in the East-Zenith-West plane of the azimuth drive\n"		\
  "plane with respect to the real horizontal. Positive means the\n"	\
  "upward normal to the azimuth drive plane points to the west"

#define TT_TCP_EL_FLEX_A \
  "Coefficient of flexure proportional to cos(elevation)"

#define TT_TCP_FP_COLL \
  "Eastward pointing angle of the focus point with respect\n"	\
  "to the elevation axis when the telescope is stowed"

#define TT_TCP_EL_FLEX_B \
  "Coefficient of flexure proportional to sin(2 * elevation)"

#define TT_TCP_AZ_ENC							\
  "Ratio of 1 degree as measured by the azimuth encoder\n"		\
  "to 1 real degree. Probably best left set as 1.0"

#define TT_TCP_EL_ENC							\
  "Ratio of 1 degree as measured by the elevation encoder\n"		\
  "to 1 real degree. Probably best left set as 1.0"

#define TT_TCP_AZ_POS_VFF_S						\
  "Ratio of azimuth positional look-ahead (in degrees) to target\n"	\
  "azimuth speed (in degrees per second)"

#define TT_TCP_EL_POS_VFF_S						\
  "Ratio of elevation positional look-ahead (in degrees) to target\n"	\
  "elevation speed (in degrees per second)"

#define TT_TCP_AZ_POS_VFF_I			\
  "Offset of azimuth positional look-ahead"

#define TT_TCP_EL_POS_VFF_I			\
  "Offset of elevation positional look-ahead"

#define TT_TCP_AZ_NEG_VFF_S						\
  "Ratio of azimuth positional look-ahead (in degrees) to target\n"	\
  "azimuth speed (in degrees per second)"

#define TT_TCP_EL_NEG_VFF_S						\
  "Ratio of elevation positional look-ahead (in degrees) to target\n"	\
  "elevation speed (in degrees per second)"

#define TT_TCP_AZ_NEG_VFF_I			\
  "Offset of azimuth positional look-ahead"

#define TT_TCP_EL_NEG_VFF_I			\
  "Offset of elevation positional look-ahead"

// ****************************************************************************
// TOOLTIPS FOR CORRECTIONS SOLVER PANE
// ****************************************************************************

#define TT_SOLVER_DELETE			\
  "Delete selected measurement"

#define TT_SOLVER_SAVE				\
  "Save measurements"

#define TT_SOLVER_LOAD				\
  "Load measurements"

#define TT_SOLVER_EXPORT			\
  "Export full measurement table to text file"

#define TT_SOLVER_AZ_OFF			\
  "Azimuth encoder offset"

#define TT_SOLVER_EL_OFF			\
  "Elevation encoder offset"

#define TT_SOLVER_AZ_NS				\
  "Azimuth table inclination in NS direction"

#define TT_SOLVER_AZ_EW				\
  "Azimuth table inclination in EW direction"

#define TT_SOLVER_EL_AZ				\
  "Elevation-Azimuth axes non-perpendicularity"

#define TT_SOLVER_FP_COLL			\
  "Focal plane collimation error"

#define TT_SOLVER_EL_FLEX_A				\
  "Coefficient of flexure proportional to cos(elevation)"

#define TT_SOLVER_EL_FLEX_B					\
  "Coefficient of flexure proportional to sin(2 * elevation)"

#define TT_SOLVER_AZ_ENC			\
  "Azimuth encoder ratio"

#define TT_SOLVER_EL_ENC			\
  "Elevation encoder ratio"

#define TT_SOLVER_CP_LOAD			\
  "Load correction parameters from file"

#define TT_SOLVER_CP_SAVE			\
  "Save correction parameters to file"

#define TT_SOLVER_AZ_RMS			\
  "RMS residual of Az*cos(El) after corrections"

#define TT_SOLVER_AZ_MAX				\
  "Maximum residual of Az*cos(El) after corrections"

#define TT_SOLVER_EL_RMS			\
  "RMS residual of El after corrections"

#define TT_SOLVER_EL_MAX			\
  "Maximum residual of El after corrections"

#define TT_SOLVER_RMS				\
  "RMS residual after corrections"

#define TT_SOLVER_MAX				\
  "Maximum residual after corrections"

#define TT_SOLVER_MINIMIZE			\
  "Minimize selected parameters within ranges"

#define TT_SOLVER_MIN_CHOOSE				\
  "Minimize with respect to RMS or Maximum residual"

// ****************************************************************************
// TOOLTIPS FOR ARRAY PANE
// ****************************************************************************

#define TT_ARRAY_PANE_LOCK						\
  "Lock the telescope as a participant in the array or in\n"		\
  "stand-alone mode. Prevents inadvertent mouse clicks\n"		\
  "from changing the mode in the box to the left."

#define TT_ARRAY_PANE_SOLICIT_TARGET					\
  "While in stand-alone mode, transfer the selected array target\n"	\
  "to this telescope. This button may be useful for configuring\n"	\
  "each telescope to track a different target, or to add a telescope\n"	\
  "into the array, when the other telescopes are already tracking."

#if 0
#define TT_ARRAY_ARRAY_STANDALONE					\
  "Choose whether telescope should receive commands generated\n"	\
  "by clicking on buttons in the array target selection frame\n"	\
  "at the bottom of this tab. As an \"array participant\" the\n"	\
  "telescope will react to target selected in this tab, and\n"		\
  "to clicks on the Stop and Go buttons. In \"stand alone\" mode\n"	\
  "the telescope will only react to commands generated in the\n"	\
  "appropriate telescope tab. Irrespective of this setting, all\n"	\
  "telescopes react to a click on the \"Panic all\" button and\n"	\
  "to the \"Stop all\" command in the \"Motion\" menu."
#endif

#define TT_ARRAY_ARRAY_STANDALONE					\
  "Choose whether telescope should receive commands generated\n"	\
  "by clicking on buttons in the array target selection frame at\n"	\
  "the bottom of this tab. Irrespective of how this is set, all\n"	\
  "telescopes react to a click on the \"Panic all\" button and to\n"	\
  "the \"Stop all\" command in the \"Motion\" menu."

#define TT_ARRAY_SB_STAT						\
  "State of telescope controller, one of the following:\n"		\
  "STOPPED: Stopped with brakes energized.\n"				\
  "SLEWING: Slewing to target position.\n"				\
  "TRACKING: Tracking target to within 0.01 degrees.\n"			\
  "RESTRICTED MOTION: Cannot slew directly to target from\n"		\
  "   current position. Motion is modified to avoid restricted\n"	\
  "   regions outside of limits.\n"					\
  "ERROR: Telescope is indicating an error condition.\n"		\
  "INTERLOCK: An interlock is set on the telescope.\n"			\
  "RAMP DOWN: Slowing down, prior to applying brakes.\n"		\
  "NO SERVER: Communication failure between local program\n"		\
  "   and the server. Indicates CORBA or network error.\n"		\
  "SCOPE COM FAIL: Remote controller indicates a failure\n"		\
  "   communicating with the telescope. Communication with the\n"	\
  "   CORBA control server is OK."

// ****************************************************************************
// TOOLTIPS FOR GRB PANE
// ****************************************************************************

#define TT_GRB_MONITOR_STATUS						\
  "Status of the connection with the GRB socket monitor software\n"

#define TT_GRB_MONITOR_UPTIME			\
  "Up time of GRB socket monitor software.\n"

#define TT_GRB_GCN_CONNECTION						\
  "Status of the connection between the socket monitor software\n"	\
  "and the GRB Global Coordinates Network (GCN) based at Goddard\n"	\
  "Space Flight Center (GSFC) in Maryland."

#define TT_GRB_GCN_PACK_TIME			\
  "Time since receipt of last packet from GCN."

// ****************************************************************************
// LABEL FOR GRB WIZARD
// ****************************************************************************

#define TTL_GRB_WIZARD_1 "GRB Alert Wizard (1/3)"
#define TTL_GRB_WIZARD_2 "GRB Alert Wizard (2/3)"
#define TTL_GRB_WIZARD_3 "GRB Alert Wizard (3/3)"

#define LAB_GRB_WIZARD_1                                               \
  "An observable GRB has occurred. This wizard will help you\n"        \
  "configure the array to observe the burst. Begin by reviewing\n"     \
  "the information below.\n\n"					       \
  "Click \"next\" to immediately stop all of the telescopes\n"         \
  "and load the GRB as the target.\n\n"                                \
  "Click \"cancel\" to abort and configure the run manually.\n\n"      \

#define LAB_GRB_WIZARD_2                                               \
  "The telescopes are now stopped and are ready to slew to the GRB.\n" \
  "Click \"Slew!\" to slew to the location of the GRB. If the moon\n"  \
  "is up, take care not to get too close with the HV on!\n\n"          \

#define LAB_GRB_WIZARD_3                                               \
  "The telescopes should be slewing to the GRB location.\n"            \
  "If there is no run in progress, start one in VAC immediately.\n"    \
  "If there is a run in progress, extend the duration in VAC by\n"     \
  "20 minutes\n\n"                                                     \
  "If this is a Swift of Fermi LAT burst, track the GRB for up\n"      \
  "to 3 hours or until it goes below ~20 degrees elevation\n\n"        \
  "If this is a GBM burst, track the GRB for up to 1 hour\n"           \
  "or until it goes below ~20 degrees in elevation\n\n"                \
  "Monitor the VERITAS GRB webpage or the GCN:\n"		       \
  "http://scipp.ucsc.edu/~stburst/veritas.html\n"		       \
  "http://gcn.gsfc.nasa.gov/burst_info.html\n\n"		       \
  "Click \"finish\" to exit the wizard.\n"

// ****************************************************************************
// LABEL FOR CORRECTIONS DIALOGS
// ****************************************************************************

#define LAB_CORR_DIALOG_SAVE_COMMENT					\
  "Please enter a comment describing the corrections which will be saved\n" \
  "to the database. For example: \"Corrections after biased alignment of\n" \
  "2007-07-19 with measurements of 40 stars from 2007-07-22.\""
