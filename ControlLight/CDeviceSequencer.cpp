#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "EthernetControllerFirefly.h"
#include "AD9852.h"
#include "AD9858.h"


using namespace std;
#include <format>
using namespace std;
#include <string>
using namespace std;
#include <sstream>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


CDeviceSequencer::CDeviceSequencer(
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
	bool _connect) {
	MySequencer = this;
	for (unsigned int i = 0; i < MaxParallelBusDevices; i++)
		ParallelBusDeviceList[i] = nullptr;
	for (unsigned int i = 0; i < MaxSerialBusDevices; i++)
		SerialBusDeviceList[i] = nullptr;
	//The Sequencer can also take a few configuration commands.
	//By registering it just as any other device on the bus, we can access those virtual configuration registers.
	ParallelBusDeviceList[MaxParallelBusDevices - 1] = this;
	MyAddress = MaxParallelBusDevices - 1;
	id = _id;
	type = _type;
	ip = _ip;
	port = _port;
	master = _master;
	startDelay = _startDelay;
	clockFrequency = _clockFrequency;
	FPGAClockToBusClockRatio = _FPGAClockToBusClockRatio;
	useExternalClock = _useExternalClock;
	useStrobeGenerator = _useStrobeGenerator;
	connect = _connect;
	currentBuffer = 0;
	for (int n = 0; n < MaxBuffer; n++) Buffer[n] = nullptr;
	LastCommandWasSpecialCommand = true;

	AbsoluteTime_in_ms = 0;
	BufferPosition = 0;
	BusFrequency = 2000000;
	// Initialize the device
	MyEthernetMultiIOControllerFirefly = new CEthernetControllerFirefly(this);
	bool success = MyEthernetMultiIOControllerFirefly->ConnectSocket(ip.c_str(), port, FPGAClockToBusClockRatio, clockFrequency, useExternalClock, useStrobeGenerator, /* ExternalTrigger*/ !master);
	if (!success) {
		NotifyError(_T("Failed to connect to sequencer"));
		BusFrequency = MyEthernetMultiIOControllerFirefly->GetBusFrequency();
	}

	//Sequencer0->SwitchDebugMode(On);
	//Timestamp.StartDebug(DebugFilePath + "TimingDebug.dat");
	//NI653xEthernet->Debug(DebugFilePath + "DebugNI653xEthernet.dat");
	//NI653xEthernet->DebugBuffer(DebugFilePath + "DebugNI653xEthernetBuffer.dat");
	MyEthernetMultiIOControllerFirefly->MeasureEthernetBandwidth(1024 * 128, 20);
}

CDeviceSequencer::~CDeviceSequencer() {
	//the -1 is needed because ParallelBusDeviceList[MaxParallelBusDevices-1] points to the sequencer itself
	for (unsigned int i = 0; i < MaxParallelBusDevices - 1; i++)
		if (ParallelBusDeviceList[i]) {
			//some devices use more than one address. Only delete them once.
			if (ParallelBusDeviceList[i] != reinterpret_cast<CDeviceSequencer*>(1)) {
				delete ParallelBusDeviceList[i];
				ParallelBusDeviceList[i] = nullptr;
			}
		}
	for (unsigned int i = 0; i < MaxSerialBusDevices; i++) {
		if (SerialBusDeviceList[i]) {
			if (SerialBusDeviceList[i] != reinterpret_cast<CDeviceSequencer*>(1)) {
				delete SerialBusDeviceList[i];
				SerialBusDeviceList[i] = nullptr;
			}
		}
	}
	for (int n = 0; n < MaxBuffer; n++) {
		if (Buffer[n]) delete Buffer[n];
		Buffer[n] = nullptr;
	}
	if (MyEthernetMultiIOControllerFirefly) {
		delete MyEthernetMultiIOControllerFirefly;
		MyEthernetMultiIOControllerFirefly = nullptr;
	}
}

