#pragma once

#include "CDevice.h"

class CEthernetControllerFirefly;

class CDeviceSequencer : public CDevice
{
public:
	unsigned int id;
	std::string type;
	std::string ip;
	unsigned int port;
	bool master;
	bool IsMaster() { return master; }
	unsigned int startDelay;
	double clockFrequency;
	unsigned int FPGAClockToBusClockRatio;
	bool useExternalClock;
	bool useStrobeGenerator;
	bool connect;
	double BusFrequency;
	CEthernetControllerFirefly* MyEthernetMultiIOControllerFirefly;
private:
	unsigned long PCBufferSize_in_u64;
	static const uint8_t MaxBuffer = 2;
	uint8_t currentBuffer;
	uint32_t* Buffer[MaxBuffer];
	uint64_t AbsoluteTime_in_ms;
	uint32_t BufferPosition;
	double Delay_in_ms;
	double TimeDebt_in_ms; 
	double MaxTimeDebt_in_ms;
	double CurrentTimeDebt_in_ms;
	bool LastCommandWasSpecialCommand;
	uint32_t LastBusData;
public:
	static const unsigned int MaxParallelBusDevices = 8 * 256 + 1;
	static const unsigned int MaxSerialBusDevices = 8;
	CDevice* ParallelBusDeviceList[MaxParallelBusDevices];
	CDevice* SerialBusDeviceList[MaxSerialBusDevices];
	CDeviceSequencer(
		unsigned int _id,
		std::string _type,
		std::string _ip,
		unsigned int _port,
		bool _master,
		unsigned int _startDelay,
		double _clockFrequency,
		unsigned int _FPGAClockToBusClockRatio,
		bool _useExternalClock,
		bool _useStrobeGenerator,
		bool _connect);
	virtual ~CDeviceSequencer();
	void Initialize(unsigned long _PCBufferSize_in_u64);
	void SwitchDebugMode(bool OnOff, const std::string &FileName);
	bool IsSequencerConnected();
	void StartAssemblingSequence();
	void StartAssemblingNextSequence();
	void SendSequence();
	void GetBufferLength(uint32_t& FilledBufferLength, uint32_t& MaxBufferLength);
	void SendStartSequenceCommand();
	bool IsSequenceRunning(bool& running, unsigned long long& DataPointsWritten);
	bool WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned long& EndTimeOfCycle, double timeout_in_s);
	void AddCommandToSequence(const uint32_t& high_word, const uint32_t& low_word);
	void WriteBusAddressAndDataToBuffer(const uint16_t& MultiIOAddress, const uint16_t& Data);
	void AddBusCommandAndWait(uint32_t data, uint32_t delay);
	void AddBusCommandToSequence(const uint32_t& content);
	bool SetValue_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	bool SetRegister_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	bool SetValueSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	bool SetRegisterSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);

	bool Wait_ms(double time_in_ms);
	double GetTime_ms();
	double GetTimeDebt_ms() { return TimeDebt_in_ms; };
	unsigned long GetNextBufferPosition() { return BufferPosition; }
	void StartAssemblingCPUCommandSequence();
	void AddCPUCommand(const char* command);
	void ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms);
	void StopCPUCommandSequence();
	void InterruptCPUCommandSequence();
	void GetCPUCommandErrorMessages();
	void PrintCPUCommandErrorMessages();
	void PrintCPUCommandSequence();
public:
	//the following are commands to control the sequencer itself
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
public:
	//the following commands are intended to be used by devices connected to the sequencer, such as analog input cards.
	void SequencerStartAnalogInAcquisition(const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
	void SequencerWriteInputMemory(unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
	void SequencerWriteSystemTimeToInputMemory();
	void SequencerCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift);
	void SequencerSwitchDebugLED(unsigned int OnOff);
	void SequencerIgnoreTCPIP(bool OnOff);
	void SequencerAddMarker(unsigned char marker);
	void SequencerSetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms) { this->MaxTimeDebt_in_ms = MaxTimeDebt_in_ms; }
	void SequencerSetLoopCount(unsigned int loop_count);
	void SequencerJumpBackward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool loop_count_greater_zero = false);
	void SequencerJumpForward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false);

public:
	//the following functions are used by CControlAPI to find the desired device. Used for convenience functions.
	CDevice* GetParallelBusDevice(const unsigned int& Address) { 
		if (Address > MaxParallelBusDevices) return nullptr; 
		if (ParallelBusDeviceList[Address] == reinterpret_cast<CDeviceSequencer*>(1)) return nullptr; 
		return ParallelBusDeviceList[Address]; 
	}
	CDevice* GetSerialBusDevice(const unsigned int& Address) { 
		if (Address > MaxSerialBusDevices) return nullptr; 
		if (SerialBusDeviceList[Address] == reinterpret_cast<CDeviceSequencer*>(1)) return nullptr; 
		return SerialBusDeviceList[Address]; 
	}
private:
	void AdvanceTime();
};