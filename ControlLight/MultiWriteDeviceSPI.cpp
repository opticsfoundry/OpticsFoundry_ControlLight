// MultiWriteDevice.cpp: implementation of the CMultiWriteDeviceSPI class.
//
//////////////////////////////////////////////////////////////////////

#include "std.h"
#include "control.h"
#include "MultiWriteDeviceSPI.h"

#include "CDeviceSequencer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiWriteDeviceSPI::CMultiWriteDeviceSPI(unsigned short aBus, unsigned long aBaseAddress, CDeviceSequencer* _MyDeviceSequencer)
	: CMultiWriteDevice()
{
	QSPIMode = false;
	MyDeviceSequencer = _MyDeviceSequencer;
	ForceWriting=false;	
	Enabled=true;
	MultiIOAddress=0;
	Bus = aBus;
	BaseAddress = aBaseAddress;
	MultiIOAddress = Bus + (unsigned char)BaseAddress;

	for (unsigned int i = 0; i < MultiWriteDeviceSPIMaxBusBuffer; i++) BusBuffer[i] = 0;
	BusBufferStart = 0;
	BusBufferEnd = 0;
	BusBufferLength = 0;

	ControlRegisterContent = 0;
	SPI_CS_bit = 0;
	SDIO_0_bit = 0;
	SPI_SCLK_bit = 0;
	/*if (DebugSPICommunication) {
		CString filename;
		filename.Format(*DebugFilePath + "DebugSPICommunication_%u_%u.txt", Bus, BaseAddress);
		DebugFile = new ofstream(filename, ios::out);
	}
	else DebugFile = NULL;*/
}

CMultiWriteDeviceSPI::~CMultiWriteDeviceSPI()
{
/*	if (DebugFile) {
		DebugFile->close();
		delete DebugFile;
		DebugFile = NULL;
	}*/
}

void CMultiWriteDeviceSPI::AddToBusBuffer(unsigned short value) {
	if (!Enabled) return;
	if (BusBufferLength >= MultiWriteDeviceSPIMaxBusBuffer) {
		CString buf;
		buf.Format("CMultiWriteDeviceSPI::AddToBusBuffer : Bus Buffer exceeded (%d)", MultiWriteDeviceSPIMaxBusBuffer);
		ControlMessageBox(buf);
		return;
	}
	BusBuffer[BusBufferEnd] = value;
	BusBufferLength++;
	BusBufferEnd++;
	if (BusBufferEnd >= MultiWriteDeviceSPIMaxBusBuffer) BusBufferEnd = 0;
}

bool CMultiWriteDeviceSPI::WriteToBus()
{
    if (!Enabled) return false;
    if (BusBufferLength == 0) return false;
	MyDeviceSequencer->WriteBusAddressAndDataToBuffer(MultiIOAddress, BusBuffer[BusBufferStart]);
	BusBufferStart++;
	if (BusBufferStart >= MultiWriteDeviceSPIMaxBusBuffer) BusBufferStart = 0;
	BusBufferLength--;
    return true;
}

void CMultiWriteDeviceSPI::WriteAllToBus()
{
	while (WriteToBus()) {
	}
}

void CMultiWriteDeviceSPI::SetControlRegister(unsigned char start_bit, unsigned char nr_bits, unsigned short value, bool write_immediately) {
	unsigned short mask = (1 << nr_bits) - 1;
	unsigned short shifted_value = (value & mask) << start_bit;
	ControlRegisterContent = (ControlRegisterContent & ~(mask << start_bit)) | shifted_value;
	if (write_immediately) AddToBusBuffer(ControlRegisterContent);
}

void CMultiWriteDeviceSPI::SetSPIPortBits(unsigned char _SPI_CS_bit, unsigned char  _SDIO_0_bit, unsigned char _SDIO_1_bit, unsigned char _SDIO_2_bit, unsigned char _SDIO_3_bit, unsigned char _SPI_SCLK_bit) {
	SPI_CS_bit = _SPI_CS_bit;
	SDIO_0_bit = _SDIO_0_bit;
	SDIO_1_bit = _SDIO_1_bit;
	SDIO_2_bit = _SDIO_2_bit;
	SDIO_3_bit = _SDIO_3_bit;
	SPI_SCLK_bit = _SPI_SCLK_bit;
}

