// ControlLight.cpp : Defines the entry point for the application.
//

//#include "std.h"
#include "ControlAPI.h"
#include "std.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <cstdio>
#include <bitset>


using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#ifdef WIN32
#define ConfigFileName "..\\..\\..\\ControlHardwareConfig.json"
#else
#define ConfigFileName "./ControlHardwareConfig.json"
#endif


#if !defined(BUILDING_DLL) && defined(USING_DLL)

//if you use Namespace CLA, you need to put CLA:: in front of all DLL function names; you can then remove the CLA_ prefix from all functions



#ifdef THROW_EXCEPTIONS

#ifdef API_CLASS

ControlLight_API CLA;


bool LoadControlHardwareInterface() {
	bool ControlHardwareInterfaceLoadedSuccessfully = false;
	try {
		bool success = true;
		try { CLA.LoadFromJSONFile(ConfigFileName); }
		catch (...) {
			success = false;
		}
		CLA.Initialize();
		bool ready = true;
		try { CLA.IsReady(); }
		catch (...) { ready = false; }
		if (ready) {
			if (!success) {
				ControlMessageBox("Warning: Loading of hardware configuration file worked only partially.");
			}
			ControlHardwareInterfaceLoadedSuccessfully = true;
			//AddErrorMessage("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessage("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (const CLA_Exception& e) {
		ControlMessageBox(e.what());
	}
	catch (...) {
		ControlMessageBox("Error loading hardware configuration file 1");
	}
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		try {
			CLA.Cleanup();
			CLA.Create(/*InitializeAfx*/ false, /*InitializeAfxSocket*/ false);
		}
		catch (...) {
			ControlMessageBox("Error during cleanup");
		}
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}


int main() {
	cout << "Class wrapped API, using exceptions" << endl;
	//try {  //happens in class constructor
	//	CLA.Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true);
	//}
	//catch (...) {
	//	AddErrorMessage("Initialization failed");
	//	return 1; // Initialization failed
	//}
	if (!CLA.IsCreated()) {
		AddErrorMessage("Initialization failed");
		return 1; // Initialization failed
	}
	try {
		if (!LoadControlHardwareInterface()) {
			AddErrorMessage("Error loading hardware configuration file 2");

			CLA.AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 2000000, false, true, true);
			CLA.AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
			CLA.AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
			CLA.AddDeviceDigitalOut(0, 1, 16);
			CLA.AddDeviceDigitalOut(0, 2, 16);
			CLA.AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
			CLA.AddDeviceAD9858(0, 652, 1200000000, 1);
			CLA.AddDeviceAD9958(0, 21, 1000000000, 1);
			CLA.Initialize();
		}
	}
	catch (...) {
		ControlMessageBox("Error loading hardware configuration file 3");
		try {
			CLA.Cleanup();
		}
		catch (...) {
			ControlMessageBox("Error during cleanup");
		}
		//CLA.Cleanup();  //happens in class destructor
		return -1;
	}



	CLA.SwitchDebugMode(true, "DebugSequencer");
	try { CLA.IsReady(); }
	catch (...) {
		AddErrorMessage("Not all sequencers connected");
		CLA.Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		cout << "Iteration " << i << ": ";
		try {
			Time starttime = Clock::now();
			CLA.StartAssemblingSequence();

			//start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL. 
			//In this somewhat convoluted manner one can achieve anything without API interface changes
			/*
			uint8_t ChannelNumber = 0;
			uint32.t NumberOfDataPoints = 1000;
			double DelayBetweenDataPoints.in.ms.in.ms = 0.02;
			CLA.SetValueSerialDevice(0, 0, 0, (uint8_t*)&ChannelNumber, 8);
			CLA.SetValueSerialDevice(0, 0, 1, (uint8_t*)&NumberOfDataPoints, 32);
			CLA.SetValueSerialDevice(0, 0, 2, (uint8_t*)&DelayBetweenDataPoints.in.ms.in.ms, 64);
			CLA.SetValueSerialDevice(0, 0, 3, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
			*/

			//this is the same command using a convenience function
			CLA.SequencerStartAnalogInAcquisition(0, 0, 1000, 0.02);

			for (int j = 1; j < 100; j++) {
				CLA.SetVoltage(0, 24, 10.0 * j / 100.0);
				uint16_t data = 0xffff;
				CLA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
				CLA.Wait_ms(0.1);
				data = 0;
				CLA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
				CLA.Wait_ms(0.1);
				double Frequency = 1000.0 * j / 100.0;
				CLA.SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
			}
			CLA.Wait_ms(10);
			CLA.ExecuteSequence();
			bool running = false;
			unsigned long long DataPointsWritten = 0;
			CLA.GetSequenceExecutionStatus(running, DataPointsWritten);

			unsigned long buffer_length = 0;
			unsigned long EndTimeOfCycle = 0;
			CLA.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
			Duration duration = Clock::now() - starttime;
			cout << "Duration: " << milliSeconds(duration) << " ms  Buffer length : " << buffer_length << endl;
		}
		catch (const CLA_Exception& e) {
			ControlMessageBox(e.what());
		}
		catch (...) {
			AddErrorMessage("Error during sequence execution");
		}

	}
	try {
		CLA.Cleanup();
	}
	catch (...) {
		AddErrorMessage("Error during cleanup");
	}
	return 0;
}

#else


