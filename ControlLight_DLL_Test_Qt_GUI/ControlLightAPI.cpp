// ControlAPI.cpp: implementation of the API.
//
//////////////////////////////////////////////////////////////////////

#include  "ControlLightAPI.h"


#include <QDebug>



CControlLightAPI::CControlLightAPI()
{
    DLL_Loaded = false;
    Set_CLA_CallsToNull();
}



void CControlLightAPI::Set_CLA_CallsToNull() {
    
/*ChatGPT, please provide code to set these function pointers to nullptr, see examples further down
    CreateFunc CLA_Create;
    ConfigureFunc CLA_Configure;
    CleanupFunc CLA_Cleanup;
    GetLastErrorFunc CLA_GetLastError;
    LoadFromJSONFileFunc CLA_LoadFromJSONFile;
    InitializeFunc CLA_Initialize;
    SwitchDebugModeFunc CLA_SwitchDebugMode;
    IsReadyFunc CLA_IsReady;
    StartAssemblingSequenceFunc CLA_StartAssemblingSequence;
    SetValueFunc CLA_SetValue;
    SetRegisterFunc CLA_SetRegister;
    SetValueSerialDeviceFunc CLA_SetValueSerialDevice;
    SetRegisterSerialDeviceFunc CLA_SetRegisterSerialDevice;
    Wait_msFunc CLA_Wait_ms;
    GetTime_msFunc CLA_GetTime_ms;
    SetVoltageFunc CLA_SetVoltage;
    ExecuteSequenceFunc CLA_ExecuteSequence;
    GetSequenceExecutionStatusFunc CLA_GetSequenceExecutionStatus;
    WaitTillEndOfSequenceThenGetInputDataFunc CLA_WaitTillEndOfSequenceThenGetInputData;
    SetTimeDebtGuardFunc CLA_SetTimeDebtGuard_in_ms;
    SequencerStartAnalogInAcquisitionFunc CLA_SequencerStartAnalogInAcquisition;
    SequencerWriteInputMemoryFunc CLA_SequencerWriteInputMemory;
    SequencerWriteSystemTimeToInputMemoryFunc CLA_SequencerWriteSystemTimeToInputMemory;
    SequencerSwitchDebugLEDFunc CLA_SequencerSwitchDebugLED;
    SequencerIgnoreTCPIPFunc CLA_SequencerIgnoreTCPIP;
    SequencerAddMarkerFunc CLA_SequencerAddMarker;
    AddDeviceSequencerFunc CLA_AddDeviceSequencer;
    AddDeviceAnalogOut16bitFunc CLA_AddDeviceAnalogOut16bit;
    AddDeviceDigitalOutFunc CLA_AddDeviceDigitalOut;
    AddDeviceAD9854Func CLA_AddDeviceAD9854;
    AddDeviceAD9858Func CLA_AddDeviceAD9858;
    AddDeviceAnalogIn12bitFunc CLA_AddDeviceAnalogIn12bit;
       SetDigitalOutputFunc CLA_SetDigitalOutput;
    SetStartFrequencyFunc CLA_SetStartFrequency;
    SetStopFrequencyFunc CLA_SetStopFrequency;
    SetModulationFrequencyFunc CLA_SetModulationFrequency;
    SetPowerFunc CLA_SetPower;
    SetAttenuationFunc CLA_SetAttenuation;
    SetStartFrequencyTuningWordFunc CLA_SetStartFrequencyTuningWord;
    SetStopFrequencyTuningWordFunc CLA_SetStopFrequencyTuningWord;
    SetFSKModeFunc CLA_SetFSKMode;
    SetRampRateClockFunc CLA_SetRampRateClock;
    SetClearACC1Func CLA_SetClearACC1;
    SetTriangleBitFunc CLA_SetTriangleBit;
    SetFSKBitFunc CLA_SetFSKBit;
    SetFrequencyFunc CLA_SetFrequency;
    SetFrequencyTuningWordFunc CLA_SetFrequencyTuningWord;
    SetAD9958FrequencyFunc CLA_SetAD9958Frequency;
    SetAD9958FrequencyTuningWordFunc CLA_SetAD9958FrequencyTuningWord;
    SetAD9958PhaseFunc CLA_SetAD9958Phase;
    SetAD9958PowerFunc CLA_SetAD9958Power;

    */

    //CLA_GetInstance = nullptr;
    CLA_Create = nullptr;
    CLA_Configure = nullptr;
    CLA_DidErrorOccur = nullptr;
    CLA_GetLastError = nullptr;
    CLA_LoadFromJSONFile = nullptr;
    CLA_Initialize = nullptr;
    CLA_SwitchDebugMode = nullptr;
    CLA_IsReady = nullptr;
    CLA_StartAssemblingSequence = nullptr;
    CLA_SetValue = nullptr;
    CLA_SetRegister = nullptr;
    CLA_SetValueSerialDevice = nullptr;
    CLA_SetRegisterSerialDevice = nullptr;
    CLA_Wait_ms = nullptr;
    CLA_GetTime_ms = nullptr;
    CLA_GetTimeOfSequencer_ms = nullptr;
    CLA_GetTimeDebtOfSequencer_ms = nullptr;
    CLA_SetVoltage = nullptr;
    CLA_ExecuteSequence = nullptr;
    CLA_GetSequenceExecutionStatus = nullptr;
    CLA_WaitTillEndOfSequenceThenGetInputData = nullptr;
    CLA_SetTimeDebtGuard_in_ms = nullptr;
    CLA_SequencerStartAnalogInAcquisition = nullptr;
    CLA_SequencerWriteInputMemory = nullptr;
    CLA_SequencerWriteSystemTimeToInputMemory = nullptr;
    CLA_SequencerSwitchDebugLED = nullptr;
    CLA_SequencerIgnoreTCPIP = nullptr;
    CLA_SequencerAddMarker = nullptr;
    CLA_AddDeviceSequencer = nullptr;
    CLA_AddDeviceAnalogOut16bit = nullptr;
    CLA_AddDeviceDigitalOut = nullptr;
    CLA_AddDeviceAD9854 = nullptr;
    CLA_AddDeviceAD9858 = nullptr;
    CLA_AddDeviceAnalogIn12bit = nullptr;
    CLA_SetDigitalOutput = nullptr;
    CLA_SetStartFrequency = nullptr;
    CLA_SetStopFrequency = nullptr;
    CLA_SetModulationFrequency = nullptr;
    CLA_SetPower = nullptr;
    CLA_SetAttenuation = nullptr;
    CLA_SetStartFrequencyTuningWord = nullptr;
    CLA_SetStopFrequencyTuningWord = nullptr;
    CLA_SetFSKMode = nullptr;
    CLA_SetRampRateClock = nullptr;
    CLA_SetClearACC1 = nullptr;
    CLA_SetTriangleBit = nullptr;
    CLA_SetFSKBit = nullptr;
    CLA_SetFrequency = nullptr;
    CLA_SetFrequencyTuningWord = nullptr;
    CLA_SetFrequencyOfChannel = nullptr;
    CLA_SetFrequencyTuningWordOfChannel = nullptr;
    CLA_SetPhaseOfChannel = nullptr;
    CLA_SetPowerOfChannel = nullptr;

}


