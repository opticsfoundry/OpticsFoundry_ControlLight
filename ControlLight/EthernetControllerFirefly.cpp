// OvenControl.cpp: implementation of the CEthernetControllerFirefly class.
//
//////////////////////////////////////////////////////////////////////

#include <afxwin.h>
#include "EthernetControllerFirefly.h"
#include "std.h"
#include "ControlAPI.h"
#include "CDeviceSequencer.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


constexpr uint32_t MaxReconnectAttempts = 100;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const uint32_t MaxFPGAProgramLength = 0x100000; //*8 to convert to bytes
const unsigned long MaxSequencerCommandListSize = 1024*16;
constexpr bool MinimizeEthernetCommunicationDirectionChanges = true;

CEthernetControllerFirefly::CEthernetControllerFirefly(CDeviceSequencer* _MySequencer) :
	CNetworkClient(/*ethernet communication mode */ 3, /* FastWrite*/false) {
	DebugFilename = "DebugBufferSequencer";
	MySequencer = _MySequencer;
	previous_command_buffer = NULL;
	previous_command_buffer_length = 0;
	DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible = false;
//	SequencerCommandList = new uint32_t[2 * MaxSequencerCommandListSize];
//	SequencerCommandListSize = 0;
	SequencerCommandList = nullptr;
	Connected = false;
	DebugBufferOn = false;
	DebugBufferFile = NULL;
	ExternalTrigger0 = false;
	ExternalTrigger1 = false;
	ExternalClock0 = false;
	ExternalClock1 = false;
	PeriodicTriggerPeriod_in_s = 0;
	PeriodicTriggerAllowedWait_in_s = 0;
	FPGAUseStrobeGenerator = false;
	//FPGABuffer = new uint32_t[MaxFPGAProgramLength * 4];
	//FPGAAbsoluteTime = new uint32_t[MaxFPGAProgramLength];
	//FPGABufferUsed = 0;
	core_option_LED = false;
	core_option_SPI_CS = 0;
	core_option_dig_out = 0;
	core_option_PL_to_PS = 0;
	SetPeriodicTriggerAtBeginningOfNextSequence = false;
	WaitForPeriodicTriggerAtBeginningOfSequence = false;
	LastPeriodicTriggerPeriod_in_s = 0;
	DebugModeOn = false;
	previous_command_buffer_ptr = nullptr;
}

CEthernetControllerFirefly::~CEthernetControllerFirefly()
{
	if (DebugBufferFile) delete DebugBufferFile;
	//if (previous_command_buffer) delete previous_command_buffer;
	if (Connected) CloseConnection();
	//delete FPGABuffer;
	//delete FPGAAbsoluteTime;
	if (SequencerCommandList) delete SequencerCommandList;
	if (previous_command_buffer_ptr) delete previous_command_buffer_ptr;
}

bool CEthernetControllerFirefly::ConnectSocket(LPCTSTR lpszAddress, UINT port, unsigned int aFPGAClockToBusClockRatio, double aFPGAClockFrequencyInHz, bool aFPGAUseExternalClock, bool aFPGAUseStrobeGenerator, bool aExternalTrigger) {
	Connected = CNetworkClient::ConnectSocket(lpszAddress, port, "EthernetMultiIOController");
	//BusFrequency = aBusFrequency;
	FPGAClockToBusClockRatio = aFPGAClockToBusClockRatio;
	FPGAClockFrequencyInHz = aFPGAClockFrequencyInHz;
	FPGAUseExternalClock = aFPGAUseExternalClock;
	FPGAUseStrobeGenerator = aFPGAUseStrobeGenerator;
	ExternalTrigger0 = aExternalTrigger;
	return Connected;
}

double CEthernetControllerFirefly::GetBusFrequency() {
	//return BusFrequency;
	return FPGAClockFrequencyInHz / FPGAClockToBusClockRatio;  
}

void CEthernetControllerFirefly::AddSequencerCommandToBuffer(uint32_t* buffer, uint32_t n, uint32_t high_buffer, uint32_t low_buffer) {

	buffer[n * 2 + 0] = low_buffer;
	buffer[n * 2 + 1] = high_buffer;

	if (DebugBufferFile) {
		CString buf;
		buf.Format("%8u CMD %x buffer = %08X %08X ", n, high_buffer, low_buffer);
		(*DebugBufferFile) << buf << endl;
	}
}

void CEthernetControllerFirefly::ClearSequencerCommandList() {
	//SequencerCommandListSize = 0;
}

void CEthernetControllerFirefly::AddSequencerCommand(uint32_t high_word, uint32_t low_word) {
	/*if (SequencerCommandListSize >= MaxSequencerCommandListSize) {
		AddErrorMessageCString("EthernetMultiIOControllerFirefly.cpp: AddSequencerCommand(): sequencer command list has too little memory, increase MaxSequencerCommandListSize.");
		return MaxSequencerCommandListSize - 1;
	}
	SequencerCommandList[SequencerCommandListSize * 2] = low_word;
	SequencerCommandList[SequencerCommandListSize * 2 + 1] = high_word;
	unsigned int CurrentCommandNr = SequencerCommandListSize;
	SequencerCommandListSize++;
	return CurrentCommandNr;*/
	if (DebugBufferFile) {
		CString buf;
		buf.Format("data = %08X %08X", high_word, low_word);
		(*DebugBufferFile) << buf << endl;
	}
	MySequencer->AddCommandToSequence(high_word, low_word);
}



void CEthernetControllerFirefly::DebugBuffer(CString filename) {
	DebugBufferFile = new ofstream(filename, ios::out);
	DebugBufferOn = true;
}

/*
Assumptions for removal of unused bus entries:
Buffer[2 * i] = Data;
Buffer[2 * i + 1] = DirectionOutBit | StrobeBit | Address;
Address consists of 8 low lying main address bits and three (in principle up to six) high lying sub address bits
const unsigned int StrobeBit = 0x0100;

const unsigned int BusBitShift = 8;  //Attention: the subbus bits will get shifted by an additional two bits in CMultiIO::GetBusContents to make space for direction and strobe bits
const unsigned int Bus0 = 0x0 << BusBitShift;
...
const unsigned int BusSequencerSpecialCommand = 0x7 << BusBitShift;
const unsigned int Lock = 0x8 << BusBitShift;
*/


constexpr uint8_t CMD_STOP = 0;
constexpr uint8_t CMD_STEP = 1;
constexpr uint8_t CMD_STEP_AND_ENTER_FAST_MODE = 2;
constexpr uint8_t CMD_SET_OPTIONS = 3;
constexpr uint8_t CMD_LOAD_REG_LOW = 4;
constexpr uint8_t CMD_LOAD_REG_HIGH = 5;
constexpr uint8_t CMD_LATCH_STATE = 6;
constexpr uint8_t CMD_RESET_WAIT_CYCLES = 7;
constexpr uint8_t CMD_LONG_WAIT = 8;
constexpr uint8_t CMD_SET_STROBE_OPTIONS = 9;
constexpr uint8_t CMD_SET_INPUT_BUF_MEM = 10;
constexpr uint8_t CMD_WAIT_FOR_TRIGGER = 11;
constexpr uint8_t CMD_SET_LOOP_COUNT = 12;
constexpr uint8_t CMD_CONDITIONAL_JUMP_FORWARD = 13;
constexpr uint8_t CMD_CONDITIONAL_JUMP_BACKWARD = 14;
constexpr uint8_t CMD_I2C_OUT = 15;
constexpr uint8_t CMD_SPI_OUT_IN = 16;
constexpr uint8_t CMD_INPUT_REPEATED_OUT_IN = 17;
constexpr uint8_t CMD_SET_PERIODIC_TRIGGER_PERIOD = 18;
constexpr uint8_t CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME = 19;
constexpr uint8_t CMD_WAIT_FOR_PERIODIC_TRIGGER = 20;
constexpr uint8_t CMD_WAIT_FOR_WAIT_CYCLE_NR = 21;
constexpr uint8_t CMD_DIG_IN = 22;
constexpr uint8_t CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT = 23;
constexpr uint8_t CMD_ANALOG_IN_OUT = 24;
constexpr uint8_t CMD_PL_TO_PS_COMMAND = 25;
constexpr uint8_t CMD_LOAD_COMMAND_BUFFER = 26;
constexpr uint8_t CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM = 27;
constexpr unsigned char CMD_CALC_AD9854_FREQUENCY_TUNING_WORD = 28;

