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
#include <cctype>
#include <iomanip>
#include "AutoConfig.h"


using namespace std;

namespace {
	constexpr uint8_t NrSlots = 13; // "Slot" 13 is the backplane memory.
	constexpr uint8_t I2CMultAddr[2] = { 0xE0, 0xEE};
	constexpr uint8_t I2CPortNr[NrSlots] = { 6, 2, 3, 1, 0, 4, 5, 6, 7, 0, 1, 2, 5 };
	constexpr uint8_t I2CMux[NrSlots] =    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 };
	constexpr uint8_t I2CChainMult = 0;
	constexpr uint8_t I2CChainPortNr = 7;
	constexpr uint8_t I2CMux1PortNrOnMux0 = 4;
	constexpr uint8_t MaxSupportedRackNr = 6;
	constexpr uint8_t Write = 0;
	constexpr uint8_t Read = 1;
	constexpr uint8_t EEPROMAddress = 0xA2;
	constexpr uint8_t ConfigAddressIOExpanderAddress = 0x40;
	constexpr uint32_t I2CClockFrequencyInHz = 100000;
	constexpr size_t EEPROMSizeInBytes = 256;

	std::string GetModelPrefix(const json& board_json) {
		if (!board_json.contains("Model") || !board_json["Model"].is_string()) {
			return "";
		}

		std::string model = board_json["Model"];
		const size_t version_marker = model.find(" V");
		if (version_marker != std::string::npos) {
			model = model.substr(0, version_marker);
		}
		return model;
	}

	void CopyFieldIfPresent(const json& source, json& destination, const char* source_key, const char* destination_key = nullptr) {
		const char* target_key = destination_key ? destination_key : source_key;
		if (source.contains(source_key)) {
			destination[target_key] = source[source_key];
		}
	}

	void AddCommonMetadata(const json& source, json& destination, uint8_t sequencer_nr, uint8_t rack_nr, uint8_t slot_nr) {
		destination["Sequencer"] = sequencer_nr;
		destination["RackNr"] = rack_nr;
		destination["SlotNr"] = slot_nr;
		CopyFieldIfPresent(source, destination, "Model");
		CopyFieldIfPresent(source, destination, "SN");
	}

	void PrintEEPROMData(const char* label, const uint8_t* data, const size_t length) {
		cout << label << " (" << length << " byte(s)):" << endl;
		cout << "ASCII: ";
		for (size_t index = 0; index < length; ++index) {
			const uint8_t value = data[index];
			cout << (isprint(value) ? static_cast<char>(value) : '.');
		}
		cout << endl;

		cout << "Hex:   ";
		for (size_t index = 0; index < length; ++index) {
			cout << hex << setw(2) << setfill('0') << static_cast<unsigned int>(data[index]);
			if (index + 1 < length) {
				cout << ' ';
			}
		}
		cout << dec << setfill(' ') << endl;
	}

	std::string MakeConfigOutputFilename(const std::string& filename) {
		std::string output_filename = filename;
		if (output_filename.size() >= 5 && output_filename.substr(output_filename.size() - 5) == ".json") {
			output_filename = output_filename.substr(0, output_filename.size() - 5);
		}
		return output_filename + "_config.json";
	}

	void ReadEEPROMBytes(const uint8_t start_address, uint8_t* data, const size_t length) {
		if (length == 0) {
			return;
		}

		uint8_t address = start_address;
		CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Write, /*send_length*/ 1, &address, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
		CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ static_cast<uint16_t>(length), data, I2CClockFrequencyInHz);
	}
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
	//SlotNr 0..11 are rack slots.
	//SlotNr 12 is the memory on the rack backplane.
	if (SlotNr >= NrSlots) {
		cout << "SelectRackI2CSlot failed: slot number " << static_cast<unsigned int>(SlotNr) << " too high (0..11 are rack slots, 12 is the backplane memory)." << endl;
		return;
	}

	if (RackNr > 6) {
		cout << "SelectRackI2CSlot failed: RackNr " << static_cast<unsigned int>(RackNr) << " is > 6 and therefore not supported. The rack still works, but auto-config doesn't." << endl;
		return;
	}

	ResetRackI2CMultiplexers(SequencerID);

	//Mux 0 of Rack N has address N. It's port I2CMultRackAddr is connected to the next rack. Let's select the target rack.
	uint8_t mux_select = static_cast<uint8_t>(1u << I2CChainPortNr);
	for (uint8_t I2CMultRackAddr=0; I2CMultRackAddr < RackNr; I2CMultRackAddr++) {
		//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port to access the chain rack; repeat till we reach the correct rack
		CLA_TransmitI2CPort(/*I2C_port*/ 0, 0xE0 + (I2CMultRackAddr << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
	}
	uint8_t mux_address = RackNr;
	//If the desired slot is 
	if (I2CMux[SlotNr] == 1) {
		mux_select = static_cast<uint8_t>(1u << I2CMux1PortNrOnMux0);
		CLA_TransmitI2CPort(/*I2C_port*/ 0, 0xE0 + (RackNr << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);	
		mux_address = 1+2+4;
	}

	//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port for the slot, or the backplane memory (for SlotNr == 12).
	mux_select = static_cast<uint8_t>(1u << I2CPortNr[SlotNr]);
	CLA_TransmitI2CPort(/*I2C_port*/ 0, 0xE0 + (mux_address << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
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


	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Now write the data to the EEPROM of type M24C02-F (2kbit I2C EEPROM), see datasheet in folder datasheet, starting from memory address 0.

	constexpr size_t EEPROMPageSizeInBytes = 16;
	size_t address = 0;
	while (address < length) {
		const size_t write_length = (length - address >= EEPROMPageSizeInBytes) ? EEPROMPageSizeInBytes : (length - address);
		uint8_t write_buffer[EEPROMPageSizeInBytes + 1] = {};
		write_buffer[0] = static_cast<uint8_t>(address);
		memcpy(&write_buffer[1], &data[address], write_length);

		CLA_TransmitI2CPort(/*I2C_port*/ 0, EEPROMAddress + Write, /*send_length*/ static_cast<uint16_t>(write_length + 1), write_buffer, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);
		this_thread::sleep_for(chrono::milliseconds(1));
		address += write_length;
	}


	//Now read the data back to verify that it was written correctly
	vector<uint8_t> read_back(length);
	ReadEEPROMBytes(/*start_address*/ 0, read_back.data(), read_back.size());

	if (length == 0 || memcmp(data, read_back.data(), length) == 0) {
		cout << "Wrote: " << string(data, length) << endl;
		cout << "EEPROM write verification succeeded for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ", " << length << " byte(s)." << endl;
	}
	else {
		cout << "EEPROM write verification failed for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< "." << endl;
		PrintEEPROMData("EEPROM data expected", reinterpret_cast<const uint8_t*>(data), length);
		PrintEEPROMData("EEPROM data read back", read_back.data(), read_back.size());
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

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	//Read the complete EEPROM contents starting from memory address 0.
	vector<uint8_t> read_back(EEPROMSizeInBytes);
	ReadEEPROMBytes(/*start_address*/ 0, read_back.data(), read_back.size());

	memcpy(data, read_back.data(), read_back.size());
	//length = read_back.size();
	
	const void* endofstring = memchr(data, 0, read_back.size());
	length = endofstring ? static_cast<const char*>(endofstring) - data + 1 : 0;

	cout << "Rack " << static_cast<unsigned int>(RackNr)
		<< ", slot " << static_cast<unsigned int>(SlotNr)
		<< ", EEPROM read length: " << length << " byte(s): ";
	if (length > 0) cout << read_back.data();//PrintEEPROMData("EEPROM data read back", read_back.data(), read_back.size());
	//cout << endl;
}


void WriteConfigAddress(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, const uint8_t address) {

	if (SlotNr >= NrSlots) {
		cout << "Config address write failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Now write the address to the I2C 8-bit IO chip PCF8574AP, which has all 3 address lines on ground. See datasheet in folder datasheet.
	uint8_t write_value = address;
	CLA_TransmitI2CPort(/*I2C_port*/ 0, ConfigAddressIOExpanderAddress + Write, /*send_length*/ 1, &write_value, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz);

	// Now verify by reading the address back. Display an error message if no success.
	uint8_t read_back = 0;
	CLA_TransmitI2CPort(/*I2C_port*/ 0, ConfigAddressIOExpanderAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ 1, &read_back, I2CClockFrequencyInHz);

	if (read_back == address) {
		cout << "Config address write verification succeeded for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ": 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
	}
	else {
		cout << "Config address write verification failed for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ". Wrote 0x" << hex << static_cast<unsigned int>(address)
			<< ", read back 0x" << static_cast<unsigned int>(read_back) << dec << "." << endl;
	}

}


void ReadConfigAddress(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, uint8_t &address) {

	if (SlotNr >= NrSlots) {
		cout << "Config address read failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Read the address from the I2C 8-bit IO chip PCF8574AP, which has all 3 address lines on ground. See datasheet in folder datasheet.
	CLA_TransmitI2CPort(/*I2C_port*/ 0, ConfigAddressIOExpanderAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ 1, &address, I2CClockFrequencyInHz);
	
	// Display the address on cout.
	//cout << "Config address read succeeded for rack " << static_cast<unsigned int>(RackNr)
	//	<< ", slot " << static_cast<unsigned int>(SlotNr)
	//	<< ": 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
	cout << " Address: 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
}

json ReadConfiguration(const std::string& filename) {
	json config;

	//go over every rack slot and the backplane memory, constructs json file containing whole configuration, including addresses stored in EEPROMS, sequencer, rack and slot number of each board or rack beackplane.
	//store in file if filename is not empty.
	constexpr uint8_t MaxNrSequencers = 7;
	for (uint8_t SequencerNr = 0 ; SequencerNr < MaxNrSequencers; ++SequencerNr) {
		for (uint8_t RackNr = 0; RackNr <= MaxSupportedRackNr; ++RackNr) {
			for (uint8_t SlotNr = 0; SlotNr < NrSlots; ++SlotNr) {
				char buffer[EEPROMSizeInBytes] = {};
				size_t length = sizeof(buffer);
				ReadConfigEEPROM(SequencerNr, RackNr, SlotNr, buffer, length);
				uint8_t address = 0;
				ReadConfigAddress(SequencerNr, RackNr, SlotNr, address);

				size_t json_length = 0;
				while (json_length < length && buffer[json_length] != '\0') {
					++json_length;
				}

				std::string json_str(buffer, json_length);
				if (!json_str.empty()) {
					try {
						json slot_config = json::parse(json_str);
						slot_config["Address"] = address;
						if (SlotNr == NrSlots-1) {
							config["Sequencer" + std::to_string(SequencerNr)]["Rack" + std::to_string(RackNr)] = slot_config;
						}
						else {
							config["Sequencer" + std::to_string(SequencerNr)]["Rack" + std::to_string(RackNr)]["Slot" + std::to_string(SlotNr)] = slot_config;
						}
					}
					catch (const json::parse_error& e) {
						cout << "Failed to parse JSON from EEPROM of sequencer " << static_cast<unsigned int>(SequencerNr)
							<< ", rack " << static_cast<unsigned int>(RackNr)
							<< ", slot " << static_cast<unsigned int>(SlotNr)
							<< ": " << e.what() << endl;
					}
				}
			}
		}
	}

	if (!filename.empty()) {
		//add ".json" to the filename if it doesn't already end with ".json"
		std::string output_filename = filename;
		if (output_filename.size() < 5 || output_filename.substr(output_filename.size() - 5) != ".json") {
			output_filename += ".json";
		}
		std::ofstream file(output_filename);
		if (file.is_open()) {
			file << config.dump(4); // pretty print with 4 spaces indent
			file.close();
			cout << "Configuration saved to " << output_filename << endl;
		}
		else {
			cout << "Failed to open file " << output_filename << " for writing." << endl;
		}
	}
	
	return config;
}

json GetAutoConfigJSON(const std::string& filename) {

	json discovered_config = ReadConfiguration(filename);
	json auto_config = {
		{"FileOrigin", "This file is automatically generated by GetAutoConfigJSON. Do not edit it manually."},
		{"ConfigurationName", "AutoConfig"},
		{"PCSequenceBufferSize", 134217728},
		{"LineFrequency", 50},
		{"Sequencers", json::array()},
		{"Rack", json::array()},
		{"AnalogOutBoards16bit", json::array()},
		{"DigitalOutBoards", json::array()},
		{"SerialPortBoards", json::array()},
		{"DDSAD9854Boards", json::array()},
		{"DDSAD9858Boards", json::array()},
		{"DDSAD9958Boards", json::array()},
		{"AnalogInBoards12bit", json::array()}
	};

	for (uint8_t SequencerNr = 0; SequencerNr < CLA_GetNumberOfSequencers(); ++SequencerNr) {
		const std::string sequencer_key = "Sequencer" + std::to_string(SequencerNr);
		if (!discovered_config.contains(sequencer_key)) {
			continue;
		}

		for (uint8_t RackNr = 0; RackNr <= MaxSupportedRackNr; ++RackNr) {
			const std::string rack_key = "Rack" + std::to_string(RackNr);
			if (!discovered_config[sequencer_key].contains(rack_key)) {
				continue;
			}

			const json& rack_json = discovered_config[sequencer_key][rack_key];

			for (uint8_t SlotNr = 0; SlotNr < NrSlots; ++SlotNr) {
				const json* board_json = nullptr;
				if (SlotNr == NrSlots - 1) {
					if (rack_json.is_object() && rack_json.contains("Address")) {
						board_json = &rack_json;
					}
				}
				else {
					const std::string slot_key = "Slot" + std::to_string(SlotNr);
					if (rack_json.contains(slot_key)) {
						board_json = &rack_json[slot_key];
					}
				}

				if (board_json == nullptr || !board_json->is_object()) {
					continue;
				}

				const std::string model_prefix = GetModelPrefix(*board_json);
				if (model_prefix.empty()) {
					continue;
				}

				if (model_prefix == "AnalogOut16bit") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["StartAddress"] = (*board_json).value("Address", 24);
					entry["NumberChannels"] = (*board_json).value("NumberChannels", 4);
					entry["Signed"] = (*board_json).value("Signed", true);
					entry["MinVoltage"] = (*board_json).value("MinVoltage", -10.0);
					entry["MaxVoltage"] = (*board_json).value("MaxVoltage", 10.0);
					auto_config["AnalogOutBoards16bit"].push_back(entry);
				}
				else if (model_prefix == "DigitalOut") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 1);
					entry["NumberChannels"] = (*board_json).value("NumberChannels", 16);
					auto_config["DigitalOutBoards"].push_back(entry);
				}
				else if (model_prefix == "SerialPort") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 1);
					auto_config["SerialPortBoards"].push_back(entry);
				}
				else if (model_prefix == "DDSAD9854") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 132);
					entry["Version"] = (*board_json).value("Version", 2);
					if (board_json->contains("ExternalClockFrequency")) {
						entry["ExternalClockFrequency"] = (*board_json)["ExternalClockFrequency"];
					}
					else if (board_json->contains("ExternalClockFrequencyinMHz")) {
						entry["ExternalClockFrequency"] = (*board_json)["ExternalClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ExternalClockFrequencyinMHz"] = (*board_json)["ExternalClockFrequencyinMHz"];
					}
					else {
						entry["ExternalClockFrequency"] = 300000000.0;
					}
					entry["PLLReferenceMultiplier"] = (*board_json).value("PLLReferenceMultiplier", 1);
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9854Boards"].push_back(entry);
				}
				else if (model_prefix == "DDSAD9858") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 50);
					if (board_json->contains("ClockFrequency")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequency"];
					}
					else if (board_json->contains("ClockFrequencyinMHz")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ClockFrequencyinMHz"] = (*board_json)["ClockFrequencyinMHz"];
					}
					else {
						entry["ClockFrequency"] = 300000000.0;
					}
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9858Boards"].push_back(entry);
				}
				else if ((model_prefix == "DDSAD9958") || (model_prefix == "DDSAD9959")) {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 21);
					if (board_json->contains("ClockFrequency")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequency"];
					}
					else if (board_json->contains("ClockFrequencyinMHz")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ClockFrequencyinMHz"] = (*board_json)["ClockFrequencyinMHz"];
					}
					else {
						entry["ClockFrequency"] = 300000000.0;
					}
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9958Boards"].push_back(entry);
				}
				else if (model_prefix == "AnalogIn12bit") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["ChipSelect"] = (*board_json).value("ChipSelect", (*board_json).value("Address", 1));
					entry["Signed"] = (*board_json).value("Signed", false);
					entry["MinVoltage"] = (*board_json).value("MinVoltage", 0.0);
					entry["MaxVoltage"] = (*board_json).value("MaxVoltage", 10.0);
					auto_config["AnalogInBoards12bit"].push_back(entry);
				}
				else {
					json entry = *board_json;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					auto_config["Rack"].push_back(entry);
				}
			}
		}
	}

	if (!filename.empty()) {
		const std::string output_filename = MakeConfigOutputFilename(filename);
		std::ofstream file(output_filename);
		if (file.is_open()) {
			file << auto_config.dump(4);
			file.close();
			cout << "Auto configuration saved to " << output_filename << endl;
		}
		else {
			cout << "Failed to open file " << output_filename << " for writing." << endl;
		}
	}

	return auto_config;
}