void CMultiWriteDeviceSPI::SetQSPIMode(bool OnOff) {
	QSPIMode = OnOff;
}

void CMultiWriteDeviceSPI::SetSPIClock(bool clock, bool write_immediately) {
	SetControlRegister(SPI_SCLK_bit, 1, (clock) ? 1 : 0, write_immediately);
}

void CMultiWriteDeviceSPI::SetSPIChipSelect(bool cs) {
	SetControlRegister(SPI_CS_bit, 1, (cs) ? 1 : 0);
}
void CMultiWriteDeviceSPI::SetSPIDataOut(bool data) {
	SetControlRegister(SDIO_0_bit, 1, (data) ? 1 : 0);
}

std::string format_binary_64(unsigned __int64 data, unsigned int number_of_bits_out) {
	std::string result;
	for (int i = number_of_bits_out - 1; i >= 0; --i) {
		result += (data & (1ULL << i)) ? '1' : '0';
		if (i % 4 == 0 && i != 0) result += ' ';
	}
	return result;
}

void CMultiWriteDeviceSPI::WriteSPIBitBanged(unsigned int number_of_bits_out, unsigned __int64 data) {
	//ToDo: make data_sent calculation more efficient
	unsigned __int64 data_sent = 0;
	//This could easily be expanded into a 4-bit SPI interface
	SetSPIChipSelect(false);
	SetSPIClock(false);
	if (QSPIMode) {
		unsigned int bit_to_send_0;
		unsigned int bit_to_send_1;
		unsigned int bit_to_send_2;
		unsigned int bit_to_send_3;
		for (int i = 0; i < number_of_bits_out; i = i + 4) {
			bit_to_send_3 = data & 0x1;
			data >>= 1;
			data_sent <<= 1;
			data_sent |= bit_to_send_3;

			bit_to_send_2 = data & 0x1;
			data >>= 1;
			data_sent <<= 1;
			data_sent |= bit_to_send_2;

			bit_to_send_1 = data & 0x1;
			data >>= 1;
			data_sent <<= 1;
			data_sent |= bit_to_send_1;

			bit_to_send_0 = data & 0x1;
			data >>= 1;
			data_sent <<= 1;
			data_sent |= bit_to_send_0;

			SetControlRegister(SDIO_0_bit, 1, bit_to_send_0, /*write_immediately*/ false);
			SetControlRegister(SDIO_1_bit, 1, bit_to_send_1, /*write_immediately*/ false);
			SetControlRegister(SDIO_2_bit, 1, bit_to_send_2, /*write_immediately*/ false);
			SetControlRegister(SDIO_3_bit, 1, bit_to_send_3, /*write_immediately*/ true);

			SetSPIClock(true);
			SetSPIClock(false, /*write_immediately*/ false);
		}

	}
	else {
		unsigned int bit_to_send = 0x0;
		for (int i = 0; i < number_of_bits_out; i++) {
			data_sent <<= 1;
			bit_to_send = data & 0x1;
			data >>= 1;
			SetSPIDataOut(bit_to_send > 0);
			data_sent |= bit_to_send;
			SetSPIClock(true);
			SetSPIClock(false, /*write_immediately*/ false);
		}
	}
	SetSPIChipSelect(true);

	/*if (DebugFile) {
		CString buf;
		unsigned long data_low = data_sent & 0xFFFFFFFF;
		unsigned long data_high = (data_sent >> 32) & 0xFFFFFFFF;
		buf.Format("Wrote %d bits, data = %08X %08X = first bit sent %s last bit sent", number_of_bits_out, data_high, data_low, format_binary_64(data_sent, number_of_bits_out).c_str());
		*DebugFile << buf << endl;
	}*/
}