bool LoadControlHardwareInterface() {
	bool ControlHardwareInterfaceLoadedSuccessfully = false;
	try {
		bool success = true;
		try { CLA_LoadFromJSONFile(ConfigFileName); }
		catch (...) {
			success = false;
		}
		CLA_Initialize();
		bool ready = true;
		try { CLA_IsReady(); }
		catch (...) { ready = false; }
		if (ready) {
			if (!success) {
				ControlMessageBox("Warning: Loading of hardware configuration file worked only partially.");
			}
			ControlHardwareInterfaceLoadedSuccessfully = true;
			//AddErrorMessage("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessage("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (const CLA_Exception& e) {
		ControlMessageBox(e.what());
	}
	catch (...) {
		ControlMessageBox("Error loading hardware configuration file 1");
	}
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		try {
			CLA_Cleanup();
			CLA_Create(/*InitializeAfx*/ false, /*InitializeAfxSocket*/ false);
		}
		catch (...) {
			ControlMessageBox("Error during cleanup");
		}
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}


int main() {
	cout << "Bare function API, using exceptions" << endl;
	try {
		CLA_Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true);
	}
	catch (...) {
		AddErrorMessage("Initialization failed");
		return 1; // Initialization failed
	}
	try {
		if (!LoadControlHardwareInterface()) {
			AddErrorMessage("Error loading hardware configuration file 2");

			CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 2000000, false, true, true);
			CLA_AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
			CLA_AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
			CLA_AddDeviceDigitalOut(0, 1, 16);
			CLA_AddDeviceDigitalOut(0, 2, 16);
			CLA_AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
			CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
			CLA.AddDeviceAD9958(0, 21, 1000000000, 1);
			CLA_Initialize();
		}
	}
	catch (...) {
		ControlMessageBox("Error loading hardware configuration file 3");
		try {
			CLA_Cleanup();
		}
		catch (...) {
			ControlMessageBox("Error during cleanup");
		}
		CLA_Cleanup();
		return -1;
	}



	CLA_SwitchDebugMode(true, "DebugSequencer");
	try { CLA_IsReady(); }
	catch (...) {
		AddErrorMessage("Not all sequencers connected");
		CLA_Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		cout << "Iteration " << i << ": ";
		try {
			Time starttime = Clock::now();
			CLA_StartAssemblingSequence();

			//start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL. 
			//In this somewhat convoluted manner one can achieve anything without API interface changes
			/*
			uint8_t ChannelNumber = 0;
			uint32_t NumberOfDataPoints = 1000;
			double DelayBetweenDataPoints_in_ms_in_ms = 0.02;
			CLA_SetValueSerialDevice(0, 0, 0, (uint8_t*)&ChannelNumber, 8);
			CLA_SetValueSerialDevice(0, 0, 1, (uint8_t*)&NumberOfDataPoints, 32);
			CLA_SetValueSerialDevice(0, 0, 2, (uint8_t*)&DelayBetweenDataPoints_in_ms_in_ms, 64);
			CLA_SetValueSerialDevice(0, 0, 3, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
			*/
			
			//this is the same command using a convenience function
			CLA_SequencerStartAnalogInAcquisition(0, 0, 1000, 0.02);
			
			for (int j = 1; j < 100; j++) {
				CLA_SetVoltage(0, 24, 10.0 * j / 100.0);
				uint16_t data = 0xffff;
				CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
				CLA_Wait_ms(0.1);
				data = 0;
				CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
				CLA_Wait_ms(0.1);
				double Frequency = 1000.0 * j / 100.0;
				CLA_SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
			}
			CLA_Wait_ms(10);
			CLA_ExecuteSequence();
			bool running = false;
			unsigned long long DataPointsWritten = 0;
			CLA_GetSequenceExecutionStatus(running, DataPointsWritten);

			unsigned long buffer_length = 0;
			unsigned long EndTimeOfCycle = 0;
			CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
			Duration duration = Clock::now() - starttime;
			cout << "Duration: " << milliSeconds(duration) << " ms  Buffer length : " << buffer_length << endl;
		}
		catch (const CLA_Exception& e) {
			ControlMessageBox(e.what());
		}
		catch (...) {
			AddErrorMessage("Error during sequence execution");
		}

	}
	try {
		CLA_Cleanup();
	}
	catch (...) {
		AddErrorMessage("Error during cleanup");
	}
	return 0;
}

#endif //API_CLASS

#else //THROW_EXCEPTIONS

#ifdef API_CLASS

ControlLight_API CLA;


