#pragma once
#include "MultiWriteDeviceSPI.h"
#include <cstdint> // Include this header to define uint8_t


class CDeviceSequencer;
//#include "stdafx.h"

//#include <stdint.h>
//#include <stdbool.h>

/*! typedef enum E_AD9958_PWR
    \brief DDS Power modes.
*/
typedef enum
{
  E_AD9958_PWR_POWERED, /*!< Fully powered */
  E_AD9958_PWR_PARTIAL, /*!< Fast recovery   - Powers down digital logic and DAC digital logic */
  E_AD9958_PWR_DOWN     /*!< Full power down - Powers down all functions including DAC and PLL */
} E_AD9958_PWR;

/*! typedef enum E_AD9958_CHANNEL
    \brief DDS Channel.
*/
typedef enum
{
  E_AD9958_CHANNEL_BOTH, /*!< Select both channels */
  E_AD9958_CHANNEL_0,    /*!< Select channel 0 only */
  E_AD9958_CHANNEL_1     /*!< Select channel 1 only */
} E_AD9958_CHANNEL;

// Pin defines - Change to suit
#define PWR_DWN_CTL_PORT     GPIOA
#define PWR_DWN_CTL_PIN      GPIO_PIN_1
#define MASTER_RESET_PORT    GPIOA
#define MASTER_RESET_PIN     GPIO_PIN_1
#define IO_UPDATE_PORT       GPIOA
#define IO_UPDATE_PIN        GPIO_PIN_1
#define SDIO_3_PORT          GPIOA
#define SDIO_3_PIN           GPIO_PIN_1
#define CS_GPIO_PORT         GPIOA
#define CS_PIN               GPIO_PIN_1

const unsigned int SPIBufferLength = 14;
constexpr unsigned int AD9958NumberOfRegisters = 48;

constexpr unsigned int AD9958SPIBufferLength = 14;

class CAD9958 : public CMultiWriteDeviceSPI
{
private:
    uint8_t SPIBuffer[AD9958SPIBufferLength];
    unsigned char BytesToTransmit;
    double ClockSpeed;
    double InputClockSpeed;
    double MaxFrequency;
    double FrequencyScale;
    bool UpdateRegistersModeAutomatic;
    
    uint32_t AktValueContents[AD9958NumberOfRegisters]; //keeps track of Value, contains value after bus buffer has been finished to be written out
    unsigned char WritePrecision[AD9958NumberOfRegisters];
public:
    double FrequencyMultiplier;

public:
    CAD9958(unsigned short aBus, unsigned long aBaseAddress, double aExternalClockSpeed, double aFrequencyMultiplier, CDeviceSequencer* _MyDeviceSequencer);
    
    virtual ~CAD9958();
    void SetIOUpdate(bool OnOff);
    void SetReset(bool OnOff);
    void SetPowerDown(bool OnOff);
    void SetSyncIO(bool OnOff);
    void SetP0(bool OnOff);
    void SetP1(bool OnOff);
    void SetP2(bool OnOff);
    void SetP3(bool OnOff);
    void SetSpare2(bool OnOff);
    void SetSpare1(bool OnOff);
    void SetSpare0(bool OnOff);
    void SetSDIO_3(bool OnOff);
    void SetSDIO_2(bool OnOff);
    void SetSDIO_1(bool OnOff);

    void Initialise(void);
    void SyncIO(void);
//    void Initialise_GPIO(void);
    //void TestFunctions(void);

    /*! void SetFrequency(uint8_t channel, float frequency)
        \brief Set the frequency output on a given channel of the DDS.

		\param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param frequency Desired output frequency in MHz.
    */
    void SetFrequency(uint8_t channel, float frequency);


    /*! void SetAmplitude(uint8_t channel, float amplitude)
        \brief Set the amplitude scale factor on a given channel of the DDS.

        \param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param amplitude Desired amplitude scale factor (0-100).
    */
    void SetAmplitude(uint8_t channel, float amplitude);

	void SetAttenuation(uint8_t channel, double attenuation);

    /*! void DDS_Set_Phase_Offset(uint8_t channel, float phase)
        \brief Set the phase offset on a given channel of the DDS.

        \param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param phase Desired phase offset.
    */
    void SetPhaseOffset(uint8_t channel, float phase);

    /*! void DDS_Set_Power_State(E_AD9958_PWR state)
        \brief Set the power state of the DDS.

        \param state Desired power state.
    */
    void SetPowerState(E_AD9958_PWR state);


    void SetFrequencyCh0(double frequency);
    void SetFrequencyCh1(double frequency);
    void SetIntensityCh0(double Intensity);
	void SetAttenuationCh0(double Attenuation);
	void SetAttenuationCh1(double Attenuation);
    void SetIntensityCh1(double Intensity);
    void SetPhaseOffsetCh0(double phase);
    void SetPhaseOffsetCh1(double phase);

    void SelectChannelCh0() { SetRegisterBits(0, 6, 2, 1); }
    void SelectChannelCh1() { SetRegisterBits(0, 6, 2, 2); }
    void SelectChannelCh0And1() { SetRegisterBits(0, 6, 2, 3); }

    void SetFrequencyTuningWord(uint32_t ftw) { SetValue(4, ftw); }
    void SetFrequencyTuningWord(uint8_t channel, uint32_t ftw) { SetWriteChannels(channel);  SetValue(4, ftw); }
    void SetFrequencyTuningWordCh0(uint32_t ftw) {
        SelectChannelCh0(); SetFrequencyTuningWord(ftw);
    }
    void SetFrequencyTuningWordCh1(uint32_t ftw) {
        SelectChannelCh1(); SetFrequencyTuningWord(ftw);
    }

private:
    void IO_Update_Toggle();
    //void Reset(void);
    void Disable_SYNC_CLK(void);



    void SetFrequency_SPI(uint8_t channel, float frequency);
    void SetAmplitude_SPI(uint8_t channel, uint32_t amplitude);
    void SetIntensity_SPI(uint8_t channel, uint32_t intensity);
    void SetPhaseOffset_SPI(uint8_t channel, float phase);
    void SetPowerDown_partial_SPI();
    void SetPowerDown_full_SPI();
    uint8_t GetChannelBits(E_AD9958_CHANNEL channel);
    uint32_t calcFTW(float frequency);
    float calcFrequency(uint32_t FTW);
    uint32_t calcPOW(float degrees);
    void Dev_Select(void);
    void Dev_Deselect(bool read = false, uint8_t number_of_bits_in = 0);
    void SPI_Transmit_Byte(uint8_t byte);
public:
    void SetWriteChannels(uint8_t channels);
    void SetWriteChannels(bool channel0, bool channel1);
    //Functions for CMultiWriteDevice
    void MasterReset();
    
    //bool SetControlBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue);
    bool SetRegisterBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue = false, bool DoIOUpdate = true);
    uint32_t SetRegisterBits(unsigned char RegisterNr, unsigned char LowestBitNr, unsigned char NrBits, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true);
    uint32_t SetValueDirect(unsigned char RegisterNr, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true);
    uint32_t SetValue(unsigned char RegisterNr, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true);
    //void LoadLatches();
    //void UpdateRegisters();
};
