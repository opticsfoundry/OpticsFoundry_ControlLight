// AD9958.cpp: implementation of the CAD9958 class.
//
//////////////////////////////////////////////////////////////////////

#include "std.h"
#include "control.h"
#include "AD9958.h"
#include "MultiWriteDevice.h"
#include "control.h"
#include "ControlAPI.h"
#include "CDeviceSequencer.h"




// Register addresses, writing to (MSB is 0 for a write)
#define CSR     0x00            //!< Channel select register            1 Byte
#define FR1     0x01            //!< Function register 1                3 Bytes
#define CFR     0x03            //!< Channel Function register          3 Bytes
#define CFTW0   0x04            //!< Channel Frequency Tuning Word      4 Bytes
#define CPOW    0x05            //!< Channel Phase Offset Word          2 Bytes
#define ACR     0x06            //!< Amplitude Control Register         3 Bytes







//AktValueContents[0..2] is the control register
//AktValueContents[3..24] is the channel 0 register map. This function allows writing to these registers, which writes to channel 0 and/or 1 depending on bit 6 and 7 of the CSR. When reading it provides values of channel 0
//AktValueContents[25..46] is the channel 1 register map, and can't be written by this function. When reading it provides values of channel 1
//AktValueContents[47] is the digital out port, and can't be written by this function
uint32_t AD9958MasterResetValueContents[AD9958NumberOfRegisters] =      { 0xF0,   0,   0,   0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };
unsigned char AD9958ValueLength[AD9958NumberOfRegisters] =              {    1,   3,   2,          3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,           3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   2 };
unsigned char AD9958ValueBaseAddress[AD9958NumberOfRegisters] =         { 0x00,0x01,0x02,       0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,        0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19 };
const unsigned long AD9958ValueNotInBusBuffer = 99999;


// Public Functions --------------------------------------------------

void AD9958DoNothing(bool bit) {}

void AD9958WaitNothing(double time) {}

uint32_t AD9958WriteReadSPINothing(unsigned int chip_select, unsigned int number_of_bits_out, unsigned __int64 data_high, unsigned __int64 data_low, unsigned int number_of_bits_in) { return 0; }



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAD9958::CAD9958(unsigned short aBus, unsigned long aBaseAddress, double aExternalClockSpeed, double aFrequencyMultiplier, CDeviceSequencer* _MyDeviceSequencer)
    : CMultiWriteDeviceSPI(aBus, aBaseAddress, _MyDeviceSequencer) {
    SetSPIPortBits(/* SPI_CS_bit*/14, /*SDIO_0_bit = SPI_MOSI_bit*/ 12, /*SDIO_1_bit = */ 11, /*SDIO_2_bit */ 10, /*SDIO_3_bit = Sync_IO */ 9, /*SPI_SCLK_bit*/ 13);  //SDIO_3 is the sync_io pin
    BytesToTransmit = 0;
    
    InputClockSpeed = 1E6 * aExternalClockSpeed; //conversion MHz to Hz
    FrequencyMultiplier = aFrequencyMultiplier;  //This is the external frequency doubler option
    ClockSpeed = InputClockSpeed;
    MaxFrequency = ClockSpeed * 0.45E-6;
    //4294967296=2^32
    FrequencyScale = 4294967296.0 * ((1.0 / FrequencyMultiplier) * 1.0E6 / ClockSpeed);
    
    
    for (int i = 0; i < AD9958NumberOfRegisters; i++) {
        AktValueContents[i] = AD9958MasterResetValueContents[i];
        WritePrecision[i] = AD9958ValueLength[i];
    }
    UpdateRegistersModeAutomatic = true;
}

CAD9958::~CAD9958() {

}

//AD9958 DDS0 digital port control lines