bool LoadControlHardwareInterface() {
	bool ControlHardwareInterfaceLoadedSuccessfully = false;
	try {
		bool success = CLA.LoadFromJSONFile(ConfigFileName);
		CLA.Initialize();
		if (CLA.IsReady()) {
			if (!success) {
				ControlMessageBox("Warning: Loading of hardware configuration file worked only partially.");
			}
			ControlHardwareInterfaceLoadedSuccessfully = true;
			//AddErrorMessage("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessage("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (...) {
		AddErrorMessage("Error loading hardware configuration file 1");
	}
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		CLA.Cleanup();
		CLA.Create(/*InitializeAfx*/ false, /*InitializeAfxSocket*/ false);
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}


int main() {
	cout << "Class wrapped API, using bool error return value" << endl;
	if (!CLA.IsCreated()) {
		ControlMessageBox("Error while initializing CLA class");
		return 1; // Initialization failed
	}
	if (!LoadControlHardwareInterface()) {
		AddErrorMessage("Error loading hardware configuration file 2");

		CLA.AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 2000000, false, true, true);
		CLA.AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
		CLA.AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
		CLA.AddDeviceDigitalOut(0, 1, 16);
		CLA.AddDeviceDigitalOut(0, 2, 16);
		CLA.AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
		CLA.AddDeviceAD9858(0, 652, 1200000000, 1);
		CLA.AddDeviceAD9958(0, 21, 1000000000, 1);
		CLA.Initialize();
	}



	CLA.SwitchDebugMode(true, "DebugSequencer");
	if (!CLA.IsReady()) {
		AddErrorMessage("Not all sequencers connected");
		CLA.Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		Time starttime = Clock::now();
		cout << "Iteration " << i << ": ";
		CLA.StartAssemblingSequence();

		//start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL. 
		//In this somewhat convoluted manner one can achieve anything without API interface changes
		/*
		uint8_t ChannelNumber = 0;
		uint32_t NumberOfDataPoints = 1000;
		double DelayBetweenDataPoints_in_ms_in_ms = 0.02;
		CLA.SetValueSerialDevice(0, 0, 0, (uint8_t*)&ChannelNumber, 8);
		CLA.SetValueSerialDevice(0, 0, 1, (uint8_t*)&NumberOfDataPoints, 32);
		CLA.SetValueSerialDevice(0, 0, 2, (uint8_t*)&DelayBetweenDataPoints_in_ms_in_ms, 64);
		CLA.SetValueSerialDevice(0, 0, 3, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
		*/

		//this is the same command using a convenience function
		CLA.SequencerStartAnalogInAcquisition(0, 0, 1000, 0.02);

		for (int j = 1; j < 100; j++) {
			CLA.SetVoltage(0, 24, 10.0 * j / 100.0);
			uint16_t data = 0xffff;
			CLA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
			CLA.Wait_ms(0.1);
			data = 0;
			CLA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
			CLA.Wait_ms(0.1);
			double Frequency = 1000.0 * j / 100.0;
			CLA.SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
		}
		CLA.Wait_ms(10);
		CLA.ExecuteSequence();
		bool running = false;
		unsigned long long DataPointsWritten = 0;
		CLA.GetSequenceExecutionStatus(running, DataPointsWritten);

		unsigned long buffer_length = 0;
		unsigned long EndTimeOfCycle = 0;
		CLA.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
		Duration duration = Clock::now() - starttime;
		cout << "Duration: " << milliSeconds(duration) << " ms  Buffer length : " << buffer_length << endl;

	}
	//CLA.Cleanup();//this happens in CLA desctuctor
	return 0;
}

#else //API_CLASS

bool LoadControlHardwareInterface() {
	bool ControlHardwareInterfaceLoadedSuccessfully = false;
	try {
		bool success = CLA_LoadFromJSONFile(ConfigFileName);
		CLA_Initialize();
		if (CLA_IsReady()) {
			if (!success) {
				ControlMessageBox("Warning: Loading of hardware configuration file worked only partially.");
			}
			ControlHardwareInterfaceLoadedSuccessfully = true;
			//AddErrorMessage("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessage("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (...) {
		AddErrorMessage("Error loading hardware configuration file 1");
	}	
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		CLA_Cleanup();
		CLA_Create(/*InitializeAfx*/ false, /*InitializeAfxSocket*/ false);
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}

void RampVoltage(unsigned int Sequencer, unsigned int Address, double StartVoltage, double TargetVoltage, double Duration_in_ms, double StepSize_in_ms) {
	unsigned long NumberOfSteps = (unsigned long)(Duration_in_ms / StepSize_in_ms);
	double StepSize = (TargetVoltage - StartVoltage) / (double)NumberOfSteps;
	double Voltage = StartVoltage;
	for (unsigned long i = 0; i < NumberOfSteps; i++) {
		CLA_SetVoltage(Sequencer, Address, Voltage);
		CLA_Wait_ms(StepSize_in_ms);
		Voltage += StepSize;
	}
	CLA_SetVoltage(Sequencer, Address, TargetVoltage);
}

bool InitializeSystem() {
	cout << "Bare function API, using bool error return value" << endl;


	bool DemoSmartSequencer = false;

	if (!CLA_Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true)) {
		return false; // Initialization failed
	}
	if (!LoadControlHardwareInterface()) {
		AddErrorMessage("Error loading hardware configuration file 2");

		CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 2000000, false, true, true);
		CLA_AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
		CLA_AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
		CLA_AddDeviceDigitalOut(0, 1, 16);
		CLA_AddDeviceDigitalOut(0, 2, 16);
		CLA_AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
		CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
		CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
		CLA_Initialize();
	}


	//CLA_UseEdgeTriggeredLatches(0, true);  //ToDo: check that edge triggered latches work
	//CLA_SwitchDebugMode(true, "DebugSequencer");  //If on, FPGA sends debug info over USB COM port, which slows it down
	if (!CLA_IsReady()) {
		AddErrorMessage("Not all sequencers connected");
		CLA_Cleanup();
		return false;
	}
	return true;
}