void CDeviceSequencer::Initialize(unsigned long _PCBufferSize_in_u64) {
	PCBufferSize_in_u64 = _PCBufferSize_in_u64;
	for (int n = 0; n < MaxBuffer; n++) {
		if (Buffer[n]) delete Buffer[n];
		Buffer[n] = new uint32_t[2 * PCBufferSize_in_u64];
		for (uint32_t i = 0; i < 2 * PCBufferSize_in_u64; i++) {
			Buffer[n][i] = 0;
		}
	}
	currentBuffer = 0;
	BufferPosition = 0;
	AbsoluteTime_in_ms = 0;
	Delay_in_ms = 0;
	MaxTimeDebt_in_ms = 9999999999; //1ms
	TimeDebt_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;
	LastBusData = 0;
	LastCommandWasSpecialCommand = true;
}

bool CDeviceSequencer::IsSequencerConnected() {
	return MyEthernetMultiIOControllerFirefly->CheckReady();
}

void CDeviceSequencer::SwitchDebugMode(bool OnOff, const std::string& FileName) {
	MyEthernetMultiIOControllerFirefly->SwitchDebugMode(OnOff, FileName);
}

void CDeviceSequencer::StartAssemblingSequence() {
	//There can be multiple sequences in the buffer. This function starts the very first of these sequences.
	BufferPosition = 0;
	LastBusData = 0;
	LastCommandWasSpecialCommand = true;
	StartAssemblingNextSequence();
}

void CDeviceSequencer::StartAssemblingNextSequence() {
	//ToDo (optional): To be very precise we could add the small delay between the trigger and the first command.
	if (BufferPosition > 0) {
		//We need to finish the last sequence with a stop command to make sure it stops
		MyEthernetMultiIOControllerFirefly->AddCommandStop();
	}
	AbsoluteTime_in_ms = 0;
	Delay_in_ms = 0;
	TimeDebt_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;

	MyEthernetMultiIOControllerFirefly->AddSequencePreamble();
}

//Why does declaring it inline not work for this function? It would be useful to do so, as it's called very often from just one place.
void CDeviceSequencer::AddCommandToSequence(const uint32_t& high_word, const uint32_t& low_word) {
	if (!LastCommandWasSpecialCommand) {
		//this is a special command. We need to first send out LastBusData and then wait as needed before executing it.
		AddBusCommandToSequence(LastBusData);
	}
	if (BufferPosition >= PCBufferSize_in_u64) {
		NotifyError(_T("Buffer overflow")); //ToDo: throw exception
		return;
	}
	Buffer[currentBuffer][BufferPosition * 2] = low_word;
	Buffer[currentBuffer][BufferPosition * 2 + 1] = high_word;
	BufferPosition++;
	LastCommandWasSpecialCommand = true;
	//Most special commands take only 3 FPGA clock cycles.
	//ToDo: some commands can take more, e.g. the LongWait command. Would need table of commands with their wait time to calculate this better. For now: don't use those commands.
	double added_time = 3 / clockFrequency * 1000.0;
	CurrentTimeDebt_in_ms += added_time;
	TimeDebt_in_ms += added_time;
}

inline void CDeviceSequencer::AddBusCommandAndWait(uint32_t busdata, uint32_t delay) {
	//MyEthernetMultiIOControllerFirefly->AddCommandStep( busdata, delay);
	//this is the most often occuring command. For efficiency I program it here and don't go through MyEthernetMultiIOControllerFirefly.
	//this also makes sure that it's not confused with a special command.
	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	uint8_t command = 1; //CMD_STEP

	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & busdata) << 4) | ((delay >> 27) & delay_mask_high);

	if (BufferPosition >= PCBufferSize_in_u64 / 2) {
		AddErrorMessageCString(_T("Buffer overflow")); //ToDo: throw exception
		return;
	}
	Buffer[currentBuffer][BufferPosition * 2] = low_buffer;
	Buffer[currentBuffer][BufferPosition * 2 + 1] = high_buffer;
	BufferPosition++;
}

