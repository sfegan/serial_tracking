//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TenMeter.cpp
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

#include<iostream>

#include<float.h>

#include "TenMeter.h"

using namespace VTracking;
using namespace VMessaging;

#ifdef BRUCMD_BUILDMAP
// MUST COME BEFORE ANY BRUCMD INITIALIZATIONS
std::map<unsigned, BRUDrive::BRUCmd::CmdInfo> BRUDrive::BRUCmd::s_command_map;
#endif

static uint8_t s_ver_min[] = { 0x00, 0x00 };
static uint8_t s_ver_max[] = { 0xFF, 0xFF };
static int16_t s_ratio_min[] = { 0x8001, 0x0001 };
static int16_t s_ratio_max[] = { 0x7FFF, 0x7FFF };
static uint8_t s_mti_min[] = { 0x00, 0x00, 0x00 };
static uint8_t s_mti_max[] = { 0xFF, 0xFF, 0x1F };
static uint16_t s_mtrs_min[] = { 0x0000, 0x0000 };
static uint16_t s_mtrs_max[] = { 0x0000, 0x0000 };
static uint8_t s_mtv_min[] = { 0x00, 0x00 };
static uint8_t s_mtv_max[] = { 0xFF, 0xFF };
const int16_t s_cdata_min[] = { 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
				0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001 };
const int16_t s_cdata_max[] = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
				0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF };

const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdProductType("Product Type",0x0000,0xFFFF,0x00,0x03);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdPowerupStatus("Powerup Status",0x0001,0xFFFF,0x00,0xFF);
const BRUDrive::BRUUInt8ArrayCmd BRUDrive::sc_CmdMainFirmwareVersion("Main Firmware Version",0x0002,0xFFFF,2,s_ver_min,s_ver_max);
const BRUDrive::BRUUInt8ArrayCmd BRUDrive::sc_CmdBootFirmwareVersion("Boot Firmware Version",0x0003,0xFFFF,2,s_ver_min,s_ver_max);

const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdResetPersonalityEEPROM("Reset Personality EEPROM",0xFFFF,0x0010);
const BRUDrive::BRUStringCmd BRUDrive::sc_CmdDriveName("Drive Name",0x0011,0x0012,32);
const BRUDrive::BRUFloat32Cmd BRUDrive::sc_CmdPositionUserScaleValue("Position User Scale Value",0x0013,0x0014,FLT_MIN,FLT_MAX);
const BRUDrive::BRUStringCmd BRUDrive::sc_CmdPositionUserScaleText("Position User Scale Text",0x0015,0x0016,8);
const BRUDrive::BRUFloat32Cmd BRUDrive::sc_CmdVelocityUserScaleValue("Velocity User Scale Value",0x0017,0x0018,FLT_MIN,FLT_MAX);
const BRUDrive::BRUStringCmd BRUDrive::sc_CmdVelocityUserScaleText("Velocity User Scale Text",0x0019,0x001A,8);
const BRUDrive::BRUFloat32Cmd BRUDrive::sc_CmdAccelerationUserScaleValue("Acceleration User Scale Value",0x001B,0x001C,FLT_MIN,FLT_MAX);
const BRUDrive::BRUStringCmd BRUDrive::sc_CmdAccelerationUserScaleText("Acceleration User Scale Text",0x001D,0x001E,8);
const BRUDrive::BRUFloat32Cmd BRUDrive::sc_CmdTorqueUserScaleValue("Torque User Scale Value",0x001F,0x0020,FLT_MIN,FLT_MAX);
const BRUDrive::BRUStringCmd BRUDrive::sc_CmdTorqueUserScaleText("Torque User Scale Text",0x0021,0x0022,8);