void DemoSequence(unsigned long CycleNumber) {
	CLA_StartAssemblingSequence();
	CLA_SequencerWriteSystemTimeToInputMemory(/*SequencerNr*/ 0);
	CLA_SequencerWriteInputMemory(/*SequencerNr*/ 0, CycleNumber);
	CLA_SequencerSwitchDebugLED(/*SequencerNr*/ 0, 1);
	CLA_SequencerAddMarker(/*SequencerNr*/ 0, 1);//for debug: displays marker (here "1") on ZYNQ USB port output (use Termite or similar to see it)

	//Test input board:
	//for (uint8_t n = 0; n<12; n++) {
	//	CLA_SelectRackSlot(/*SequencerNr*/ 0, /*RackNr*/ 0, n);
	//	CLA_Wait_ms(100);
	//}
	CLA_SelectRackSlot(/*SequencerNr*/ 0, /*RackNr*/ 0, 5);


	//Test analogIn, pedestrian way
	//start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL. 
	//In this somewhat convoluted manner one can achieve anything without API interface changes
	/*
	uint8_t ChannelNumber = 0;
	uint32_t NumberOfDataPoints = 1000;
	double DelayBetweenDataPoints_in_ms_in_ms = 0.02;
	CLA_SetValueSerialDevice(0, 0, 0, (uint8_t*)&AnalogInType, 8);
	CLA_SetValueSerialDevice(0, 0, 1, (uint8_t*)&SPI_CS, 8);
	CLA_SetValueSerialDevice(0, 0, 2, (uint8_t*)&ChannelNumber, 8);
	CLA_SetValueSerialDevice(0, 0, 3, (uint8_t*)&NumberOfDataPoints, 32);
	CLA_SetValueSerialDevice(0, 0, 4, (uint8_t*)&DelayBetweenDataPoints_in_ms_in_ms, 64);
	CLA_SetValueSerialDevice(0, 0, 5, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
	*/

	//Test analog in with convenience function
	//@param analog_in_type Analog in board type. 0: AQuRA MCP3208 analog in board; 1: MCP3208 12-bit ADC on SerialPortBoard; 2: ADS1256  24-bit ADC 
	//CLA_SequencerStartAnalogInAcquisition(/*Sequencer_Nr*/ 0, /*AnalogInType*/ 2, /*SPI_CS*/ 0, /*AnalogInChannelNr*/ 0, /*NumberOfDataPoints*/ 10000, /*SamplingPeriod_in_ms*/ 1);
	//CLA_Wait_ms(1000);
	
	//Test repeated digital in
	/// @param RepeatedOutInCommand the command to execute for each data point. 0: stop; 1: repeated SPI transfer; 2: repeated digital in; 3: digital in event tagger 
	/// for 3: if dig in changes, safes dig in on input memory bit 0:7, bit 8: counter overflow, bit 9: 4-entry fifo overflow, bit 10:31: clock cycle counter; runs till stopped by setting RepeatedOutInCommand to 0 with new SequencerRepeatedOutIn command.
	CLA_SequencerRepeatedOutIn(/*SequencerNr*/ 0, /*NumberOfDataPoints*/ 10000, /*SamplingPeriod_in_ms*/ 1, /* RepeatedOutInCommand*/ 2);
	CLA_Wait_ms(1000);

	//Test event time tagger
	/// @param RepeatedOutInCommand the command to execute for each data point. 0: stop; 1: repeated SPI transfer; 2: repeated digital in; 3: digital in event tagger 
	/// for 3: if dig in changes, safes dig in on input memory bit 0:7, bit 8: counter overflow, bit 9: 4-entry fifo overflow, bit 10:31: clock cycle counter; runs till stopped by setting RepeatedOutInCommand to 0 with new SequencerRepeatedOutIn command.
	//CLA_SequencerRepeatedOutIn(/*SequencerNr*/ 0, /*NumberOfDataPoints*/ 10000, /*SamplingPeriod_in_ms*/ 1, /* RepeatedOutInCommand*/ 3);
	//CLA_Wait_ms(1000);
	//CLA_SequencerRepeatedOutIn(/*SequencerNr*/ 0, /*NumberOfDataPoints*/ 1, /*SamplingPeriod_in_ms*/ 1, /* RepeatedOutInCommand*/ 0);

	for (int j = 1; j < 100; j++) {
		CLA_SetVoltage(0, 24, 10.0 * j / 100.0);
		uint16_t data = 0xffff;
		CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
		CLA_Wait_ms(0.1);
		data = 0;
		CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
		CLA_Wait_ms(0.1);
		double Frequency = 1000.0 * j / 100.0;
		CLA_SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
	}
	CLA_Wait_ms(1000);
	RampVoltage(/*Sequencer*/ 0, /*Address*/ 24, /*StartVoltage*/ -10, /* TargetVoltage*/ 10, /*Duration_in_ms*/ 100, /*StepSize_in_ms*/ 0.1);
	CLA_SequencerSwitchDebugLED(/*SequencerNr*/ 0, 0);
	CLA_SequencerWriteSystemTimeToInputMemory(/*SequencerNr*/ 0);
	//CLA_SelectRackSlot(/*SequencerNr*/ 0, /*RackNr*/ 0, 0);
}



void SaveInputDataToFile(const std::string& filename,
                         const uint32_t* buffer,
                         unsigned long buffer_length)
{
    FILE* file = std::fopen(filename.c_str(), "w");
    if (!file) {
		cerr << "Couldn't open file for writing"  <<  endl;
        return;
    }

    for (unsigned long i = 0; i < buffer_length; ++i) {

		//To test repeated digital input reading
		//uint8_t low_byte = buffer[i];
		//std::string bin = std::bitset<8>(low_byte).to_string();
		//std::fprintf(file, "%lu %u %s\n", i, buffer[i], bin.c_str());


		//To test digital input as event time tagger
		uint8_t low_byte = buffer[i];
		uint8_t second_byte = buffer[i] >> 8;
		std::string bin = std::bitset<8>(low_byte).to_string();
		std::string bin2 = std::bitset<8>(second_byte).to_string();
		std::fprintf(file, "%lu %lu %s %s\n", i, buffer[i] >> 10, bin2.c_str(), bin.c_str());


		//To test analog input
		//std::fprintf(file, "%lu %lu\n", i, buffer[i]);
    }

    std::fclose(file);
}

