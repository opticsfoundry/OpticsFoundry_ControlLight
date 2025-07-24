#pragma once

#include "NetworkClient.h"	// Added by ClassView
#include "EthernetControllerFirefly.h"
#include <functional>



#pragma once

typedef function<bool(void)> tBoolFunction;

class CDeviceSequencer;

class CEthernetControllerFirefly : public CNetworkClient
{
public:
	unsigned long SequencerCommandListSize;
	uint32_t* SequencerCommandList;
	unsigned int MyMultiIO;
public:
	bool Connected;
	bool DebugBufferOn;
	ofstream* DebugBufferFile;
	bool ExternalTrigger0;
	bool ExternalTrigger1;
	double PeriodicTriggerPeriod_in_s;
	double PeriodicTriggerAllowedWait_in_s;
	bool ExternalClock0;
	bool ExternalClock1;
	//double BusFrequency;
	unsigned int FPGAClockToBusClockRatio;
	double FPGAClockFrequencyInHz;
	bool FPGAUseExternalClock;
	bool FPGAUseStrobeGenerator;
	unsigned  long StartTickCounts;
	//uint32_t* FPGABuffer;
	//uint32_t* FPGAAbsoluteTime;
	//uint32_t FPGABufferUsed;
	//COutput* myOutput;
	double LastPeriodicTriggerPeriod_in_s;
	bool DebugModeOn;
private:
	bool core_option_LED; 
	uint8_t core_option_SPI_CS;
	uint8_t core_option_dig_out;  
	uint8_t core_option_PL_to_PS;
	bool SetPeriodicTriggerAtBeginningOfNextSequence;
	bool WaitForPeriodicTriggerAtBeginningOfSequence;
	uint32_t* previous_command_buffer;
	unsigned long previous_command_buffer_length;
	CDeviceSequencer* MySequencer;
	uint8_t* previous_command_buffer_ptr;
	std::string DebugFilename;
public:
	void StartXADCAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
private:
	void StartSPIAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	void AddCommandAnalogInOut(uint8_t adc_register_address, uint8_t adc_write_enable, unsigned __int16 adc_programming_out, uint8_t dont_execute_now, uint8_t only_read_write, uint32_t wait_time);
	void AddCommandSetCoreOption_LED(bool a_core_option_LED);
	void AddCommandSetCoreOption_SPI_CS(uint8_t a_core_option_SPI_CS);
	void AddCommandSetCoreOption_dig_out(uint8_t a_core_option_dig_out);
	void AddCommandSetCoreOption_PL_to_PS(uint8_t a_core_option_PL_to_PS);
	void AddCommandSetCoreOptions();
	void AddCommandWait(unsigned long Wait_in_FPGA_clock_cycles);
	void AddCommandSetPLToPSCommand(unsigned int PLToPSCommand);
	bool DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible;
	bool AttemptNetworkCommand(tBoolFunction fCommand);
public:
	void AddCommandStop();
public:	
	CEthernetControllerFirefly(CDeviceSequencer* _MySequencer);
	virtual ~CEthernetControllerFirefly();
	bool SendSequenceToFPGA(uint32_t* buffer);
	void AddSequencerCommandToSequenceList(uint32_t high_buffer, uint32_t low_buffer);
	void StartAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapooints, double delay_between_datapoints);
	void WriteReadSPI(unsigned int chip_select, unsigned int number_of_bits_out, unsigned __int64 data_high, unsigned __int64 data_low, unsigned int number_of_bits_in);
	//bool AddData(uint32_t* BusData, uint32_t* Spacing, /*uint32_t* AbsoluteTime,*/ unsigned long Count);
	bool AddSequencePreamble();
	bool GetAktWaveformPoint(unsigned long long& DataPointsWritten, bool &running);
	bool GetNextCycleNumber(long& NextCycleNumber);
	bool ResetCycleNumber();
	bool CheckReady(double timeout_in_s = 1);
	bool Reset();
	bool WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s = 10);
	void AddProgramLineToSequenceList(uint8_t command, uint32_t data, uint32_t delay);
	void AddProgramLine( uint8_t command, uint32_t data, uint32_t delay);
	//void AddCommandStep(uint32_t data, uint32_t delay);
	void SetStrobeOptions( uint8_t strobe_choice, uint8_t strobe_low_length, uint8_t strobe_high_length);
	void SetTriggerOptions( bool ExternalTrigger0, bool ExternalTrigger1);
	void AddExternalTrigger( bool ExternalTrigger0, bool ExternalTrigger1, bool FPGASoftwareTrigger );
	void WriteBufferToFile(uint32_t* buffer, unsigned long length);
	bool ConnectSocket(LPCTSTR lpszAddress, UINT port, unsigned int aFPGAClockToBusClockRatio, double aFPGAClockFrequencyInHz, bool aFPGAUseExternalClock, bool aFPGAUseStrobeGenerator, bool ExternalTrigger);
	double GetBusFrequency();
	bool WaitTillFinished();
	bool Start();
	bool Stop();
	bool CloseConnection();
	//void ResetProgramBuffer() { FPGABufferUsed = 0; }
	bool SetFrequency(double Frequency);
	bool GetFrequency(double& Frequency);
	bool GetPeriodicTriggerError(bool& Error);
	void SetExternalTrigger(bool aExternalTrigger0, bool aExternalTrigger1);
	void SetPeriodicTrigger(double aPeriodicTriggerPeriod_in_s, double aPeriodicTriggerAllowedWaitTime_in_s);
	void WaitForPeriodicTrigger(bool aWaitForPeriodicTriggerAtBeginningOfSequence);
	bool SetExternalClock(bool ExternalClock0, bool ExternalClock1);
	void DebugBuffer(CString filename);
	void AddSequencerCommandToBuffer(uint32_t* buffer, uint32_t n, uint32_t high_buffer, uint32_t low_buffer);
	void ClearSequencerCommandList();
	void AddSequencerCommand(uint32_t high_word, uint32_t low_word);
	bool SwitchDebugMode(bool OnOff, const std::string& aFilename);
	void SwitchDebugLED(bool OnOff);
	void IgnoreTCPIP(bool OnOff);
	void AddMarker(uint8_t Marker);
	void AddCommandWriteSystemTimeToInputMemory();
	void AddCommandCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift);
	void AddCommandWriteInputBuffer(unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
	void AddCommandSetLoopCount(unsigned int loop_count);
	void AddCommandJumpBackward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool loop_count_greater_zero = false);
	void AddCommandJumpForward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false);
	void TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool OnOff);
	double MeasureEthernetBandwidth(uint32_t DataSize = 1024 * 1024, double MinimumExpected = -1);
	bool OptimizedCommand(CString CommandString);
	bool AttemptGetAktWaveformPoint(unsigned long long& DataPointsWritten, bool& running);

	
	bool StartAssemblingCPUCommandSequence();
	bool AddCPUCommand(const char* command);
	bool ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms);
	bool StopCPUCommandSequence();
	bool InterruptCPUCommandSequence();
	bool GetCPUCommandErrorMessages();
	bool PrintCPUCommandErrorMessages();
	bool PrintCPUCommandSequence();


private:
	bool ModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]);
	bool AttemptModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]);
	bool SendSequence(uint32_t DataSize, uint32_t* buffer);
	bool AttemptSendSequence(uint32_t DataSize, uint32_t* buffer);
	bool AttemptGetNextCycleNumber(long& NextCycleNumber);
	bool AttemptSetFrequency(double Frequency);
	bool AttemptGetFrequency(double& Frequency);
	bool AttemptGetPeriodicTriggerError(bool& Error);
	bool AttemptWaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
};
