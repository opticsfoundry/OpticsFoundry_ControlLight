#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceRack.h"


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


//This is the rack backplane, which has an arbiter to select one slot as input slot
CDeviceRack::CDeviceRack(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyRackAddress
) : CDevice(_MySequencer, _MyRackAddress, "Rack") {
	MyAddress = 0xFE;
	MyRackAddress = _MyRackAddress;
	LastValue = 0;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) MySequencer->ParallelBusDeviceList[MyAddress] = this;
	else {
		std::ostringstream oss;
		oss << "CDeviceRack::CDeviceRack: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		return;
	}
}


bool CDeviceRack::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (SubAddress > 0) return false;
	if (DataLength_in_bit > 15) return false;
	if ((StartBit + DataLength_in_bit) > 16) return false;
	if (Data == nullptr) return false;
	if (DataLength_in_bit == 1)
	{
		bool On = Data[0] > 0;
		if (On) {
			LastValue |= (1 << StartBit);
		}
		else {
			LastValue &= ~(1 << StartBit);
		}
	}
	else if (DataLength_in_bit == 16) {
		uint16_t* data = (uint16_t*)(Data);
		LastValue = data[0];
	}
	else {
		uint16_t* data = (uint16_t*)(Data);
		LastValue &= ~(0xFFFF << StartBit);
		LastValue |= (data[0] << StartBit);
	}

	uint32_t content = MyAddress + (LastValue << 8);
	MySequencer->AddBusCommandToSequence(content);
	return true;
}