CControlLightAPI::~CControlLightAPI()
{
    Cleanup();
}

void CControlLightAPI::MessageBox(QString aMessage) {
    qDebug() << aMessage;
}

#define TRY_RESOLVE(name) \
name = (decltype(name))CLA_Lib->resolve(#name); \
    if (!name) qDebug() << "Failed to resolve:" << #name;


/*

void listAllExports(const QString& dllPath) {
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModule = LoadLibraryW((LPCWSTR)dllPath.utf16());
    if (!hModule) {
        qDebug() << "LoadLibraryW failed.";
        return;
    }

    ULONG size;
    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
        hModule, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

    if (!exports) {
        qDebug() << "No exports found.";
        return;
    }

    DWORD* names = (DWORD*)((BYTE*)hModule + exports->AddressOfNames);
    for (DWORD i = 0; i < exports->NumberOfNames; i++) {
        const char* funcName = (const char*)hModule + names[i];
        qDebug() << "Exported:" << funcName;
    }
}
*/

bool CControlLightAPI::LoadDLL() {


   // listAllExports("ControlLight.dll");//for debug

    CLA_Lib = new QLibrary("ControlLight.dll");

    //for debug

    TRY_RESOLVE(CLA_Configure);
    TRY_RESOLVE(CLA_Cleanup);
    TRY_RESOLVE(CLA_Create);
    TRY_RESOLVE(CLA_GetLastError);
    TRY_RESOLVE(CLA_DidErrorOccur);
    TRY_RESOLVE(CLA_LoadFromJSONFile);
    TRY_RESOLVE(CLA_Initialize);
    TRY_RESOLVE(CLA_SwitchDebugMode);
    TRY_RESOLVE(CLA_IsReady);
    TRY_RESOLVE(CLA_StartAssemblingSequence);
    TRY_RESOLVE(CLA_SetValue);
    TRY_RESOLVE(CLA_SetRegister);
    TRY_RESOLVE(CLA_SetValueSerialDevice);
    TRY_RESOLVE(CLA_SetRegisterSerialDevice);
    TRY_RESOLVE(CLA_Wait_ms);
    TRY_RESOLVE(CLA_GetTime_ms);
    TRY_RESOLVE(CLA_GetTimeOfSequencer_ms);
    TRY_RESOLVE(CLA_GetTimeDebtOfSequencer_ms);
    TRY_RESOLVE(CLA_SetVoltage);
    TRY_RESOLVE(CLA_ExecuteSequence);
    TRY_RESOLVE(CLA_GetSequenceExecutionStatus);
    TRY_RESOLVE(CLA_WaitTillEndOfSequenceThenGetInputData);
    TRY_RESOLVE(CLA_SetTimeDebtGuard_in_ms);
    TRY_RESOLVE(CLA_SequencerStartAnalogInAcquisition);
    TRY_RESOLVE(CLA_SequencerWriteInputMemory);
    TRY_RESOLVE(CLA_SequencerWriteSystemTimeToInputMemory);
    TRY_RESOLVE(CLA_SequencerSwitchDebugLED);
    TRY_RESOLVE(CLA_SequencerIgnoreTCPIP);
    TRY_RESOLVE(CLA_SequencerAddMarker);
    TRY_RESOLVE(CLA_AddDeviceSequencer);
    TRY_RESOLVE(CLA_AddDeviceAnalogOut16bit);
    TRY_RESOLVE(CLA_AddDeviceDigitalOut);
    TRY_RESOLVE(CLA_AddDeviceAD9854);
    TRY_RESOLVE(CLA_AddDeviceAD9858);
    TRY_RESOLVE(CLA_AddDeviceAnalogIn12bit);
    TRY_RESOLVE(CLA_SetDigitalOutput);
    TRY_RESOLVE(CLA_SetStartFrequency);
    TRY_RESOLVE(CLA_SetStopFrequency);
    TRY_RESOLVE(CLA_SetModulationFrequency);
    TRY_RESOLVE(CLA_SetPower);
    TRY_RESOLVE(CLA_SetAttenuation);
    TRY_RESOLVE(CLA_SetStartFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetStopFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetFSKMode);
    TRY_RESOLVE(CLA_SetRampRateClock);
    TRY_RESOLVE(CLA_SetClearACC1);
    TRY_RESOLVE(CLA_SetTriangleBit);
    TRY_RESOLVE(CLA_SetFSKBit);
    TRY_RESOLVE(CLA_SetFrequency);
    TRY_RESOLVE(CLA_SetFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetFrequencyOfChannel);
    TRY_RESOLVE(CLA_SetFrequencyTuningWordOfChannel);
    TRY_RESOLVE(CLA_SetPhaseOfChannel);
    TRY_RESOLVE(CLA_SetPowerOfChannel);

    //TRY_RESOLVE(CLA_GetInstance);

























    if (!CLA_Lib->load()) {
        qDebug() << "Failed to load DLL:" << CLA_Lib->errorString();
        return false;
    }

    /*
 ChatGPT, please provide function pointer types for the following functions, similar to the examples further down
    API_EXPORT HControlLightAPI CLA_GetInstance();
    API_EXPORT int CLA_Create(bool InitializeAfx, bool InitializeAfxSocket);
    API_EXPORT void CLA_Configure(bool _DisplayErrors);
    API_EXPORT void CLA_Cleanup();
    API_EXPORT const char* CLA_GetLastError();
    API_EXPORT bool CLA_LoadFromJSONFile(const char* filename);
    API_EXPORT void CLA_Initialize();
    API_EXPORT void CLA_SwitchDebugMode(bool OnOff);
    API_EXPORT bool CLA_IsReady();
    API_EXPORT void CLA_StartAssemblingSequence();
    API_EXPORT bool CLA_SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_Wait_ms(double time_in_ms);
    API_EXPORT double CLA_GetTime_ms();
    API_EXPORT bool CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
    API_EXPORT void CLA_ExecuteSequence();
    API_EXPORT bool CLA_GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten);
    API_EXPORT bool CLA_WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
    API_EXPORT void CLA_SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms);
    API_EXPORT bool CLA_SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
    API_EXPORT bool CLA_SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
    API_EXPORT bool CLA_SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer);
    API_EXPORT bool CLA_SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff);
    API_EXPORT bool CLA_SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff);
    API_EXPORT bool CLA_SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker);
    API_EXPORT bool CLA_AddDeviceSequencer(
        unsigned int id,
        const char* type,
        const char* ip,
        unsigned int port,
        bool master,
        unsigned int startDelay,
        double clockFrequency,
        unsigned long FPGAClockToBusClockRatio,
        bool useExternalClock,
        bool useStrobeGenerator,
        bool connect);
    API_EXPORT bool CLA_AddDeviceAnalogOut16bit(
        unsigned int sequencer,
        unsigned int startAddress,
        unsigned int numberChannels,
        bool signedValue,
        double minVoltage,
        double maxVoltage);
    API_EXPORT bool CLA_AddDeviceDigitalOut(
        unsigned int sequencer,
        unsigned int address,
        unsigned int numberChannels);
    API_EXPORT bool CLA_AddDeviceAD9854(
        unsigned int sequencer,
        unsigned int address,
        unsigned int version,
        double externalClockFrequency,
        uint8_t PLLReferenceMultiplier,
        unsigned int frequencyMultiplier);
    API_EXPORT bool CLA_AddDeviceAD9858(
        unsigned int sequencer,
        unsigned int address,
        double externalClockFrequency,
        unsigned int frequencyMultiplier);
    API_EXPORT bool CLA_AddDeviceAnalogIn12bit(
        unsigned int sequencer,
        unsigned int chipSelect,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

API_EXPORT ERROR_CODE_TYPE CLA_SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);
//AD9854
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetPower(const unsigned int& Sequencer, const unsigned int& Address, double Power);
API_EXPORT ERROR_CODE_TYPE CLA_SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);
API_EXPORT ERROR_CODE_TYPE CLA_SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);
API_EXPORT ERROR_CODE_TYPE CLA_SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

//AD9858
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

//AD9958
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Frequency(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958FrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Phase(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Power(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);


*/





    CLA_Configure = (ConfigureFunc)CLA_Lib->resolve("CLA_Configure");
    CLA_Create = (CreateFunc)CLA_Lib->resolve("CLA_Create");
    CLA_Cleanup = (CleanupFunc)CLA_Lib->resolve("CLA_Cleanup");
    CLA_GetLastError = (GetLastErrorFunc)CLA_Lib->resolve("CLA_GetLastError");
    CLA_DidErrorOccur = (DidErrorOccurFunc)CLA_Lib->resolve("CLA_DidErrorOccur");
    CLA_LoadFromJSONFile = (LoadFromJSONFileFunc)CLA_Lib->resolve("CLA_LoadFromJSONFile");
    CLA_Initialize = (InitializeFunc)CLA_Lib->resolve("CLA_Initialize");
    CLA_SwitchDebugMode = (SwitchDebugModeFunc)CLA_Lib->resolve("CLA_SwitchDebugMode");
    CLA_IsReady = (IsReadyFunc)CLA_Lib->resolve("CLA_IsReady");
    CLA_StartAssemblingSequence = (StartAssemblingSequenceFunc)CLA_Lib->resolve("CLA_StartAssemblingSequence");
    CLA_SetValue = (SetValueFunc)CLA_Lib->resolve("CLA_SetValue");
    CLA_SetRegister = (SetRegisterFunc)CLA_Lib->resolve("CLA_SetRegister");
    CLA_SetValueSerialDevice = (SetValueSerialDeviceFunc)CLA_Lib->resolve("CLA_SetValueSerialDevice");
    CLA_SetRegisterSerialDevice = (SetRegisterSerialDeviceFunc)CLA_Lib->resolve("CLA_SetRegisterSerialDevice");
    CLA_Wait_ms = (Wait_msFunc)CLA_Lib->resolve("CLA_Wait_ms");
    CLA_GetTime_ms = (GetTime_msFunc)CLA_Lib->resolve("CLA_GetTime_ms");
    CLA_GetTimeOfSequencer_ms = (GetTimeOfSequencer_msFunc)CLA_Lib->resolve("CLA_GetTimeOfSequencer_ms");
    CLA_GetTimeDebtOfSequencer_ms = (GetTimeDebtOfSequencer_msFunc)CLA_Lib->resolve("CLA_GetTimeDebtOfSequencer_ms");
    CLA_SetVoltage = (SetVoltageFunc)CLA_Lib->resolve("CLA_SetVoltage");
    CLA_ExecuteSequence = (ExecuteSequenceFunc)CLA_Lib->resolve("CLA_ExecuteSequence");
    CLA_GetSequenceExecutionStatus = (GetSequenceExecutionStatusFunc)CLA_Lib->resolve("CLA_GetSequenceExecutionStatus");
    CLA_WaitTillEndOfSequenceThenGetInputData = (WaitTillEndOfSequenceThenGetInputDataFunc)CLA_Lib->resolve("CLA_WaitTillEndOfSequenceThenGetInputData");
    CLA_SetTimeDebtGuard_in_ms = (SetTimeDebtGuardFunc)CLA_Lib->resolve("CLA_SetTimeDebtGuard_in_ms");
    CLA_SequencerStartAnalogInAcquisition = (SequencerStartAnalogInAcquisitionFunc)CLA_Lib->resolve("CLA_SequencerStartAnalogInAcquisition");
    CLA_SequencerWriteInputMemory = (SequencerWriteInputMemoryFunc)CLA_Lib->resolve("CLA_SequencerWriteInputMemory");
    CLA_SequencerWriteSystemTimeToInputMemory = (SequencerWriteSystemTimeToInputMemoryFunc)CLA_Lib->resolve("CLA_SequencerWriteSystemTimeToInputMemory");
    CLA_SequencerSwitchDebugLED = (SequencerSwitchDebugLEDFunc)CLA_Lib->resolve("CLA_SequencerSwitchDebugLED");
    CLA_SequencerIgnoreTCPIP = (SequencerIgnoreTCPIPFunc)CLA_Lib->resolve("CLA_SequencerIgnoreTCPIP");
    CLA_SequencerAddMarker = (SequencerAddMarkerFunc)CLA_Lib->resolve("CLA_SequencerAddMarker");
    CLA_AddDeviceSequencer = (AddDeviceSequencerFunc)CLA_Lib->resolve("CLA_AddDeviceSequencer");
    CLA_AddDeviceAnalogOut16bit = (AddDeviceAnalogOut16bitFunc)CLA_Lib->resolve("CLA_AddDeviceAnalogOut16bit");
    CLA_AddDeviceDigitalOut = (AddDeviceDigitalOutFunc)CLA_Lib->resolve("CLA_AddDeviceDigitalOut");
    CLA_AddDeviceAD9854 = (AddDeviceAD9854Func)CLA_Lib->resolve("CLA_AddDeviceAD9854");
    CLA_AddDeviceAD9858 = (AddDeviceAD9858Func)CLA_Lib->resolve("CLA_AddDeviceAD9858");
    CLA_AddDeviceAnalogIn12bit = (AddDeviceAnalogIn12bitFunc)CLA_Lib->resolve("CLA_AddDeviceAnalogIn12bit");
    CLA_SetDigitalOutput = (SetDigitalOutputFunc)CLA_Lib->resolve("CLA_SetDigitalOutput");
    CLA_SetStartFrequency = (SetStartFrequencyFunc)CLA_Lib->resolve("CLA_SetStartFrequency");
    CLA_SetStopFrequency = (SetStopFrequencyFunc)CLA_Lib->resolve("CLA_SetStopFrequency");
    CLA_SetModulationFrequency = (SetModulationFrequencyFunc)CLA_Lib->resolve("CLA_SetModulationFrequency");
    CLA_SetPower = (SetPowerFunc)CLA_Lib->resolve("CLA_SetPower");
    CLA_SetAttenuation = (SetAttenuationFunc)CLA_Lib->resolve("CLA_SetAttenuation");
    CLA_SetStartFrequencyTuningWord = (SetStartFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetStartFrequencyTuningWord");
    CLA_SetStopFrequencyTuningWord = (SetStopFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetStopFrequencyTuningWord");
    CLA_SetFSKMode = (SetFSKModeFunc)CLA_Lib->resolve("CLA_SetFSKMode");
    CLA_SetRampRateClock = (SetRampRateClockFunc)CLA_Lib->resolve("CLA_SetRampRateClock");
    CLA_SetClearACC1 = (SetClearACC1Func)CLA_Lib->resolve("CLA_SetClearACC1");
    CLA_SetTriangleBit = (SetTriangleBitFunc)CLA_Lib->resolve("CLA_SetTriangleBit");
    CLA_SetFSKBit = (SetFSKBitFunc)CLA_Lib->resolve("CLA_SetFSKBit");
    CLA_SetFrequency = (SetFrequencyFunc)CLA_Lib->resolve("CLA_SetFrequency");
    CLA_SetFrequencyTuningWord = (SetFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetFrequencyTuningWord");
    CLA_SetFrequencyOfChannel = (SetFrequencyOfChannelFunc)CLA_Lib->resolve("CLA_SetFrequencyOfChannel");
    CLA_SetFrequencyTuningWordOfChannel = (SetFrequencyTuningWordOfChannelFunc)CLA_Lib->resolve("CLA_SetFrequencyTuningWordOfChannel");
    CLA_SetPhaseOfChannel = (SetPhaseOfChannelFunc)CLA_Lib->resolve("CLA_SetPhaseOfChannel");
    CLA_SetPowerOfChannel = (SetPowerOfChannelFunc)CLA_Lib->resolve("CLA_SetPowerOfChannel");

    
    //now check if all functions were loaded correctly

    if (
        !CLA_Configure ||
        !CLA_Create ||
        !CLA_Cleanup ||
        !CLA_GetLastError ||
        !CLA_DidErrorOccur ||
        !CLA_LoadFromJSONFile ||
        !CLA_Initialize ||
        !CLA_SwitchDebugMode ||
        !CLA_IsReady ||
        !CLA_StartAssemblingSequence ||
        !CLA_SetValue ||
        !CLA_SetRegister ||
        !CLA_SetValueSerialDevice ||
        !CLA_SetRegisterSerialDevice ||
        !CLA_Wait_ms ||
        !CLA_GetTime_ms ||
        !CLA_GetTimeOfSequencer_ms ||
        !CLA_GetTimeDebtOfSequencer_ms ||
        !CLA_SetVoltage ||
        !CLA_ExecuteSequence ||
        !CLA_GetSequenceExecutionStatus ||
        !CLA_WaitTillEndOfSequenceThenGetInputData ||
        !CLA_SetTimeDebtGuard_in_ms ||
        !CLA_SequencerStartAnalogInAcquisition ||
        !CLA_SequencerWriteInputMemory ||
        !CLA_SequencerWriteSystemTimeToInputMemory ||
        !CLA_SequencerSwitchDebugLED ||
        !CLA_SequencerIgnoreTCPIP ||
        !CLA_SequencerAddMarker ||
        !CLA_AddDeviceSequencer ||
        !CLA_AddDeviceAnalogOut16bit ||
        !CLA_AddDeviceDigitalOut ||
        !CLA_AddDeviceAD9854 ||
        !CLA_AddDeviceAD9858 ||
        !CLA_AddDeviceAnalogIn12bit ||
        !CLA_SetDigitalOutput ||
        !CLA_SetStartFrequency ||
        !CLA_SetStopFrequency ||
        !CLA_SetModulationFrequency ||
        !CLA_SetPower ||
        !CLA_SetAttenuation ||
        !CLA_SetStartFrequencyTuningWord ||
        !CLA_SetStopFrequencyTuningWord ||
        !CLA_SetFSKMode ||
        !CLA_SetRampRateClock ||
        !CLA_SetClearACC1 ||
        !CLA_SetTriangleBit ||
        !CLA_SetFSKBit ||
        !CLA_SetFrequency ||
        !CLA_SetFrequencyTuningWord ||
        !CLA_SetFrequencyOfChannel ||
        !CLA_SetFrequencyTuningWordOfChannel ||
        !CLA_SetPhaseOfChannel ||
        !CLA_SetPowerOfChannel

        
        ) {
        qDebug() << "Failed to resolve one or more ControlAPI symbols.";
        Cleanup();
        return false;
    }
    Configure(true);
    CLA_Handle = GetInstance();
    DLL_Loaded = true;
    return true;
}

void CControlLightAPI::Cleanup() {
    if (CLA_Lib) {
        Set_CLA_CallsToNull();
        Call_CLA_Cleanup();
        CLA_Cleanup = NULL;
        CLA_Lib->unload();
        CLA_Handle=nullptr;
        delete CLA_Lib;
        CLA_Lib = NULL;
    }
    DLL_Loaded = false;
}



/*ChatGPT, load the following functions, see example further down
    HControlLightAPI CLA_GetInstance();
    //Call these funcitons in roughly this order
    int CLA_Create(bool InitializeAfx, bool InitializeAfxSocket); //you must call this first, otherwise the API will not work
    void CLA_Configure(bool _DisplayErrors); //optional
    void CLA_Cleanup(); //You must call this before leaving your program, otherwise the API can provoke errors because memory is not freed
    const char* CLA_GetLastError();
    

    //you can load the configuration from a JSON file, or define the IO devices with the functions further below (mixing is also possible)
    bool CLA_LoadFromJSONFile(const char* filename);

    //Once all devices have been declared, you must initialize the system, otherwise the API will not work
    void CLA_Initialize();
    //Some optional commands
    void CLA_SwitchDebugMode(bool OnOff);
    bool CLA_IsReady();

    //To start a sequence, first call StartAssemblingSequence, then add all the commands you want to execute in the sequence

    void CLA_StartAssemblingSequence();

    //here are possible commands
    bool CLA_SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_Wait_ms(double time_in_ms);
    double CLA_GetTime_ms();

    //the following are convenience functions, which allow us to define nice names to the few most important functions
    //You can add as many convenience functions as you like. Make sure to copy them also into the list of convenience functions in CDevice, CDevice.h, to assure they can always be called in any device.
    //Then define them in the device that provides the function. In this way we use the inheritance mechanism to automatically check if the function is available in the device and (optionally) produce an error if not.
    bool CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);


    // once the sequence is assembled, then execute it
    void CLA_ExecuteSequence();
    //check how far the sequence has been executed
    bool CLA_GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten);
    //Wait till the sequence is finished, and get the data from the input devices
    bool CLA_WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
    void CLA_SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms);
    bool CLA_SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
    bool CLA_SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
    bool CLA_SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer);
    bool CLA_SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff);
    bool CLA_SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff);
    bool CLA_SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker);




    //the following functions are used to add devices to the sequencer. I placed them here to avoid clutter above. They have to be called before Initialize().
    bool CLA_AddDeviceSequencer(
        unsigned int id,
        const char* type,
        const char* ip,
        unsigned int port,
        bool master,
        unsigned int startDelay,
        double clockFrequency,
        unsigned long FPGAClockToBusClockRatio,
        bool useExternalClock,
        bool useStrobeGenerator,
        bool connect);

    bool CLA_AddDeviceAnalogOut16bit(
        unsigned int sequencer,
        unsigned int startAddress,
        unsigned int numberChannels,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

    bool CLA_AddDeviceDigitalOut(
        unsigned int sequencer,
        unsigned int address,
        unsigned int numberChannels);

    bool CLA_AddDeviceAD9854(
        unsigned int sequencer,
        unsigned int address,
        unsigned int version,
        double externalClockFrequency,
        uint8_t PLLReferenceMultiplier,
        unsigned int frequencyMultiplier);

    bool CLA_AddDeviceAD9858(
        unsigned int sequencer,
        unsigned int address,
        double externalClockFrequency,
        unsigned int frequencyMultiplier);

    bool CLA_AddDeviceAnalogIn12bit(
        unsigned int sequencer,
        unsigned int chipSelect,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

 */


HControlLightAPI CControlLightAPI::GetInstance() {
    if (CLA_GetInstance)
        return CLA_GetInstance();
    else
        return NULL;
}


bool CControlLightAPI::Create(bool InitializeAfx, bool InitializeAfxSocket) {
    if (CLA_Create)
        return CLA_Create(InitializeAfx, InitializeAfxSocket);
}

void CControlLightAPI::Configure(bool DisplayCommandErrors) {
    if (CLA_Configure)
        CLA_Configure(DisplayCommandErrors);
}

void CControlLightAPI::Call_CLA_Cleanup() {
    if (CLA_Cleanup)
        CLA_Cleanup();
}

const char* CControlLightAPI::GetLastError() {
    if (CLA_GetLastError)
        return CLA_GetLastError();
    else
        return "Error: GetLastError function not loaded.";
}

bool CControlLightAPI::DidErrorOccur() {
    if (CLA_DidErrorOccur)
        return CLA_DidErrorOccur();
    else
        return false;
}

bool CControlLightAPI::LoadFromJSONFile(const char* filename) {
    if (CLA_LoadFromJSONFile)
        return CLA_LoadFromJSONFile(filename);
    else
        return false;
}

void CControlLightAPI::Initialize() {
    if (CLA_Initialize)
        CLA_Initialize();
}

void CControlLightAPI::SwitchDebugMode(bool OnOff, const char* Filename) {
    if (CLA_SwitchDebugMode)
        CLA_SwitchDebugMode(OnOff, Filename);
}

bool CControlLightAPI::IsReady() {
    if (CLA_IsReady)
        return CLA_IsReady();
    else
        return false;
}

void CControlLightAPI::StartAssemblingSequence() {
    if (CLA_StartAssemblingSequence)
        CLA_StartAssemblingSequence();
}

bool CControlLightAPI::SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetValue)
        return CLA_SetValue(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetRegister)
        return CLA_SetRegister(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetValueSerialDevice)
        return CLA_SetValueSerialDevice(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetRegisterSerialDevice)
        return CLA_SetRegisterSerialDevice(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::Wait_ms(double time_in_ms) {
    if (CLA_Wait_ms)
        return CLA_Wait_ms(time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTime_ms(double &time_in_ms) {
    if (CLA_GetTime_ms)
        return CLA_GetTime_ms(time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTimeOfSequencer_ms(const unsigned int& Sequencer, double &time_in_ms) {
    if (CLA_GetTimeOfSequencer_ms)
        return CLA_GetTimeOfSequencer_ms(Sequencer, time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTimeDebtOfSequencer_ms(const unsigned int& Sequencer, double &time_in_ms) {
    if (CLA_GetTimeDebtOfSequencer_ms)
        return CLA_GetTimeDebtOfSequencer_ms(Sequencer, time_in_ms);
    else
        return false;
}


/*
API_EXPORT ERROR_CODE_TYPE CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
		
API_EXPORT ERROR_CODE_TYPE CLA_SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);
//AD9854
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetPower(const unsigned int& Sequencer, const unsigned int& Address, double Power);
API_EXPORT ERROR_CODE_TYPE CLA_SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);
API_EXPORT ERROR_CODE_TYPE CLA_SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);
API_EXPORT ERROR_CODE_TYPE CLA_SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

//AD9858
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

//AD9958
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Frequency(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958FrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Phase(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9958Power(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);

*/

bool CControlLightAPI::SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage) {
    if (CLA_SetVoltage)
        return CLA_SetVoltage(Sequencer, Address, Voltage);
    else
        return false;
}

bool CControlLightAPI::SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff) {
    if (CLA_SetDigitalOutput)
        return CLA_SetDigitalOutput(Sequencer, Address, BitNr, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetStartFrequency)
        return CLA_SetStartFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetStopFrequency)
        return CLA_SetStopFrequency(Sequencer, Address,  Frequency);
    else
        return false;
}

bool CControlLightAPI::SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetModulationFrequency)
        return CLA_SetModulationFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetPower(const unsigned int& Sequencer, const unsigned int& Address,  double Power) {
    if (CLA_SetPower)
        return CLA_SetPower(Sequencer, Address,  Power);
    else
        return false;
}

bool CControlLightAPI::SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address,  double Attenuation) {
    if (CLA_SetAttenuation)
        return CLA_SetAttenuation(Sequencer, Address, Attenuation);
    else
        return false;
}

bool CControlLightAPI::SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetStartFrequencyTuningWord)
        return CLA_SetStartFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetStopFrequencyTuningWord)
        return CLA_SetStopFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode) {
    if (CLA_SetFSKMode)
        return CLA_SetFSKMode(Sequencer, Address, mode);
    else
        return false;
}

bool CControlLightAPI::SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate) {
    if (CLA_SetRampRateClock)
        return CLA_SetRampRateClock(Sequencer, Address, rate);
    else
        return false;
}

bool CControlLightAPI::SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetClearACC1)
        return CLA_SetClearACC1(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetTriangleBit)
        return CLA_SetTriangleBit(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetFSKBit)
        return CLA_SetFSKBit(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetFrequency)
        return CLA_SetFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetFrequencyTuningWord)
        return CLA_SetFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency) {
    if (CLA_SetFrequencyOfChannel)
        return CLA_SetFrequencyOfChannel(Sequencer, Address, channel, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyTuningWordOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord) {
    if (CLA_SetFrequencyTuningWordOfChannel)
        return CLA_SetFrequencyTuningWordOfChannel(Sequencer, Address, channel, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetPhaseOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase) {
    if (CLA_SetPhaseOfChannel)
        return CLA_SetPhaseOfChannel(Sequencer, Address, channel, Phase);
    else
        return false;
}

bool CControlLightAPI::SetPowerOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power) {
    if (CLA_SetPowerOfChannel)
        return CLA_SetPowerOfChannel(Sequencer, Address, channel, Power);
    else
        return false;
}




//end of convenience functions


void CControlLightAPI::ExecuteSequence() {
    if (CLA_ExecuteSequence)
        CLA_ExecuteSequence();
}

bool CControlLightAPI::GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten) {
    if (CLA_GetSequenceExecutionStatus)
        return CLA_GetSequenceExecutionStatus(running, DataPointsWritten);
    else
        return false;
}