inline void CDeviceSequencer::AddBusCommandToSequence(const uint32_t& content) {
	if (BufferPosition >= PCBufferSize_in_u64) {
		AddErrorMessageCString(_T("Buffer overflow")); //ToDo: throw exception
		return;
	}
	uint32_t spacing = Delay_in_ms * clockFrequency / 1000.0;
	if (spacing == 0) {
		spacing = FPGAClockToBusClockRatio;
		double added_time = FPGAClockToBusClockRatio / clockFrequency * 1000.0;
		CurrentTimeDebt_in_ms += added_time;
		TimeDebt_in_ms += added_time;
	}
	AdvanceTime();
	if (LastCommandWasSpecialCommand) {
		//at the beginning of a special command, the LastBusData is written out and the remaining wait time is waited.
		//Therefore nothing to write out now.
		//We just put this bus data onto the todo list for next time.
		LastBusData = content;
	}
	else {
		//Challenge: each bus data command is accompanied by the wait time till the next command, after the bus was updated, not before.
		// This is the inverse logic from here.
		// Solution: we store the bus data that should be put out in LastBusData.
		// Then we here execute the last bus data command and the wait time till this one.
		// Finally we store the requested bus data update as to be done next time.
		AddBusCommandAndWait(LastBusData, spacing);
		LastBusData = content;
	}
	LastCommandWasSpecialCommand = false;
}

void CDeviceSequencer::WriteBusAddressAndDataToBuffer(const uint16_t& MultiIOAddress, const uint16_t& Data) {
	uint32_t content = MultiIOAddress << 16 | Data;
	AddBusCommandToSequence(content);
}

void CDeviceSequencer::GetBufferLength(uint32_t& FilledBufferLength, uint32_t& MaxBufferLength) {
	FilledBufferLength = BufferPosition;
	MaxBufferLength = PCBufferSize_in_u64;
}

void CDeviceSequencer::SendSequence() {
	AddBusCommandToSequence(0); //just in case there is still something in LastBusData.
	MyEthernetMultiIOControllerFirefly->SendSequenceToFPGA(Buffer[currentBuffer]);
	currentBuffer++;
	if (currentBuffer >= MaxBuffer) {
		currentBuffer = 0;
	}
}

void CDeviceSequencer::SendStartSequenceCommand() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->Start();
	}
}

bool CDeviceSequencer::IsSequenceRunning(bool& running, unsigned long long& DataPointsWritten) {
	if (master) {
		return MyEthernetMultiIOControllerFirefly->AttemptGetAktWaveformPoint(DataPointsWritten, running);
	}
	else {
		DataPointsWritten = 0;
		running = false;
		return false;
	}
}

bool CDeviceSequencer::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned long& EndTimeOfCycle, double timeout_in_s) {
	if (master) {
		return MyEthernetMultiIOControllerFirefly->WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s);
	}
	else {
		buffer = nullptr;
		buffer_length = 0;
		EndTimeOfCycle = 0;
		return false;
	}
}

void CDeviceSequencer::StartAssemblingCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->StartAssemblingCPUCommandSequence();
	}
}

void CDeviceSequencer::AddCPUCommand(const char* command) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->AddCPUCommand(command);
	}
}

void CDeviceSequencer::ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->ExecuteCPUCommandSequence(ethernet_check_period_in_ms);
	}
}

void CDeviceSequencer::StopCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->StopCPUCommandSequence();
	}
}

void CDeviceSequencer::InterruptCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->InterruptCPUCommandSequence();
	}
}

void CDeviceSequencer::GetCPUCommandErrorMessages() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->GetCPUCommandErrorMessages();
	}
}

void CDeviceSequencer::PrintCPUCommandErrorMessages() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->PrintCPUCommandErrorMessages();
	}
}

void CDeviceSequencer::PrintCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->PrintCPUCommandSequence();
	}
}





