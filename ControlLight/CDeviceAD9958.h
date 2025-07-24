#pragma once

#include "CDevice.h"



class CAD9958;
class CDeviceAD9958 : public CDevice
{
public:
	double externalClockFrequency;
	double frequencyMultiplier;

public:
	CDeviceAD9958(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		double _externalClockFrequency,
		unsigned int _frequencyMultiplier
	);
	virtual ~CDeviceAD9958();
	virtual bool SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);

	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();

	virtual bool SetFrequency(uint8_t channel, double Frequency);
	virtual bool SetFrequencyTuningWord(uint8_t channel, uint64_t FrequencyTuningWord);
	virtual bool SetPhase(uint8_t channel, double Phase);
	virtual bool SetPower(uint8_t channel, double Power);
	virtual bool SetAttenuation(uint8_t channel, double Power);
private:
	CAD9958* MyAD9958;
};