const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionLoopProportionalGain("Position Loop Proportional Gain",0x0030,0x0031,0x0000,0x0FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionLoopIntegralGain("Position Loop Integral Gain",0x0032,0x0033,0x0000,0x0FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionLoopDerivativeGain("Position Loop Derivative Gain",0x0034,0x0035,0x0000,0x0FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionLoopFeedForwardGain("Position Loop Feed Forward Gain",0x0036,0x0037,0x0000,0x00C8);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdIntegratorZone("Integrator Zone",0x0038,0x0039,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionWindowSize("Position Window Size",0x003A,0x003B,0x0000,0x7FFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdPositionWindowTime("Position Window Time",0x003C,0x003D,0x00,0xFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdPositionErrorLimit("Position Error Limit",0x003E,0x003F,0x00000001,0x7FFFFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositionErrorTime("Position Error Time",0x0040,0x0041,0x0000,0xFFFF);
const BRUDrive::BRUInt16ArrayCmd BRUDrive::sc_CmdGearRatio("Gear Ratio",0x0042,0x0043,2,s_ratio_min,s_ratio_max);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdMasterRotationDirection("Master Rotation Direction",0x0044,0x0045,0x00,0x01);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdSlewRate("Slew Rate",0x0046,0x0047,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdSlewEnable("Slew Enable",0x0048,0x0049,0x00,0x01);

const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdVelocityLoopProportionalGain("Velocity Loop Proportional Gain",0x004A,0x004B,0x0000,0x03E8);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdVelocityLoopIntegralGain("Velocity Loop Integral Gain",0x004C,0x004D,0x0000,0x03E8);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdVelocityLoopDerivativeGain("Velocity Loop Derivative Gain",0x004E,0x004F,0xFC18,0x03E8);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdZeroSpeedLimit("Zero Speed Limit",0x0050,0x0051,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdSpeedWindowSize("Speed Window Size",0x0052,0x0053,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdOverSpeedLimit("Over Speed Limit",0x0054,0x0055,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdAtSpeedLimit("At Speed Limit",0x0056,0x0057,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdVelocityLoopUpdatePeriod("Velocity Loop Update Period",0x0058,0x0059,0x00,0x07);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdVelocityErrorLimit("Velocity Error Limit",0x005A,0x005B,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdVelocityErrorTime("Velocity Error Time",0x005C,0x005D,0x0000,0xFFFF);

const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdLowPassFilterBandwidth("Low Pass Filter Bandwidth",0x0070,0x0071,0x0001,0x03E0);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdLowPassFilterEnable("Low Pass Filter Enable",0x0076,0x0077,0x00,0x01);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdPositiveCurrentLimit("Positive Current Limit",0x007A,0x007B,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdNegativeCurrentLimit("Negative Current Limit",0x007C,0x007D,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdFaultCurrent("Fault Current",0x007E,0x007F,0x0000,0x7FFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdPWMFrequencySwitchingDisable("PWM Frequency Switching Disable",0x01A8,0x01A9,0x00,0x01);

const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMotorID("Motor ID",0x0090,0x0091,0x0000,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdEncoderLines("Encoder Lines",0x0092,0x0093,0x0064,0x3A98);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdMaximumMotorSpeed("Maximum Motor Speed",0x0094,0x0095,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMotorPeakCurrent("Motor Peak Current",0x0096,0x0097,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMotorContinuousCurrent("Motor Continuous Current",0x0098,0x0099,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdTorqueConstantKt("Torque Constant Kt",0x009A,0x009B,0x0001,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdRotorIntertiaJm("Rotor Intertia Jm",0x009C,0x009D,0x0001,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdBackEMFConstantKe("Back EMF Constant Ke",0x009E,0x009F,0x0001,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdWindingResistance("Winding Resistance",0x00A0,0x00A1,0x0001,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdWindingInductance("Winding Inductance",0x00A2,0x00A3,0x0001,0xFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdThermostatFlag("Thermostat Flag",0x00A4,0x00A5,0x00,0x01);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdCommutationType("Commutation Type",0x00A6,0x00A7,0x00,0x04);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdThermalTimeConstantEnable("Thermal Time Constant Enable",0x01A6,0x01A7,0x00,0x01);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdThermalTimeConstant("Thermal Time Constant",0x00AA,0x00AB,0x0000,0xFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdPoleCount("Pole Count",0x00AC,0x00AD,0x00,0x03);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdHallOffset("Hall Offset",0x00AE,0x00AF,0x0000,0x167);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdIndexOffset("Index Offset",0x00B0,0x00B1,0x0000,0x0167);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdMotorForwardDirectionFlag("Motor Forward Direction Flag",0x01AA,0x01AB,0x00,0x01);

const BRUDrive::BRUUInt8ArrayCmd BRUDrive::sc_CmdMotorTableInformation("Motor Table Information",0x00B2,0xFFFF,3,s_mti_min,s_mti_max);
const BRUDrive::BRUUInt16ArrayCmd BRUDrive::sc_CmdMotorTableRecordSize("Motor Table Record Size",0x00B3,0xFFFF,2,s_mtrs_min,s_mtrs_max);
const BRUDrive::BRUUInt8ArrayCmd BRUDrive::sc_CmdMotorTableVersion("Motor Table Version",0x00B4,0xFFFF,2,s_mtv_min,s_mtv_max);

const BRUDrive::BRUIndex8UInt16Cmd BRUDrive::sc_CmdDigitalInputConfigurationRegister("Digital Input Configuration Register",0x00C0,0x00C1,0x00,0x03,0x0000,0x7FFF);
const BRUDrive::BRUIndex8UInt16Cmd BRUDrive::sc_CmdDigitalOutputConfigurationRegister("Digital Output Configuration Register",0x00C2,0x00C3,0x00,0x03,0x0000,0x03FF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDigitalOutputOverrideEnable("Digital Output Override Enable",0x00C4,0x00C5,0x00,0x01);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdDigitalOutputOverrideWriteMask("Digital Output Override Write Mask",0x00C6,0x00C7,0x00,0x3F);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdBRAKEOutputActiveDelay("BRAKE Output Active Delay",0x00C8,0x00C9,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdBRAKEOutputInactiveDelay("BRAKE Output Inactive Delay",0x00CA,0x00CB,0x8001,0x7FFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdExpandedDigitalOutputConfigurationRegister("Expanded Digital Output Configuration Register",0x01D0,0x01D1,0x00,0x03,0x00000000,0x03FFFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdFaultResetInputConfigurationRegister("Fault Reset Input Configuration Register",0x01CE,0x01CF,0x0000,0x7FFF);

const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputVelocityOffset("COMMAND Input Velocity Offset",0x00CC,0x00CD,0xD8F0,0x2710);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputVelocityScale("COMMAND Input Velocity Scale",0x00CE,0x00CF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputTorqueOffset("COMMAND Input Torque Offset",0x00D0,0x00D1,0xD8F0,0x2710);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputTorqueScale("COMMAND Input Torque Scale",0x00D2,0x00D3,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputPositionOffset("COMMAND Input Position Offset",0x0060,0x0061,0xD8F0,0x2710);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInputPositionScale("COMMAND Input Position Scale",0x005E,0x005F,0x8001,0x7FFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdAnalogOutputConfigurationRegister("Analog Output Configuration Register",0x00D4,0x00D5,0x00,0x01,0x00,0x20);
const BRUDrive::BRUIndex8Int16Cmd BRUDrive::sc_CmdAnalogOutputOffset("Analog Output Offset",0x00D6,0x00D7,0x00,0x01,0x8001,0x7FFF);
const BRUDrive::BRUIndex8Int16Cmd BRUDrive::sc_CmdAnalogOutputScale("Analog Output Scale",0x00D8,0x00D9,0x00,0x01,0x8001,0x7FFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdAnalogOutputOverrideEnable("Analog Output Override Enable",0x00DA,0x00DB,0x00,0x01);
const BRUDrive::BRUIndex8Int16Cmd BRUDrive::sc_CmdAnalogOutputOverrideWriteValue("Analog Output Override Write Value",0x00DC,0x00DD,0x00,0x01,0xD8F0,0x2710);

const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdSerialPortBaudRate("Serial Port Baud Rate",0x00DE,0x00DF,0x00,0x04);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdSerialPortFrameFormat("Serial Port Frame Format",0x00E0,0x00E1,0x00,0x04);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDriveID("Drive ID",0x00E2,0x00E3,0x00,0xFF);

const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdEncoderOutputConfigurationRegister("Encoder Output Configuration Register",0x00F0,0x00F1,0x00,0x03);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdCommandSource("Command Source",0x00F2,0x00F3,0x00,0x06);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDriveMode("Drive Mode",0x00F4,0x00F5,0x00,0x01);
const BRUDrive::BRUIndex8Int32Cmd BRUDrive::sc_CmdVelocityPreset("Velocity Preset",0x00F6,0x00F7,0x00,0x07,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUIndex8Int16Cmd BRUDrive::sc_CmdTorquePreset("Torque Preset",0x00F8,0x00F9,0x00,0x07,0x8001,0x7FFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdAnalogInputAccelerationLimitsEnable("Analog Input Acceleration Limits Enable",0x01A2,0x01A3,0x00,0x01);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdAnalogInputAccelerationLimit("Analog Input Acceleration Limit",0x00FA,0x00FB,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdAnalogInputDecelerationLimit("Analog Input Deceleration Limit",0x00FC,0x00FD,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdPresetAccelerationLimitsEnable("Preset Acceleration Limits Enable",0x01A4,0x01A5,0x00,0x01);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdPresetAccelerationLimit("Preset Acceleration Limit",0x00FE,0x00FF,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdPresetDecelerationLimit("Preset Deceleration Limit",0x0100,0x0101,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdOperatingMode("Operating Mode",0x0102,0x0103,0x00,0x05);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdOperatingModeStatus("Operating Mode Status",0x0104,0xFFFF,0x00,0x7F);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdAutoTuneMaximumCurrent("Auto Tune Maximum Current",0x0105,0x0106,0x0001,0x7FFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdAutoTuneMaximumDistance("Auto Tune Maximum Distance",0x0107,0x0108,0x00000001,0x7FFFFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdManualTunePositionPeriod("Manual Tune Position Period",0x0109,0x010A,0x0001,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdManualTunePositionStep("Manual Tune Position Step",0x010B,0x010C,0x0001,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdManualTuneVelocityPeriod("Manual Tune Velocity Period",0x010D,0x010E,0x0001,0x7FFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdManualTuneVelocityStep("Manual Tune Velocity Step",0x010F,0x0110,0x00000001,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdTuningDirectionFlag("Tuning Direction Flag",0x01A0,0x01A1,0x00,0x02);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdEncoderAlignmentOffset("Encoder Alignment Offset",0x0111,0x0112,0xFF4C,0x00B3);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdRemoveAlignmentOffset("Remove Alignment Offset",0xFFFF,0x0113);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMotorEncoderResolution("Motor Encoder Resolution",0x0114,0xFFFF,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMasterEncoderResolution("Master Encoder Resolution",0x0115,0xFFFF,0x0000,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMotorIndexPosition("Motor Index Position",0x0116,0xFFFF,0x0000,0xFFFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdMasterIndexPosition("Master Index Position",0x0117,0xFFFF,0x0000,0xFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdChangeDirectionFlag("Change Direction Flag",0x01AC,0x01AD,0x00,0x01);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdOverrideCommandSource("Override Command Source",0x00BC,0x00BD,0x00,0x06);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdOverrideDriveMode("Override Drive Mode",0x00BE,0x00BF,0x00,0x01);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdDefineHomePosition("Define Home Position",0xFFFF,0x012F);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdHostIndexingControlEnableFlag("Host Indexing Control Enable Flag",0x012C,0x012D,0x00,0x01);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdHostIndex("Host Index",0x01B0,0x01B1,0x00,0x08);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdStartIndex("Start Index",0xFFFF,0x012E);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdAutoStartIndexingFlag("Auto Start Indexing Flag",0x00EE,0x00EF,0x00,0x01);
const BRUDrive::BRUIndex8UInt8Cmd BRUDrive::sc_CmdIndexType("Index Type",0x01B2,0x01B3,0x00,0x08,0x00,0x02);
const BRUDrive::BRUIndex8Int32Cmd BRUDrive::sc_CmdIndexDistanceAndPosition("Index Distance And Position",0x01B4,0x01B5,0x00,0x08,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdRegistrationDistance("Registration Distance",0x01B6,0x01B7,0x00,0x08,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdIndexVelocity("Index Velocity",0x01B8,0x01B9,0x00,0x08,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdIndexAcceleration("Index Acceleration",0x01BA,0x01BB,0x00,0x08,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt32Cmd BRUDrive::sc_CmdIndexDeceleration("Index Deceleration",0x01BC,0x01BD,0x00,0x08,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt16Cmd BRUDrive::sc_CmdIndexDwell("Index Dwell",0x01BE,0x01BF,0x00,0x08,0x0000,0xFFFF);
const BRUDrive::BRUIndex8UInt16Cmd BRUDrive::sc_CmdIndexBatchCount("Index Batch Count",0x01C0,0x01C1,0x00,0x08,0x0000,0xFFFF);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdStartHoming("Start Homing",0xFFFF,0x01D3);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdAutoStartHomingFlag("Auto Start Homing Flag",0x01C2,0x01C3,0x00,0x02);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdHomingType("Homing Type",0x01C4,0x01C5,0x00,0x02);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdHomingVelocity("Homing Velocity",0x01C6,0x01C7,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdHomingAcceleration("Homing Acceleration",0x01C8,0x01C9,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdHomingOffsetMoveDistance("Homing Offset Move Distance",0x01CA,0x01CB,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdHomePosition("Home Position",0x01CC,0x01CD,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUIndex8UInt8Cmd BRUDrive::sc_CmdIndexTermination("Index Termination",0x01D4,0x01D5,0x00,0x08,0x00,0x02);
const BRUDrive::BRUIndex8UInt8Cmd BRUDrive::sc_CmdIndexPointer("Index Pointer",0x01D6,0x01D7,0x00,0x08,0x00,0x08);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdHomeSensorBackOffFlag("Home Sensor Back Off Flag",0x01D8,0x01D9,0x00,0x01);

const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdResetDrive("Reset Drive",0xFFFF,0x0120);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdHostDriveEnableDisable("Host Drive Enable Disable",0x0121,0x0122,0x00,0x01);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdTorqueSetpoint("Torque Setpoint",0x0123,0x0124,0x8001,0x7FFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdVelocitySetpoint("Velocity Setpoint",0x0125,0x0126,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdSetpointAcceleration("Setpoint Acceleration",0x0127,0x0128,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdSetpointControlEnableFlag("Setpoint Control Enable Flag",0x0129,0x012A,0x00,0x01);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdResetFaults("Reset Faults",0xFFFF,0x012B);

const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdDriveStatus("Drive Status",0x0134,0xFFFF,0x00000000,0xFFFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdFaultStatus("Fault Status",0x0135,0xFFFF,0x00000000,0xFFFFFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdRunState("Run State",0x0136,0xFFFF,0x00,0x20);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdDigitalInputStates("Digital Input States",0x0137,0xFFFF,0x00,0x3F);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdDigitalOutputStates("Digital Output States",0x0138,0xFFFF,0x00,0x3F);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdExpandedOutputStatus("Expanded Output Status",0x01D2,0xFFFF,0x00000000,0xFFFFFFFF);

const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdResetPeaks("Reset Peaks",0xFFFF,0x0140);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCOMMANDInput("COMMAND Input",0x0141,0xFFFF,0xD8F0,0x2710);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdPositiveCurrentLimitInput("Positive Current Limit Input",0x0142,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdNegativeCurrentLimitInput("Negative Current Limit Input",0x0143,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUIndex8Int16Cmd BRUDrive::sc_CmdAnalogOutput("Analog Output",0x0144,0xFFFF,0x00,0x01,0xD8F0,0x2710);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdMotorPosition("Motor Position",0x0145,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdMasterPosition("Master Position",0x0146,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdPositionCommand("Position Command",0x0147,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdPositionError("Position Error",0x0148,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdPositionPositivePeakError("Position Positive Peak Error",0x0149,0xFFFF,0x00000000,0x7FFFFFFF);
const BRUDrive::BRUUInt32Cmd BRUDrive::sc_CmdPositionNegativePeakError("Position Negative Peak Error",0x014A,0xFFFF,0x80000001,0x00000000);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdVelocityCommand("Velocity Command",0x014B,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdMotorVelocity("Motor Velocity",0x014C,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt32Cmd BRUDrive::sc_CmdVelocityError("Velocity Error",0x014D,0xFFFF,0x80000001,0x7FFFFFFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCurrentCommand("Current Command",0x014E,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdAverageCurrent("Average Current",0x014F,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCurrentPositivePeak("Current Positive Peak",0x0150,0xFFFF,0x0000,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdCurrentNegativePeak("Current Negative Peak",0x0151,0xFFFF,0x8001,0x0000);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdBusVoltage("Bus Voltage",0x0152,0xFFFF,0x0000,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdFieldCurrent("Field Current",0x0153,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdTorqueCurrent("Torque Current",0x0154,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdRPhaseCurrent("R Phase Current",0x0155,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdTPhaseCurrent("T Phase Current",0x0156,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdFieldVoltageCommand("Field Voltage Command",0x0157,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdTorqueVoltageCommand("Torque Voltage Command",0x0158,0xFFFF,0x8001,0x7FFF);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdAverageMotorCurrent("Average Motor Current",0x0159,0xFFFF,0x0000,0x7FFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdIndexNumber("Index Number",0x013E,0xFFFF,0x00,0x08);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdIndexCount("Index Count",0x013F,0xFFFF,0x0000,0xFFFF);

const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdChannel1DataCollectionSource("Channel 1 Data Collection Source",0x0160,0x0161,0x00,0x20);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdChannel2DataCollectionSource("Channel 2 Data Collection Source",0x0162,0x0163,0x00,0x20);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDataCollectionTriggerSource("Data Collection Trigger Source",0x0164,0x0165,0x00,0x20);
const BRUDrive::BRUUInt16Cmd BRUDrive::sc_CmdDataCollectionTimebase("Data Collection Timebase",0x0166,0x0167,0x0000,0xFFFF);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDataCollectionTriggerMode("Data Collection Trigger Mode",0x0168,0x0169,0x00,0x02);
const BRUDrive::BRUInt16Cmd BRUDrive::sc_CmdDataCollectionTriggerThreshold("Data Collection Trigger Threshold",0x016A,0x016B,0x8001,0x7FFF);
const BRUDrive::BRUEmptyCmd BRUDrive::sc_CmdArmDataCollectionTriggering("Arm Data Collection Triggering",0xFFFF,0x016C);
const BRUDrive::BRUUInt8Cmd BRUDrive::sc_CmdDataCollectionTriggerStatus("Data Collection Trigger Status",0x016D,0xFFFF,0x00,0x02);
const BRUDrive::BRUIndex8Int16ArrayCmd BRUDrive::sc_CmdCollectedData("Collected Data",0x016E,0xFFFF,0x00,0x0F,16,s_cdata_min,s_cdata_max);

BRUDrive::~BRUDrive()
{
  // nothing to see here
}

#ifdef TEST_BRU
bool BRUDrive::simpleEmulator(const std::string& packet, std::string& response)
{
#ifndef BRUDRIVE_BUILD_MEMBERS
#include "BRUDrive_emulator_cpp.txt"
#else
  response="";
  return false;
#endif
}
#endif

// ----------------------------------------------------------------------------
// Member Functions Generated Externally By "./makeBRUDriveMembers.pl"
// ----------------------------------------------------------------------------
#ifndef BRUDRIVE_BUILD_MEMBERS
#include "BRUDrive_members_cpp.txt"
#endif

BRUDrive::ProtocolError::~ProtocolError() throw()
{
  // nothing to see here
}

BRUDrive::ErrorResponse::~ErrorResponse() throw()
{
  // nothing to see here
}

// ****************************************************************************
// BRUCmd
// ****************************************************************************

const char BRUDrive::BRUCmd::sc_SOM(':');
const char BRUDrive::BRUCmd::sc_EOM('\r');
//const char BRUDrive::BRUCmd::sc_EOM('\n');

BRUDrive::BRUCmd::BRUCmd(const std::string& cmd_name, 
			 unsigned read_cmd_num, unsigned write_cmd_num):
  m_cmd_name(cmd_name), m_read_cmd_num(read_cmd_num), m_write_cmd_num(write_cmd_num) 
{ 
#ifdef BRUCMD_BUILDMAP
  if((read_cmd_num<=0x07FF)&&(write_cmd_num<=0x07FF)&&(write_cmd_num<=read_cmd_num))
    {
      std::cerr << cmd_name << " write code " << write_cmd_num 
		<< " <= read code " << read_cmd_num << std::endl;
    }

  if(read_cmd_num<=0x07FF)
    {
      if(s_command_map.find(read_cmd_num) != s_command_map.end())
	std::cerr << "Read " << cmd_name << ": Command code " 
		  << SimpleAtomCODEC<uint8_t>::encode(read_cmd_num) << " duplicated with " 
		  << s_command_map[read_cmd_num].pre << ' ' << s_command_map[read_cmd_num].cmd->m_cmd_name 
		  << std::endl;
      else s_command_map[read_cmd_num] = CmdInfo(std::string("Read"),this);
    }
  else if(read_cmd_num!=0xFFFF)
    {
      std::cerr << "Read " << cmd_name << ": Command code " 
		<< SimpleAtomCODEC<uint8_t>::encode(read_cmd_num) 
		<< " invalid" << std::endl;
    }

  if(write_cmd_num<=0x07FF)
    {
      if(s_command_map.find(write_cmd_num) != s_command_map.end())
	std::cerr << "Write " << cmd_name << ": Command code " 
		  << SimpleAtomCODEC<uint8_t>::encode(write_cmd_num) 
		  << " duplicated with " 
		  << s_command_map[write_cmd_num].pre << ' ' << s_command_map[write_cmd_num].cmd->m_cmd_name 
		  << std::endl;
      else s_command_map[write_cmd_num] = CmdInfo(std::string("Write"),this);
    }
  else if(write_cmd_num!=0xFFFF)
    {
      std::cerr << "Write " << cmd_name << ": Command code " 
		<< SimpleAtomCODEC<uint8_t>::encode(write_cmd_num) 
		<< " invalid" << std::endl;
    }
#endif
}

BRUDrive::BRUCmd::~BRUCmd()
{
  // nothing to see here
}

#ifdef BRUCMD_BUILDMAP
void BRUDrive::BRUCmd::printCommandMap(std::ostream& stream)
{
  for(std::map<unsigned, CmdInfo>::const_iterator i = s_command_map.begin();
      i != s_command_map.end(); i++)
    stream << SimpleAtomCODEC<uint16_t>::encode(i->first).substr(1) 
	   << ' ' << std::left << std::setw(30) << i->second.cmd->commandTypeAsString()
	   << ' ' << i->second.pre << ' ' << i->second.cmd->m_cmd_name << std::endl;
}
#endif

void BRUDrive::BRUCmd::recvDataPacket(std::string& response, DataStream* ds) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool paa_last_hex = false;
  bool paa_print_space = false;
  
  if(ds->loud() >= 1)
    std::cout << "BRUCmd::getResponse() RECV  ";

  while(1)
    {
      std::string input = ds->recvData(1);

      if(ds->loud() >= 1)
	{
	  paa_last_hex = DataStream::printAsAscii(std::cout,input,paa_print_space,paa_last_hex);
	  paa_print_space=true;
	}
      
      std::string::size_type index = input.find(sc_SOM);
      if(index != std::string::npos)
	{
	  response=input.substr(index+1);
	  break;
	}
    } 

  std::string::size_type scan_start = 0;

  while(1)
    {
      static char SEOM[3] = { sc_SOM, sc_EOM, '\0' };
      std::string::size_type index = response.find_first_of(SEOM, scan_start, 2);
      
      if(index == std::string::npos)
	{
	  scan_start = response.length();
	  response += ds->recvData(1);	  

	  if(ds->loud() >= 1)
	    paa_last_hex = DataStream::printAsAscii(std::cout,response.substr(scan_start),paa_print_space,paa_last_hex);
	}
      else if(response[index] == sc_SOM)
	{
	  if(ds->loud() >= 1)std::cout << std::endl;
	  throw ProtocolError("Spurious SOM found in response",
			      std::string("Response: ")+response);
	}
      else if(index != response.length()-1)
	{
	  if(ds->loud() >= 1)std::cout << std::endl;
	  throw ProtocolError("Spurious characters found in response after EOM",
			      std::string("Response: ")+response);
	}
      else
	{
	  if(ds->loud() >= 1)std::cout << std::endl;
	  response = response.substr(0,response.length()-1);
	  break;
	}
    }

  if(response.length() < 7) // At minimum: AA FFF CC
    {
      throw ProtocolError("Response was smaller than minimum allowed length of seven characters (AAFFFCC)",
			  std::string("Response: ")+response);
    }

  std::string exp_sum = SimpleAtomCODEC<uint8_t>::encode(checkSum(response.substr(0,response.length()-2)));

  if(response.substr(response.length()-2,2) != exp_sum)
    {
      std::ostringstream details;
      details << "Response: " << response << std::endl
	      << "Expected checksum: " << exp_sum << std::endl;
      throw ProtocolError("Checksum error",details.str());
    }      
}

void BRUDrive::BRUCmd::getResponse(std::string& response,
				   DataStream* ds, const std::string& function,
				   const std::string& cmd_address,
				   const std::string& cmd_code,
				   const std::string& err_code,
				   int return_payload_size) const
{
  recvDataPacket(response,ds);

  bool error_response;
  size_t expected_response_size;

  std::string temp(response.substr(0,5));

  if(/* response.substr(0,5) */ temp == cmd_address+cmd_code)
    {
      // GOOD RESPONSE
      error_response = false;
      expected_response_size = 7;
      if(return_payload_size>0)expected_response_size += return_payload_size;
    }
  else if(/* response.substr(0,5) */ temp == cmd_address+err_code)
    {
      // ERROR RESPONSE
      error_response = true;
      expected_response_size = 9;
    }
  else
    {
      std::ostringstream details;
      details << "Response: " << response << std::endl
	      << "Expected address: " << cmd_address << std::endl
	      << "Expected function: " << cmd_code << std::endl;
      throw ProtocolError("Response was to unknown address or function",
			  details.str());
    }

  if((response.length() < expected_response_size)||
     ((response.length() > expected_response_size)&&
      ((error_response==true)||(return_payload_size>=0))))
    {
      std::ostringstream details;
      details << "Response: " << response << std::endl
	      << "Expected length: " << expected_response_size << std::endl;
      throw ProtocolError("Response length did not match expected",
			  details.str());
    }

  std::string payload = response.substr(5,response.length()-7);

  if(error_response)
    {
      unsigned code = SimpleAtomCODEC<uint8_t>::decode(payload);
      std::ostringstream details;
      details << "Response: " << response << std::endl
	      << "Error code: " << code << std::endl;
      throw ErrorResponse("BRU command returned an error",
			  details.str(),code);
    }

  response=payload;

  return;
}

void
BRUDrive::BRUCmd::doRead(std::string& response, DataStream* ds, unsigned address,
			 int return_payload_size, const std::string& payload) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_read_cmd_num > 0x07FF)
    {
      std::ostringstream details;
      details << "Command: Read " << m_cmd_name << std::endl 
	      << "Code: " << SimpleAtomCODEC<uint16_t>::encode(m_read_cmd_num).substr(1,3) << std::endl;
      throw(Exception("Internal error", "Invalid BRU command called",details.str()));
    }
  
  std::string cmd_address = SimpleAtomCODEC<uint8_t>::encode(address);
  std::string cmd_code = SimpleAtomCODEC<uint16_t>::encode(m_read_cmd_num).substr(1,3);
  std::string err_code = SimpleAtomCODEC<uint16_t>::encode(m_read_cmd_num|0x0800).substr(1,3);
  std::string cmd_sum = SimpleAtomCODEC<uint8_t>::encode(checkSum(cmd_address+cmd_code+payload));

  std::string command = sc_SOM + cmd_address + cmd_code + payload + cmd_sum + sc_EOM;
  ds->sendData(command);

  if(ds->loud() >= 1)
    {
      std::cout << "BRUCmd::doRead()      SEND  ";
      DataStream::printAsAscii(std::cout,command);
      std::cout << std::endl;
    }

  getResponse(response,ds,std::string("Read ")+m_cmd_name,
	      cmd_address,cmd_code,err_code,return_payload_size);
}

void 
BRUDrive::BRUCmd::doWrite(DataStream* ds, unsigned address, 
			  const std::string& payload) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_write_cmd_num > 0x07FF)
    {
      std::ostringstream details;
      details << "Command: Write " << m_cmd_name << std::endl 
	      << "Code: " << SimpleAtomCODEC<uint16_t>::encode(m_write_cmd_num).substr(1,3) << std::endl;      
      throw(Exception("Internal error", "Invalid BRU command called",details.str()));
    }
  
  std::string cmd_address = SimpleAtomCODEC<uint8_t>::encode(address);
  std::string cmd_code = SimpleAtomCODEC<uint16_t>::encode(m_write_cmd_num).substr(1,3);
  std::string err_code = SimpleAtomCODEC<uint16_t>::encode(m_write_cmd_num|0x0800).substr(1,3);
  std::string cmd_sum = SimpleAtomCODEC<uint8_t>::encode(checkSum(cmd_address+cmd_code+payload));
  
  std::string command = sc_SOM + cmd_address + cmd_code + payload + cmd_sum + sc_EOM;
  ds->sendData(command);

  if(ds->loud() >= 1)
    {
      std::cout << "BRUCmd::doWrite()     SEND  ";
      DataStream::printAsAscii(std::cout,command);
      std::cout << std::endl;
    }
  
  std::string response;
  getResponse(response,ds,std::string("Write ")+m_cmd_name,
	      cmd_address,cmd_code,err_code,0);
}

uint8_t
BRUDrive::BRUCmd::checkSum(const std::string& data)
{
  uint8_t sum = 0;
  for(unsigned i=0;i<data.length();i++)sum+=uint8_t(data[i]);
  return ~sum+1;
}

BRUDrive::BRUEmptyCmd::~BRUEmptyCmd()
{
  // nothing to see here
}

void BRUDrive::BRUEmptyCmd::read(DataStream* ds, int address) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::string payload;
  doRead(payload,ds,address,0);
  return;
}

void BRUDrive::BRUEmptyCmd::write(DataStream* ds, int address) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  doWrite(ds,address,"");
}

std::string 
BRUDrive::BRUEmptyCmd::commandTypeAsString() const
{
  return std::string("BRUEmptyCmd");
}

BRUDrive::BRUStringCmd::~BRUStringCmd()
{
  // nothing to see here
}

std::string BRUDrive::BRUStringCmd::read(DataStream* ds, int address) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::string payload;
  doRead(payload,ds,address,-1); //m_string_len*2);

  if(payload.length()%2 != 0)
    {
      throw ProtocolError("The response did not contain an even number of hex charcters",
			  std::string("Response: "+payload));
    }

  std::string str;
  unsigned payload_len = payload.length()/2;

  for(unsigned i=0; (i<m_string_len)&&(i<payload_len); i++)
    str.push_back(SimpleAtomCODEC<uint8_t>::decode(payload.substr(i*2,2)));
  return str;
}

void BRUDrive::BRUStringCmd::write(DataStream* ds, int address, const std::string& data) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::string payload;
  unsigned i=0;
  while((i<data.length())&&(i<m_string_len))
    payload += SimpleAtomCODEC<uint8_t>::encode(data[i++]);
  doWrite(ds,address,payload);
}

std::string 
BRUDrive::BRUStringCmd::commandTypeAsString() const
{
  std::ostringstream stream;
  stream << "BRUStringCmd<" << m_string_len << '>';
  return stream.str();
}

#ifdef TEST_BRU
BRULoopDataStream::~BRULoopDataStream()
{
  // nothing to see here
}

void BRULoopDataStream::sendData(const datastring& out, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(loud()>=2)
    {
      std::ostringstream s; 
      std::cout << "SEND(" << std::dec << out.size() << ',' 
	      << to_sec << ',' << to_usec << "):";
      printAsAscii(std::cout, out);
      std::cout << std::endl;
    }

  m_send_buffer += out;
  std::string::size_type som_idx = m_send_buffer.find(':');
  std::string::size_type eom_idx = m_send_buffer.find('\r');
  while((som_idx != std::string::npos)&&(eom_idx != std::string::npos))
    {
      std::string packet = m_send_buffer.substr(som_idx+1,eom_idx-1);

      for(std::vector<BRUDrive*>::iterator i = m_drives.begin();
	  i != m_drives.end(); i++)
	{
	  std::string response;
	  if((*i)->simpleEmulator(packet,response))
	    m_recv_buffer += std::string(":")+response+
	      BRUDrive::SimpleAtomCODEC<uint8_t>::encode(BRUDrive::BRUCmd::checkSum(response))+
	      std::string("\r");
	}

      m_send_buffer = m_send_buffer.substr(eom_idx+1);
      som_idx = m_send_buffer.find(':');
      eom_idx = m_send_buffer.find('\r');
    }
}

datastring BRULoopDataStream::recvData(size_t req_count, 
						   long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(loud()>=2)
    std::cout << "RECV(" << std::dec << req_count << ',' 
	      << to_sec << ',' << to_usec << "): ";

  datastring return_buffer;

  if(m_recv_buffer.length() == 0)
    {
      if(loud()>=2)std::cout << "Timeout" << std::endl;
      throw Timeout();
    }
  else if(m_recv_buffer.length() <= req_count)
    {
      return_buffer = m_recv_buffer;
      m_recv_buffer.clear();
    }
  else
    {
      return_buffer = m_recv_buffer.substr(0,req_count);
      m_recv_buffer = m_recv_buffer.substr(req_count);
    }

  if(loud()>=2)
    {
      printAsAscii(std::cout, return_buffer);
      std::cout << std::endl;
    }

  return return_buffer;
}

void BRULoopDataStream::resetDataStream()
{
  m_recv_buffer.clear();
}
#endif

#ifdef BRUDRIVE_BUILD_MEMBERS
int main()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // GENERATE LIST OF PROTOCOL FUNCTIONS
  BRUDrive::printBRUCommandMap(std::cout);
}
#endif

#ifdef TEST_BRU
int main()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  try
    {
      struct termios port;
      memset(&port,0,sizeof(port));
      port.c_iflag = IGNBRK;
      port.c_cflag = B9600 | CS8 | CLOCAL | CREAD | !CRTSCTS;
      port.c_lflag = NOFLSH | !ISIG;
      
      // SerialPortDataStream ds("/dev/ttyS0",0,0,1000000,&port);
      BRULoopDataStream ds;
      
      BRUDrive drive0(&ds,0);
      BRUDrive drive1(&ds,1);
     
      ds.pushDrive(&drive0);
      ds.pushDrive(&drive1);
 
      std::vector<uint8_t> vec;

#if 0
      for(unsigned i=0; i<1q00000; i++)
	drive0.readPositionLoopProportionalGain();
#else
      drive0.writeDriveName("Azimuth");
      drive1.writeDriveName("Elevation");

      for(unsigned i=0;i<2;i++)
	{
	  BRUDrive* drive = &drive0;
	  if(i==1)drive = &drive1;

	  std::cout << "Drive Name:     " << drive->readDriveName() << std::endl;
	  
	  std::cout << "Product Type:   " << int(drive->readProductType()) << std::endl;
	  vec = drive->readMainFirmwareVersion();
	  std::cout << "Main Firmware Version: " << int(vec[0]) << '.' << int(vec[1]) << std::endl;
	  vec = drive->readBootFirmwareVersion();
	  std::cout << "Boot Firmware Version: " << int(vec[0]) << '.' << int(vec[1]) << std::endl;
	  
	  std::cout << "Powerup Status: " << int(drive->readPowerupStatus()) << std::endl; 
	  std::cout << "Drive Status:   " << drive->readDriveStatus() << std::endl; 
	  std::cout << "Fault Status:   " << drive->readFaultStatus() << std::endl; 
	  std::cout << "Run State:      " << int(drive->readRunState()) << std::endl; 
	  
	  std::cout << "Position Kp:    " << drive->readPositionLoopProportionalGain() << std::endl;
	  std::cout << "Position Ki:    " << drive->readPositionLoopIntegralGain() << std::endl;
	  std::cout << "Position Kd:    " << drive->readPositionLoopDerivativeGain() << std::endl;
	  std::cout << "Position Kvff:  " << drive->readPositionLoopFeedForwardGain() << std::endl;
	  
	  std::cout << "Velocity Kp:    " << drive->readVelocityLoopProportionalGain() << std::endl;
	  std::cout << "Velocity Ki:    " << drive->readVelocityLoopIntegralGain() << std::endl;
	  std::cout << "Velocity Kd:    " << drive->readVelocityLoopDerivativeGain() << std::endl << std::endl;
	}
#endif
    }
  catch(const Exception& x)
    {
      x.print(std::cout);
    }

  return 0;
}
#endif
