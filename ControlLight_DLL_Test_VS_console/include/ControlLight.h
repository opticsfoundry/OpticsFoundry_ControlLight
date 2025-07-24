#pragma once



//#ifndef USING_DLL
#define _AFXDLL
#include <afxwin.h>         // MFC core and standard components
#include <fstream>
//bool InitializeMFC();
//#endif

#include <cstdint>      // for uint8_t, uint32_t, etc.

#ifdef PYTHON_API
#undef C_API
#define THROW_EXCEPTIONS
#define API_CLASS
#define BUILDING_DLL
#endif

#ifdef API_CLASS
#undef C_API
#endif

#ifdef C_API
#ifdef __cplusplus
extern "C" {
#endif
#else
#ifdef NAMESPACE_CLA
namespace CLA { //optional, for C++ APIs, use namespace CLA, instead of preceding function names with CLA_.
#endif
#endif

#define API_EXPORT
#define API_EXPORT_CLASS


#ifdef API_CLASS

#define API_EXPORT

#ifdef BUILDING_DLL
#ifdef _WIN32
#define API_EXPORT_CLASS __declspec(dllexport)
#else
#define API_EXPORT_CLASS
#endif
#endif

#ifdef USING_DLL 
#ifdef _WIN32
#define API_EXPORT_CLASS __declspec(dllimport)
#else
#define API_EXPORT_CLASS
#endif
#endif

#else

#define API_EXPORT_CLASS
#ifdef BUILDING_DLL
#ifdef _WIN32
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT
#endif
#endif

#ifdef USING_DLL 
#ifdef _WIN32
#define API_EXPORT __declspec(dllimport)
#else
#define API_EXPORT
#endif
#endif

#endif



#ifdef THROW_EXCEPTIONS
#define ERROR_CODE_TYPE void

#include <stdexcept>

	class CLA_Exception : public std::runtime_error {
	public:
		explicit CLA_Exception(const std::string& message)
			: std::runtime_error(message) {}
	};

#else 
#define ERROR_CODE_TYPE bool
#endif

	constexpr int MaxLastError = 10;
	extern std::string LastError[MaxLastError];
	extern int LastErrorIndex;
	extern bool ErrorListWrapAround;
	static bool DisplayErrors = true;

	ERROR_CODE_TYPE AddErrorMessage(const std::string& error_message);


	#if !defined(BUILDING_DLL) && defined(USING_DLL)
	#define API_EXPORT 

			//InitilizeMFC() is needed when using this code directly in an exe, not through a DLL. 
			//If you don't have MFC already initialized, you need to call this function, or a similar one adapted to your purposes.
			//This is not exported to the DLL, because it is automatically called in the DLLMain function when the DLL is loaded.
			//extern bool CLA_InitializeMFC();

	#endif



	#if defined(BUILDING_DLL) || defined(USING_DLL)


			/*
			class CControlApp : public CObject
			{
			public:
				CControlApp();
				virtual ~CControlApp();
				bool Initialize();
				HINSTANCE m_hInstance;
			};
			*/


			//typedef void* HControlLightAPI;
	#ifdef BUILDING_DLL
			BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID);
	#endif
			
	#endif



#ifdef API_CLASS

#define CLA_FN(name) name
#define CLA_FNDEF(name) ControlLight_API::##name

	class API_EXPORT_CLASS ControlLight_API {
	public:
		bool Created;
		ControlLight_API(bool InitializeAfx = true, bool InitializeAfxSocket = true);
		~ControlLight_API();
		bool IsCreated();

#else
#define CLA_FN(name) CLA_##name
#define CLA_FNDEF(name) CLA_##name
#endif



		//API_EXPORT HControlLightAPI CLA_GetInstance();

		//Call these functions in roughly this order

		/** @brief Initializes the ControlAPI.
		 * If you use a bare function C API, then this function must be called before using any other functions in the API.
		* If you didn't load MFC yet (which is the case if you use Qt), then set
		* @param InitializeAfx to true to initialize MFC core
		* @param InitializeAfxSocket to true to initialize MFC socket layer
		* The class wrapped version of this API calls this function in its constructor.
		*/
		/// @param  
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Create)(bool InitializeAfx, bool InitializeAfxSocket); //you must call this first, otherwise the API will not work

		/**  @brief This function must be called when you are finished using the API.
		* The class wrapped version of this API calls this function in its destructor.
		*/
		/// @param  
		/// @return 
		API_EXPORT void CLA_FN(Cleanup)(); //You must call this before leaving your program, otherwise the API can provoke errors because memory is not freed

		/// @brief Check if an error has occurred since the last call to GetLastError
		/// @param  
		/// @return true if an error has occurred since the last call to GetLastError
		API_EXPORT bool CLA_FN(DidErrorOccur)();

		/// @brief returns the last error messages
		/// @param  
		/// @return 
		API_EXPORT const char* CLA_FN(GetLastError)();

		/// @brief Configures if the ControlAPI should display error messages.
		/// @param  _DisplayErrors true to display error messages, false to suppress them.
		/// @return 
		API_EXPORT void CLA_FN(Configure)(bool _DisplayErrors); //optional

		//you can load the configuration from a JSON file, or define the IO devices with the functions further below )(mixing is also possible)
		API_EXPORT ERROR_CODE_TYPE CLA_FN(LoadFromJSONFile)(const char* filename);

		//Once all devices have been declared, you must initialize the system, otherwise the API will not work
		/// @brief After Configuring the hardware, e.g. by loading a json configuration file, this function must be called to initialize the hardware.
		/// @param  
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Initialize)();
		//Some optional commands

		/**  @brief Switches Debug mode on.
		 * In Debug mode, the FPGA sequencers display more information on their USB-UART port, being slowed down a bit by that.
		*/
		/// @param OnOff true to switch on debug mode, false to switch it off.
		/// @param FileName the name of the file to write the debug information to without filename extension.
		API_EXPORT void CLA_FN(SwitchDebugMode)(bool OnOff, const char* FileName);

		/// @brief Checks if the ControlAPI is ready to be used.
		/// @param IsReady true if the ControlAPI is ready to be used, false otherwise.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(IsReady)();

		//To start a sequence, first call StartAssemblingSequence, then add all the commands you want to execute in the sequence

		/// @brief Starts assembling a sequence. Clears any previous sequence. Must be called before adding commands to the sequence.
		/// @param  
		/// @return 
		API_EXPORT void CLA_FN(StartAssemblingSequence)();

		//here are possible commands
		/**  @brief This function sets a register for a device on the sequencer.
		* What this means depends on the device type. This function gives us an easy way to add new functionality to a device without having to programm new DLL functions.
		* SetRegister maps to the register map given in the device's datasheet. Note: this is not currently implemented in the API.
		* @param Sequencer the sequencer to use.
		* @param Address the address of the device to set the value for.
		* @param SubAddress the subaddress of the device to set the value for.
		* @param Data the data to set.
		* @param DataLength_in_bit the length of the data in bits.
		* @param StartBit the start bit of the data to set.
		*/
		/// @param  
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRegister)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);

		/**  @brief This function sets a value for a device on the sequencer.
		* What this means depends on the device type. This function gives us an easy way to add new functionality to a device without having to programm new DLL functions.
		* In contrast to SetRegister, SetValue can execute more complex operations, such as calculating a DDS frequency tuning word from the given frequency and then programming that.
		* There can be several SetValue functions for the same SetRegister function.
		* There can be SetValue functions that set some parameter, or that trigger some action, not even necessarily related to the registers of the device.
		* @param Sequencer the sequencer to use.
		* @param Address the address of the device to set the value for.
		* @param SubAddress the subaddress of the device to set the value for.
		* @param Data the data to set.
		* @param DataLength_in_bit the length of the data in bits.
		* @param StartBit the start bit of the data to set.
		*/
		/// @param  
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetValue)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief As SetValue, but for serial devices.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the value for.
		/// @param SubAddress the subaddress of the device to set the value for.
		/// @param Data the data to set.
		/// @param DataLength_in_bit the length of the data in bits.
		/// @param StartBit the start bit of the data to set.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetValueSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief As SetRegister, but for serial devices.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the value for.
		/// @param SubAddress the subaddress of the device to set the value for.
		/// @param Data the data to set.
		/// @param DataLength_in_bit the length of the data in bits.
		/// @param StartBit the start bit of the data to set.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRegisterSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief Wait for a given time in ms.
		/// @param time_in_ms the time to wait in ms.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Wait_ms)(double time_in_ms);

		/// @brief Get the current time in the currently assembled sequence ms.
		/// @param time_in_ms the current time in ms.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetTime_ms)(double& time_in_ms);


		/// @brief Get the current time of a specific sequencer in the currently assembled sequence ms.
		/// @param Sequencer the sequencer to use.
		/// @param time_in_ms the current time in ms.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetTimeOfSequencer_ms)(const unsigned int& Sequencer, double& time_in_ms);
		
		/// @brief Get the time debtof a specific sequencer in the currently assembled sequence ms.
		/// @param Sequencer the sequencer to use.
		/// @param time_debt_in_ms the current time in ms.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetTimeDebtOfSequencer_ms)(const unsigned int& Sequencer, double& time_debt_in_ms);

		//the following are convenience functions, which allow us to define nice names to the few most important functions
		//You can add as many convenience functions as you like. Make sure to copy them also into the list of convenience functions in CDevice, CDevice.h, to assure they can always be called in any device.
		//Then define them in the device that provides the function. In this way we use the inheritance mechanism to automatically check if the function is available in the device and )(optionally) produce an error if not.

		//Analog out
		// Convenience functions to easily access the most important functions of the devices
		/// @brief Sets the voltage of an analog output device.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the voltage for.
		/// @param Voltage the voltage to set.
		//Analog out
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetVoltage)(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
		
		//Digital out
		/// @brief Sets a digital output to high or low.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the digital output for.
		/// @param BitNr the bit number of the digital output to set.
		/// @param OnOff true to set the output to high, false to set it to low.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetDigitalOutput)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);

		//AD9854
		/// @brief Sets the start frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the start frequency for.
		/// @param Frequency the frequency to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStartFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the stop frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the stop frequency for.
		/// @param Frequency the frequency to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStopFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the modulation frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the modulation frequency for.
		/// @param Frequency the frequency to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetModulationFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the power of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the power for.
		/// @param Power the power to set in % (0...100).
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPower)(const unsigned int& Sequencer, const unsigned int& Address, double Power);

		/// @brief Sets the attenuation of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the attenuation for.
		/// @param Attenuation the attenuation to set in dB (-xxx ... 0).
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetAttenuation)(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);

		/// @brief Sets the start frequency tuning word of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the start frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStartFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		/// @brief Sets the stop frequency tuning word of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the stop frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStopFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		/// @brief Sets the FKS mode of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the FKS mode for.
		/// @param mode the FKS mode to set (0..4).
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFSKMode)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);

		/// @brief Sets the ramp rate clock of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the ramp rate clock for.
		/// @param rate the ramp rate clock to set (1...1048576).
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRampRateClock)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);

		/// @brief Clears the ACC1 bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the ACC1 bit for.
		/// @param OnOff true to set the ACC1 bit, false to clear it.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetClearACC1)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		/// @brief Sets the triangle bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the triangle bit for.
		/// @param OnOff true to set the triangle bit, false to clear it.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetTriangleBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		/// @brief Sets the FSK bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the FSK bit for.
		/// @param OnOff true to set the FSK bit, false to clear it.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFSKBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		
		//AD9858
		/// @brief Sets the frequency of a DDS.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency for.
		/// @param Frequency the frequency to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the frequency tuning word of a DDS.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set. 
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		//those two functions have been defined already in the context of the AD9854, so we don't need to redefine them here
		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPower)(const unsigned int& Sequencer, const unsigned int& Address, double Power);//same as for AD9854, no need to redefine
		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SetAttenuation)(const unsigned int& Sequencer, const unsigned int& Address, double Power);

		//AD9958
		/// @brief Sets the frequency of a multi channel DDS (for now the AD9958).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency for.
		/// @param channel the channel number to set the frequency for.
		/// @param Frequency the frequency to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
		
		/// @brief Sets the frequency tuning word of a multi channel DDS (for now the AD9958).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency tuning word for.
		/// @param channel the channel number to set the frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyTuningWordOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
		
		/// @brief Sets the phase of a multi channel DDS (for now the AD9958).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the phase for.
		/// @param channel the channel number to set the phase for.
		/// @param Phase the phase to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPhaseOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);

		/// @brief Sets the phase of a multi channel DDS (for now the AD9958).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the phase for.
		/// @param channel the channel number to set the phase for.
		/// @param Phase the phase to set.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPowerOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);





		// once the sequence is assembled, then execute it 
		/// @brief Executes the sequence that was previously assembled.
		/// @param  
		/// @return 
		API_EXPORT void CLA_FN(ExecuteSequence)();

		//check how far the sequence has been executed
		/// @brief Get the current status of the sequence execution.
		/// @param  running returns true if the sequence is running, false if it is not.
		/// @param DataPointsWritten returns number of data points that were already written out by the FPGA sequencer(s).
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetSequenceExecutionStatus)(bool& running, unsigned long long& DataPointsWritten);

		//Wait till the sequence is finished, and get the data from the input devices
		/// @brief Waits until the sequence is finished, then gets the input data from the FPGA sequencer.  
		/// @param buffer pointer to the input data buffer. Don't delete this buffer, it is managed by the API.
		/// @param buffer_length length of the input data buffer in bytes.
		/// @param EndTimeOfCycle returns the end time of the cycle in ms.
		/// @param timeout_in_s timeout in seconds. If the sequence is not finished within this time, the function returns false or throws an exception (depending on mode selected mode when compiling API).
		API_EXPORT ERROR_CODE_TYPE CLA_FN(WaitTillEndOfSequenceThenGetInputData)(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
		
		/// @brief Sets a guard time. If sequencer commands make the time advance more than the guard time beyond what's allowed by Wait_ms, an error will be recorded (check with DidErrorOccur() if that happened) or thrown.
		/// @param MaxTimeDebt_in_ms maximum time debt in ms. If the sequencer commands make the time advance more than this, the sequencer will stop and wait for the next command.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetTimeDebtGuard_in_ms)(const double& MaxTimeDebt_in_ms);

		/** @brief Start analog acquisition on the specified channel.
		 * This function places a commandto for the FPGA in the sequencer buffer.
		 * The analog in acquisition will start when this command is executed by the FPGA.
		 * Only one analog in acquisition can hapen at each moment.
		 * A new one can start right after the previous one is finished.
		 * The data will be returned at the end of a sequence when using
		 * CLA_WaitTillEndOfSequenceThenGetInputData.
		 * @param Sequencer the sequencer to use.
		 * @param ChannelNumber the channel number to use.
		 * @param NumberOfDataPoints the number of data points to acquire.
		 * @param DelayBetweenDataPoints_in_ms the delay between data points in ms.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerStartAnalogInAcquisition)(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
		
		/**  @brief Writes a value to the input memory of the sequencer.
		 * This is useful to mark the start or end of a data acquisition, or to mark the type of experimental run in the full fledged version of the ControlAPI, which can cycle sequences automatically in the background.
		* To execute this function, a command for the FPGA must be placed in the sequencer buffer.
		* @param Sequencer the sequencer to use.
		* @param input_buf_mem_data the data to write to the input memory.
		* @param write_next_address true to write the next address, false to write the address given.
		* @param input_buf_mem_address the address to write to, if write_next_address is false.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerWriteInputMemory)(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
		
		/// @brief Writes the current FPGA clock ticks to the input memory of the sequencer.
		/// @param  Sequencer the sequencer to use.
		/// 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerWriteSystemTimeToInputMemory)(const unsigned int& Sequencer);

		/// @brief Switches the debug LED of the FPGA on or off.
		/// @param  Squencer the sequencer to use.
		/// @param  OnOff true to switch on the LED, false to switch it off.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSwitchDebugLED)(const unsigned int& Sequencer, unsigned int OnOff);

		/**  @brief Switches the sequencer to ignore TCP/IP commands.
		 * This is useful to prevent the sequencer from being interrupted by TCP/IP commands while executing a timing critical task, such as transferring input data from the FPGA BRAM to the DDR.
		 * This can be useful if lots of data is acquired at a high rate. Lot's of meaning: more than the size of the BRAM input buffer.
		 * How much that is depends how you configured the Vivado project that generates the FPGA sequencer firmware.
		 */
		 /// @param  
		 /// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerIgnoreTCPIP)(const unsigned int& Sequencer, bool OnOff);

		/// @brief Places a marker in the sequencer buffer. The FPGA SOM's CPU will see this marker and can react to it, e.g. write it to the USB-UART for debugging.
		/// @param  
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerAddMarker)(const unsigned int& Sequencer, unsigned char marker);




		//the following functions are used to add devices to the sequencer. I placed them here to aAPI_EXPORT void clutter above. They have to be called before Initialize)().	
		/// @brief Add a device sequencer to the list.
		/// @param id the id of the device sequencer to add (this is the number that all other commands are using when sending data to a device on this sequencer).
		/// @param type the type of the device sequencer to add.
		/// @param ip the ip address of the device sequencer to add.
		/// @param port the port of the device sequencer to add.
		/// @param master true if the device sequencer is a master, false if it is a slave.
		/// @param startDelay the start delay of the device sequencer to add.
		/// @param clockFrequency the clock frequency of the device sequencer to add.
		/// @param FPGAClockToBusClockRatio the FPGA clock to bus clock ratio of the device sequencer to add.
		/// @param useExternalClock true if the device sequencer should use an external clock, false if it should use the internal clock.
		/// @param useStrobeGenerator true if the device sequencer should use a strobe generator, false if it should not.
		/// @param connect true if the device sequencer should connect to the device, false if it should not. 
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceSequencer)(
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

		/// @brief Add a 16 bit analog output device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param startAddress the start address of the device to add.
		/// @param numberChannels the number of channels of the device to add.
		/// @param signedValue true if the device is signed, false if it is unsigned.
		/// @param minVoltage the minimum voltage of the device to add.
		/// @param maxVoltage the maximum voltage of the device to add.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAnalogOut16bit)(
			unsigned int sequencer,
			unsigned int startAddress,
			unsigned int numberChannels,
			bool signedValue,
			double minVoltage,
			double maxVoltage);

		/// @brief Add a digital output device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param numberChannels the number of channels of the device to add.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceDigitalOut)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int numberChannels);


		/// @brief Add a AD9854 device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param version the version of the device to add.
		/// @param externalClockFrequency the external clock frequency of the device to add.
		/// @param PLLReferenceMultiplier the PLL reference multiplier of the device to add.
		/// @param frequencyMultiplier the frequency multiplier of the device to add.
		/// @return 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAD9854)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int version,
			double externalClockFrequency,
			uint8_t PLLReferenceMultiplier,
			unsigned int frequencyMultiplier);

		/// @brief Add a AD9858 device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param externalClockFrequency the external clock frequency of the device to add.
		/// @param frequencyMultiplier the frequency multiplier of the device to add.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAD9858)(
			unsigned int sequencer,
			unsigned int address,
			double externalClockFrequency,
			unsigned int frequencyMultiplier);

		/// @brief Add a 12 bit analog input device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param chipSelect the chip select of the device to add.
		/// @param signedValue true if the device is signed, false if it is unsigned.
		/// @param minVoltage the minimum voltage of the device to add.
		/// @param maxVoltage the maximum voltage of the device to add.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAnalogIn12bit)(
			unsigned int sequencer,
			unsigned int chipSelect,
			bool signedValue,
			double minVoltage,
			double maxVoltage);


#ifdef API_CLASS
		};

#endif
	

#ifdef C_API
#ifdef __cplusplus
}
#endif
#else
#ifdef NAMESPACE_CLA
	} //option namespace CLA
#endif
#endif
