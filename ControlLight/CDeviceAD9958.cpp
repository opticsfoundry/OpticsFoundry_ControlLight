
#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAD9958.h"
#include "AD9958.h"


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




CDeviceAD9958::CDeviceAD9958(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyAddress,
	double _externalClockFrequency,
	unsigned int _frequencyMultiplier
) : CDevice(_MySequencer, _MyAddress, "AD9958") {
	externalClockFrequency = _externalClockFrequency;
	frequencyMultiplier = _frequencyMultiplier;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) {
		MySequencer->ParallelBusDeviceList[MyAddress] = this;
		MyAD9958 = new CAD9958(/*bus*/0, MyAddress, externalClockFrequency, frequencyMultiplier, MySequencer);
	}
	else {
		std::ostringstream oss;
		oss << "CDeviceAD9958::CDeviceAD9958: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		MyAD9958 = nullptr;
		return;
	}
}

CDeviceAD9958::~CDeviceAD9958() {
	if (MyAD9958) {
		delete MyAD9958;
		MyAD9958 = nullptr;
	}
}


bool CDeviceAD9958::SetValue(const unsigned int& SubAddress, const uint8_t* data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//ToDo: do range checks
	double* dValueptr = (double*)data;
	double dValue = *dValueptr;

	switch (SubAddress) {
	case 0:if (DataLength_in_bit == 64) MyAD9958->SetFrequencyCh0(dValue); break;
	case 1:if (DataLength_in_bit == 64) MyAD9958->SetFrequencyCh1(dValue); break;
	case 2:if (DataLength_in_bit == 64) MyAD9958->SetIntensityCh0(dValue); break;
	case 3:if (DataLength_in_bit == 64) MyAD9958->SetIntensityCh1(dValue); break;
	case 4:if (DataLength_in_bit == 64) MyAD9958->SetPhaseOffsetCh0(dValue); break;
	case 5:if (DataLength_in_bit == 64) MyAD9958->SetPhaseOffsetCh1(dValue); break;
	case 6:if (DataLength_in_bit == 32) MyAD9958->SetFrequencyTuningWordCh0((uint32_t)dValue); break;
	case 7:if (DataLength_in_bit == 32) MyAD9958->SetFrequencyTuningWordCh1((uint32_t)dValue); break;

	default: return false;//To do: throw exception
	}
	MyAD9958->WriteAllToBus();
	return true;
}

bool CDeviceAD9958::SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//Here we could give direct access to the registers, e.g. as specified in the datasheet.
	MyAD9958->SetRegisterBits(
		/*RegisterNr*/SubAddress,
		/*BitNr*/StartBit,
		/*BitLength*/DataLength_in_bit,
		/*Value*/Data[0],
		/*GetValue*/false,
		/*DoIOUpdate*/true);
	return true;
}


bool CDeviceAD9958::SetFrequency(uint8_t channel, double Frequency) {
	MyAD9958->SetFrequency(channel, Frequency);
	return true;
}

bool CDeviceAD9958::SetFrequencyTuningWord(uint8_t channel, uint64_t FrequencyTuningWord) {
	MyAD9958->SetFrequencyTuningWord(channel, FrequencyTuningWord);
	return true;
}

bool CDeviceAD9958::SetPhase(uint8_t channel, double Phase) {
	MyAD9958->SetPhaseOffset(channel, Phase);
	return true;
}

bool CDeviceAD9958::SetPower(uint8_t channel, double Power) {
	MyAD9958->SetAmplitude(channel, Power);
	return true;
}

bool CDeviceAD9958::SetAttenuation(uint8_t channel, double Attenuation) {
	MyAD9958->SetAttenuation(channel, Attenuation);
	return true;
}