void DemoSequenceAnalyseData(unsigned long CycleNumber, uint32_t* buffer, const unsigned long& buffer_length, const unsigned long& EndTimeOfCycle) {
	static unsigned long long PreviousFPGASystemTime = 0;
	static unsigned int NumberOfTimesFailedRun = 0;
	static double PeriodicTriggerPeriod_in_ms = 0;
	static Time starttime = Clock::now();
	static Time last_starttime = Clock::now();

	bool CycleSuccessful = true;
	//FPGA soft trigger SystemTime is in first 8 bytes thanks to CLA automatically putting WriteSystemTimeToInputMemory before cyclic trigger
	//Bytes 8 to 15 contain the FPGA SystemTime directly after the cyclic trigger, thanks to the
	//CA.Command("WriteSystemTimeToInputMemory();"); command in the sequence.
	//We use it to check if the time between two triggers is correct, i.e. if the total cycle duration is correct.
	unsigned long long FPGASystemTimeStart = ((unsigned long long*)buffer)[0]; // in units of the clock period, i.e. usually 10ns
	unsigned long FPGASystemTimeLow = buffer[2];
	unsigned long FPGASystemTimeHigh = buffer[3];
	unsigned long long FPGASystemTime = ((unsigned long long*)buffer)[1]; // in units of the clock period, i.e. usually 10ns
	//unsigned long long FPGASystemTimeAtSequenceEnd = ((unsigned long long*)Buffer)[BufferLength/2-2]; // in units of the clock period, i.e. usually 10ns
	unsigned long CycleNrFromBuffer = buffer[4];
	double WaitForTriggerTime = 0.00001 * (FPGASystemTime - FPGASystemTimeStart);
	//We check if the MOT loading time was ok. 
	//For that, PeriodicTriggerPeriod_in_ms must contain the duration of the previous sequence plus the desired MOT loading time.
	//The only variable part of the sequence is the blue MOT duration. 
	//We measure this blue MOT's duration by determining the time between the start of the last sequence and the start of this sequence.
	//The blue MOT duration is ElapsedFPGASystemTime - the duration of the last sequence.
	//We don't calculate the blue MOT duration explicitly, but check if ElapsedFPGASystemTime is within the expected range.
	unsigned long long ElapsedFPGASystemTime = FPGASystemTime - PreviousFPGASystemTime;
	std::string ErrorMessages = "";
	if (ElapsedFPGASystemTime > PeriodicTriggerPeriod_in_ms * 100000 + 10) {
		ErrorMessages += " Overtime.";
		CycleSuccessful = false;
	}
	else if (ElapsedFPGASystemTime < PeriodicTriggerPeriod_in_ms * 100000 - 10) {
		ErrorMessages += " Undertime.";
		CycleSuccessful = false;
	}
	if (CycleNumber != CycleNrFromBuffer) {
		ErrorMessages += " Cycle number slip (expected " + std::format("{}", CycleNumber) + ", got " + std::format("{}", CycleNrFromBuffer) + ").";
		CycleSuccessful = false;
	}
	PreviousFPGASystemTime = FPGASystemTime;
	
	last_starttime = starttime;
	starttime = Clock::now();
	Duration duration = last_starttime - starttime;
	std::string out_buf = std::format("%4u %4u %4u %4.0f %10llu %03X %08X f%03u rc%u %u",
		CycleNrFromBuffer,
		buffer_length,
		milliSeconds(duration),
		WaitForTriggerTime,
		ElapsedFPGASystemTime,
		FPGASystemTimeHigh,
		FPGASystemTimeLow,
		NumberOfTimesFailedRun,
		(CycleSuccessful) ? 1 : 0);
	std::string status = out_buf + ErrorMessages;
	cout << status << endl;

	if (buffer != NULL) {
		//process input data
		std::string filename = std::format("C:\\data\\input%04u.dat", CycleNumber);
		SaveInputDataToFile(filename, buffer, buffer_length);
		//freeing buffer is done in CAL and shouldn't be done here.
	}
	else {
		cerr << "no input data received" << endl;
		CycleSuccessful = false;
	}
	if (!CycleSuccessful) NumberOfTimesFailedRun++;
}

void DemoFPGASequencerSingleRun() {
	if (!InitializeSystem()) {
		return;
	}
	uint8_t* buffer = nullptr;
	constexpr unsigned long NrCycles = 3;
	for (unsigned long CycleNr = 0; CycleNr < NrCycles; CycleNr++) {
		Time starttime = Clock::now();
		cout << "Iteration " << CycleNr << ": ";
		DemoSequence(CycleNr);
		CLA_ExecuteSequence();
		bool running = false;
		unsigned long long DataPointsWritten = 0;
		CLA_GetSequenceExecutionStatus(running, DataPointsWritten);

		unsigned long buffer_length = 0;
		unsigned long EndTimeOfCycle = 0;
		CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
		DemoSequenceAnalyseData(CycleNr, (uint32_t*)buffer, buffer_length/4, EndTimeOfCycle);

		//Test SerialPortBoardI2Cboard with signals from PS; make sure that slot is selected.
		//uint8_t address = 0xAB;
 		//CLA_TransmitI2CPort(/*I2C_port*/ 1, 0xFE, /*send_length*/ 1, &address, /*receive_length*/ 0, nullptr, /*I2CClockFrequencyInHz*/100000);

		//Duration duration = Clock::now() - starttime;
		//cout << "Duration: " << milliSeconds(duration) << " ms  Buffer length : " << buffer_length << endl;
	}
	CLA_Cleanup();
}

void DemoFPGASequencerCyclicSequencing() {
	if (!InitializeSystem()) {
		return;
	}
	uint8_t* buffer = nullptr;
	//assemble sequence
	DemoSequence(0);
	double SequenceDuration_in_ms;
	CLA_GetTime_ms(SequenceDuration_in_ms);
	double WaitTimeAfterSequence_in_ms = 300;
	double PeriodicTriggerPeriod_in_s = 0.001*(SequenceDuration_in_ms + WaitTimeAfterSequence_in_ms);
	double PeriodicTriggerAllowedWaitTime_in_s = 2;
	cout << "Cycling with " << PeriodicTriggerPeriod_in_s*1000 << " ms period of which " << SequenceDuration_in_ms << " ms sequence duration." << endl;


	//Tell sequencer that we'll use cyclic sequencing. This updates trigger settings.
	CLA_SetPeriodicTrigger(PeriodicTriggerPeriod_in_s, PeriodicTriggerAllowedWaitTime_in_s);
	
	//to speed up TCP/IP transmission of data from PC to sequencer, transmit only changes of sequence, if possible.
	CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(true);

	for (unsigned long CycleNr = 0; CycleNr < 100; CycleNr++) {
		Time starttime = Clock::now();
		cout << "Iteration " << CycleNr << ": ";
		//We create sequence from scratch to update trigger settings and cycle number dependent sequence entries.
		DemoSequence(CycleNr);
		CLA_ExecuteSequence();
		bool running = false;
		unsigned long long DataPointsWritten = 0;
		CLA_GetSequenceExecutionStatus(running, DataPointsWritten);

		unsigned long buffer_length = 0;
		unsigned long EndTimeOfCycle = 0;
		CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
		DemoSequenceAnalyseData(CycleNr, (uint32_t*)buffer, buffer_length/4, EndTimeOfCycle);
		//Duration duration = Clock::now() - starttime;
		//cout << "Duration: " << milliSeconds(duration) << " ms  Buffer length : " << buffer_length << endl;
	}
	//Switch periodic trigger off by setting its period to 0ms.
	CLA_SetPeriodicTrigger(0, 0);
}

