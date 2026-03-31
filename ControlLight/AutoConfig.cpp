// ControlLight.cpp : Defines the entry point for the application.
//

//#include "std.h"
#include "ControlAPI.h"
#include "std.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdio>
#include "AutoConfig.h"


using namespace std;

namespace {
	constexpr uint8_t NrSlots = 12;
	constexpr uint8_t I2CPortNr[NrSlots] = { 2, 1, 0, 7, 6, 5, 4, 0, 1, 3, 2, 6 };
	constexpr uint8_t I2CMultAddr[2] = { 0xE0, 0xE2 };
	constexpr uint8_t I2CAddress[NrSlots] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	constexpr uint8_t I2CChainMult = 0;
	constexpr uint8_t I2CChainPortNr = 7;
	constexpr uint8_t Write = 0;
	constexpr uint8_t Read = 1;
	constexpr uint8_t EEPROMAddress = 0xA0;
	constexpr uint32_t I2CClockFrequencyInHz = 100000;
	constexpr size_t EEPROMSizeInBytes = 256;
}

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


void ResetRackI2CMultiplexers(const uint8_t SequencerID) {
	//This function resets the I2C multiplexers of the specified rack, by writing 0 to the corresponding configuration register of the sequencer.
	//This is needed before writing to the EEPROM, to make sure that the I2C communication is working and that we are writing to the correct device.
	CLA_StartAssemblingSequence();
	uint8_t data = 1;
	CLA_SetValue(SequencerID, /*Address*/ 0xFE, /*SubAddress*/ 0, /*Data*/ &data, /*DataLength_in_bit*/ 1, /*StartBit*/ 7);
	CLA_Wait_ms(0.01);
	data = 0;
	CLA_SetValue(SequencerID, /*Address*/ 0xFE, /*SubAddress*/ 0, /*Data*/ &data, /*DataLength_in_bit*/ 1, /*StartBit*/ 7);
	CLA_Wait_ms(0.01);
	CLA_ExecuteSequence();
	uint8_t *buffer = nullptr;
	unsigned long buffer_length = 0;
	unsigned long EndTimeOfCycle = 0;
	CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
}

void SelectRackI2CSlot(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr) {
	if (SlotNr >= NrSlots) {
		cout << "EEPROM write failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	if (RackNr != 0) {
		cout << "EEPROM write failed: RackNr " << static_cast<unsigned int>(RackNr) << " is not supported yet." << endl;
		return;
	}

	ResetRackI2CMultiplexers(SequencerID);

	for (uint8_t i=0; i < RackNr; i++) {
		//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port to access the chain rack; repeat till we reach the correct rack
		uint8_t mux_select = static_cast<uint8_t>(1u << I2CChainPortNr);
		CLA_TransmitI2CPort(/*I2C_port*/ 0, I2CMultAddr[I2CChainMult] + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
	}

	//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port for the slot
	uint8_t mux_select = static_cast<uint8_t>(1u << I2CPortNr[SlotNr]);
	CLA_TransmitI2CPort(/*I2C_port*/ 0, I2CMultAddr[I2CAddress[SlotNr]] + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
	
}

void WriteConfigEEPROM(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, const char* data, const size_t length) {
	if (data == nullptr) {
		cout << "EEPROM write failed: data pointer is null." << endl;
		return;
	}

	if (SlotNr >= NrSlots) {
		cout << "EEPROM write failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	if (length > EEPROMSizeInBytes) {
		cout << "EEPROM write failed: length " << length << " exceeds EEPROM size of " << EEPROMSizeInBytes << " bytes." << endl;
		return;
	}

	if (RackNr != 0) {
		cout << "EEPROM write failed: RackNr " << static_cast<unsigned int>(RackNr) << " is not supported yet." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Now write the data to the EEPROM of type M24C01-W (2kbit I2C EEPROM), see datasheet in folder datasheet, starting from memory address 0.
	for (size_t address = 0; address < length; ++address) {
		uint8_t write_buffer[2] = {
			static_cast<uint8_t>(address),
			static_cast<uint8_t>(data[address])
		};
		CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Write, /*send_length*/ 2, write_buffer, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
		this_thread::sleep_for(chrono::milliseconds(10));
	}


	//Now read the data back to verify that it was written correctly
	vector<uint8_t> read_back(length);
	uint8_t start_address = 0;
	if (length > 0) {
		CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Read, /*send_length*/ 1, &start_address, /*receive_length*/ static_cast<uint16_t>(length), read_back.data(), I2CClockFrequencyInHz);
	}

	if (length == 0 || memcmp(data, read_back.data(), length) == 0) {
		cout << "EEPROM write verification succeeded for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ", " << length << " byte(s)." << endl;
	}
	else {
		cout << "EEPROM write verification failed for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< "." << endl;
	}

}


void ReadConfigEEPROM(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, char* data, size_t &length) {
	if (data == nullptr) {
		cout << "EEPROM read failed: data pointer is null." << endl;
		return;
	}

	if (SlotNr >= NrSlots) {
		cout << "EEPROM read failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	if (length < EEPROMSizeInBytes) {
		cout << "EEPROM read failed: output buffer is too small. Need " << EEPROMSizeInBytes << " bytes." << endl;
		return;
	}

	if (RackNr != 0) {
		cout << "EEPROM read failed: RackNr " << static_cast<unsigned int>(RackNr) << " is not supported yet." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	//Read the complete EEPROM contents starting from memory address 0.
	vector<uint8_t> read_back(EEPROMSizeInBytes);
	uint8_t start_address = 0;
	CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Read, /*send_length*/ 1, &start_address, /*receive_length*/ static_cast<uint16_t>(read_back.size()), read_back.data(), I2CClockFrequencyInHz);

	memcpy(data, read_back.data(), read_back.size());
	length = read_back.size();

	cout << "EEPROM read succeeded for rack " << static_cast<unsigned int>(RackNr)
		<< ", slot " << static_cast<unsigned int>(SlotNr)
		<< ", " << length << " byte(s)." << endl;
}
