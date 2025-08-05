// ControlLight.cpp : Defines the entry point for the application.
//

//#include "std.h"
#include "ControlAPI.h"
#include "std.h"
#include <iostream>
#include <string>

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#define ConfigFileName "..\\..\\..\\ControlHardwareConfig.json"


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
			//AddErrorMessageCString("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessageCString("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
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
	//	AddErrorMessageCString("Initialization failed");
	//	return 1; // Initialization failed
	//}
	if (!CLA.IsCreated()) {
		AddErrorMessageCString("Initialization failed");
		return 1; // Initialization failed
	}
	try {
		if (!LoadControlHardwareInterface()) {
			AddErrorMessageCString("Error loading hardware configuration file 2");

			CLA.AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true);
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
		AddErrorMessageCString("Not all sequencers connected");
		CLA.Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		cout << "Iteration " << i << ": ";
		try {
			DWORD starttime = GetTickCount();
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
			DWORD Duration = GetTickCount() - starttime;
			cout << "Duration: " << Duration << " ms  Buffer length : " << buffer_length << endl;
		}
		catch (const CLA_Exception& e) {
			ControlMessageBox(e.what());
		}
		catch (...) {
			AddErrorMessageCString("Error during sequence execution");
		}

	}
	try {
		CLA.Cleanup();
	}
	catch (...) {
		AddErrorMessageCString("Error during cleanup");
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
			//AddErrorMessageCString("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessageCString("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
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
		AddErrorMessageCString("Initialization failed");
		return 1; // Initialization failed
	}
	try {
		if (!LoadControlHardwareInterface()) {
			AddErrorMessageCString("Error loading hardware configuration file 2");

			CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true);
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
		AddErrorMessageCString("Not all sequencers connected");
		CLA_Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		cout << "Iteration " << i << ": ";
		try {
			DWORD starttime = GetTickCount();
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
			DWORD Duration = GetTickCount() - starttime;
			cout << "Duration: " << Duration << " ms  Buffer length : " << buffer_length << endl;
		}
		catch (const CLA_Exception& e) {
			ControlMessageBox(e.what());
		}
		catch (...) {
			AddErrorMessageCString("Error during sequence execution");
		}

	}
	try {
		CLA_Cleanup();
	}
	catch (...) {
		AddErrorMessageCString("Error during cleanup");
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
			//AddErrorMessageCString("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessageCString("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (...) {
		AddErrorMessageCString("Error loading hardware configuration file 1");
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
		AddErrorMessageCString("Error loading hardware configuration file 2");

		CLA.AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true);
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
		AddErrorMessageCString("Not all sequencers connected");
		CLA.Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		DWORD starttime = GetTickCount();
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
		DWORD Duration = GetTickCount() - starttime;
		cout << "Duration: " << Duration << " ms  Buffer length : " << buffer_length << endl;

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
			//AddErrorMessageCString("Hardware configuration file loaded");
		}
		else {
			if (success) {
				AddErrorMessageCString("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (...) {
		AddErrorMessageCString("Error loading hardware configuration file 1");
	}	
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		CLA_Cleanup();
		CLA_Create(/*InitializeAfx*/ false, /*InitializeAfxSocket*/ false);
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}

void DemoFPGASequencer() {
	cout << "Bare function API, using bool error return value" << endl;


	bool DemoSmartSequencer = false;

	if (!CLA_Create(/*InitializeAfx*/ true, /*InitializeAfxSocket*/ true)) {
		return; // Initialization failed
	}
	if (!LoadControlHardwareInterface()) {
		AddErrorMessageCString("Error loading hardware configuration file 2");

		CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true);
		CLA_AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
		CLA_AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
		CLA_AddDeviceDigitalOut(0, 1, 16);
		CLA_AddDeviceDigitalOut(0, 2, 16);
		CLA_AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
		CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
		CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
		CLA_Initialize();
	}



	CLA_SwitchDebugMode(true, "DebugSequencer");
	if (!CLA_IsReady()) {
		AddErrorMessageCString("Not all sequencers connected");
		CLA_Cleanup();
		return;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		DWORD starttime = GetTickCount();
		cout << "Iteration " << i << ": ";
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
		DWORD Duration = GetTickCount() - starttime;
		cout << "Duration: " << Duration << " ms  Buffer length : " << buffer_length << endl;

	}
	CLA_Cleanup();
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

	CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.104", 7, true, 0, 100000000, 3, false, true, true);
	CLA_AddDeviceAnalogOut16bit(0, AnalogOutBoardStartAddress, 4, true, -10, 10);
	CLA_AddDeviceDigitalOut(0, DigitalOutAddress, 16);
	CLA_AddDeviceAD9854(0, AD98450Address, 2, 300000000, 1, 1);
	//CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
	//CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
	CLA_Initialize();

	//CLA_SwitchDebugMode(true, "DebugSequencer");
	if (!CLA_IsReady()) {
		AddErrorMessageCString("Not all sequencers connected");
		CLA_Cleanup();
		return;
	}
	
	uint8_t* buffer = nullptr;
	DWORD starttime = GetTickCount();
	CLA_GetCPUCommandErrorMessages(); 
	
	//This is the FPGA sequence that the CPU sequence will execute. It has two parts: initialization and the main loop.
	CLA_StartAssemblingSequence();
	CLA_SwitchDebugMode(true, "DebugSequencer");
	
	
	CLA_StartAssemblingSequence();

	bool DoInitialization = false;
	bool DoFastVCOLoop = true;
	if (DoInitialization) {
		CLA_SequencerStartAnalogInAcquisition(0, 0, 1, 0.1);
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
	CLA_SequencerStartAnalogInAcquisition(0, 0, 1, 0.1);
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
	
	Sleep(1000);
	CLA_GetCPUCommandErrorMessages();

	CLA_StopCPUCommandSequence(); //Set "ContinueExecution" to false, so that the CPU command sequence stops at the next stop point 

	Sleep(1000);

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

	CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.104", 7, true, 0, 100000000, 3, false, true, true);
	CLA_AddDeviceAnalogOut16bit(0, AnalogOutBoardStartAddress, 4, true, -10, 10);
	CLA_AddDeviceDigitalOut(0, DigitalOutAddress, 16);
	CLA_AddDeviceAD9854(0, AD98450Address, 2, 300000000, 1, 1);
	//CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
	//CLA_AddDeviceAD9958(0, 21, 1000000000, 1);
	CLA_Initialize();

	//CLA_SwitchDebugMode(true, "DebugSequencer");
	if (!CLA_IsReady()) {
		AddErrorMessageCString("Not all sequencers connected");
		CLA_Cleanup();
		return;
	}

	uint8_t* buffer = nullptr;
	DWORD starttime = GetTickCount();
	CLA_GetCPUCommandErrorMessages();

	//This is the FPGA sequence that the CPU sequence will execute. It has two parts: initialization and the main loop.
	CLA_StartAssemblingSequence();
	CLA_SwitchDebugMode(true, "DebugSequencer");


	CLA_StartAssemblingSequence();

	bool DoInitialization = false;
	bool DoFastVCOLoop = true;
	if (DoInitialization) {
		CLA_SequencerStartAnalogInAcquisition(0, 0, 1, 0.1);
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
	CLA_SequencerStartAnalogInAcquisition(0, 0, 1, 0.1);
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

int main() {
	DemoFPGASequencer();
	//DemoSmartSequencer();
	//DemoDDSVCO();
	return 0;
}

#endif //API_CLASS

#endif	

#endif

//ToDo:
// Pull additional convenience commands for digital out and DDS devices from CDevice.h into the API
// Finish SetRegister and SetValue functions for DDS
//(optional, as we do take care of TimeDebt now): wait for startDelay tick counts to synchronize several sequencers

