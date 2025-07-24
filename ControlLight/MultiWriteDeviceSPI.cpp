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
	SPI_MOSI_bit = 0;
	SPI_SCLK_bit = 0;
}

CMultiWriteDeviceSPI::~CMultiWriteDeviceSPI()
{

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

void CMultiWriteDeviceSPI::SetSPIPortBits(unsigned char _SPI_CS_bit, unsigned char _SPI_MOSI_bit, unsigned char _SPI_SCLK_bit) {
	SPI_CS_bit = _SPI_CS_bit;
	SPI_MOSI_bit = _SPI_MOSI_bit;
	SPI_SCLK_bit = _SPI_SCLK_bit;
}

void CMultiWriteDeviceSPI::SetSPIClock(bool clock, bool write_immediately) {
	SetControlRegister(SPI_SCLK_bit, 1, (clock) ? 1 : 0, write_immediately);
}

void CMultiWriteDeviceSPI::SetSPIChipSelect(bool cs) {
	SetControlRegister(SPI_CS_bit, 1, (cs) ? 1 : 0);
}

void CMultiWriteDeviceSPI::SetSPIDataOut(bool data) {
	SetControlRegister(SPI_MOSI_bit, 1, (data) ? 1 : 0);
}

void CMultiWriteDeviceSPI::WriteSPIBitBanged(unsigned int number_of_bits_out, unsigned __int64 data) {
	//This could easily be expanded into a 4-bit SPI interface
	SetSPIChipSelect(false);
	SetSPIClock(false);
	unsigned int bit_to_send = 0x0;
	for (int i = 0; i < number_of_bits_out; i++) {
		bit_to_send = data & 0x1;
		data >>= 1;
		SetSPIDataOut((bit_to_send) > 0);

		SetSPIClock(true);
		SetSPIClock(false, /*write_immediately*/ true); //if it works, test if it also works with write_immediately=false, as that would be faster
	}
	SetSPIChipSelect(true);
}