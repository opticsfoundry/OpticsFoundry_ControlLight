#pragma once

#include <cstddef>
#include <cstdint>

void WriteConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length);
void ReadConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length);
void WriteConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address);
void ReadConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address);