void DemoSmartSequencer() {

	cout << "Bare function API, using bool error return value" << endl;


	bool DemoSmartSequencer = false;

	if (!CLA_Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true)) {
		return; // Initialization failed
	}

	//the CPU sequence code needs the addresses on which devices are installed. Declare those here.
	unsigned int AD98450Address = 16;
	unsigned int AnalogInChannel = 0;
	unsigned int AnalogOutBoardStartAddress = 20;
	unsigned int DigitalOutAddress = 10;

	CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 3, false, true, true);
	CLA_AddDeviceAnalogOut16bit(0, AnalogOutBoardStartAddress, 4, true, -10, 10);
	CLA_AddDeviceDigitalOut(0, DigitalOutAddress, 16);
	CLA_AddDeviceAD9854(0, AD98450Address, 2, 300000000, 1, 1);
	//CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
	//CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
	CLA_Initialize();

	//CLA_SwitchDebugMode(true, "DebugSequencer");
	if (!CLA_IsReady()) {
		AddErrorMessage("Not all sequencers connected");
		CLA_Cleanup();
		return;
	}
	
	uint8_t* buffer = nullptr;
	Time starttime = Clock::now();
	CLA_GetCPUCommandErrorMessages(); 
	
	//This is the FPGA sequence that the CPU sequence will execute. It has two parts: initialization and the main loop.
	CLA_StartAssemblingSequence();
	CLA_SwitchDebugMode(true, "DebugSequencer");
	
	
	CLA_StartAssemblingSequence();

	bool DoInitialization = false;
	bool DoFastVCOLoop = true;
	if (DoInitialization) {
		CLA_SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1, 0.1);
		//initialize devices
		CLA_SetStartFrequency(0, AD98450Address, 1000000.0);
		//CLA_SetPower(0, AD98450Address, 100.0);
		//CLA_SetVoltage(0, AnalogOutBoardStartAddress, 0.0);
		CLA_Wait_ms(0.00001); //wait for analog in acquisition to end
		CLA_SetDigitalOutput(0, DigitalOutAddress, 0, false);
		CLA_StartAssemblingNextSequence();
	}


	//The CPU sequence will modify the FPGA sequence. For that it needs to know the buffer positions of the commands in the FPGA sequence.
	unsigned long CycleStartBufferPosition;
	CLA_GetNextBufferPositionOfMasterSequencer(CycleStartBufferPosition);
	//set digital output to high to indicate end of FPGA sequence loop
	CLA_SetDigitalOutput(0, DigitalOutAddress, 0, true);
	CLA_SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1, 0.1);
	//unsigned long SetVoltageBufferPosition;
	//CLA_GetNextBufferPositionOfMasterSequencer(SetVoltageBufferPosition);
	//CLA_SetVoltage(0, AnalogOutBoardStartAddress, 10.0);
	unsigned long SetFrequencyBufferPosition;
	CLA_GetNextBufferPositionOfMasterSequencer(SetFrequencyBufferPosition);
	CLA_SetFrequency(0, AD98450Address, 1000000.0);
	CLA_Wait_ms(0.00001); //wait for ADC to finish conversion
	//set digital output to low to indicate end of FPGA sequence loop
	CLA_SetDigitalOutput(0, DigitalOutAddress, 0, false);
	//send sequence to FPGA, but do not execute it yet
	CLA_SendSequence(); //sends sequence to FPGA, but does not execute it yet

	CLA_GetCPUCommandErrorMessages();
	CLA_StartAssemblingCPUCommandSequence();
	CLA_GetCPUCommandErrorMessages();
	CLA_AddCPUCommand("ExecuteFPGASequence(0);");
	CLA_AddCPUCommand("WaitTillSequenceFinished(0);");
	//CLA_AddCPUCommand("GetInputBufferValue(Input, 0);");
	//CLA_AddCPUCommand("Print(Input);"); //just for debug
	
	//CLA_AddCPUCommand(("ExecuteFPGASequence(" + std::to_string(CycleStartBufferPosition) + ");").c_str());
	//CLA_AddCPUCommand("ExecuteFPGASequence(0);");
	//CLA_AddCPUCommand("WaitTillSequenceFinished(0);");
	//CLA_AddCPUCommand("GetInputBufferValue(Input, 0);");

	if (DoFastVCOLoop) {
		CLA_AddCPUCommand("PrintFPGABuffer(70);");
		//CLA_AddCPUCommand(("RunFastVCOLoop(" + std::to_string(CycleStartBufferPosition) + "," + std::to_string(SetFrequencyBufferPosition) + ");").c_str());
		CLA_AddCPUCommand(("RunFastVCOLoop(0," + std::to_string(SetFrequencyBufferPosition) + ");").c_str());
		CLA_AddCPUCommand("PrintFPGABuffer(70);");
	}
	else {
		CLA_AddCPUCommand("LoopStart:");
		CLA_AddCPUCommand(("ExecuteFPGASequence(" + std::to_string(CycleStartBufferPosition) + ");").c_str());
		//CLA_AddCPUCommand("ExecuteFPGASequence(0);");
		//while FPGA sequence is running, use data of last run to calculate and set new frequency and voltage
		//CLA_AddCPUCommand("WaitTillSequenceFinished(0);");
		CLA_AddCPUCommand("GetInputBufferValue(Input, 0);");
		//CLA_AddCPUCommand(("SetAnalogOut(" + std::to_string(SetVoltageBufferPosition) + ",Input);").c_str());
		CLA_AddCPUCommand("Mul(ScaledInput, Input, 1.23);");
		CLA_AddCPUCommand("Add(FrequencyTuningWord, ScaledInput, 123);");
		CLA_AddCPUCommand(("SetAD9854Frequency(" + std::to_string(SetFrequencyBufferPosition) + ",FrequencyTuningWord);").c_str());
		CLA_AddCPUCommand("WaitTillSequenceFinished(0);");
		CLA_AddCPUCommand("JumpIfNotZero(ContinueExecution, LoopStart:);");
	}
	CLA_GetCPUCommandErrorMessages();

	CLA_ExecuteCPUCommandSequence(100); //execute the CPU command sequence, which will execute the FPGA sequence and modify it in a loop
	
	this_thread::sleep_for(1000ms);
	CLA_GetCPUCommandErrorMessages();

	CLA_StopCPUCommandSequence(); //Set "ContinueExecution" to false, so that the CPU command sequence stops at the next stop point 

	this_thread::sleep_for(1000ms);

	CLA_InterruptCPUCommandSequence(); //stop the CPU command sequence immediately, just in case the above didn't stop it
	CLA_GetCPUCommandErrorMessages();


	CLA_Cleanup();
}



