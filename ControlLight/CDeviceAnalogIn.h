#pragma once

#include "CDevice.h"


class CDeviceAnalogIn12bit : public CDevice
{
public:
	bool signedValue;
	double minVoltage;
	double maxVoltage;
private:
	uint8_t ChannelNumber;
	uint32_t NumberOfDataPoints;
	double DelayBetweenDataPoints_in_ms;
public:
	CDeviceAnalogIn12bit(
		CDeviceSequencer* _MySequencer,
		unsigned int _chipSelect,
		bool _signedValue,
		double _minVoltage,
		double _maxVoltage
	);
	virtual ~CDeviceAnalogIn12bit() = default;
	bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();
};