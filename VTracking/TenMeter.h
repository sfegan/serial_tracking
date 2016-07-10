//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TenMeter.h
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

#ifndef VTRACKING_TENMETERAPI_H
#define VTRACKING_TENMETERAPI_H

#include<string>
#include<sstream>
#include<iomanip>
#include<vector>
#include<map>
#include<stdint.h>

#include<zthread/RecursiveMutex.h>

#include<Angle.h>

#include"Exception.h"
#include"ScopeAPI.h"
#include"ScopeProtocolServer.h"
#include"DataStream.h"

#define BRUCMD_BUILDMAP

namespace VTracking
{
  class BRUDrive;

#ifdef TEST_BRU
  class BRULoopDataStream: public DataStream
  {
  public:
    BRULoopDataStream(int loud=0): 
      DataStream(loud,0,0), m_drives(), m_send_buffer(), m_recv_buffer() { }
    virtual ~BRULoopDataStream();
    virtual void sendData(const datastring& out, long to_sec, long to_usec);
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec);
    virtual void resetDataStream();
    void pushDrive(BRUDrive* drive) { m_drives.push_back(drive); }
  private:
    std::vector<BRUDrive*> m_drives;
    std::string m_send_buffer;
    std::string m_recv_buffer;
  };
#endif
  
  class BRUDrive
  {
  public:
    BRUDrive(DataStream* ds, unsigned address):
      m_datastream(ds), m_address(address) { }
    virtual ~BRUDrive();

    class ProtocolError: public ScopeAPI::ScopeAPIError
    {
    public:
      ProtocolError(const std::string& message, const datastring& details=""):
	ScopeAPIError("BRU Protocol Error", message, details) {}
      virtual ~ProtocolError() throw();
    };

    class ErrorResponse: public ScopeAPI::ScopeAPIError
    {
    public:
      ErrorResponse(const std::string& message, const datastring& details, 
		    unsigned error_code):
	ScopeAPIError("BRU Command Error", message, details), 
	m_error_code(error_code) {}
      virtual ~ErrorResponse() throw();
      unsigned errorCode() const { return m_error_code; }
    private:
      unsigned m_error_code;
    };

    // ------------------------------------------------------------------------
    // Member Functions Generated Externally By "./makeBRUDriveMembers.pl"
    // ------------------------------------------------------------------------
#ifndef BRUDRIVE_BUILD_MEMBERS
#include "BRUDrive_members_hpp.txt"
#endif

#ifdef BRUCMD_BUILDMAP
    static void printBRUCommandMap(std::ostream& stream) { BRUCmd::printCommandMap(stream); }
#endif

  private:
#ifdef TEST_BRU
    friend class BRULoopDataStream;
    std::map<std::string, std::string> m_emulator_memory;
    bool simpleEmulator(const std::string& packet, std::string& response);
#endif

    DataStream* m_datastream;
    unsigned m_address;

    enum ControllerError { CE_INVALID_DATA=0x01, 
			   CE_EEPROM_WRITE=0x03, CE_LIMITED_TO_MIN=0x04,
			   CE_LIMITED_TO_MAX=0x05, CE_CMD_DISABLED=0x06 };

    template<class T> class SimpleAtomCODEC
    {
    public:
      static inline size_t length();
      static inline std::string encode(const T& atom);
      static inline T decode(const std::string& payload);
      static inline std::string typeAsString();
    };

    class BRUCmd
    {
    public:
      BRUCmd(const std::string& cmd_name, 
	     unsigned read_cmd_num, unsigned write_cmd_num);
      virtual ~BRUCmd();
      virtual std::string commandTypeAsString() const = 0;

#ifdef BRUCMD_BUILDMAP
      static void printCommandMap(std::ostream& stream);
#endif

      static uint8_t checkSum(const std::string& data);

    protected:
      void doRead(std::string& response, DataStream* ds, unsigned address,
		  int return_payload_size, const std::string& payload="") const;
      void doWrite(DataStream* ds, unsigned address, 
		   const std::string& payload) const;     

      void recvDataPacket(std::string& response, DataStream* ds) const;
  
      void getResponse(std::string& response,
		       DataStream* ds, const std::string& function,
		       const std::string& cmd_address,
		       const std::string& cmd_code,
		       const std::string& err_code,
		       int return_payload_size) const;
      
      std::string   m_cmd_name;
      unsigned      m_read_cmd_num;
      unsigned      m_write_cmd_num;

      static const char sc_SOM;
      static const char sc_EOM;
      
    private:
#ifdef BRUCMD_BUILDMAP
      class CmdInfo 
      {
      public:
	CmdInfo(): pre(), cmd() { }
	CmdInfo(const std::string& p, const BRUCmd* c): pre(p), cmd(c) { }
	std::string pre;
	const BRUCmd* cmd;
      };

      static std::map<unsigned, CmdInfo> s_command_map;
#endif
    };
    
    class BRUEmptyCmd: public BRUCmd
    {
    public:
      BRUEmptyCmd(const std::string& cmd_name, 
		 unsigned read_cmd_num, unsigned write_cmd_num):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num) { }
      virtual ~BRUEmptyCmd();

      void read(DataStream* ds, int address) const;
      void write(DataStream* ds, int address) const;
      virtual std::string commandTypeAsString() const;
    };

    template<class T> class BRUAtomCmd: public BRUCmd
    {
    public:
      BRUAtomCmd(const std::string& cmd_name, 
		 unsigned read_cmd_num, unsigned write_cmd_num,
		 const T& min, const T& max):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num), 
	m_min(min), m_max(max) { }
      virtual ~BRUAtomCmd();
      
      T read(DataStream* ds, int address) const;
      void write(DataStream* ds, int address, const T& data) const;
      virtual std::string commandTypeAsString() const;
      const T& min() const { return m_min; }
      const T& max() const { return m_max; }
    private:
      T m_min;
      T m_max;
    };

    template<class T> class BRUAtomArrayCmd: public BRUCmd
    {
    public:
      BRUAtomArrayCmd(const std::string& cmd_name, 
		      unsigned read_cmd_num, unsigned write_cmd_num,
		      unsigned array_len, const T* min, const T* max):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num), 
	m_array_len(array_len), m_min(), m_max()
      {
	m_min.insert(m_min.end(), min, min+m_array_len);
	m_max.insert(m_max.end(), max, max+m_array_len);
      }
      virtual ~BRUAtomArrayCmd();
      
      std::vector<T> read(DataStream* ds, int address) const;
      void write(DataStream* ds, int address, const std::vector<T>& data) const;
      virtual std::string commandTypeAsString() const;
      unsigned arrayLen() const { return m_array_len; }
      const std::vector<T>& min() const { return m_min; }
      const std::vector<T>& max() const { return m_max; }
    private:
      unsigned m_array_len;
      std::vector<T> m_min;
      std::vector<T> m_max;
    };

    template<class T> class BRUIndex8AtomCmd: public BRUCmd
    {
    public:
      BRUIndex8AtomCmd(const std::string& cmd_name, 
		       unsigned read_cmd_num, unsigned write_cmd_num,
		       uint8_t index_min, uint8_t index_max,
		       const T& min, const T& max):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num), 
	m_index_min(m_index_min), m_index_max(index_max), m_min(min), m_max(max) { }
      virtual ~BRUIndex8AtomCmd();
      
      T read(DataStream* ds, int address, uint8_t index) const;
      void write(DataStream* ds, int address, uint8_t index, const T& data) const;
      virtual std::string commandTypeAsString() const;
      const T& indexMin() const { return m_index_min; }
      const T& indexMax() const { return m_index_max; }
      const T& min() const { return m_min; }
      const T& max() const { return m_max; }
    private:
      uint8_t m_index_min;
      uint8_t m_index_max;
      T m_min;
      T m_max;
    };

    template<class T> class BRUIndex8AtomArrayCmd: public BRUCmd
    {
    public:
      BRUIndex8AtomArrayCmd(const std::string& cmd_name, 
			    unsigned read_cmd_num, unsigned write_cmd_num,
			    uint8_t index_min, uint8_t index_max,
			    unsigned array_len, const T* min, const T* max):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num), 
	m_index_min(m_index_min), m_index_max(index_max),
	m_array_len(array_len), m_min(), m_max()
      {
	m_min.insert(m_min.end(),min,min+array_len);
	m_max.insert(m_max.end(),max,max+array_len);
      }
      virtual ~BRUIndex8AtomArrayCmd();
      
      std::vector<T> read(DataStream* ds, int address, uint8_t index) const;
      void write(DataStream* ds, int address, uint8_t index, 
		 const std::vector<T>& data) const;
      virtual std::string commandTypeAsString() const;
      const T& indexMin() const { return m_index_min; }
      const T& indexMax() const { return m_index_max; }
      unsigned arrayLen() const { return m_array_len; }
      const std::vector<T>& min() const { return m_min; }
      const std::vector<T>& max() const { return m_max; }
    private:
      uint8_t m_index_min;
      uint8_t m_index_max;
      unsigned m_array_len;
      std::vector<T> m_min;
      std::vector<T> m_max;
    };

    typedef BRUAtomCmd<uint8_t> BRUUInt8Cmd;
    typedef BRUAtomCmd<int8_t> BRUInt8Cmd;
    typedef BRUAtomCmd<uint16_t> BRUUInt16Cmd;
    typedef BRUAtomCmd<int16_t> BRUInt16Cmd;
    typedef BRUAtomCmd<uint32_t> BRUUInt32Cmd;
    typedef BRUAtomCmd<int32_t> BRUInt32Cmd;

    typedef BRUAtomCmd<float> BRUFloat32Cmd;

    typedef BRUAtomArrayCmd<uint8_t> BRUUInt8ArrayCmd;
    typedef BRUAtomArrayCmd<int8_t> BRUInt8ArrayCmd;
    typedef BRUAtomArrayCmd<uint16_t> BRUUInt16ArrayCmd;
    typedef BRUAtomArrayCmd<int16_t> BRUInt16ArrayCmd;
    typedef BRUAtomArrayCmd<uint32_t> BRUUInt32ArrayCmd;
    typedef BRUAtomArrayCmd<int32_t> BRUInt32ArrayCmd;

    typedef BRUIndex8AtomCmd<uint8_t> BRUIndex8UInt8Cmd;
    typedef BRUIndex8AtomCmd<int8_t> BRUIndex8Int8Cmd;
    typedef BRUIndex8AtomCmd<uint16_t> BRUIndex8UInt16Cmd;
    typedef BRUIndex8AtomCmd<int16_t> BRUIndex8Int16Cmd;
    typedef BRUIndex8AtomCmd<uint32_t> BRUIndex8UInt32Cmd;
    typedef BRUIndex8AtomCmd<int32_t> BRUIndex8Int32Cmd;

    typedef BRUIndex8AtomArrayCmd<uint8_t> BRUIndex8UInt8ArrayCmd;
    typedef BRUIndex8AtomArrayCmd<int8_t> BRUIndex8Int8ArrayCmd;
    typedef BRUIndex8AtomArrayCmd<uint16_t> BRUIndex8UInt16ArrayCmd;
    typedef BRUIndex8AtomArrayCmd<int16_t> BRUIndex8Int16ArrayCmd;
    typedef BRUIndex8AtomArrayCmd<uint32_t> BRUIndex8UInt32ArrayCmd;
    typedef BRUIndex8AtomArrayCmd<int32_t> BRUIndex8Int32ArrayCmd;
    
    class BRUStringCmd: public BRUCmd
    {
    public:
      BRUStringCmd(const std::string& cmd_name, 
		   unsigned read_cmd_num, unsigned write_cmd_num,
		   unsigned string_len):
        BRUCmd(cmd_name,read_cmd_num,write_cmd_num), m_string_len(string_len) { }
      virtual ~BRUStringCmd();

      std::string read(DataStream* ds, int address) const;
      void write(DataStream* ds, int address, const std::string& data) const;
      virtual std::string commandTypeAsString() const;
    private:
      unsigned m_string_len;
    };

    // Common Product Line Commands -- Page 3
    static const BRUUInt8Cmd sc_CmdProductType;
    static const BRUUInt8Cmd sc_CmdPowerupStatus;
    static const BRUUInt8ArrayCmd sc_CmdMainFirmwareVersion;
    static const BRUUInt8ArrayCmd sc_CmdBootFirmwareVersion;

    // General Commands -- Page 5
    static const BRUEmptyCmd sc_CmdResetPersonalityEEPROM;
    static const BRUStringCmd sc_CmdDriveName;
    static const BRUFloat32Cmd sc_CmdPositionUserScaleValue;
    static const BRUStringCmd sc_CmdPositionUserScaleText;
    static const BRUFloat32Cmd sc_CmdVelocityUserScaleValue;
    static const BRUStringCmd sc_CmdVelocityUserScaleText;
    static const BRUFloat32Cmd sc_CmdAccelerationUserScaleValue;
    static const BRUStringCmd sc_CmdAccelerationUserScaleText;
    static const BRUFloat32Cmd sc_CmdTorqueUserScaleValue;
    static const BRUStringCmd sc_CmdTorqueUserScaleText;

    // Position Loop Commands -- Page 14
    static const BRUUInt16Cmd sc_CmdPositionLoopProportionalGain;
    static const BRUUInt16Cmd sc_CmdPositionLoopIntegralGain;
    static const BRUUInt16Cmd sc_CmdPositionLoopDerivativeGain;
    static const BRUUInt16Cmd sc_CmdPositionLoopFeedForwardGain;
    static const BRUUInt16Cmd sc_CmdIntegratorZone;
    static const BRUUInt16Cmd sc_CmdPositionWindowSize;
    static const BRUUInt8Cmd sc_CmdPositionWindowTime;
    static const BRUUInt32Cmd sc_CmdPositionErrorLimit;
    static const BRUUInt16Cmd sc_CmdPositionErrorTime;
    static const BRUInt16ArrayCmd sc_CmdGearRatio;
    static const BRUUInt8Cmd sc_CmdMasterRotationDirection;
    static const BRUUInt32Cmd sc_CmdSlewRate;
    static const BRUUInt8Cmd sc_CmdSlewEnable;

    // Velocity Loop Commands -- Page 25
    static const BRUUInt16Cmd sc_CmdVelocityLoopProportionalGain;
    static const BRUUInt16Cmd sc_CmdVelocityLoopIntegralGain;
    static const BRUInt16Cmd sc_CmdVelocityLoopDerivativeGain;
    static const BRUUInt32Cmd sc_CmdZeroSpeedLimit;
    static const BRUUInt32Cmd sc_CmdSpeedWindowSize;
    static const BRUUInt32Cmd sc_CmdOverSpeedLimit;
    static const BRUUInt32Cmd sc_CmdAtSpeedLimit;
    static const BRUUInt8Cmd sc_CmdVelocityLoopUpdatePeriod;
    static const BRUUInt32Cmd sc_CmdVelocityErrorLimit;
    static const BRUUInt16Cmd sc_CmdVelocityErrorTime;

    // Torque Current Conditioning Commands -- Page 34
    static const BRUUInt16Cmd sc_CmdLowPassFilterBandwidth;
    static const BRUUInt8Cmd sc_CmdLowPassFilterEnable;
    static const BRUUInt16Cmd sc_CmdPositiveCurrentLimit;
    static const BRUUInt16Cmd sc_CmdNegativeCurrentLimit;
    static const BRUUInt16Cmd sc_CmdFaultCurrent;
    static const BRUUInt8Cmd sc_CmdPWMFrequencySwitchingDisable;

    // Motor Commands -- Page 39
    static const BRUUInt16Cmd sc_CmdMotorID;
    static const BRUUInt16Cmd sc_CmdEncoderLines;
    static const BRUUInt32Cmd sc_CmdMaximumMotorSpeed;
    static const BRUUInt16Cmd sc_CmdMotorPeakCurrent;
    static const BRUUInt16Cmd sc_CmdMotorContinuousCurrent;
    static const BRUUInt16Cmd sc_CmdTorqueConstantKt;
    static const BRUUInt16Cmd sc_CmdRotorIntertiaJm;
    static const BRUUInt16Cmd sc_CmdBackEMFConstantKe;
    static const BRUUInt16Cmd sc_CmdWindingResistance;
    static const BRUUInt16Cmd sc_CmdWindingInductance;
    static const BRUUInt8Cmd sc_CmdThermostatFlag;
    static const BRUUInt8Cmd sc_CmdCommutationType;
    static const BRUUInt8Cmd sc_CmdThermalTimeConstantEnable;
    static const BRUUInt16Cmd sc_CmdThermalTimeConstant;
    static const BRUUInt8Cmd sc_CmdPoleCount;
    static const BRUUInt16Cmd sc_CmdHallOffset;
    static const BRUUInt16Cmd sc_CmdIndexOffset;
    static const BRUUInt8Cmd sc_CmdMotorForwardDirectionFlag;
    static const BRUUInt8ArrayCmd sc_CmdMotorTableInformation;
    static const BRUUInt16ArrayCmd sc_CmdMotorTableRecordSize;
    static const BRUUInt8ArrayCmd sc_CmdMotorTableVersion;

    // Digital I/O Commands -- Page 57
    static const BRUIndex8UInt16Cmd sc_CmdDigitalInputConfigurationRegister;
    static const BRUIndex8UInt16Cmd sc_CmdDigitalOutputConfigurationRegister;
    static const BRUUInt8Cmd sc_CmdDigitalOutputOverrideEnable;
    static const BRUUInt16Cmd sc_CmdDigitalOutputOverrideWriteMask;
    static const BRUInt16Cmd sc_CmdBRAKEOutputActiveDelay;
    static const BRUInt16Cmd sc_CmdBRAKEOutputInactiveDelay;
    static const BRUIndex8UInt32Cmd sc_CmdExpandedDigitalOutputConfigurationRegister;
    static const BRUUInt16Cmd sc_CmdFaultResetInputConfigurationRegister;

    // Analog I/O Commands -- Page 66
    static const BRUInt16Cmd sc_CmdCOMMANDInputVelocityOffset;
    static const BRUInt16Cmd sc_CmdCOMMANDInputVelocityScale;
    static const BRUInt16Cmd sc_CmdCOMMANDInputTorqueOffset;
    static const BRUInt16Cmd sc_CmdCOMMANDInputTorqueScale;
    static const BRUInt16Cmd sc_CmdCOMMANDInputPositionOffset;
    static const BRUInt16Cmd sc_CmdCOMMANDInputPositionScale;
    static const BRUIndex8UInt32Cmd sc_CmdAnalogOutputConfigurationRegister;
    static const BRUIndex8Int16Cmd sc_CmdAnalogOutputOffset;
    static const BRUIndex8Int16Cmd sc_CmdAnalogOutputScale;
    static const BRUUInt8Cmd sc_CmdAnalogOutputOverrideEnable;
    static const BRUIndex8Int16Cmd sc_CmdAnalogOutputOverrideWriteValue;

    // Serial Port Commands -- Page 76
    static const BRUUInt8Cmd sc_CmdSerialPortBaudRate;
    static const BRUUInt8Cmd sc_CmdSerialPortFrameFormat;
    static const BRUUInt8Cmd sc_CmdDriveID;

    // Operating Mode Commands -- Page 79
    static const BRUUInt8Cmd sc_CmdEncoderOutputConfigurationRegister;
    static const BRUUInt8Cmd sc_CmdCommandSource;
    static const BRUUInt8Cmd sc_CmdDriveMode;
    static const BRUIndex8Int32Cmd sc_CmdVelocityPreset;
    static const BRUIndex8Int16Cmd sc_CmdTorquePreset;
    static const BRUUInt8Cmd sc_CmdAnalogInputAccelerationLimitsEnable;
    static const BRUUInt32Cmd sc_CmdAnalogInputAccelerationLimit;
    static const BRUUInt32Cmd sc_CmdAnalogInputDecelerationLimit;
    static const BRUUInt8Cmd sc_CmdPresetAccelerationLimitsEnable;
    static const BRUUInt32Cmd sc_CmdPresetAccelerationLimit;
    static const BRUUInt32Cmd sc_CmdPresetDecelerationLimit;
    static const BRUUInt8Cmd sc_CmdOperatingMode;
    static const BRUUInt16Cmd sc_CmdOperatingModeStatus;
    static const BRUUInt16Cmd sc_CmdAutoTuneMaximumCurrent;
    static const BRUUInt32Cmd sc_CmdAutoTuneMaximumDistance;
    static const BRUUInt16Cmd sc_CmdManualTunePositionPeriod;
    static const BRUUInt16Cmd sc_CmdManualTunePositionStep;
    static const BRUUInt16Cmd sc_CmdManualTuneVelocityPeriod;
    static const BRUUInt32Cmd sc_CmdManualTuneVelocityStep;
    static const BRUUInt8Cmd sc_CmdTuningDirectionFlag;
    static const BRUInt16Cmd sc_CmdEncoderAlignmentOffset;
    static const BRUEmptyCmd sc_CmdRemoveAlignmentOffset;
    static const BRUUInt16Cmd sc_CmdMotorEncoderResolution;
    static const BRUUInt16Cmd sc_CmdMasterEncoderResolution;
    static const BRUUInt16Cmd sc_CmdMotorIndexPosition;
    static const BRUUInt16Cmd sc_CmdMasterIndexPosition;
    static const BRUUInt8Cmd sc_CmdChangeDirectionFlag;
    static const BRUUInt8Cmd sc_CmdOverrideCommandSource;
    static const BRUUInt8Cmd sc_CmdOverrideDriveMode;
    static const BRUEmptyCmd sc_CmdDefineHomePosition;
    static const BRUUInt8Cmd sc_CmdHostIndexingControlEnableFlag;
    static const BRUUInt8Cmd sc_CmdHostIndex;
    static const BRUEmptyCmd sc_CmdStartIndex;
    static const BRUUInt8Cmd sc_CmdAutoStartIndexingFlag;
    static const BRUIndex8UInt8Cmd sc_CmdIndexType;
    static const BRUIndex8Int32Cmd sc_CmdIndexDistanceAndPosition;
    static const BRUIndex8UInt32Cmd sc_CmdRegistrationDistance;
    static const BRUIndex8UInt32Cmd sc_CmdIndexVelocity;
    static const BRUIndex8UInt32Cmd sc_CmdIndexAcceleration;
    static const BRUIndex8UInt32Cmd sc_CmdIndexDeceleration;
    static const BRUIndex8UInt16Cmd sc_CmdIndexDwell;
    static const BRUIndex8UInt16Cmd sc_CmdIndexBatchCount;
    static const BRUEmptyCmd sc_CmdStartHoming;
    static const BRUUInt8Cmd sc_CmdAutoStartHomingFlag;
    static const BRUUInt8Cmd sc_CmdHomingType;
    static const BRUInt32Cmd sc_CmdHomingVelocity;
    static const BRUUInt32Cmd sc_CmdHomingAcceleration;
    static const BRUInt32Cmd sc_CmdHomingOffsetMoveDistance;
    static const BRUInt32Cmd sc_CmdHomePosition;
    static const BRUIndex8UInt8Cmd sc_CmdIndexTermination;
    static const BRUIndex8UInt8Cmd sc_CmdIndexPointer;
    static const BRUUInt8Cmd sc_CmdHomeSensorBackOffFlag;

    // Runtime Command and Control -- Page 124
    static const BRUEmptyCmd sc_CmdResetDrive;
    static const BRUUInt8Cmd sc_CmdHostDriveEnableDisable;
    static const BRUInt16Cmd sc_CmdTorqueSetpoint;
    static const BRUInt32Cmd sc_CmdVelocitySetpoint;
    static const BRUUInt32Cmd sc_CmdSetpointAcceleration;
    static const BRUUInt8Cmd sc_CmdSetpointControlEnableFlag;
    static const BRUEmptyCmd sc_CmdResetFaults;

    // Runtime Status Commands -- Page 129
    static const BRUUInt32Cmd sc_CmdDriveStatus;
    static const BRUUInt32Cmd sc_CmdFaultStatus;
    static const BRUUInt8Cmd sc_CmdRunState;
    static const BRUUInt16Cmd sc_CmdDigitalInputStates;
    static const BRUUInt16Cmd sc_CmdDigitalOutputStates;
    static const BRUUInt32Cmd sc_CmdExpandedOutputStatus;

    // Runtime Command and Control -- Page 124    // Runtime Data Commands -- Page 133
    static const BRUEmptyCmd sc_CmdResetPeaks;
    static const BRUInt16Cmd sc_CmdCOMMANDInput;
    static const BRUInt16Cmd sc_CmdPositiveCurrentLimitInput;
    static const BRUInt16Cmd sc_CmdNegativeCurrentLimitInput;
    static const BRUIndex8Int16Cmd sc_CmdAnalogOutput;
    static const BRUInt32Cmd sc_CmdMotorPosition;
    static const BRUInt32Cmd sc_CmdMasterPosition;
    static const BRUInt32Cmd sc_CmdPositionCommand;
    static const BRUInt32Cmd sc_CmdPositionError;
    static const BRUUInt32Cmd sc_CmdPositionPositivePeakError;
    static const BRUUInt32Cmd sc_CmdPositionNegativePeakError;
    static const BRUInt32Cmd sc_CmdVelocityCommand;
    static const BRUInt32Cmd sc_CmdMotorVelocity;
    static const BRUInt32Cmd sc_CmdVelocityError;
    static const BRUInt16Cmd sc_CmdCurrentCommand;
    static const BRUInt16Cmd sc_CmdAverageCurrent;
    static const BRUInt16Cmd sc_CmdCurrentPositivePeak;
    static const BRUInt16Cmd sc_CmdCurrentNegativePeak;
    static const BRUUInt16Cmd sc_CmdBusVoltage;
    static const BRUInt16Cmd sc_CmdFieldCurrent;
    static const BRUInt16Cmd sc_CmdTorqueCurrent;
    static const BRUInt16Cmd sc_CmdRPhaseCurrent;
    static const BRUInt16Cmd sc_CmdTPhaseCurrent;
    static const BRUInt16Cmd sc_CmdFieldVoltageCommand;
    static const BRUInt16Cmd sc_CmdTorqueVoltageCommand;
    static const BRUUInt16Cmd sc_CmdAverageMotorCurrent;
    static const BRUUInt8Cmd sc_CmdIndexNumber;
    static const BRUUInt16Cmd sc_CmdIndexCount;

    // RunTimeDataCollectionCommands -- Page 151
    static const BRUUInt8Cmd sc_CmdChannel1DataCollectionSource;
    static const BRUUInt8Cmd sc_CmdChannel2DataCollectionSource;
    static const BRUUInt8Cmd sc_CmdDataCollectionTriggerSource;
    static const BRUUInt16Cmd sc_CmdDataCollectionTimebase;
    static const BRUUInt8Cmd sc_CmdDataCollectionTriggerMode;
    static const BRUInt16Cmd sc_CmdDataCollectionTriggerThreshold;
    static const BRUEmptyCmd sc_CmdArmDataCollectionTriggering;
    static const BRUUInt8Cmd sc_CmdDataCollectionTriggerStatus;
    static const BRUIndex8Int16ArrayCmd sc_CmdCollectedData;    
  };

  class TenMeterAPI: public ScopeAPI, public ScopeProtocolServer
  {
  public:
    TenMeterAPI(DataStream* ds, bool emulator=false, int loud=0) /*: 
      m_data_stream(ds), m_emulator(emulator), m_loud(loud),
      m_server(0), m_max_az_vel(0.3), m_max_el_vel(0.3) */ { }

    TenMeterAPI(ScopeAPI* server, DataStream* ds, 
		long to_sec=0, long to_usec=0, 
		bool emulator=false, int loud=0) /*: 
      m_data_stream(ds), m_emulator(emulator), m_loud(loud),
      m_server(server), m_max_az_vel(0.3), m_max_el_vel(0.3) */{ }

    virtual ~TenMeterAPI();

    // ScopeAPI members
    virtual void reqStat(PositionerStatus& state);
    virtual void cmdStandby();
    virtual void cmdPoint(const SEphem::Angle& az_angle, double az_vel,
			  const SEphem::Angle& el_angle, double el_vel);
    virtual void cmdSlew(double az_vel, double el_vel);

    virtual void reqOffsets(SEphem::Angle& az_angle,
			    SEphem::Angle& el_angle);
    virtual void setOffsets(const SEphem::Angle& az_angle,
			    const SEphem::Angle& el_angle);

    virtual void reqPIDParameters(PIDParameters& az_param,
				  PIDParameters& el_param);
    virtual void setPIDParameters(const PIDParameters& az_param,
				  const PIDParameters& el_param);

    virtual void resetCommunication();

    // ScopeProtocolServer members
    virtual void processOneCommand(long cmd_to_sec=0, long cmd_to_usec=0);

#if 0
  private:
    DataStream*  m_data_stream;
    unsigned     m_com_tries;

    ScopeAPI*    m_server;

    double       m_max_az_vel;
    double       m_max_el_vel;
#endif
  }; // class TenMeterAPI
  

  // **************************************************************************
  // SimpleAtomCODEC
  // **************************************************************************
    
  template<class T> 
  inline size_t 
  BRUDrive::SimpleAtomCODEC<T>::length()
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    throw(VMessaging::Exception("Internal error",
				"Unspecialized SimpleAtomicCODEC called",""));
    return 0;
  }

  template<class T>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<T>::encode(const T& atom)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    throw(VMessaging::Exception("Internal error",
				"Unspecialized SimpleAtomicCODEC called",""));
    return std::string();
  }

  template<class T>
  inline T
  BRUDrive::SimpleAtomCODEC<T>::decode(const std::string& payload)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    throw(VMessaging::Exception("Internal error",
				"Unspecialized SimpleAtomicCODEC called",""));
    return T();
  }

  template<class T>
  inline std::string
  BRUDrive::SimpleAtomCODEC<T>::typeAsString()
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    throw(VMessaging::Exception("Internal error",
				"Unspecialized SimpleAtomicCODEC called",""));
    return std::string("UnSpecialized");
  }

  // **************************************************************************
  // SimpleAtomCODEC -- UINT 8
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<uint8_t>::length()
  {
    return sizeof(uint8_t)*2;
  }
  
  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<uint8_t>::typeAsString()
  {
    return std::string("uint8_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<uint8_t>::encode(const uint8_t& atom)
  {
    std::ostringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(uint8_t)*2) << uint16_t(atom);
    return stream.str();
  }

  template<>
  inline uint8_t
  BRUDrive::SimpleAtomCODEC<uint8_t>::decode(const std::string& payload)
  {
    if(payload.length()!=sizeof(uint8_t)*2)
      {
	VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
	std::ostringstream msg;
	msg << "Datum: >" << payload << "<\nRequired length: " << sizeof(uint8_t)*2;
	throw(ProtocolError("Datum did not have expected length",msg.str()));
      }
    uint16_t big_atom;
    std::istringstream stream(payload);
    stream >> std::hex >> std::uppercase >> std::setw(sizeof(uint8_t)*2) >> big_atom;    
    return big_atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- INT 8
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<int8_t>::length()
  {
    return SimpleAtomCODEC<uint8_t>::length();    
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<int8_t>::typeAsString()
  {
    return std::string("int8_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<int8_t>::encode(const int8_t& atom)
  {
    uint8_t aa = *reinterpret_cast<const uint8_t*>(&atom);
    return SimpleAtomCODEC<uint8_t>::encode(aa);
  }

  template<>
  inline int8_t
  BRUDrive::SimpleAtomCODEC<int8_t>::decode(const std::string& payload)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    uint8_t aa = SimpleAtomCODEC<uint8_t>::decode(payload);
    int8_t atom = *reinterpret_cast<const int8_t*>(&aa);
    return atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- UINT 16
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<uint16_t>::length()
  {
    return sizeof(uint16_t)*2;
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<uint16_t>::typeAsString()
  {
    return std::string("uint16_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<uint16_t>::encode(const uint16_t& atom)
  {
    std::ostringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(uint16_t)*2) << atom;
    return stream.str();
  }

  template<>
  inline uint16_t
  BRUDrive::SimpleAtomCODEC<uint16_t>::decode(const std::string& payload)
  {
    if(payload.length()!=sizeof(uint16_t)*2)
      {
	VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
	std::ostringstream msg;
	msg << "Datum: >" << payload << "<\nRequired length: " << sizeof(uint16_t)*2;
	throw(ProtocolError("Datum did not have expected length",msg.str()));
      }
    uint16_t atom;
    std::istringstream stream(payload);
    stream >> std::hex >> std::uppercase >> std::setw(sizeof(uint16_t)*2) >> atom;
    return atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- INT 16
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<int16_t>::length()
  {
    return SimpleAtomCODEC<uint16_t>::length();
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<int16_t>::typeAsString()
  {
    return std::string("int16_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<int16_t>::encode(const int16_t& atom)
  {
    uint16_t aa = *reinterpret_cast<const uint16_t*>(&atom);
    return SimpleAtomCODEC<uint16_t>::encode(aa);
  }

  template<>
  inline int16_t
  BRUDrive::SimpleAtomCODEC<int16_t>::decode(const std::string& payload)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    uint16_t aa = SimpleAtomCODEC<uint16_t>::decode(payload);
    int16_t atom = *reinterpret_cast<const int16_t*>(&aa);
    return atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- UINT 32
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<uint32_t>::length()
  {
    return sizeof(uint32_t)*2;
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<uint32_t>::typeAsString()
  {
    return std::string("uint32_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<uint32_t>::encode(const uint32_t& atom)
  {
    std::ostringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(uint32_t)*2) << atom;
    return stream.str();
  }

  template<>
  inline uint32_t
  BRUDrive::SimpleAtomCODEC<uint32_t>::decode(const std::string& payload)
  {
    if(payload.length()!=sizeof(uint32_t)*2)
      {
	VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
	std::ostringstream msg;
	msg << "Datum: >" << payload << "<\nRequired length: " << sizeof(uint32_t)*2;
	throw(ProtocolError("Datum did not have expected length",msg.str()));
      }
    uint32_t atom;
    std::istringstream stream(payload);
    stream >> std::hex >> std::uppercase >> std::setw(sizeof(uint32_t)*2) >> atom;
    return atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- INT 32
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<int32_t>::length()
  {
    return SimpleAtomCODEC<uint32_t>::length();
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<int32_t>::typeAsString()
  {
    return std::string("int32_t");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<int32_t>::encode(const int32_t& atom)
  {
    uint32_t aa = *reinterpret_cast<const uint32_t*>(&atom);
    return SimpleAtomCODEC<uint32_t>::encode(aa);
  }

  template<>
  inline int32_t
  BRUDrive::SimpleAtomCODEC<int32_t>::decode(const std::string& payload)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    uint32_t aa = SimpleAtomCODEC<uint32_t>::decode(payload);
    int32_t atom = *reinterpret_cast<const int32_t*>(&aa);
    return atom;
  }

  // **************************************************************************
  // SimpleAtomCODEC -- FLOAT 32
  // **************************************************************************

  template<>
  inline size_t 
  BRUDrive::SimpleAtomCODEC<float>::length()
  {
    return SimpleAtomCODEC<uint32_t>::length();
  }

  template<>
  inline std::string
  BRUDrive::SimpleAtomCODEC<float>::typeAsString()
  {
    return std::string("float");
  }

  template<>
  inline std::string 
  BRUDrive::SimpleAtomCODEC<float>::encode(const float& atom)
  {
    uint32_t aa = *reinterpret_cast<const uint32_t*>(&atom);
    return SimpleAtomCODEC<uint32_t>::encode(aa);
  }

  template<>
  inline float
  BRUDrive::SimpleAtomCODEC<float>::decode(const std::string& payload)
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    uint32_t aa = SimpleAtomCODEC<uint32_t>::decode(payload);
    float atom = *reinterpret_cast<const float*>(&aa);
    return atom;
  }

  // **************************************************************************
  // BRUAtomCmd
  // **************************************************************************
  
  template<class T> 
  BRUDrive::BRUAtomCmd<T>::~BRUAtomCmd()
  {
    //nothing to see here
  }

  template<class T> 
  T 
  BRUDrive::BRUAtomCmd<T>::read(DataStream* ds, int address) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string payload;
    doRead(payload,ds,address,SimpleAtomCODEC<T>::length());
    return SimpleAtomCODEC<T>::decode(payload);
  }
  
  template<class T> 
  void 
  BRUDrive::BRUAtomCmd<T>::write(DataStream* ds, int address, const T& data) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string payload;
    payload = SimpleAtomCODEC<T>::encode(data);
    doWrite(ds,address,payload);
  }

  template<class T> 
  std::string 
  BRUDrive::BRUAtomCmd<T>::commandTypeAsString() const
  {
    std::ostringstream stream;
    stream << "BRUAtomCmd<" << SimpleAtomCODEC<T>::typeAsString() << '>';
    return stream.str();
  }

  // **************************************************************************
  // BRUAtomArrayCmd
  // **************************************************************************
  
  template<class T> 
  BRUDrive::BRUAtomArrayCmd<T>::~BRUAtomArrayCmd()
  {
    //nothing to see here
  }

  template<class T> 
  std::vector<T>
  BRUDrive::BRUAtomArrayCmd<T>::read(DataStream* ds, int address) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string payload;
    doRead(payload,ds,address,SimpleAtomCODEC<T>::length()*m_array_len);
    std::vector<T> data;
    for(unsigned i=0; i<m_array_len; i++)
      data.push_back(SimpleAtomCODEC<T>::decode(payload.substr(i*SimpleAtomCODEC<T>::length(),
							       SimpleAtomCODEC<T>::length())));
    return data;
  }
  
  template<class T> 
  void 
  BRUDrive::BRUAtomArrayCmd<T>::write(DataStream* ds, int address, 
				      const std::vector<T>& data) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    if(data.size() != m_array_len)
      {
	std::ostringstream msg;
	msg << "Elements received: " << data.size() << std::endl 
	    << "Elements expected: " << m_array_len << std::endl;
	throw(VMessaging::Exception("Internal error",
				    "Number of array elements did not match expected",msg.str()));
      }
    std::string payload;
    for(unsigned i=0; i<m_array_len; i++)
      payload += SimpleAtomCODEC<T>::encode(data[i]);
    doWrite(ds,address,payload);
  }
   
  template<class T> 
  std::string 
  BRUDrive::BRUAtomArrayCmd<T>::commandTypeAsString() const
  {
    std::ostringstream stream;
    stream << "BRUAtomArrayCmd<" << SimpleAtomCODEC<T>::typeAsString() << ',' 
	   << m_array_len << '>';
    return stream.str();
  }

  // **************************************************************************
  // BRUIndex8AtomCmd
  // **************************************************************************

  template<class T> 
  BRUDrive::BRUIndex8AtomCmd<T>::~BRUIndex8AtomCmd()
  {
    //nothing to see here
  }

  template<class T> 
  T 
  BRUDrive::BRUIndex8AtomCmd<T>::read(DataStream* ds, int address, 
				      uint8_t index) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string in_payload;
    std::string payload;
    in_payload = SimpleAtomCODEC<uint8_t>::encode(index);
    doRead(payload,ds,address,SimpleAtomCODEC<T>::length(),in_payload);
    return SimpleAtomCODEC<T>::decode(payload);
  }

  template<class T> 
  void 
  BRUDrive::BRUIndex8AtomCmd<T>::write(DataStream* ds, int address, 
				       uint8_t index, const T& data) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string payload;
    payload = SimpleAtomCODEC<uint8_t>::encode(index)+SimpleAtomCODEC<T>::encode(data);
    doWrite(ds,address,payload);
  }

  template<class T> 
  std::string 
  BRUDrive::BRUIndex8AtomCmd<T>::commandTypeAsString() const
  {
    std::ostringstream stream;
    stream << "BRUIndex8AtomCmd<" << SimpleAtomCODEC<T>::typeAsString() << '>';
    return stream.str();
  }

  // **************************************************************************
  // BRUIndex8AtomArrayCmd
  // **************************************************************************

  template<class T> 
  BRUDrive::BRUIndex8AtomArrayCmd<T>::~BRUIndex8AtomArrayCmd()
  {
    //nothing to see here
  }

  template<class T> 
  std::vector<T>   
  BRUDrive::BRUIndex8AtomArrayCmd<T>::read(DataStream* ds, int address, 
					   uint8_t index) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    std::string in_payload;
    std::string payload;
    in_payload = SimpleAtomCODEC<uint8_t>::encode(index);
    doRead(payload,ds,address,SimpleAtomCODEC<T>::length()*m_array_len,in_payload);
    std::vector<T> data;
    for(unsigned i=0; i<m_array_len; i++)
      data.push_back(SimpleAtomCODEC<T>::decode(payload.substr(i*SimpleAtomCODEC<T>::length(),
							       SimpleAtomCODEC<T>::length())));
    return data;
  }

  template<class T> 
  void 
  BRUDrive::BRUIndex8AtomArrayCmd<T>::write(DataStream* ds, int address, 
					    uint8_t index, const std::vector<T>& data) const
  {
    VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
    if(data.size() != m_array_len)
      {
	std::ostringstream msg;
	msg << "Elements received: " << data.size() << std::endl 
	    << "Elements expected: " << m_array_len << std::endl;
	throw(VMessaging::Exception("Internal error",
				    "Number of array elements did not match expected",msg.str()));
      }
    std::string payload;
    payload = SimpleAtomCODEC<uint8_t>::encode(index);
    for(unsigned i=0; i<m_array_len; i++)
      payload += SimpleAtomCODEC<T>::encode(data[i]);
    doWrite(ds,address,payload);
  }

  template<class T> 
  std::string 
  BRUDrive::BRUIndex8AtomArrayCmd<T>::commandTypeAsString() const
  {
    std::ostringstream stream;
    stream << "BRUIndex8AtomArrayCmd<" << SimpleAtomCODEC<T>::typeAsString() << ',' 
	   << m_array_len << '>';
    return stream.str();
  }

}; // namespace VTracking

#endif // VTRACKING_TENMETER_H