void DemoDDSVCO() {

	cout << "Bare function API, using bool error return value" << endl;


	bool DemoSmartSequencer = false;

	if (!CLA_Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true)) {
		return; // Initialization failed
	}

	//the CPU sequence code needs the addresses on which devices are installed. Declare those here.
	unsigned int AD98450Address = 16;
	unsigned int AnalogInChannel = 0;
	unsigned int AnalogOutBoardStartAddress = 20;
	unsigned int DigitalOutAddress = 10;

	CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.1.90", 7, true, 0, 100000000, 3, false, true, true);
	CLA_AddDeviceAnalogOut16bit(0, AnalogOutBoardStartAddress, 4, true, -10, 10);
	CLA_AddDeviceDigitalOut(0, DigitalOutAddress, 16);
	CLA_AddDeviceAD9854(0, AD98450Address, 2, 300000000, 1, 1);
	//CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
	//CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
	CLA_Initialize();

	//CLA_SwitchDebugMode(true, "DebugSequencer");
	if (!CLA_IsReady()) {
		AddErrorMessage("Not all sequencers connected");
		CLA_Cleanup();
		return;
	}

	uint8_t* buffer = nullptr;
	Time starttime = Clock::now();
	CLA_GetCPUCommandErrorMessages();

	//This is the FPGA sequence that the CPU sequence will execute. It has two parts: initialization and the main loop.
	CLA_StartAssemblingSequence();
	CLA_SwitchDebugMode(true, "DebugSequencer");


	CLA_StartAssemblingSequence();

	bool DoInitialization = false;
	bool DoFastVCOLoop = true;
	if (DoInitialization) {
		CLA_SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1, 0.1);
		//initialize devices
		CLA_SetStartFrequency(0, AD98450Address, 1000000.0);
		CLA_SetPower(0, AD98450Address, 100.0);
		//CLA_SetVoltage(0, AnalogOutBoardStartAddress, 0.0);
		CLA_SetDigitalOutput(0, DigitalOutAddress, 0, false);
		CLA_Wait_ms(0.02); //wait for ADC to finish conversion
	}

	//The Sequencer will run in a loop, so we need to set the start of the loop here.
	unsigned long CycleStartBufferPosition;
	CLA_GetNextBufferPositionOfMasterSequencer(CycleStartBufferPosition);
	//we reset the input memory pointer to zero in order to avoid the Zynq CPU constantly having to copy input data from BRAM to DRAM
	CLA_SequencerWriteInputMemory(/*Sequencer*/ 0, /*input_buf_mem_data*/0, /*write_next_address*/0, /*input_buf_mem_address*/0);
	//We start a single ADC acquisition
	CLA_SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1, 0.1);
	//set digital output to high to indicate start of FPGA sequence loop
	CLA_SetDigitalOutput(0, DigitalOutAddress, 0, true);
	double Frequency = 80.0; //set the frequency to 80 MHz
	double SYSCLK = 300.0; //set the system clock to 300 MHz
	uint64_t FTW0 = (Frequency * (2 << 48)) / SYSCLK;
	uint8_t bit_shift = 0;
	/*
	Assuming the ADC provides 16 bits
	min frequency change: (1 << bit_shift) SYSCLK / (2<<48)     frequency range:  (1 << (bit_shift+16)) SYSCLK / (2<<48)
	
	2^48=	281,474,976,710,656	SYSCLCK	80000000
	bitshift	1<<bitshift	deltaf_min	deltaf_max
	0	1			2.84217E-07	0.018626451
	1	2			5.68434E-07	0.037252903
	2	4			1.13687E-06	0.074505806
	3	8			2.27374E-06	0.149011612
	4	16			4.54747E-06	0.298023224
	5	32			9.09495E-06	0.596046448
	6	64			1.81899E-05	1.192092896
	7	128			3.63798E-05	2.384185791
	8	256			7.27596E-05	4.768371582
	9	512			0.000145519	9.536743164
	10	1024		0.000291038	19.07348633
	11	2048		0.000582077	38.14697266
	12	4096		0.001164153	76.29394531
	13	8192		0.002328306	152.5878906
	14	16384		0.004656613	305.1757813
	15	32768		0.009313226	610.3515625
	16	65536		0.018626451	1220.703125
	17	131072		0.037252903	2441.40625
	18	262144		0.074505806	4882.8125
	19	524288		0.149011612	9765.625
	20	1048576		0.298023224	19531.25
	21	2097152		0.596046448	39062.5
	22	4194304		1.192092896	78125
	23	8388608		2.384185791	156250
	24	16777216	4.768371582	312500
	25	33554432	9.536743164	625000
	26	67108864	19.07348633	1250000
	27	134217728	38.14697266	2500000
	28	268435456	76.29394531	5000000
	29	536870912	152.5878906	10000000
	30	1073741824	305.1757813	20000000
	31	2147483648	610.3515625	40000000
	*/
	//convert ADC value of previous conversion into FTW.
	//for debugging commented out the next line
	CLA_Wait_ms(10);
	CLA_SequencerCalcAD9854FrequencyTuningWord(0, FTW0, bit_shift);//calc FTW and put FPGA into mode in which the FTW will be sent out over the next 6 bus cycles, which must be an AD9854 SetFrequency command.
	CLA_SetFrequency(0, AD98450Address, 1000000.0); //the frequency given here will automatically be replaced by the FTW that was just calculated
	//CLA_SetVoltage(0, AnalogOutBoardStartAddress, 10.0);
	//set digital output to low to indicate end of FPGA sequence loop
	CLA_Wait_ms(0.01); //wait for ADC to finish conversion (shorten as much as possible)
	CLA_SetDigitalOutput(0, DigitalOutAddress, 0, false);
	unsigned long CycleEndBufferPosition;
	CLA_GetNextBufferPositionOfMasterSequencer(CycleEndBufferPosition);
	//for debugging, commente out the next line
	CLA_Wait_ms(10);
	CLA_SequencerJumpBackward(0, CycleEndBufferPosition - CycleStartBufferPosition);
	//for debugging, added next line
	CLA_Wait_ms(10);
	CLA_ExecuteSequence(); //sends sequence to FPGA and executes it

	CLA_Cleanup();
}