void CAD9958::SetIOUpdate(bool OnOff) {
    SetControlRegister(15, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSDIO_3(bool OnOff) {
    SetControlRegister(9, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSDIO_2(bool OnOff) {
    SetControlRegister(10, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSDIO_1(bool OnOff) {
    SetControlRegister(11, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSyncIO(bool OnOff) {  //identical to SDIO_3
    SetControlRegister(2, 1, (OnOff) ? 1 : 0); //ToDo: check this bit, it should probably be 9, not 2.
}

void CAD9958::SetP0(bool OnOff) {
    SetControlRegister(8, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetP1(bool OnOff) {
    SetControlRegister(7, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetP2(bool OnOff) {
    SetControlRegister(6, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetP3(bool OnOff) {
    SetControlRegister(5, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetReset(bool OnOff) {
    SetControlRegister(4, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetPowerDown(bool OnOff, bool write_immediately) {
    SetControlRegister(3, 1, (OnOff) ? 1 : 0, write_immediately);
}

void CAD9958::SetSpare2(bool OnOff) {
    SetControlRegister(2, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSpare1(bool OnOff) {
    SetControlRegister(1, 1, (OnOff) ? 1 : 0);
}

void CAD9958::SetSpare0(bool OnOff) {
    SetControlRegister(0, 1, (OnOff) ? 1 : 0);
}


void CAD9958::SyncIO(void) {
    SetSyncIO(true);
    SetSyncIO(false);
}

// Select the output frequency of desired channel/s
void CAD9958::SetFrequency(uint8_t channel, float frequency) //frequency in MHz
{
  float freq_Hz = (frequency * 1000000.0);
  SetFrequency_SPI(channel, freq_Hz);
}

// Set the amplitude scale factor (max 1023)
void CAD9958::SetAmplitude(uint8_t channel, float amplitude) //amplitude in %
{
  uint32_t amp = (uint32_t)(amplitude * 10.23);
  if (amp > 1023) amp = 1023;
  SetAmplitude_SPI(channel, amp);
}

void CAD9958::SetAttenuation(uint8_t channel, double Attenuation) {
    if (Attenuation > 0) Attenuation = 0;
    double Intensity = (unsigned int)(0x03FF * pow(10.0, Attenuation / 20.0));
	SetAmplitude(channel, Intensity);
}

// Offset the desired channel from the other
void CAD9958::SetPhaseOffset(uint8_t channel, float phase) //phase in degree
{
  SetPhaseOffset_SPI(channel, phase);
}



// DDS Power control
void CAD9958::SetPowerState(E_AD9958_PWR state)
{
  switch(state)
  {
    case E_AD9958_PWR_POWERED:
    {
      // Set PWR-DWN-CTL pin LOW to disable power-down control
      SetPowerDown(false);
    } 
    break;
    
    case E_AD9958_PWR_PARTIAL:
    {
      SetPowerDown_partial_SPI();
      
      // Set PWR-DWN-CTL pin HIGH to enable the selected power-down mode
      SetPowerDown(true);
    } 
    break;
    
    case E_AD9958_PWR_DOWN:
    {
      SetPowerDown_full_SPI();
      
      // Set PWR-DWN-CTL pin HIGH to enable the selected power-down mode
      SetPowerDown(true);
    } 
    break;
  
    default:
      break; 
  }
}



void CAD9958::SetFrequencyCh0(double frequency) { 
    SetFrequency_SPI(1, frequency); 
}

void CAD9958::SetFrequencyCh1(double frequency) { 
    SetFrequency_SPI(2, frequency); 
}

void CAD9958::SetIntensityCh0(double Intensity) {
    SetIntensity_SPI(1, Intensity);
}

void CAD9958::SetIntensityCh1(double Intensity) {
    SetIntensity_SPI(2, Intensity);
}

void CAD9958::SetAttenuationCh0(double Attenuation) {
    SetAttenuation(1, Attenuation);
}

void CAD9958::SetAttenuationCh1(double Attenuation) {
    SetAttenuation(2, Attenuation);
}

void CAD9958::SetPhaseOffsetCh0(double phase) { 
    SetPhaseOffset_SPI(1, phase); 
}

void CAD9958::SetPhaseOffsetCh1(double phase) { 
    SetPhaseOffset_SPI(2, phase); 
}

void CAD9958::SetQSPIMode(bool OnOff) {
    /*
    from datasheet:
    Note that when programming the device for 4-bit serial mode, it is important to keep the SDIO_3 pin at Logic 0 until the device is
    programmed out of the single-bit serial mode. Failure to do so can result in the serial I/O port controller being out of sequence.
    */
    SetSDIO_3(false);
    SetRegisterBits(/*RegisterNr*/CSR, /*LowestBitNr*/ 1, /* NrBits*/ 2, (OnOff) ? 3 : 0, /*GetValue*/false, /*DoIOUpdate*/ true, /*ForceWrite*/ false);
    CMultiWriteDeviceSPI::SetQSPIMode(OnOff);
}

// Private Functions --------------------------------------------------

/*! static uint8_t GetChannelBits(E_AD9958_CHANNEL channel)
    \brief Convert enum to bit pattern

    \param channel Desired DDS channel.
    \return Register bit pattern for a given channel.
*/


uint8_t CAD9958::GetChannelBits(E_AD9958_CHANNEL channel)
{
    uint8_t Channel = 0;

    switch (channel)
    {
    case E_AD9958_CHANNEL_BOTH:
    {
        Channel = 3;
    }
    break;
    case E_AD9958_CHANNEL_0:
    {
        Channel = 1;
    }
    break;
    case E_AD9958_CHANNEL_1:
    {
        Channel = 2;
    }
    break;
    default:
        break;
    }

    return Channel;
}


#define PHASE_OFF_BITS  16384.0         //!< 2^14
#define MAX_DEGREES     360.0           //!< For phase calculation

/*! static uint32_t calcPOW(f32 degrees)
    \brief Calculates the Phase Offset Word.

    \param degrees (0-360).
    \return Phase Offset Word.
*/
uint32_t CAD9958::calcPOW(float degrees)
{
    uint32_t pow = 0;

    pow = (uint32_t)(degrees * (PHASE_OFF_BITS / MAX_DEGREES));

    return pow;
}


#define TUNEWORD_BITS   4294967296.0    //!< 2^32

/*! static uint32_t calcFTW(float frequency)
    \brief Calculates the Frequency Tuning Word.

    \param frequency Desired frequency.
    \return Frequency Tuning Word.
*/
uint32_t CAD9958::calcFTW(float frequency)
{
    uint32_t setPoint = 0;

    setPoint = (uint32_t)(frequency / (ClockSpeed / TUNEWORD_BITS));

    return setPoint;
}


float CAD9958::calcFrequency(uint32_t FTW)
{
    float frequency = 0;
    frequency = (FTW * (ClockSpeed / TUNEWORD_BITS));
    return frequency;
}

/*! static void IO_Update_Toggle()
    \brief Toggles I/O Update pin on DDS to force register changes.

    \warning Minimum pulse width of > 1 SYNC_CLK period (~160ns) required.
*/
void CAD9958::IO_Update_Toggle(void)
{
    SetIOUpdate(true);
    // Minimum pulse width needs to be > 1 SYNC_CLK period (~160ns)
    // ToDo: NEED TO MAKE SURE THIS IS THE CASE FOR YOUR PLATFORM.
    SetIOUpdate(false);
   
}


// SPI Functions --------------------------------------------------


void CAD9958::Dev_Select(void)
{
    BytesToTransmit = 0;
}

void CAD9958::SPI_Transmit_Byte(uint8_t byte) {
    SPIBuffer[BytesToTransmit] = byte;
    if (BytesToTransmit < (SPIBufferLength - 1)) BytesToTransmit++; else ControlMessageBox("CAD9958::SPI_Transmit_Byte : too many bytes");
}

uint8_t AD9958reverseBits(uint8_t num)
{
    uint8_t count =  8 - 1;
    uint8_t reverse_num = num;

    num >>= 1;
    while (num) {
        reverse_num <<= 1;
        reverse_num |= num & 1;
        num >>= 1;
        count--;
    }
    reverse_num <<= count;
    return reverse_num;
}

void CAD9958::Dev_Deselect(bool read, uint8_t number_of_bits_in)
{
    uint8_t InvertedSPIBuffer[SPIBufferLength];
    for (int i = 0; i < SPIBufferLength; i++)InvertedSPIBuffer[i] = 0;
    uint8_t DataToTransmit = BytesToTransmit - 1;
    InvertedSPIBuffer[0] = SPIBuffer[0];
    for (uint8_t i = 0; i < DataToTransmit; i++) {
        InvertedSPIBuffer[DataToTransmit - i] = SPIBuffer[i + 1];
    }
    //AD9958 has MSB first as default mode.
    //the FPGA SPI uses LSB first -> we have to sort the bits
    //As we never transfer more than 4 bytes we can use data_low and set data_high to zero
    unsigned __int64 data_low = 0;
    for (unsigned char i = 0; i < BytesToTransmit; i++) {

        //data_low = data_low | (AD9958reverseBits(SPIBuffer[i]) << (8 * i));
        unsigned __int64 help = AD9958reverseBits(InvertedSPIBuffer[i]);
        data_low |= (help << (8 * i));
    }
    WriteSPIBitBanged(/*  number_of_bits_out*/BytesToTransmit * 8, /*unsigned __int64*/ data_low);
}

void CAD9958::Initialise(void)
{
    MasterReset();
}

void CAD9958::Disable_SYNC_CLK(void)
{
	SetRegisterBit(/*RegisterNr*/FR1, /*BitNr*/ 6, /*Value*/ false, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9958::SetFrequency_SPI(uint8_t channel, float frequency)
{  
    float freq_Hz = (frequency * 1000000.0);
    uint32_t FTW = calcFTW(freq_Hz);
    SetWriteChannels(channel);
	SetValueDirect(CFTW0, FTW,/*GetValue*/ false, /*DoIOUpdate*/true);
}

#define MAN_AMP         0x1000    //!< Enable manual amplitude control
void CAD9958::SetAmplitude_SPI(uint8_t channel, uint32_t amplitude)
{
    SetWriteChannels(channel);
    //SetValueDirect(ACR, (MAN_AMP << 8), /*GetValue*/ false, /*DoIOUpdate*/ false);
    SetValueDirect(ACR, (amplitude & 0x3ff) | MAN_AMP, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9958::SetIntensity_SPI(uint8_t channel, uint32_t intensity)
{
    double Amplitude = sqrt(intensity) * 102.3;
    if (Amplitude > 1023) Amplitude = 1023;
    if (Amplitude < 0) Amplitude = 0;
    SetAmplitude_SPI(channel, Amplitude);
}


void CAD9958::SetPhaseOffset_SPI(uint8_t channel, float phase)
{
    uint32_t POW = calcPOW(phase);
    SetWriteChannels(channel);
	SetValueDirect(CPOW, POW, /*GetValue*/ false, /*DoIOUpdate*/ false);
    //clear phase accumulators of both channels
    SetWriteChannels(3);
    SetValueDirect(CFR, 0x000302, /*GetValue*/ false, /*DoIOUpdate*/ true);
    SetValueDirect(CFR, 0x000300, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

#define PWR_DWN_FULL    0x2    //!< Select full power down mode
#define PWR_DWN_PART    0x0    //!< Select partial power down mode
#define SYNC_DISABLE    0x1    //!< Disable the SYNC_CLK signal
void CAD9958::SetPowerDown_partial_SPI(void)
{
	SetRegisterBits(/*RegisterNr*/ FR1, /*LowestBitNr*/ 5, /* NrBits*/ 2, PWR_DWN_PART | SYNC_DISABLE, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9958::SetPowerDown_full_SPI(void)
{
    SetRegisterBits(/*RegisterNr*/ FR1, /*LowestBitNr*/ 5, /* NrBits*/ 2, PWR_DWN_FULL | SYNC_DISABLE, /*GetValue*/ false, /*DoIOUpdate*/ true);
}


void CAD9958::MasterReset() {
    if (!Enabled) return;
    for (int i = 0; i < AD9958NumberOfRegisters; i++) {
        AktValueContents[i] = AD9958MasterResetValueContents[i];
        WritePrecision[i] = AD9958ValueLength[i];
    }
    // Toggle MASTER_RESET
    SetReset(true);
    // Minimum pulse width needs to be > 1 SYNC_CLK period (~160ns)
    // NEED TO MAKE SURE THIS IS THE CASE FOR YOUR PLATFORM.
    SetReset(false);
    // Set PWR-DWN-CTL pin LOW to disable power-down control
    SetPowerDown(false);
    SetSyncIO(false);
    // Toggle external power down pin
    // This resets the digital logic in the DDS to ensure SPI comms is possible
    SetPowerState(E_AD9958_PWR_DOWN);
    SetPowerState(E_AD9958_PWR_POWERED);    
    // Not using multiple DDS devices so not needed
    Disable_SYNC_CLK();

    //Wait(10);
    //ToDo: check if SetSyncIO needed, seems not to have been on correct pin

    SetSyncIO(true);
    //Wait(0.001);
    //ToDo: check if SetSyncIO needed, seems not to have been on correct pin
    // sync_io low (To enable SPI reception)
    SetSyncIO(false);    
    //Switch to QSPI mode
    SetQSPIMode(true);
}

void CAD9958::SetWriteChannels(bool channel0, bool channel1) {
    uint8_t channelcode = ((channel0) ? 1 : 0) + ((channel1) ? 2 : 0);
    SetWriteChannels(channelcode);
}

//channels = 1: write to channel 0
//channels = 2: write to channel 1
//channels = 3: write to channel 0 and 1
void CAD9958::SetWriteChannels(uint8_t channels) {
    SetRegisterBits(/*RegisterNr*/CSR, /*LowestBitNr*/ 6, /* NrBits*/ 2, channels, /*GetValue*/false, /*DoIOUpdate*/ false, /*ForceWrite*/ false);
}

//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
//RegisterNr in [47..68]: write channel 0 and 1 register map 
//RegisterNr = 69: write & read digital out port of this board
bool CAD9958::SetRegisterBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue, bool DoIOUpdate)
{
    return SetRegisterBits(RegisterNr, BitNr, 1, (Value) ? 1 : 0, GetValue) > 0;
}

//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
//RegisterNr in [47..68]: write channel 0 and 1 register map simultaneously. If Value doesn't specify all bits of a register write, the values of channel 0 will be taken to complement Value.
//RegisterNr = 69: write & read digital out port of this board
constexpr uint8_t ControlRegisterNr = 69;
uint32_t CAD9958::SetRegisterBits(unsigned char RegisterNr, unsigned char LowestBitNr, unsigned char NrBits, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return false;
    if (((RegisterNr > ControlRegisterNr) && (!GetValue)) || ((RegisterNr > 46) && (RegisterNr != ControlRegisterNr) && (GetValue))) {
        CString buf;
        if (GetValue) buf.Format("CAD9958::SetRegisterBits : RegisterNr (%d) is not in range for reading, [0..46] or 68.", RegisterNr);
        else buf.Format("CAD9958::SetRegisterBits : RegisterNr (%d) is not in range for writing, [0..68]", RegisterNr);
        ControlMessageBox(buf);
        return 0;
    }
    uint8_t LengthTableRegisterNr = (RegisterNr < 47) ? RegisterNr : (RegisterNr < 69) ? RegisterNr - 44 : 47;

    if (AD9958ValueLength[LengthTableRegisterNr] * 8 < NrBits) {
        CString buf;
        buf.Format("CAD9958::SetRegisterBits : NrBits (%d) exceeds RegisterNr (%d) length (%d)", NrBits, RegisterNr, AD9958ValueLength[LengthTableRegisterNr] * 8);
        ControlMessageBox(buf);
        return 0;
    }
    uint32_t mask = 0xFFFFFFFF >> (32 - NrBits);
    uint8_t ReadRegisterNr = (RegisterNr < 47) ? RegisterNr : (RegisterNr < 69) ? RegisterNr - 44 : 47;
    if (GetValue) {
		if (RegisterNr == ControlRegisterNr) return (ControlRegisterContent >> LowestBitNr) & mask;
        return ((AktValueContents[ReadRegisterNr] >> LowestBitNr) & mask);
    }
    else {
        uint32_t NewValue;
        uint32_t help = (RegisterNr == ControlRegisterNr) ? ControlRegisterContent : AktValueContents[ReadRegisterNr];
        uint32_t shiftedmask = mask << LowestBitNr;
        uint32_t invertedmask = ~shiftedmask;
        NewValue = (help & invertedmask) | ((Value & mask) << LowestBitNr);
        SetValue(RegisterNr, NewValue, false, DoIOUpdate, forceWrite);
        return Value;
    }
}


//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
//RegisterNr in [47..68]: write channel 0 and 1 register map 
//RegisterNr = 69: write & read digital out port of this board
uint32_t CAD9958::SetValue(unsigned char RegisterNr, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return 0;
    if ( ((RegisterNr > ControlRegisterNr) && (!GetValue)) || ((RegisterNr > 46) && (RegisterNr!= ControlRegisterNr) && (GetValue)) ) {
        CString buf;
        if (GetValue) buf.Format("CAD9958::SetValue : RegisterNr (%d) is not in range for reading, [0..46] or 68.", RegisterNr);
        else buf.Format("CAD9958::SetValue : RegisterNr (%d) is not in range for writing, [0..68]", RegisterNr);
        ControlMessageBox(buf);
        return 0;
    }
    if (GetValue) {
		if (RegisterNr == 68) {
			//Digital out port is not in the bus buffer, so we return the value directly
            return ControlRegisterContent;
		}
        return AktValueContents[RegisterNr];
    }
    else {
        if (RegisterNr < 3) {
            SetValueDirect(RegisterNr, Value, GetValue, DoIOUpdate, forceWrite);
            return 0;
        }
        if (RegisterNr == ControlRegisterNr) {
			SetControlRegister(0, 16, Value, /*write_immediately*/ true);
			return 0;
        }

        bool WriteCh0 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 0 is selected
        bool WriteCh1 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 1 is selected
		uint8_t WriteRegisterNr = (RegisterNr <= 24) ? RegisterNr : (RegisterNr <= 46) ? RegisterNr - 22 : RegisterNr - 44;
        bool WriteCh0Desired = (RegisterNr <= 24) || (RegisterNr > 46);
        bool WriteCh1Desired = (RegisterNr > 24);
		bool DoSetWriteChannels = (WriteCh0 != WriteCh0Desired) || (WriteCh1 != WriteCh1Desired);
        if ((WriteRegisterNr > 2) && DoSetWriteChannels) SetWriteChannels(WriteCh0Desired, WriteCh1Desired);
        SetValueDirect(WriteRegisterNr, Value, GetValue, DoIOUpdate, forceWrite);
    }
    return 0;
}


//This function sets the values of the AD9985 registers as described in the datasheet. 
//The SetValue function can also set "virtual" registers to provide a more general way to access all functions of our board.

//AktValueContents[0..2] is the control register
//AktValueContents[3..24] is the channel 0 register map. This function allows writing to these registers, which writes to channel 0 and/or 1 depending on bit 6 and 7 of the CSR. When reading it provides values of channel 0
//AktValueContents[25..46] is the channel 1 register map, and can't be written by this function. When reading it provides values of channel 1
uint32_t CAD9958::SetValueDirect(unsigned char RegisterNr, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return 0;
    if (((RegisterNr >= 25) && (!GetValue)) || ((RegisterNr >= 47) && (!GetValue))) {
        CString buf;
        buf.Format("CAD9958::SetValueDirect : RegisterNr (%d) exceeds maximum (%d)", RegisterNr, AD9958NumberOfRegisters - 1);
        ControlMessageBox(buf);
        return 0;
    }
    if (GetValue) {
        return AktValueContents[RegisterNr];
    }
    else {
        bool WriteCh0 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 0 is selected
        bool WriteCh1 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 1 is selected

        bool doWrite = ForceWriting;
        if (RegisterNr < 3) doWrite = (AktValueContents[RegisterNr] != Value);
        else {
            if (WriteCh0 && (AktValueContents[RegisterNr] != Value)) {
                doWrite = true;
            }
            if (WriteCh1 && (AktValueContents[RegisterNr + 22] != Value)) {
                doWrite = true;
            }
        }
        if (doWrite || forceWrite) {
            if (RegisterNr < 3) AktValueContents[RegisterNr] = Value;
            else {
                if (WriteCh0) AktValueContents[RegisterNr] = Value;
                if (WriteCh1) AktValueContents[RegisterNr + 22] = Value;
            }
            Dev_Select();
            SPI_Transmit_Byte(RegisterNr);
            for (int i = 0; i < AD9958ValueLength[RegisterNr]; i++)
                SPI_Transmit_Byte(((uint8_t*)&Value)[i]);
            Dev_Deselect();
            if (DoIOUpdate) IO_Update_Toggle();
        }
    }
    return 0;
}