bool CControlLightAPI::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
    if (CLA_WaitTillEndOfSequenceThenGetInputData)
        return CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s);
    else
        return false;
}

void CControlLightAPI::SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms) {
    if (CLA_SetTimeDebtGuard_in_ms)
        CLA_SetTimeDebtGuard_in_ms(MaxTimeDebt_in_ms);
}

bool CControlLightAPI::SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms) {
    if (CLA_SequencerStartAnalogInAcquisition)
        return CLA_SequencerStartAnalogInAcquisition(Sequencer, ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms);
    else
        return false;
}

bool CControlLightAPI::SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
    if (CLA_SequencerWriteInputMemory)
        return CLA_SequencerWriteInputMemory(Sequencer, input_buf_mem_data, write_next_address, input_buf_mem_address);
    else
        return false;
}

bool CControlLightAPI::SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer) {
    if (CLA_SequencerWriteSystemTimeToInputMemory)
        return CLA_SequencerWriteSystemTimeToInputMemory(Sequencer);
    else
        return false;
}

bool CControlLightAPI::SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff) {
    if (CLA_SequencerSwitchDebugLED)
        return CLA_SequencerSwitchDebugLED(Sequencer, OnOff);
    else
        return false;
}

bool CControlLightAPI::SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff) {
    if (CLA_SequencerIgnoreTCPIP)
        return CLA_SequencerIgnoreTCPIP(Sequencer, OnOff);
    else
        return false;
}