bool CDeviceSequencer::Wait_ms(double time_in_ms) {
	Delay_in_ms += time_in_ms;
	if (TimeDebt_in_ms > 0) {
		//let's pay off some or all of the time debt we acquired when commands took time beyond one cycle, e.g. DDS commands.
		if (Delay_in_ms > TimeDebt_in_ms) {
			Delay_in_ms -= TimeDebt_in_ms;
			CurrentTimeDebt_in_ms = 0;
			TimeDebt_in_ms = 0;
		}
		else
		{
			TimeDebt_in_ms -= Delay_in_ms;
			CurrentTimeDebt_in_ms -= Delay_in_ms;
			Delay_in_ms = 0;
		}
		if (TimeDebt_in_ms > MaxTimeDebt_in_ms) return false;//ToDo throw error
	}
	return true;
}

double CDeviceSequencer::GetTime_ms() {
	return AbsoluteTime_in_ms + Delay_in_ms;
}

void CDeviceSequencer::AdvanceTime() {
	
	AbsoluteTime_in_ms += Delay_in_ms + CurrentTimeDebt_in_ms;
	Delay_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;
}

//the following are commands to control the sequencer itself
bool CDeviceSequencer::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	return false;
}

void CDeviceSequencer::SequencerStartAnalogInAcquisition(const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms) {
	MyEthernetMultiIOControllerFirefly->StartAnalogInAcquisition(ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms);
}

void CDeviceSequencer::SequencerWriteInputMemory(unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
	MyEthernetMultiIOControllerFirefly->AddCommandWriteInputBuffer(input_buf_mem_data, write_next_address, input_buf_mem_address);
}

void CDeviceSequencer::SequencerWriteSystemTimeToInputMemory() {
	MyEthernetMultiIOControllerFirefly->AddCommandWriteSystemTimeToInputMemory();
}

void CDeviceSequencer::SequencerCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift) {
	MyEthernetMultiIOControllerFirefly->AddCommandCalcAD9854FrequencyTuningWord(ftw0,bit_shift);
}

void CDeviceSequencer::SequencerSwitchDebugLED(unsigned int OnOff) {
	MyEthernetMultiIOControllerFirefly->SwitchDebugLED(OnOff);
}

void CDeviceSequencer::SequencerIgnoreTCPIP(bool OnOff) {
	MyEthernetMultiIOControllerFirefly->IgnoreTCPIP(OnOff);
}
void CDeviceSequencer::SequencerAddMarker(unsigned char marker) {
	MyEthernetMultiIOControllerFirefly->AddMarker(marker);
}

void CDeviceSequencer::SequencerSetLoopCount(unsigned int loop_count) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetLoopCount(loop_count);
}

void CDeviceSequencer::SequencerJumpBackward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool loop_count_greater_zero) {
	MyEthernetMultiIOControllerFirefly->AddCommandJumpBackward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS, loop_count_greater_zero);
}

void CDeviceSequencer::SequencerJumpForward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS) {
	MyEthernetMultiIOControllerFirefly->AddCommandJumpForward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS);
}








bool CDeviceSequencer::SetValue_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address > MaxParallelBusDevices) return false;
	if (GetParallelBusDevice(Address)) { //make sure ParallelBusDeviceList[Address] is neither nullptr not 1
		bool success = ParallelBusDeviceList[Address]->SetValue(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetRegister_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address >= MaxParallelBusDevices) return false;
	if (GetParallelBusDevice(Address)) { //make sure ParallelBusDeviceList[Address] is neither nullptr not 1
		bool success = ParallelBusDeviceList[Address]->SetRegister(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetValueSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address > MaxSerialBusDevices) return false;
	if (GetSerialBusDevice(Address)) { //make sure SerialBusDeviceList[Address] is neither nullptr not 1
		bool success = SerialBusDeviceList[Address]->SetValue(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetRegisterSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address > MaxSerialBusDevices) return false;
	if (GetSerialBusDevice(Address)) { //make sure SerialBusDeviceList[Address] is neither nullptr not 1
		bool success = SerialBusDeviceList[Address]->SetRegister(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}