void DemoWriteConfigEEPROM() {
	if (!InitializeSystem()) {
		return;
	}

	constexpr size_t MaxEEPROMPayloadBytes = 256;
	const string model_name = "Backplane";
	const string version = "0.16";
	const string type = "1";
	
	//const string model_name = "DDSAD9959";
	//const string version = "V0.09";
	//const string type = "";

	//const string model_name = "Sequencer";
	//const string version = "0.04";
	//const string type = "Z-turn V2";

	//const string model_name = "SerialPort";
	//const string version = "0.03";
	//const string type = "";

	constexpr unsigned int SNSuffix = 0;

	constexpr uint8_t SequencerNr = 0;
	constexpr uint8_t RackNr = 0;
	constexpr uint8_t SlotNr = 12;

	if (SNSuffix > 99) {
		cout << "EEPROM write skipped: serial number suffix " << SNSuffix
			<< " is out of range 00..99." << endl;
		return;
	}

	auto now = chrono::system_clock::now();
	time_t now_time = chrono::system_clock::to_time_t(now);
	tm local_time = {};
	localtime_s(&local_time, &now_time);

	ostringstream serial_stream;
	serial_stream << put_time(&local_time, "%Y%m%d%H%M%S")
		<< setw(2) << setfill('0') << SNSuffix;

	ostringstream json_stream;
	json_stream << "{\"Model\":\"" << model_name
		<< "\", \"Version\":\"" << version;
	if (type!="") json_stream << "\", \"Type\":\"" << type;
	json_stream << "\", \"SN\":\"" << serial_stream.str() << "\"}";
	const string json_payload = json_stream.str();

	if (json_payload.size() > MaxEEPROMPayloadBytes) {
		cout << "EEPROM write skipped: JSON payload length " << json_payload.size()
			<< " exceeds " << MaxEEPROMPayloadBytes << " bytes." << endl;
		return;
	}

	CLA_WriteConfigEEPROM(SequencerNr, RackNr, SlotNr, json_payload.c_str(), json_payload.size()+1);
}

void DemoReadConfigEEPROM() {
	if (!InitializeSystem()) {
		return;
	}
	constexpr uint8_t SequencerNr = 0;
	constexpr uint8_t RackNr = 0;
	constexpr uint8_t SlotNr = 0;
	char buffer[256] = {};
	size_t length = sizeof(buffer);
	CLA_ReadConfigEEPROM(SequencerNr, RackNr, SlotNr, buffer, length);
	std::string read_data(buffer, length);
	cout << "Read from EEPROM: " << read_data << endl;
	cout << endl;
	CLA_ReadConfiguration("ConfigFromEEPROMs");
}

int main() {
	DemoFPGASequencerSingleRun();
	//DemoFPGASequencerCyclicSequencing();
	//DemoSmartSequencer();
	//DemoDDSVCO();
	//DemoWriteConfigEEPROM();
	//DemoReadConfigEEPROM();
	return 0;
}

#endif //API_CLASS

#endif	

#endif

//ToDo:
// Linux and Windows compatible version
// Test sequence cycling
// Pull additional convenience commands for digital out and DDS devices from CDevice.h into the API
// Finish SetRegister and SetValue functions for DDS
//(optional, as we do take care of TimeDebt now): wait for startDelay tick counts to synchronize several sequencers
// Add interlaced ramping & LabScript & HEROS interface
// Add calibration functions, time reordering, last value access, intermittent storage of output values, 