constexpr unsigned char NrCommands = 29;
const CString CommandNames[NrCommands] = { "CMD_STOP", "CMD_STEP", "CMD_STEP_AND_ENTER_FAST_MODE", "CMD_SET_OPTIONS", "CMD_LOAD_REG_LOW", "CMD_LOAD_REG_HIGH", "CMD_LATCH_STATE", "CMD_RESET_WAIT_CYCLES", "CMD_LONG_WAIT", "CMD_SET_STROBE_OPTIONS", "CMD_SET_INPUT_BUF_MEM", "CMD_WAIT_FOR_TRIGGER", "CMD_SET_LOOP_COUNT", "CMD_CONDITIONAL_JUMP_FORWARD", "CMD_CONDITIONAL_JUMP_BACKWARD", "CMD_I2C_OUT", "CMD_SPI_OUT_IN", "CMD_INPUT_REPEATED_OUT_IN", "CMD_SET_PERIODIC_TRIGGER_PERIOD", "CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME", "CMD_WAIT_FOR_PERIODIC_TRIGGER", "CMD_WAIT_FOR_WAIT_CYCLE_NR", "CMD_DIG_IN", "CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT", "CMD_ANALOG_IN_OUT", "CMD_PL_TO_PS_COMMAND", "CMD_LOAD_COMMAND_BUFFER", "CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM", "CMD_CALC_AD9854_FREQUENCY_TUNING_WORD" };
constexpr bool CommandUsesBuffer[NrCommands] = { false   , false     , false                         , true             , false             , false              , false            , false                  , true           , true                    , true                   , true                  , true                , true                          , true                           , true         , true            , true                       , true                             , true                                        , false                          , true                        , true        , false                                  , true               , true                  , false                , false												, true };
constexpr bool UseCommandBuffer = true;

void CEthernetControllerFirefly::StartAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {
	//if (channel_nr < 2) {
	//	StartXADCAnalogInAcquisition(channel_nr, number_of_datapoints, delay_between_datapoints_in_ms);
//	} else {
		StartSPIAnalogInAcquisition(channel_nr /* - 2*/, number_of_datapoints, delay_between_datapoints_in_ms);
	//}
}

void CEthernetControllerFirefly::AddSequencerCommandToSequenceList(uint32_t high_buffer, uint32_t low_buffer) {
	if (UseCommandBuffer) {
		const unsigned int command = low_buffer & 0x1F;
		if (command <= NrCommands) {
			if (CommandUsesBuffer[command]) {
				uint32_t low_command_buffer = (low_buffer & 0xFFFFFFE0) | (0x1F & CMD_LOAD_COMMAND_BUFFER);
				AddSequencerCommand(high_buffer, low_command_buffer);
			}
		}
	}
	AddSequencerCommand(high_buffer, low_buffer);
}