bool CControlLightAPI::SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker) {
    if (CLA_SequencerAddMarker)
        return CLA_SequencerAddMarker(Sequencer, marker);
    else
        return false;
}

bool CControlLightAPI::AddDeviceSequencer(
    unsigned int id,
    const char* type,
    const char* ip,
    unsigned int port,
    bool master,
    unsigned int startDelay,
    double clockFrequency,
    unsigned long FPGAClockToBusClockRatio,
    bool useExternalClock,
    bool useStrobeGenerator,
    bool connect) {
    if (CLA_AddDeviceSequencer)
        return CLA_AddDeviceSequencer(id, type, ip, port, master, startDelay, clockFrequency, FPGAClockToBusClockRatio, useExternalClock, useStrobeGenerator, connect);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAnalogOut16bit(
    unsigned int sequencer,
    unsigned int startAddress,
    unsigned int numberChannels,
    bool signedValue,
    double minVoltage,
    double maxVoltage) {
    if (CLA_AddDeviceAnalogOut16bit)
        return CLA_AddDeviceAnalogOut16bit(sequencer, startAddress, numberChannels, signedValue, minVoltage, maxVoltage);
    else
        return false;
}

bool CControlLightAPI::AddDeviceDigitalOut(
    unsigned int sequencer,
    unsigned int address,
    unsigned int numberChannels) {
    if (CLA_AddDeviceDigitalOut)
        return CLA_AddDeviceDigitalOut(sequencer, address, numberChannels);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAD9854(
    unsigned int sequencer,
    unsigned int address,
    unsigned int version,
    double externalClockFrequency,
    uint8_t PLLReferenceMultiplier,
    unsigned int frequencyMultiplier) {
    if (CLA_AddDeviceAD9854)
        return CLA_AddDeviceAD9854(sequencer, address, version, externalClockFrequency, PLLReferenceMultiplier, frequencyMultiplier);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAD9858(
    unsigned int sequencer,
    unsigned int address,
    double externalClockFrequency,
    unsigned int frequencyMultiplier) {
    if (CLA_AddDeviceAD9858)
        return CLA_AddDeviceAD9858(sequencer, address, externalClockFrequency, frequencyMultiplier);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAnalogIn12bit(
    unsigned int sequencer,
    unsigned int chipSelect,
    bool signedValue,
    double minVoltage,
    double maxVoltage) {
    if (CLA_AddDeviceAnalogIn12bit)
        return CLA_AddDeviceAnalogIn12bit(sequencer, chipSelect, signedValue, minVoltage, maxVoltage);
    else
        return false;
}