void CEthernetControllerFirefly::StartXADCAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {

	//only for debugging
	//for (int i = 0; i < 15; i++) {
	//	AddCommandAnalogInOut(/* adc_register_address */ 3, /* adc_write_enable */ 0, /* adc_programming_out */ 0, /* dont_execute_now*/ 0, /* only_read_write*/ 0,/*wait_time*/ 150);
	//}
	/*
	* const u8 command_mask = 0x1F;  //5 bit
	CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command[15:15];
                            adc_programming_out <= command[31:16];   
                            wait_time[29:0] <= command[63:34];
                            wait_time[47:30] <= 0;                         
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                        end*/
	uint8_t command = CMD_ANALOG_IN_OUT;
	uint32_t adc_register_address = 3;
	uint32_t adc_write_enable = 0;
	uint32_t adc_programming_out = 0;
	uint32_t wait_time = 0;
	uint8_t start_now = 0;
	uint8_t adc_conversion = 1;
	uint32_t low_buffer = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (0x1F & command);
	uint32_t high_buffer = ((wait_time & 0x3FFF) << (34 - 32)) | ((start_now & 0x01) << (32 - 32)) | ((adc_conversion & 0x01) << (33 - 32));


	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	/*

	CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
							INPUT_REPEAT_repeats <= command[27:8];
							INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
							INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
							INPUT_REPEAT_state <= INPUT_REPEAT_START;
							INPUT_REPEAT_command <= command[59:57];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end

					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
					if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
					if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;
	
	*/
	command = CMD_INPUT_REPEATED_OUT_IN;
	uint32_t INPUT_REPEAT_repeats = number_of_datapoints;
	uint32_t INPUT_REPEAT_wait = floor(delay_between_datapoints_in_ms * FPGAClockFrequencyInHz / 1000);
	uint32_t INPUT_REPEAT_command = 4; //XADC analog in
	uint32_t INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 0;

	low_buffer = (INPUT_REPEAT_repeats << 8) | command;
	high_buffer = INPUT_REPEAT_wait | (INPUT_REPEAT_trigger_secondary_interrupt_when_finished << (56-32)) | (INPUT_REPEAT_command << (57-32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
	uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	if (DelayMultiplier < 1) DelayMultiplier = 1;
	AddProgramLineToSequenceList(1, 0, DelayMultiplier*3-2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the two commands above only consume 2 FPGA cycles, but are accounted for with 2 bus cycles by COutput
}

/*
//Vitis command:
CMD_SET_INPUT_BUF_MEM: begin
							//output reg [11:0] input_buf_mem_address = 0;
							//output reg [31:0] input_buf_mem_data = 0;
							if (command_buffer[7:7] == 1) input_buf_mem_address <= command_buffer[20:8];
							//else input_buf_mem_address <= input_buf_mem_address + 1;
							input_buf_mem_data <= command_buffer[63:32];
							//input_buf_mem_write_next_cycle <= 2;
							INPUT_MEM_state <= INPUT_MEM_WRITE;
							wait_time <= 1;
							address <= address + 1;
						end
*/

//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandWriteInputBuffer(unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
	uint8_t command = CMD_SET_INPUT_BUF_MEM;
	uint32_t low_buffer = ((input_buf_mem_address & 0x7FF) << 8) | (((write_next_address) ? 0 : 1) << 7) | (0x1F & command);
	uint32_t high_buffer = input_buf_mem_data;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}


/*
CMD_SET_LOOP_COUNT: begin
	loop_count <= command_buffer[63:32];
	address <= address + 1;
	wait_time <= 1;
end
*/
void CEthernetControllerFirefly::AddCommandSetLoopCount(unsigned int loop_count) {
	unsigned char command = CMD_SET_LOOP_COUNT;
	unsigned __int32 low_buffer = command;
	unsigned __int32 high_buffer = (loop_count & 0xFFFFFFFF);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
CMD_CONDITIONAL_JUMP_FORWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
	if ((condition_0 && (command_buffer[8:8] == 1)) || (condition_1 && (command_buffer[9:9] == 1)) || (condition_PS && (command_buffer[10:10] == 1)) || (command_buffer[11:11] == 1))
		address <= address + command_buffer[15:8];
	else address <= address + 1;
	wait_time <= 1;
end
*/
void CEthernetControllerFirefly::AddCommandJumpForward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS) {
	unsigned char command = CMD_CONDITIONAL_JUMP_FORWARD;
	unsigned __int32 low_buffer = ((jump_length & 0xFF) << 8) | (condition_0 ? 0x100 : 0) | (condition_1 ? 0x200 : 0) | (condition_PS ? 0x400 : 0) | (unconditional_jump ? 0x800 : 0) | command;
	unsigned __int32 high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
CMD_CONDITIONAL_JUMP_BACKWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
	if (((condition_0 && (command_buffer[8:8] == 1)) || (condition_1 && (command_buffer[9:9] == 1)) || (condition_PS && (command_buffer[10:10] == 1)) ||
			(command_buffer[11:11] == 1) || ((command_buffer[12:12] == 1) && (loop_count > 0))))
		address <= address - command_buffer[15:8];
	else address <= address + 1;
	loop_count <= loop_count - 1;
	wait_time <= 1;
end
*/

void CEthernetControllerFirefly::AddCommandJumpBackward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool loop_count_greater_zero) {
	unsigned char command = CMD_CONDITIONAL_JUMP_BACKWARD;
	unsigned __int32 low_buffer = ((jump_length & 0xFF) << 8) | (condition_0 ? 0x100 : 0) | (condition_1 ? 0x200 : 0) | (condition_PS ? 0x400 : 0) | (unconditional_jump ? 0x800 : 0) | (loop_count_greater_zero ? 0x1000 : 0) | command;
	unsigned __int32 high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


void CEthernetControllerFirefly::AddCommandWriteSystemTimeToInputMemory() {
	uint8_t command = CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM;
	uint32_t low_buffer =  (0x1F & command);
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


/*
CMD_CALC_AD9854_FREQUENCY_TUNING_WORD: begin
							//we assume that the result of the ADC conversion is available in input_buf_mem_data_SPI
							AD9854FTWIntermediate[15:0] <= input_buf_mem_data_SPI[15:0];
							AD9854FTWIntermediate[47:16] <= 0;
							calc_ad9854_ftw_state <= CALC_AD9854_FTW_START;
							address <= address + 1;
							wait_time <= 3; //wait for ftw calculation to finish. a wait_time of 2 should be sufficient. 3 for safety.
						end


...


//The frequency tuning word is proportional to the period of the DDS output frequency, i.e. it's 1/frequency
			//The desired frequency is
			// f = f0 + c * voltage = f0 + deltaf
			// voltage is the 16-bit value provided by the DAC
			//We don't want to use a multiplication. Instead we approximate, using epsilon = deltaf/f0 << 1.
			// delta f = c* voltage
			// ftw = 1/f = 1/(f0 + deltaf) = 1/ (f0 * (1 + deltaf/f0)) = (1/f0)*(1/(1+epsilon)) ~ ftw0 * (1-epsilon)
			//     = ftw0 - ftw0*deltaf/f0 = ftw0 - ftw0 * ftw0 * c * voltage = ftw0 - scale * ADC_value
			//We replace the multiplication by a bitshift, i.e. we allow scale = 2^n with n=[0...32].
			case (calc_ad9854_ftw_state)
				CALC_AD9854_FTW_IDLE: begin
					send_AD9854_ftw <= 0;
					end
				CALC_AD9854_FTW_START: begin
					//shift 16-bit ADC value by up to 32 bit to align with 48-bit ftw
					//here we assume unsigend bit shift. For signed, use <<<
					AD9854FTWShifted <= AD9854FTWIntermediate << command_buffer[12:8];
					calc_ad9854_ftw_state <= CALC_AD9854_FTW_STOP;
				end
				CALC_AD9854_FTW_STOP: begin
				   AD9854FTW = command_buffer[63:16] - AD9854FTWShifted; //user provided FTW - shifted ADC value
				   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
				   send_AD9854_ftw <= 1;
				   AD9854_ftw_byte_shifts_to_do <= 6;
				end
				default: begin
				   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
				end
			endcase
*/

void CEthernetControllerFirefly::AddCommandCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift) {
	unsigned char command = CMD_CALC_AD9854_FREQUENCY_TUNING_WORD;
	uint16_t help = bit_shift;
	uint32_t help2 = (ftw0 << 16) & 0xFFFF0000;
	unsigned __int32 low_buffer = (ftw0 << 16) & (help << 8) & (0x1F & command);
	unsigned __int32 high_buffer = ftw0 >> 16;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
//Vitis command:
void AddCommandAnalogInOut(u8 adc_register_address, u8 adc_write_enable, u16 adc_programming_out, u8 dont_execute_now, u8 only_read_write, u32 wait_time) {
	const u8 command_mask = 0x1F;  //5 bit
	u8 command = 24; //CMD_ANALOG_IN_OUT
	source[next_command * 2 + 0] = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (command_mask & command);
	source[next_command * 2 + 1] = ((wait_time & 0x3FFF) << (34 - 32)) | ((dont_execute_now & 0x01) << (32 - 32)) | ((only_read_write & 0x01) << (33 - 32));
	next_command++;
}*/

//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandAnalogInOut(uint8_t adc_register_address, uint8_t adc_write_enable, unsigned __int16 adc_programming_out, uint8_t dont_execute_now, uint8_t only_read_write, uint32_t wait_time) {
	
	



	/*
		CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command[15:15];
                            adc_programming_out <= command[31:16];   
                            wait_time[29:0] <= command[63:34];
                            wait_time[47:30] <= 0;                         
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                        end*/
	uint8_t command = CMD_ANALOG_IN_OUT;
	uint32_t low_buffer = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (0x1F & command);
	uint32_t high_buffer = ((wait_time & 0x3FFF) << (34 - 32)) | ((dont_execute_now & 0x01) << (32 - 32)) | ((only_read_write & 0x01) << (33 - 32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}



void CEthernetControllerFirefly::AddCommandSetCoreOption_LED(bool a_core_option_LED) {
	core_option_LED = a_core_option_LED;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_SPI_CS(uint8_t a_core_option_SPI_CS) {
	core_option_SPI_CS = a_core_option_SPI_CS;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_dig_out(uint8_t a_core_option_dig_out) {
	core_option_dig_out = a_core_option_dig_out;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_PL_to_PS(uint8_t a_core_option_PL_to_PS) {
	core_option_PL_to_PS = a_core_option_PL_to_PS;
	AddCommandSetCoreOptions();
}

//Vitis command:
/*
	CMD_SET_OPTIONS:begin
			   options <= command[63:32];
						end

		options[0] = core_options_LED;
		options[1..4] = SPI_CS;
		options[8..15] = core_dig_out;
		options[24..31] = options_PL_to_PS;

	*/
	//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandSetCoreOptions() {
	uint8_t command = CMD_SET_OPTIONS;
	uint32_t low_buffer = command;
	uint32_t high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}
//Vitis command:
/*
CMD_PL_TO_PS_COMMAND: begin
PL_to_PS_command <= command_buffer[23:8];
address <= address + 1;
wait_time <= 0;
end
*/
//Vitis command converted to Visual Studio command:
constexpr unsigned int PLToPSCommandTest = 1 << 8;
constexpr unsigned int PLToPSCommandIgnoreTCPIP = (2<<8) | 1;
constexpr unsigned int PLToPSCommandAcceptTCPIP = (2 << 8);
void CEthernetControllerFirefly::AddCommandSetPLToPSCommand(unsigned int PLToPSCommand) {
	uint8_t command = CMD_PL_TO_PS_COMMAND;
	uint32_t low_buffer = ((PLToPSCommand & 0xFFFF) << 8) | command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


void CEthernetControllerFirefly::AddCommandWait(unsigned long Wait_in_FPGA_clock_cycles) {
	/*
			 CMD_STEP:begin
							wait_time[30:0] <= command[35:5];
							wait_time[47:31] <= 0;
							bus_data <= command[63:36];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end
			*/
	uint8_t command = CMD_STEP;
	if (Wait_in_FPGA_clock_cycles == 0) Wait_in_FPGA_clock_cycles = 1;
	uint32_t low_buffer = (Wait_in_FPGA_clock_cycles <<5 ) | command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddCommandStop() {
	uint8_t command = CMD_STOP;
	uint32_t low_buffer = command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

void CEthernetControllerFirefly::WriteReadSPI(unsigned int chip_select, unsigned int number_of_bits_out, unsigned __int64 data_high, unsigned __int64 data_low, unsigned int number_of_bits_in) {

	//total data length can be up to 128-(2*5) = 118 bits (because we need 2x 5 bits for the two load commands).
	uint8_t command = CMD_LOAD_REG_LOW;
	uint32_t low_buffer = ((data_low & 0x07FFFFFF) << 5)  | command; //lowest 27 bit
	uint32_t high_buffer = (data_low >> 27) & 0xFFFFFFFF; //bit 28 to 59 (5 bits missing)
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
	uint8_t NrCommands = 2;
	if (number_of_bits_out > 59) {
		command = CMD_LOAD_REG_HIGH;
		low_buffer = ((data_high & 0x003FFFFF) << 10) | ((data_low >> (27 + 32)) & 0x1F) | command; /*last 5 bits of data_low; if needed data can be extended*/
		high_buffer = (data_high >> 22) & 0xFFFFFFFF;  //data_high can have up 54 bits.
		AddSequencerCommandToSequenceList(high_buffer, low_buffer);
		NrCommands = 3;
	}


	/*
	CMD_SPI_OUT_IN : begin
							SPI_OUT_length <= command[14:8];
							SPI_IN_length <= command[20:16];
							SPI_SEL_next <= command[33:32];
							SPI_chip_select_next <= command[37:34];
							SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register
							INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
							if (command[40:40] == 0) SPI_state <= SPI_START;
						end
	*/

	command = CMD_SPI_OUT_IN;
	uint32_t SPI_out_length = number_of_bits_out;
	uint32_t SPI_in_length = number_of_bits_in;
	//uint32_t SPI_CS = 1;  
	uint32_t wait_time = 0;
	uint8_t SPI_SEL_next = 0x01; //2 SPI ports, 1 means active under PL control; if inactive they are under PS control
	uint8_t SPI_chip_select_next = chip_select; //4 CS lines, low means active.
	unsigned int start_now = 1;
	low_buffer = (SPI_in_length << 16) | (SPI_out_length << 8) | command;
	high_buffer = (start_now << (40 - 32)) | (SPI_chip_select_next << 2) | (0x03 & SPI_SEL_next);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);

	uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	if (DelayMultiplier < 1) DelayMultiplier = 1;
	//the following code needs to be checked if timing is crucial
	if (DelayMultiplier == 1) {
		AddProgramLineToSequenceList(1, 0, DelayMultiplier * NrCommands - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the three commands above only consume 3 FPGA cycles, but are accounted for with 3 bus cycles by COutput
		AddProgramLineToSequenceList(1, 0, DelayMultiplier * NrCommands - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the three commands above only consume 3 FPGA cycles, but are accounted for with 3 bus cycles by COutput
	}
	else {
		if ((DelayMultiplier - 1) *NrCommands>0)
			AddProgramLineToSequenceList(1, 0, (DelayMultiplier-1) * NrCommands); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the three commands above only consume 3 FPGA cycles, but are accounted for with 3 bus cycles by COutput
	}

}


void CEthernetControllerFirefly::StartSPIAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {
	

	//only for debugging
	//AddCommandSetCoreOption_LED(true);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 0, /*input_buf_mem_data*/0xDEADBEEF, /* wait_time_in_FPGA_cycles*/ 5);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 1, /*input_buf_mem_data*/0x1234ABCD, /* wait_time_in_FPGA_cycles*/ 5);

	/*
	CMD_LOAD_REG_LOW:begin
                            register[58:0] <= command[63:5];
                        end
    CMD_LOAD_REG_HIGH:begin
                            register[117:59] <= command[63:5];
                        end 
	*/

	uint32_t SPI_SINGLE_ENDED_INPUT = 1;
	uint32_t SPI_ANALOG_IN_NR = channel_nr;
	uint32_t SPI_IN_NR_REVERSED = (((SPI_ANALOG_IN_NR & 1)>0) ? 4 : 0) + (((SPI_ANALOG_IN_NR & 2)>0) ? 2 : 0) + (((SPI_ANALOG_IN_NR & 4)>0) ? 1 : 0);
	uint32_t SPI_DATA = 1 + (SPI_SINGLE_ENDED_INPUT << 1) + (SPI_IN_NR_REVERSED << 2);
	//SPI_DATA = 1+2;
	
	uint8_t command = CMD_LOAD_REG_LOW;
	uint32_t low_buffer = (SPI_DATA << 5) | command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
	
	command = CMD_LOAD_REG_HIGH;
	low_buffer = command;
	high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);


	/*
	CMD_SPI_OUT_IN : begin
                            SPI_OUT_length <= command[14:8];
                            SPI_IN_length <= command[20:16];
                            SPI_SEL_next <= command[33:32];
                            SPI_chip_select_next <= command[37:34];
                            SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register                                          
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[40:40] == 0) SPI_state <= SPI_START;                                                         
                        end  
	*/

	command = CMD_SPI_OUT_IN;
	uint32_t SPI_out_length = 6;
	uint32_t SPI_in_length = 13;
	//uint32_t SPI_CS = 1;  
	uint32_t wait_time = 0;
	uint8_t SPI_SEL_next = 0x01; //2 SPI ports, 1 means active under PL control; if inactive they are under PS control
	uint8_t SPI_chip_select_next = 2 + 4 + 8; //4 CS lines, low means active.
	unsigned int start_now = 0;
	low_buffer =  (SPI_in_length << 16) | (SPI_out_length << 8) | command;
	high_buffer = (start_now << (40-32)) | (SPI_chip_select_next << 2) | (0x03 & SPI_SEL_next);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	/*

	CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
							INPUT_REPEAT_repeats <= command[27:8];
							INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
							INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
							INPUT_REPEAT_state <= INPUT_REPEAT_START;
							INPUT_REPEAT_command <= command[59:57];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end

					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
					if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
					if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;

	*/
	command = CMD_INPUT_REPEATED_OUT_IN;
	uint32_t INPUT_REPEAT_repeats = number_of_datapoints;
	uint32_t INPUT_REPEAT_wait = floor(delay_between_datapoints_in_ms * FPGAClockFrequencyInHz / 1000);
	uint32_t INPUT_REPEAT_command = 1;  //SPI input
	uint32_t INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 1;//this is needed if input BRAM buffer should be copied to DDR when half buffer full 

	low_buffer = (INPUT_REPEAT_repeats << 8) | command;
	high_buffer = INPUT_REPEAT_wait | (INPUT_REPEAT_trigger_secondary_interrupt_when_finished << (56 - 32)) | (INPUT_REPEAT_command << (57 - 32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	if (DelayMultiplier < 1) DelayMultiplier = 1;
	if (DelayMultiplier == 1) {
		AddProgramLineToSequenceList(1, 0, DelayMultiplier * 3 - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
		AddProgramLineToSequenceList(1, 0, DelayMultiplier * 3 - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
	} else {
		AddProgramLineToSequenceList(1, 0, DelayMultiplier * 3 - 4); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
	}

	//debugging of BRAM operation only
	//for (unsigned long n=0;n<0xFF;n++) 
	//	AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ n*2, /*input_buf_mem_data*/0xDEAD0000+n, /* wait_time_in_FPGA_cycles*/ 5);
	
}

void CEthernetControllerFirefly::SwitchDebugLED(bool OnOff) {
	AddCommandSetCoreOption_LED(OnOff);
}

void CEthernetControllerFirefly::IgnoreTCPIP(bool OnOff) {
	AddCommandSetPLToPSCommand((OnOff) ? PLToPSCommandIgnoreTCPIP : PLToPSCommandAcceptTCPIP);
}

void CEthernetControllerFirefly::AddMarker(uint8_t Marker) {
	AddCommandSetPLToPSCommand(PLToPSCommandTest | Marker);
}

void CEthernetControllerFirefly::AddProgramLineToSequenceList(uint8_t command, uint32_t data, uint32_t delay) {
	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	//uint8_t command = 1; //CMD_STEP

	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddProgramLine( uint8_t command, uint32_t data, uint32_t delay ) {
	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	//uint8_t command = 1; //CMD_STEP
	
	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);


	/*if (DebugBufferFile) {
		CString buf;
		buf.Format("%8u CMD %x data = %08X wait = %08X", command, data, delay);
		(*DebugBufferFile) << buf << endl;
	}*/
}

//void CEthernetControllerFirefly::AddCommandStep( uint32_t data, uint32_t delay) {
//	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
//	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
//	const uint32_t bus_data_mask = 0x0FFFFFFF;
//	const uint8_t command_mask = 0x1F;  //5 bit
//	uint8_t command = 1; //CMD_STEP
//
//	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
//	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
//	
//	if (DebugBufferFile) {
//		CString buf;
//		buf.Format("step data = %08X %08X", high_buffer, low_buffer);
//		(*DebugBufferFile) << buf << endl;
//	}
//	MySequencer->AddCommandStepToSequence(high_word, low_word);
//
//}

void CEthernetControllerFirefly::SetStrobeOptions(uint8_t strobe_choice, uint8_t strobe_low_length, uint8_t strobe_high_length) {
	const uint8_t command_mask = 0x1F;  //5 bit
	uint8_t command = CMD_SET_STROBE_OPTIONS;

	uint32_t low_buffer = ((strobe_high_length & 0xFF) << 24) | ((strobe_low_length & 0xFF) << 16) | ((strobe_choice & 0x07) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);
	uint32_t high_buffer = 0;
	AddSequencerCommand(high_buffer, low_buffer);
	
	low_buffer = ((strobe_high_length & 0xFF) << 24) | ((strobe_low_length & 0xFF) << 16) | ((strobe_choice & 0x07) << 8) | (command_mask & command);
	high_buffer = 0;
	AddSequencerCommand(high_buffer, low_buffer);
	
}

void CEthernetControllerFirefly::AddExternalTrigger( bool ExternalTrigger0, bool ExternalTrigger1, bool FPGASoftwareTrigger) {
// if (UseCommandBuffer) uses two command slots	ELSE uses one command slot
	if (ExternalTrigger0 || ExternalTrigger1 || FPGASoftwareTrigger) {
		//CMD_WAIT_FOR_TRIGGER: begin
		//	if ((trigger_0 && (command[8:8] == 1)) || (trigger_1 && (command[9:9] == 1)) || (trigger_PS && (command[10:10] == 1))) address <= address + 1;
		//end

		const uint8_t command_mask = 0x1F;  //5 bit
		uint8_t command = CMD_WAIT_FOR_TRIGGER;
		uint8_t trigger0 = (ExternalTrigger0) ? 1 : 0;
		uint8_t trigger1 = (ExternalTrigger1) ? 2 : 0;
		uint8_t softtrigger = (ExternalTrigger1) ? 4 : 0;

		uint32_t low_buffer = ((trigger0 | trigger1 | softtrigger) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);
		uint32_t high_buffer = 0;
		AddSequencerCommand(high_buffer, low_buffer);
		low_buffer = ((trigger0 | trigger1 | softtrigger) << 8) | (command_mask & command);
		high_buffer = 0;
		AddSequencerCommand(high_buffer, low_buffer);
		
	}
	else {
		AddProgramLine( 1, 0, 1); //CMD_STEP
		if (UseCommandBuffer) AddProgramLine( 1, 0, 1); //CMD_STEP
	}
	
}

void CEthernetControllerFirefly::SetTriggerOptions( bool ExternalTrigger0, bool ExternalTrigger1) {
// if (UseCommandBuffer) uses six command slots	ELSE uses three command slot
	uint32_t low_buffer;
	uint32_t high_buffer;
	if (SetPeriodicTriggerAtBeginningOfNextSequence && (PeriodicTriggerPeriod_in_s > 0)) {
		SetPeriodicTriggerAtBeginningOfNextSequence = false;
		//CMD_SET_PERIODIC_TRIGGER_PERIOD: begin
		//	periodic_trigger_period <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit 
		//end
		unsigned __int64 PeriodicTriggerPeriod = floor(PeriodicTriggerPeriod_in_s * FPGAClockFrequencyInHz / 1000);
		const uint8_t command_mask = 0x1F;  //5 bit
		uint8_t command = CMD_SET_PERIODIC_TRIGGER_PERIOD;
		if (UseCommandBuffer) {
			low_buffer = ((PeriodicTriggerPeriod & 0xFFFFFF) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerPeriod >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);
			low_buffer = ((PeriodicTriggerPeriod & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerPeriod >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);
		}
		else {
			low_buffer = ((PeriodicTriggerPeriod & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerPeriod >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);
		}
		//CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME: begin 
		//	periodic_trigger_allowed_wait_cycles <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit 
		//end
		unsigned __int64 PeriodicTriggerAllowedWaitCycles = floor(PeriodicTriggerAllowedWait_in_s * FPGAClockFrequencyInHz / 1000);
		command = CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME;
		if (UseCommandBuffer) {
			low_buffer = ((PeriodicTriggerAllowedWaitCycles & 0xFFFFFF) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerAllowedWaitCycles >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);
			low_buffer  = ((PeriodicTriggerAllowedWaitCycles & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerAllowedWaitCycles >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);

		}
		else {
			low_buffer = ((PeriodicTriggerAllowedWaitCycles & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
			high_buffer = (PeriodicTriggerAllowedWaitCycles >> 24) & 0xFFFFFF; // high 24 bit
			AddSequencerCommand(high_buffer, low_buffer);
		}
		LastPeriodicTriggerPeriod_in_s = PeriodicTriggerPeriod_in_s;
		AddExternalTrigger( ExternalTrigger0, ExternalTrigger1, false);
	}
	else if (WaitForPeriodicTriggerAtBeginningOfSequence && (LastPeriodicTriggerPeriod_in_s > 0)) {


		

		if (UseCommandBuffer) {
			
			core_option_LED = true;
			uint8_t command = CMD_LOAD_COMMAND_BUFFER;
			low_buffer =  command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			command = CMD_SET_OPTIONS;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			
			
			//AddProgramLine(buffer, n, CMD_STEP, 0, 0);
			//AddProgramLine(buffer, n + 1, CMD_STEP, 0, 0);
			AddProgramLine(CMD_STEP, 0, 1);
			//buffer[(n + 2) * 2 + 0] = CMD_WAIT_FOR_PERIODIC_TRIGGER;
			//buffer[(n + 2) * 2 + 1] = 0;
			low_buffer = CMD_WAIT_FOR_PERIODIC_TRIGGER;
			high_buffer = 0;
			AddSequencerCommand(high_buffer, low_buffer);

			
			
			core_option_LED = false;
			command = CMD_LOAD_COMMAND_BUFFER;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			command = CMD_SET_OPTIONS;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			//AddProgramLine(buffer, n + 4, CMD_STEP, 0, 0);
			//AddProgramLine(buffer, n + 5, CMD_STEP, 0, 0);
		}
		else {
			AddProgramLine(CMD_STEP, 0, 1);
			AddProgramLine(CMD_STEP, 0, 1);
			low_buffer = CMD_WAIT_FOR_PERIODIC_TRIGGER;
			high_buffer = 0;
			AddSequencerCommand(high_buffer, low_buffer);
		}
	}
	else {
		if (UseCommandBuffer) {
			AddProgramLine(CMD_STEP, 0, 1);
			AddProgramLine(CMD_STEP, 0, 1);
			AddProgramLine(CMD_STEP, 0, 1);
			AddProgramLine(CMD_STEP, 0, 1);
			AddExternalTrigger(ExternalTrigger0, ExternalTrigger1, false);
		}
		else {
			AddProgramLine(CMD_STEP, 0, 1);
			AddProgramLine(CMD_STEP, 0, 1);
			AddExternalTrigger(ExternalTrigger0, ExternalTrigger1, false);
		}
	}
}


//CString CommandNames[NrCommands] = { "CMD_STOP", "CMD_STEP", "CMD_STEP_AND_ENTER_FAST_MODE", "CMD_SET_OPTIONS", "CMD_LOAD_REG_LOW", "CMD_LOAD_REG_HIGH", "CMD_LATCH_STATE", "CMD_RESET_WAIT_CYCLES", "CMD_LONG_WAIT", "CMD_SET_STROBE_OPTIONS", "CMD_SET_INPUT_BUF_MEM", "CMD_WAIT_FOR_TRIGGER", "CMD_SET_LOOP_COUNT", "CMD_CONDITIONAL_JUMP_FORWARD", "CMD_CONDITIONAL_JUMP_BACKWARD", "CMD_I2C_OUT", "CMD_SPI_OUT_IN", "CMD_INPUT_REPEATED_OUT_IN", "CMD_SET_PERIODIC_TRIGGER_PERIOD", "CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME", "CMD_WAIT_FOR_PERIODIC_TRIGGER", "CMD_WAIT_FOR_WAIT_CYCLE_NR", "CMD_DIG_IN", "CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT", "CMD_ANALOG_IN_OUT", "CMD_PL_TO_PS_COMMAND" };

void CEthernetControllerFirefly::WriteBufferToFile(uint32_t* buffer, unsigned long length) { //length in command lines
	ofstream out;
	std::string result = DebugFilename + "_" + std::to_string(MySequencer->id) + ".txt";
	out.open(result);// , CFile::modeCreate | CFile::modeWrite);
	CString buf;
	double time = 0;
	uint32_t low_command_buffer = 0;
	uint32_t high_command_buffer = 0;

	for (unsigned long n = 0; n < length; n++) {
		//buf.Format("%05lu: ", n);
		buf.Format("%05X: t= % 04.5f ms ", n, time);
		out << buf;
		uint32_t low_buffer = buffer[2 * n];
		uint32_t high_buffer = buffer[2 * n + 1];
		uint8_t command = low_buffer & 0x1F;
		buf.Format("%08X %08X %s", high_buffer, low_buffer, CommandNames[command]);
		out << buf;
		buf = "";
		if (command == CMD_LOAD_COMMAND_BUFFER) {
			low_command_buffer = low_buffer;
			high_command_buffer = high_buffer;
		}
		else if (CommandUsesBuffer[command]) {
			if (UseCommandBuffer) {
				low_buffer = low_command_buffer;
				high_buffer = high_command_buffer;
			}
		}
		if (command == CMD_STEP) {
			/*
			 CMD_STEP:begin
							wait_time[30:0] <= command[35:5];
							wait_time[47:31] <= 0;
							bus_data <= command[63:36];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end
			*/
			uint32_t delay_low = ((high_buffer & 0xF) << (32 - 5)) | (low_buffer >> 5);
			uint32_t bus_data = high_buffer >> 4;
			buf.Format(" bus = %07X ; delay = %08X", bus_data, delay_low);
			time += delay_low * 10 * 0.000001;
		}
		else if (command == CMD_INPUT_REPEATED_OUT_IN) {
			/*
			CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
								INPUT_REPEAT_repeats <= command[27:8];
								INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
								INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
								INPUT_REPEAT_state <= INPUT_REPEAT_START;
								INPUT_REPEAT_command <= command[59:57];
								if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
								strobe_generator_state <= DELAY_CYCLE;
							end
			*/
			uint32_t repeats = (low_buffer >> 8) & 0x3FFF;
			uint32_t wait = high_buffer & 0x3FFF;
			uint32_t trigger_secondary_interrupt_when_finished = (high_buffer >> 24) & 0x01;
			uint32_t command = (high_buffer >> 25) & 0x07;
			/*
					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
                    if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
                    if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;
			*/

			buf.Format(" repeats = %u ; wait = %u * 10ns ; trigger_secondary_interrupt_when_finished = %01X ; command = %01X", repeats, wait, trigger_secondary_interrupt_when_finished, command);
		}
		else if (command == CMD_SET_PERIODIC_TRIGGER_PERIOD) {
			/*
			CMD_SET_PERIODIC_TRIGGER_PERIOD: begin
				periodic_trigger_period <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
			end
			*/
			uint32_t periodic_trigger_period = (low_buffer >> 8) & 0xFFFFFF;
			buf.Format(" periodic_trigger_period = %06X%06X", high_buffer, periodic_trigger_period);
		}
		else if (command == CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME) {
			/*
			CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME: begin
				periodic_trigger_allowed_wait_cycles <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
			end
			*/
			uint32_t periodic_trigger_allowed_wait_cycles = (low_buffer >> 8) & 0xFFFFFF;
			buf.Format(" periodic_trigger_allowed_wait_cycles = %06X%06X", high_buffer, periodic_trigger_allowed_wait_cycles);
		}
		else if (command == CMD_SPI_OUT_IN) {
			/*
			CMD_SPI_OUT_IN : begin
								SPI_OUT_length <= command[14:8];
								SPI_IN_length <= command[20:16];
								SPI_SEL_next <= command[34:32];
								SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register
								INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
								if (command[40:40] == 0) SPI_state <= SPI_START;
							end
			*/
			uint32_t SPI_OUT_length = (low_buffer >> 8) & 0x3F;
			uint32_t SPI_IN_length = (low_buffer >> 16) & 0x1F;
			uint32_t SPI_SEL_next = (low_buffer >> 24) & 0x07;
			uint32_t start_now = (high_buffer >> 8) & 0x01;
			buf.Format(" SPI_OUT_length = %02X ; SPI_IN_length = %02X ; SPI_SEL_next = %01X ; start_now = %01X", SPI_OUT_length, SPI_IN_length, SPI_SEL_next, start_now);
		}
		else if (command == CMD_ANALOG_IN_OUT) {
			/*
			CMD_ANALOG_IN_OUT: begin
								adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
								adc_write_enable <= command[15:15];
								adc_programming_out <= command[31:16];
								wait_time[29:0] <= command[63:34];
								wait_time[47:30] <= 0;
								INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
								if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
									if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
									else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
								end
							end
			*/
			uint32_t adc_register_address = (low_buffer >> 8) & 0x7F;
			uint32_t adc_write_enable = (low_buffer >> 15) & 0x01;
			uint32_t adc_programming_out = (low_buffer >> 16) & 0xFFFF;
			uint32_t wait_time = (low_buffer >> 2) & 0x3FFF;
			uint32_t start_now = (high_buffer) & 0x01;
			uint32_t adc_conversion = (high_buffer >> 1) & 0x01;
			buf.Format(" adc_register_address = %02X ; adc_write_enable = %01X ; adc_programming_out = %04X ; wait_time = %04X ; start_now = %01X ; adc_conversion = %01X", adc_register_address, adc_write_enable, adc_programming_out, wait_time, start_now, adc_conversion);
		}
		else if (command == CMD_SET_STROBE_OPTIONS) {
			/*
			CMD_SET_STROBE_OPTIONS: begin
							strobe_choice <= command[11:8]; // 3 bit
							strobe_low_length <= command[23:16]; // 8 bit
							strobe_high_length <= command[31:24]; // 8 bit
						end
			*/
			uint32_t strobe_choice = (low_buffer >> 8) & 0x07;
			uint32_t strobe_low_length = (low_buffer >> 16) & 0xFF;
			uint32_t strobe_high_length = (low_buffer >> 24) & 0xFF;
			buf.Format(" strobe_choice = %01X ; strobe_low_length = %02X ; strobe_high_length = %02X", strobe_choice, strobe_low_length, strobe_high_length);
		}
		else if (command == CMD_LOAD_REG_LOW) {
			/*
			CMD_LOAD_REG_LOW:begin
							register[58:0] <= command[63:5];
						end
			*/
			uint32_t register_data = (low_buffer >> 5) & 0x7FFFFFFFF;
			buf.Format(" register_data = %010X", register_data);
		}
		else if (command == CMD_LOAD_REG_HIGH) {
			/*
			CMD_LOAD_REG_HIGH:begin
							register[117:59] <= command[63:5];
						end
			*/
			uint32_t register_data = (low_buffer >> 5) & 0x7FFFFFFFF;
			buf.Format(" register_data = %010X", register_data);
		}
		else if (command == CMD_WAIT_FOR_TRIGGER) {
			/*
			CMD_WAIT_FOR_TRIGGER: begin
							if ((trigger_0 && (command[8:8] == 1)) || (trigger_1 && (command[9:9] == 1)) || (trigger_PS && (command[10:10] == 1))) address <= address + 1;
						end
			*/
			uint32_t trigger0 = (low_buffer >> 8) & 0x01;
			uint32_t trigger1 = (low_buffer >> 9) & 0x01;
			uint32_t trigger_PS = (low_buffer >> 10) & 0x01;
			buf.Format(" trigger0 = %01X ; trigger1 = %01X ; trigger_PS = %01X", trigger0, trigger1, trigger_PS);
		}
		else if (command == CMD_SET_LOOP_COUNT) {
			/*
			CMD_SET_LOOP_COUNT: begin
							loop_count <= command[63:32];
						end
			*/
			buf.Format(" loop_count = %06X", high_buffer);
		}
		else if (command == CMD_CONDITIONAL_JUMP_FORWARD) {
			/*
			CMD_CONDITIONAL_JUMP_FORWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
							if ((condition_0 && (command[8:8] == 1)) || (condition_1 && (command[9:9] == 1)) || (condition_PS && (command[10:10] == 1)) || (command[11:11] == 1))
								address <= address + command[15:8];
							else address <= address + 1;
						end
			*/
			uint32_t condition0 = (low_buffer >> 8) & 0x01;
			uint32_t condition1 = (low_buffer >> 9) & 0x01;
			uint32_t condition_PS = (low_buffer >> 10) & 0x01;
			uint32_t condition = (low_buffer >> 11) & 0x01;
			uint32_t jump = (low_buffer >> 8) & 0xFF;
			buf.Format(" condition0 = %01X ; condition1 = %01X ; condition_PS = %01X ; condition = %01X ; jump = %02X", condition0, condition1, condition_PS, condition, jump);
		}
		out << buf << endl;
	}
	out.close();
}

void CEthernetControllerFirefly::SetPeriodicTrigger(double aPeriodicTriggerPeriod_in_s, double aPeriodicTriggerAllowedWaitTime_in_s) {
	PeriodicTriggerPeriod_in_s = aPeriodicTriggerPeriod_in_s;
	PeriodicTriggerAllowedWait_in_s = aPeriodicTriggerAllowedWaitTime_in_s;
	SetPeriodicTriggerAtBeginningOfNextSequence = true;
	WaitForPeriodicTrigger(true);
}

void CEthernetControllerFirefly::WaitForPeriodicTrigger(bool aWaitForPeriodicTriggerAtBeginningOfSequence) {
	WaitForPeriodicTriggerAtBeginningOfSequence = aWaitForPeriodicTriggerAtBeginningOfSequence;
}

void CEthernetControllerFirefly::TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool aDoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
	DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible = aDoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible;
	if (!DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
		//delete previous_command_buffer;
		previous_command_buffer = NULL;
		previous_command_buffer_length = 0;
	}
}

bool CEthernetControllerFirefly::ModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptModifySequence(differences, difference_index_table, difference_command_table))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::OptimizedCommand(CString CommandString) {
	if (MinimizeEthernetCommunicationDirectionChanges) 
		return Command(CommandString + "0", /*DontWaitForReady*/ true);
	else 
		return Command(CommandString);
}

bool CEthernetControllerFirefly::AttemptModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]) {
	if (!/*Optimized*/Command("modify_sequence")) return false;
	if (!WriteInteger(12 * differences)) return false;
	if (!SendData((uint8_t*)difference_index_table, 4 * differences)) return false;
	if (!SendData((uint8_t*)difference_command_table, 8 * differences)) return false;
	return true;
}


bool CEthernetControllerFirefly::SendSequence(uint32_t DataSize, uint32_t* buffer) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptSendSequence(DataSize, buffer))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptSendSequence(uint32_t DataSize, uint32_t* buffer) {
	if (!/*Optimized*/Command("send_sequence")) return false;
	if (!WriteInteger(DataSize)) return false;
	if (!SendData((uint8_t*)buffer, DataSize)) return false;
	return true;
}

const unsigned int BusBitShift = 8;
const unsigned int BusSequencerSpecialCommand = 0x7 << BusBitShift;

//bool CEthernetControllerFirefly::AddData(uint32_t* BusData, uint32_t* Spacing,/* uint32_t* AbsoluteTime,*/ unsigned long Count) {
bool CEthernetControllerFirefly::AddSequencePreamble() {
	constexpr bool IgnoreSpecialFPGACommand = true;
	//delete AbsoluteTime;
	//AbsoluteTime = NULL;
	if (!Connected) return false;

	if (DebugBufferFile) {
		(*DebugBufferFile) << endl;
	}
	////Timestamp.Mark("AddData");
	uint32_t PreambleProgramLines = (UseCommandBuffer) ? 8 : 4;  //make sure you put the number of program lines in the preamble here
	uint32_t PostProgramLines = 11;  //make sure you put the number of program lines in the preamble here
	//uint32_t DataSize = (PreambleProgramLines + PostProgramLines + Count) * 8; //data size in byte

	//CString buf;
	//buf.Format("CNetwork::ReceiveMsg :: sending  %u bytes", DataSize);
	//ControlMessageBox(buf);

	//uint32_t* buffer = new uint32_t[(PreambleProgramLines + PostProgramLines + Count + MaxSequencerCommandListSize) * 4];

	//Add preamble for external clock and trigger setup

	uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	if (DelayMultiplier < 1) DelayMultiplier = 1;

	unsigned int StrobeDelay = ((DelayMultiplier + 1) / 3) - 1;

	//strobe/clock output pin content: 0: clock 1: strobe, 2: low, 3: high, 4: flags_hi[31]
	SetStrobeOptions( (FPGAUseStrobeGenerator) ? 1 : 0, StrobeDelay, StrobeDelay); // this command fills 1 command lines or if (UseCommandBuffer) 2 command lines
	SetTriggerOptions(  ExternalTrigger0, ExternalTrigger1); // this command fills 3 command lines or if (UseCommandBuffer) 6 command lines

	//end of preamble

}

bool CEthernetControllerFirefly::SendSequenceToFPGA(uint32_t* buffer) {
	//uint32_t FPGA_Special_Command = BusSequencerSpecialCommand << (2 + 16);
	//uint32_t FPGA_Special_Command_Mask = 0xF << (BusBitShift + 2 + 16);
	//uint32_t AdditionalSteps = 0;

	//for (uint32_t n = 0; n < Count; n++) {
		/*if (((BusData[n] & FPGA_Special_Command_Mask) == FPGA_Special_Command) && (!IgnoreSpecialFPGACommand)) {
			//ToDo: add devices that can add special commands, e.g. analog input, I2C etc.
			unsigned __int16 bus_data_without_address = BusData[n] & 0xFFFF;
			if (bus_data_without_address < SequencerCommandListSize) {
				AddSequencerCommandToBuffer(buffer, n + AdditionalSteps + PreambleProgramLines, SequencerCommandList[bus_data_without_address * 2 + 1], SequencerCommandList[bus_data_without_address * 2]);
				if (Spacing[n] > 0) {
					//This code should be necessary for timing to be correct. However when it's added input doesn't work anymore. Also NI status bar doesn't work and WaitForSequenceEnd sent too early.
					AdditionalSteps++;
					AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, 0, Spacing[n] * DelayMultiplier - 1);
				}
			}
			else {
				ControlMessageBox("EthernetMultiIOControllerFirefly.cpp: AddData(): special command not found in special command list.");
				AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, 0, DelayMultiplier);
			}
		}
		else {*/
		//	if (Spacing[n] < 1) Spacing[n] = 1; //2024 08 18: current minimum spacing is 1 clock cycle, to give BRAM time to deliver data; would need pipeline in Vivado core.sv to reduce this
		//	AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, BusData[n], Spacing[n] * DelayMultiplier);
		//}
	//}
	//weird workaround to make program stop correctly; perhaps the last wait time in the buffer is not calculated correctly
	//AddProgramLine(buffer, PreambleProgramLines + AdditionalSteps + Count - 1, CMD_STEP, 0, DelayMultiplier);
	//for (uint32_t n = (PreambleProgramLines + AdditionalSteps + Count); n < (PreambleProgramLines + AdditionalSteps + PostProgramLines - 1 + Count); n++) { //add a few code lines to make sure program is not too short for DMA transfer to start and stop correctly; might be better solved by improving MicroZed code

	for (uint32_t n=0;n< 10;n++) {
		AddProgramLine(CMD_STOP, 0, 0);
		//AddProgramLine(buffer, n, CMD_STEP, 0, DelayMultiplier); 
	}
	//AddProgramLine(buffer, Count + PreambleProgramLines + PostProgramLines - 2, CMD_STOP, 0, 0);
	//AddProgramLine(buffer, Count + AdditionalSteps + PreambleProgramLines + PostProgramLines - 1, CMD_STOP, 0, 0);

	uint32_t FilledBufferLength; 
	uint32_t MaxBufferLength;
	
	MySequencer->GetBufferLength(FilledBufferLength, MaxBufferLength);


	uint32_t Count = FilledBufferLength;
	uint32_t DataSize = 8 * Count;
	//only for debugging
	if (DebugModeOn) WriteBufferToFile(buffer, DataSize / 8);

	if (!(DataSize < MaxFPGAProgramLength * 8)) {
		ControlMessageBox("CEthernetControllerFirefly::AddData : Program longer than FPGA SOM memory allows");
		//delete buffer;
		return false;
	}

	if (DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
		bool possible = (previous_command_buffer != NULL) && (previous_command_buffer_length == DataSize);
		if (possible) {
			constexpr unsigned long max_differences = 1000; //maximum number of differences to check; if more differences are found, the program is sent as is
			unsigned long differences = 0;
			uint32_t difference_index_table[max_differences];
			uint32_t difference_command_table[2 * max_differences];
			for (uint32_t n = 0; n < DataSize / 8; n++) {
				if ((buffer[2*n] != previous_command_buffer[2*n]) || (buffer[2*n+1] != previous_command_buffer[2*n+1])) {
					if (differences < max_differences) {
						difference_index_table[differences] = n;
						difference_command_table[2 * differences] = buffer[2 * n ];
						difference_command_table[2 * differences + 1] = buffer[2 * n + 1];
						previous_command_buffer[2 * n] = buffer[2 * n];
						previous_command_buffer[2 * n + 1] = buffer[2 * n + 1];
						differences++;
					}
					else {
						possible = false;
						//delete previous_command_buffer;
						previous_command_buffer = NULL;
						previous_command_buffer_length = 0;
						break;
					}
				}
			}			
			if (possible) {
				//delete buffer;
				//arrange data as two consecutive tables
				//the first table is a list of 32-bit values, containing the list index of the 64 bit command that needs to be modified
				//the second table contains a list of the new 64-bit commands.
				/*for (uint32_t n = 0; n < differences; n++) {
					difference_index_table[differences + 2 * n] = difference_command_table[2 * n];
					difference_index_table[differences + 2 * n + 1] = difference_command_table[2 * n + 1];
				}*/
				if (differences > 0) {
					ModifySequence(differences, difference_index_table, difference_command_table);					
				}
				//Command("print_sequence");//only for debug
				ClearSequencerCommandList();
				return true;
			}
		}
	}
	////Timestamp.Mark("send_sequence");
	bool ok = SendSequence(DataSize, buffer);
	if (ok) {
		CString buf;
		buf.Format("send data, %u bytes sent", DataSize);
		////Timestamp.Mark(buf);
		//if (previous_command_buffer) {
		//	delete previous_command_buffer;
		//}
		previous_command_buffer = buffer;
		previous_command_buffer_length = DataSize;
		ClearSequencerCommandList();
		//Command("print_sequence");//only for debug
		////Timestamp.Mark("end AddData");
		return true;
	}
	ClearSequencerCommandList();
	return false;
}

double CEthernetControllerFirefly::MeasureEthernetBandwidth(uint32_t DataSize, double MinimumExpected) {
	bool ok = AttemptCommand("send_sequence"); //cheap way of assuring ethernet is up. Can't overcome outages within the (ok) branch.
	if (ok) {
		
		WriteInteger(DataSize);
		uint32_t* buffer = new uint32_t[DataSize];
		for (uint32_t n = 0; n < DataSize; n++) {
			buffer[n] = 0;
		}
		unsigned  long StartTickCount = GetTickCount();
		SendData((uint8_t*)buffer, DataSize);
		CheckReady(10);
		unsigned  long EndTickCount = GetTickCount();
		delete buffer;
		double Bandwidth = 0.000001*DataSize * 8 / ((EndTickCount - StartTickCount) / 1000.0); //in Mbit/s
		if (Bandwidth < MinimumExpected) {
			CString message;
			message.Format("CEthernetControllerFirefly::MeasureEthernetBandwidth : Ethernet Bandwidth = %.1f MBit/s is lower than expected. Check ethernet connection.", Bandwidth);
			
			AddErrorMessageCString(message);
		}
		return Bandwidth;
	}
	if (MinimumExpected > 0) AddErrorMessageCString("CEthernetControllerFirefly::MeasureEthernetBandwidth : No ethernet connection to FPGA.");
	return -1;
}

bool CEthernetControllerFirefly::GetAktWaveformPoint(unsigned long long& DataPointsWritten, bool& running) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetAktWaveformPoint(DataPointsWritten, running))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetAktWaveformPoint(unsigned long long& DataPointsWritten, bool &running) {
	if (!Connected) return false;
	//progress bar without communication
	//unsigned  long TickCounts = GetTickCount();
	//DataPointsWritten = (TickCounts - StartTickCounts)*(BusFrequency/1000);
	//return true;
	//////Timestamp.Mark("CEthernetControllerFirefly::GetAktWaveformPoint");
	//progress bar with communication
	if (!OptimizedCommand("get_current_waveform_point")) return false;
	unsigned long long AktWaveformEntry;
	if (!ReadInt64(AktWaveformEntry)) return false; //100MHz clock cycles
	if (!ReadBool(running)) return false;
	DataPointsWritten = AktWaveformEntry / FPGAClockToBusClockRatio;// (FPGAClockFrequencyInHz / BusFrequency); //conversion to 2MHz clock cycles
	return true;	
}

bool CEthernetControllerFirefly::AttemptNetworkCommand(tBoolFunction fCommand) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!fCommand())) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::GetNextCycleNumber(long& NextCycleNumber) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetNextCycleNumber(NextCycleNumber))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetNextCycleNumber(long& NextCycleNumber) {
	if (!Connected) return false;
	if (!/*Optimized*/Command("get_sequence_number")) return false;
	return ReadLong(NextCycleNumber);
}

bool CEthernetControllerFirefly::SwitchDebugMode(bool OnOff, const std::string& aFilename) {
	DebugModeOn = OnOff;
	DebugFilename = aFilename;
	if (OnOff) return AttemptNetworkCommand([this]() {return Command("switch_debug_mode_on"); });
	else return AttemptNetworkCommand([this]() {return Command("switch_debug_mode_off"); }); 
}

bool CEthernetControllerFirefly::ResetCycleNumber() {
	return AttemptNetworkCommand([this]() {return Command("reset_sequence_number"); });
}

bool CEthernetControllerFirefly::CloseConnection() {
	Connected = false;
	return AttemptNetworkCommand([this]() {return Command("close"); });
}

bool CEthernetControllerFirefly::Reset() {
	//this commands resets the whole FPGA core. It should only be used if an error occured
	return AttemptNetworkCommand([this]() {return Command("reset"); });
}

bool CEthernetControllerFirefly::WaitTillFinished() {
	return AttemptNetworkCommand([this]() {return Command("wait_till_finished"); });
}

bool CEthernetControllerFirefly::Start() {
	if (!Connected) return false;
	StartTickCounts = GetTickCount();
	////Timestamp.Mark("CEthernetControllerFirefly::Start");
	return AttemptNetworkCommand([this]() {return Command("start"); });
}

bool CEthernetControllerFirefly::Stop() {
	return true;
	//if (!Connected) return false;
	//return AttemptNetworkCommand([this]() {return Command("stop"); });
}


bool CEthernetControllerFirefly::SetFrequency(double Frequency) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptSetFrequency(Frequency))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptSetFrequency(double Frequency) {
	if (!Command("set_frequency")) return false;
	return WriteDouble(Frequency);
}



bool CEthernetControllerFirefly::StartAssemblingCPUCommandSequence() {
	if (!Command("cs_assemble_sequence")) return false;
	return true;
}

bool CEthernetControllerFirefly::AddCPUCommand(const char* command) {
	if (!Command("cs_add_command")) return false;
	return WriteString(command);
}

bool CEthernetControllerFirefly::ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms) {
	if (!Command("cs_execute_sequence")) return false;
	WriteInteger(ethernet_check_period_in_ms);
	return true;
}

bool CEthernetControllerFirefly::StopCPUCommandSequence() {
	if (!Command("cs_stop_sequence")) return false;
	return true;
}

bool CEthernetControllerFirefly::InterruptCPUCommandSequence() {
	if (!Command("cs_interrupt_sequence")) return false;


	return true;
}

bool CEthernetControllerFirefly::GetCPUCommandErrorMessages() {
	if (!Command("cs_get_error_message")) return false;
	long NrErrorMessages;
	bool ok = ReadLong(NrErrorMessages);
	if (!ok) {
		AddErrorMessageCString("GetCPUCommandErrorMessages : couldn't get number of error messages");
		return false;
	}
	if (NrErrorMessages > 0) {
		CString allErrorMessages = "";
		for (long n = 0; n < NrErrorMessages; n++) {
			CString ErrorMessage;
			if (!GetCommand(ErrorMessage)) return false;
			allErrorMessages += ErrorMessage + "\n";
		}
		AddErrorMessageCString(allErrorMessages);
	}
	return true;
}

bool CEthernetControllerFirefly::PrintCPUCommandErrorMessages() {
	if (!Command("cs_print_error_messages")) return false;
	return true;

}

bool CEthernetControllerFirefly::PrintCPUCommandSequence() {
	if (!Command("cs_print_command_table")) return false;
	return true;
}






void CEthernetControllerFirefly::SetExternalTrigger(bool aExternalTrigger0, bool aExternalTrigger1) {
	ExternalTrigger0 = aExternalTrigger0;
	ExternalTrigger1 = aExternalTrigger1;
}

bool CEthernetControllerFirefly::SetExternalClock(bool aExternalClock0, bool aExternalClock1) {
	bool changed = (ExternalClock0 != aExternalClock0) || (ExternalClock1 != aExternalClock1);
	ExternalClock0 = aExternalClock0;
	ExternalClock1 = aExternalClock1;
	if (!Connected) return false;
	if (changed) {
		if (ExternalClock0) {
			return AttemptNetworkCommand([this]() {return Command("select_external_clock_0"); });
		} else if (ExternalClock1) {
			return AttemptNetworkCommand([this]() {return Command("select_external_clock_1"); });
		} else {
			return AttemptNetworkCommand([this]() {return Command("select_internal_clock"); });
		}
	} 
	return true;
}



bool CEthernetControllerFirefly::GetFrequency(double &Frequency) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetFrequency(Frequency))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetFrequency(double& Frequency) {
	if (!Command("get_frequency")) return false;
	return ReadDouble(Frequency);
}



bool CEthernetControllerFirefly::GetPeriodicTriggerError(bool& Error) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetPeriodicTriggerError(Error))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetPeriodicTriggerError(bool& Error) {
	if (!/*Optimized*/Command("get_periodic_trigger_error")) return false;
	int Errori;
	bool err = ReadInt(Errori);
	Error = Errori != 0;
	return err;
}


bool CEthernetControllerFirefly::CheckReady(double timeout_in_seconds) {
	return AttemptNetworkCommand([this]() {return Command("check_ready"); }); //Do not use OptimizedCommand here. The whole point is to get a "Ready" back.
}

bool CEthernetControllerFirefly::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptWaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptWaitTillEndOfSequenceThenGetInputData(uint8_t * &buffer, unsigned long & buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
	if (buffer) delete[] buffer;
	buffer = NULL;
	previous_command_buffer_ptr = NULL;
	buffer_length = 0;
	//progress bar without communication
	//unsigned  long TickCounts = GetTickCount();
	//DataPointsWritten = (TickCounts - StartTickCounts)*(BusFrequency/1000);
	//return true;
	//progress bar with communication
	if (!Command("wait_till_end_of_sequence_then_get_input_data")) return false;
	
	int PeriodicTriggerError;
	if (!ReadInt(PeriodicTriggerError, timeout_in_s)) return false;
	EndTimeOfCycle = GetTickCount();
	//int InputBufferContentOriginSequence;
	//ReadInt(InputBufferContentOriginSequence);
	//int InputBufferContentsLength;
	//ReadInt(InputBufferContentsLength);
	unsigned long long InputBufferContentOriginSequence;
	if (!ReadInt64(InputBufferContentOriginSequence)) return false;
	unsigned long long InputBufferContentsLength;
	if (!ReadInt64(InputBufferContentsLength)) return false;
	if (InputBufferContentsLength > 0) {
		buffer = new uint8_t[InputBufferContentsLength];
		previous_command_buffer_ptr = buffer;
		buffer_length = InputBufferContentsLength;	
		if (Network) return Network->ReceiveData(buffer, buffer_length, /*timeout_in_ms = */ 5000);
		else {
			delete[] buffer;
			buffer = NULL;
			previous_command_buffer_ptr = NULL;
			buffer_length = 0;
			return false;
		}
	}
	return true;
}

