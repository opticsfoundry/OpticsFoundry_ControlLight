#ifdef PYTHON_API

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>  // sometimes needed for lambdas
#include <pybind11/stl.h>         // if you return std::string, vector, etc.

#include "../ControlAPI.h"  // your class header
using namespace pybind11::literals;


namespace py = pybind11;

uint8_t* buffer = nullptr;

PYBIND11_MODULE(control_light_api, m) {
    py::register_exception<CLA_Exception>(m, "CLA_Exception");
    py::class_<ControlLight_API>(m, "ControlLightAPI")
        .def(py::init<bool, bool>(), py::arg("initialize_afx") = true, py::arg("initialize_afx_socket") = true)
        .def("is_created", &ControlLight_API::IsCreated)
        .def("did_error_occur", &ControlLight_API::DidErrorOccur)
        .def("get_last_error", &ControlLight_API::GetLastError)
        .def("configure", &ControlLight_API::Configure, py::arg("display_errors"))
        .def("load_from_json_file", &ControlLight_API::LoadFromJSONFile)
        .def("auto_configure", &ControlLight_API::AutoConfigure, py::arg("filename") = "")
        .def("initialize", &ControlLight_API::Initialize)
        .def("switch_debug_mode", &ControlLight_API::SwitchDebugMode, py::arg("on_off"), py::arg("filename"))
        .def("TransmitOnlyDifferenceBetweenCommandSequenceIfPossible", &ControlLight_API::TransmitOnlyDifferenceBetweenCommandSequenceIfPossible, py::arg("on_off"))
        .def("is_ready", &ControlLight_API::IsReady)
        .def("start_assembling_sequence", &ControlLight_API::StartAssemblingSequence)
        .def("start_assembling_next_sequence", &ControlLight_API::StartAssemblingNextSequence)
        .def("set_value", &ControlLight_API::SetValue,
            py::arg("sequencer"), py::arg("address"), py::arg("subaddress"),
            py::arg("data"), py::arg("data_length_in_bit"), py::arg("start_bit") = 0)
        .def("set_register", &ControlLight_API::SetRegister,
            py::arg("sequencer"), py::arg("address"), py::arg("subaddress"),
            py::arg("data"), py::arg("data_length_in_bit"), py::arg("start_bit") = 0)
        .def("set_value_serial_device", &ControlLight_API::SetValueSerialDevice,
            py::arg("sequencer"), py::arg("address"), py::arg("subaddress"),
            py::arg("data"), py::arg("data_length_in_bit"), py::arg("start_bit") = 0)
        .def("set_register_serial_device", &ControlLight_API::SetRegisterSerialDevice,
            py::arg("sequencer"), py::arg("address"), py::arg("subaddress"),
            py::arg("data"), py::arg("data_length_in_bit"), py::arg("start_bit") = 0)

        // Wait and time
        .def("wait_ms", &ControlLight_API::Wait_ms, py::arg("time_in_ms"))
        .def("get_time_ms", [](ControlLight_API& self) {
                double t = 0;
                self.GetTime_ms(t);
                return t;
            })
        .def("get_number_of_sequencers", &ControlLight_API::GetNumberOfSequencers)
		.def("get_time_of_sequencer_ms", [](ControlLight_API& self, unsigned int sequencer) {
		        double t = 0;
		        self.GetTimeOfSequencer_ms(sequencer, t);
		        return t;
			}, py::arg("sequencer"))
		.def("get_time_debt_of_sequencer_ms", [](ControlLight_API& self, unsigned int sequencer) {
		        double t = 0;
		        self.GetTimeDebtOfSequencer_ms(sequencer, t);
		        return t;
			}, py::arg("sequencer"))
		.def("get_next_buffer_position_of_master_sequencer", [](ControlLight_API& self) {
		        unsigned long next_buffer_position = 0;
		        self.GetNextBufferPositionOfMasterSequencer(next_buffer_position);
		        return next_buffer_position;
			})

        .def("set_periodic_trigger", &ControlLight_API::SetPeriodicTrigger, py::arg("periodic_trigger_period_in_s"), py::arg("periodic_trigger_allowed_wait_time_in_s"))
        .def("get_next_cycle_number", [](ControlLight_API& self) {
                long next_cycle_number = 0;
                self.GetNextCycleNumber(next_cycle_number);
                return next_cycle_number;
            })
        .def("reset_cycle_number", &ControlLight_API::ResetCycleNumber)
        
        //I2C port
        .def("transmit_i2c_port", &ControlLight_API::TransmitI2CPort, py::arg("i2c_port"), py::arg("i2c_address"), py::arg("send_length"), py::arg("send_data"), py::arg("receive_length"), py::arg("receive_data"), py::arg("i2c_clock_frequency_in_hz"))
        .def("write_config_eeprom", [](ControlLight_API& self, uint8_t sequencer_id, uint8_t rack_nr, uint8_t slot_nr, py::bytes data) {
                const std::string payload = static_cast<std::string>(data);
                self.WriteConfigEEPROM(sequencer_id, rack_nr, slot_nr, payload.data(), payload.size());
            }, py::arg("sequencer_id"), py::arg("rack_nr"), py::arg("slot_nr"), py::arg("data"))
        .def("read_config_eeprom", [](ControlLight_API& self, uint8_t sequencer_id, uint8_t rack_nr, uint8_t slot_nr) {
                std::string payload(256, '\0');
                size_t length = payload.size();
                self.ReadConfigEEPROM(sequencer_id, rack_nr, slot_nr, payload.data(), length);
                payload.resize(length);
                return py::dict(
                    "data"_a = py::bytes(payload),
                    "length"_a = length
                );
            }, py::arg("sequencer_id"), py::arg("rack_nr"), py::arg("slot_nr"))
        .def("write_config_address", &ControlLight_API::WriteConfigAddress,
            py::arg("sequencer_id"), py::arg("rack_nr"), py::arg("slot_nr"), py::arg("address"))
        .def("read_config_address", [](ControlLight_API& self, uint8_t sequencer_id, uint8_t rack_nr, uint8_t slot_nr) {
                uint8_t address = 0;
                self.ReadConfigAddress(sequencer_id, rack_nr, slot_nr, address);
                return address;
            }, py::arg("sequencer_id"), py::arg("rack_nr"), py::arg("slot_nr"))
        .def("read_configuration", [](ControlLight_API& self, const std::string& filename) {
                py::object json_module = py::module_::import("json");
                return json_module.attr("loads")(self.ReadConfiguration(filename.c_str()));
            }, py::arg("filename") = "")
        .def("get_auto_config_json", [](ControlLight_API& self, const std::string& filename) {
                py::object json_module = py::module_::import("json");
                return json_module.attr("loads")(self.GetAutoConfigJSON(filename.c_str()));
            }, py::arg("filename") = "")
        
        //Rack control
        .def("select_slot", &ControlLight_API::SelectSlot, py::arg("slot_nr"))
        .def("reset_i2c_multiplexer", &ControlLight_API::ResetI2CMultiplexer)   
        
        //sequencer commands
		.def("start_assembling_cpu_command_sequence", &ControlLight_API::StartAssemblingCPUCommandSequence)
		.def("add_cpu_command", &ControlLight_API::AddCPUCommand, py::arg("command"))
		.def("execute_cpu_command_sequence", &ControlLight_API::ExecuteCPUCommandSequence, py::arg("ethernet_check_period_ms") = 0)
		.def("stop_cpu_command_sequence", &ControlLight_API::StopCPUCommandSequence)
		.def("interrupt_cpu_command_sequence", &ControlLight_API::InterruptCPUCommandSequence)
        .def("get_cpu_command_error_messages", &ControlLight_API::GetCPUCommandErrorMessages)
        .def("print_cpu_command_error_messages", &ControlLight_API::PrintCPUCommandErrorMessages)
        .def("print_cpu_command_sequence", &ControlLight_API::PrintCPUCommandSequence)
        // Analog/digital output
        .def("set_voltage", &ControlLight_API::SetVoltage,
            py::arg("sequencer"), py::arg("address"), py::arg("voltage"))
        .def("set_digital_output", &ControlLight_API::SetDigitalOutput,
            py::arg("sequencer"), py::arg("address"), py::arg("bit_nr"), py::arg("on_off"))

        // AD9854
        .def("set_start_frequency", &ControlLight_API::SetStartFrequency, py::arg("sequencer"), py::arg("address"), py::arg("frequency"))
        .def("set_stop_frequency", &ControlLight_API::SetStopFrequency, py::arg("sequencer"), py::arg("address"), py::arg("frequency"))
        .def("set_modulation_frequency", &ControlLight_API::SetModulationFrequency, py::arg("sequencer"), py::arg("address"), py::arg("frequency"))
        .def("set_power", &ControlLight_API::SetPower, py::arg("sequencer"), py::arg("address"), py::arg("power"))
        .def("set_attenuation", &ControlLight_API::SetAttenuation, py::arg("sequencer"), py::arg("address"), py::arg("attenuation"))
        .def("set_start_frequency_tuning_word", &ControlLight_API::SetStartFrequencyTuningWord, py::arg("sequencer"), py::arg("address"), py::arg("ftw"))
        .def("set_stop_frequency_tuning_word", &ControlLight_API::SetStopFrequencyTuningWord, py::arg("sequencer"), py::arg("address"), py::arg("ftw"))
        .def("set_fsk_mode", &ControlLight_API::SetFSKMode, py::arg("sequencer"), py::arg("address"), py::arg("mode"))
        .def("set_ramp_rate_clock", &ControlLight_API::SetRampRateClock, py::arg("sequencer"), py::arg("address"), py::arg("rate"))
        .def("set_clear_acc1", &ControlLight_API::SetClearACC1, py::arg("sequencer"), py::arg("address"), py::arg("on_off"))
        .def("set_triangle_bit", &ControlLight_API::SetTriangleBit, py::arg("sequencer"), py::arg("address"), py::arg("on_off"))
        .def("set_fsk_bit", &ControlLight_API::SetFSKBit, py::arg("sequencer"), py::arg("address"), py::arg("on_off"))

        // AD9858
        .def("set_frequency", &ControlLight_API::SetFrequency, py::arg("sequencer"), py::arg("address"), py::arg("frequency"))
        .def("set_frequency_tuning_word", &ControlLight_API::SetFrequencyTuningWord, py::arg("sequencer"), py::arg("address"), py::arg("ftw"))

        // AD9958
        .def("set_frequency_of_channel", &ControlLight_API::SetFrequencyOfChannel, py::arg("sequencer"), py::arg("address"), py::arg("channel"), py::arg("frequency"))
        .def("set_frequency_tuning_word_of_channel", &ControlLight_API::SetFrequencyTuningWordOfChannel, py::arg("sequencer"), py::arg("address"), py::arg("channel"), py::arg("ftw"))
        .def("set_phase_of_channel", &ControlLight_API::SetPhaseOfChannel, py::arg("sequencer"), py::arg("address"), py::arg("channel"), py::arg("phase"))
        .def("set_power_of_channel", &ControlLight_API::SetPowerOfChannel, py::arg("sequencer"), py::arg("address"), py::arg("channel"), py::arg("power"))

        // Send the assembled sequence to FPGA, but do not execute it
		.def("send_sequence", & ControlLight_API::SendSequence)
        // Send sequence to FPGA and executes it 
        .def("execute_sequence", &ControlLight_API::ExecuteSequence)

        .def("repeat_sequence", &ControlLight_API::RepeatSequence)

        // Checks sequence execution status: returns (running, data_points_written)
        .def("get_sequence_execution_status", [](ControlLight_API& self) {
        bool running;
        unsigned long long data_points_written;
        self.GetSequenceExecutionStatus(running, data_points_written);
        return py::make_tuple(running, data_points_written);
            })

        // Waits for sequence end and returns data buffer, buffer length, end time
        .def("wait_till_end_of_sequence_then_get_input_data",
            [](ControlLight_API& self, double timeout_in_s) {
                unsigned long buffer_length = 0;
                unsigned long end_time = 0;

                self.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, end_time, timeout_in_s);

                // Copy the buffer to Python bytes object
                py::bytes data(reinterpret_cast<char*>(buffer), buffer_length);

                //if you want a tuple back
				//return py::make_tuple(data, buffer_length, end_time);  

                //if you want a dictionary back
				return py::dict( 
                    "data"_a = py::bytes(reinterpret_cast<char*>(buffer), buffer_length),
                    "length"_a = buffer_length,
                    "end_time_of_cycle"_a = end_time
                );

            },
            py::arg("timeout_in_s"))

        // Sets a guard on maximum allowed time debt (in ms)
        .def("set_time_debt_guard_ms", &ControlLight_API::SetTimeDebtGuard_in_ms, py::arg("max_time_debt_in_ms"))

        // Starts analog input acquisition
        .def("sequencer_start_analog_in_acquisition", &ControlLight_API::SequencerStartAnalogInAcquisition,
            py::arg("sequencer"), py::arg("analog_in_type"), py::arg("spi_cs"), py::arg("channel_number"),
            py::arg("number_of_data_points"), py::arg("delay_between_data_points_in_ms"))

        // Write system time to input memory
        .def("sequencer_write_system_time_to_input_memory", &ControlLight_API::SequencerWriteSystemTimeToInputMemory,
            py::arg("sequencer"))

        // Writes to input memory
        .def("sequencer_write_input_memory", &ControlLight_API::SequencerWriteInputMemory,
            py::arg("sequencer"), py::arg("data"),
            py::arg("write_next_address") = true, py::arg("address") = 0)

			// Calc frequency tuning word for AD9854 from ADC input value
        .def("sequencer_calc_AD9854_frequency_tuning_word", &ControlLight_API::SequencerCalcAD9854FrequencyTuningWord,
            py::arg("sequencer"), py::arg("ftw0"),
            py::arg("bit_shift"))

        // Writes system time to input memory
        .def("sequencer_write_system_time_to_input_memory", &ControlLight_API::SequencerWriteSystemTimeToInputMemory,
            py::arg("sequencer"))

        // Switches debug LED
        .def("sequencer_switch_debug_led", &ControlLight_API::SequencerSwitchDebugLED,
            py::arg("sequencer"), py::arg("on_off"))

        // Ignores TCP/IP commands
        .def("sequencer_ignore_tcpip", &ControlLight_API::SequencerIgnoreTCPIP,
            py::arg("sequencer"), py::arg("on_off"))

        .def("use_edge_triggered_latches", &ControlLight_API::UseEdgeTriggeredLatches,
            py::arg("sequencer"), py::arg("use_edge_triggered_latches"))

        //API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerAddMarker)(const unsigned int& Sequencer, unsigned char marker);
        // Adds a marker
        .def("sequencer_add_marker", &ControlLight_API::SequencerAddMarker,
            py::arg("sequencer"), py::arg("marker"))

        //API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetTimeDebtGuard_in_ms)(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms);
		// Sets the time debt guard for a sequencer
		.def("sequencer_set_time_debt_guard_ms", &ControlLight_API::SequencerSetTimeDebtGuard_in_ms,
			py::arg("sequencer"), py::arg("max_time_debt_in_ms"))
        
        //API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetLoopCount)(const unsigned int& Sequencer, unsigned int loop_count);
		// Sets the loop count for a sequencer
		.def("sequencer_set_loop_count", &ControlLight_API::SequencerSetLoopCount,
			py::arg("sequencer"), py::arg("loop_count"))
        
        //API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerJumpBackward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool loop_count_greater_zero = false);
		// Jumps backward in the sequence
		.def("sequencer_jump_backward", &ControlLight_API::SequencerJumpBackward,
			py::arg("sequencer"), py::arg("jump_length"),
			py::arg("unconditional_jump") = true, py::arg("condition_0") = false,
			py::arg("condition_1") = false, py::arg("condition_PS") = false,
			py::arg("loop_count_greater_zero") = false)

        //API_EXPORT ERROR_CODE_TYPE CLA_FN(AddCommandJumpForward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false);
		// Jumps forward in the sequence
		.def("sequencer_jump_forward", &ControlLight_API::SequencerJumpForward,
			py::arg("sequencer"), py::arg("jump_length"),
			py::arg("unconditional_jump") = true, py::arg("condition_0") = false,
			py::arg("condition_1") = false, py::arg("condition_PS") = false)
		.def("sequencer_write_i2c", [](ControlLight_API& self, unsigned int sequencer, uint8_t i2c_port, uint8_t i2c_length, py::bytes data_out) {
				std::string payload = static_cast<std::string>(data_out);
				self.SequencerWriteI2C(sequencer, i2c_port, i2c_length, reinterpret_cast<uint8_t*>(payload.data()));
			}, py::arg("sequencer"), py::arg("i2c_port"), py::arg("i2c_length"), py::arg("data_out"))
		.def("sequencer_transmit_spi", [](ControlLight_API& self, unsigned int sequencer, uint8_t chip_select, uint16_t number_of_bits_out, py::bytes data_out, uint8_t number_of_bits_in, bool start_now) {
				std::string payload = static_cast<std::string>(data_out);
				self.SequencerTransmitSPI(sequencer, chip_select, number_of_bits_out, reinterpret_cast<const uint8_t*>(payload.data()), number_of_bits_in, start_now);
			}, py::arg("sequencer"), py::arg("chip_select"), py::arg("number_of_bits_out"), py::arg("data_out"), py::arg("number_of_bits_in"), py::arg("start_now"))
		.def("sequencer_repeated_out_in", &ControlLight_API::SequencerRepeatedOutIn,
			py::arg("sequencer"), py::arg("number_of_datapoints"), py::arg("delay_between_datapoints_in_ms"), py::arg("repeated_out_in_command"))
		.def("sequencer_set_spi_timing", &ControlLight_API::SequencerSetSPITiming,
			py::arg("sequencer"), py::arg("spi_delay_cs_low_start_wait"), py::arg("spi_delay_write"),
			py::arg("spi_delay_pause_before_read"), py::arg("spi_delay_read"), py::arg("spi_delay_cs_low_end_wait"))
		.def("sequencer_set_i2c_parameters", &ControlLight_API::SequencerSetI2CParameters,
			py::arg("sequencer"), py::arg("i2c_0_destination"))


            // AddDeviceSequencer
            .def("add_device_sequencer", &ControlLight_API::AddDeviceSequencer,
                py::arg("id"),
                py::arg("type"),
                py::arg("ip"),
                py::arg("port"),
                py::arg("master"),
                py::arg("start_delay"),
                py::arg("clock_frequency"),
                py::arg("fpga_clock_to_bus_clock_ratio"),
                py::arg("use_external_clock"),
                py::arg("use_strobe_generator"),
                py::arg("connect"))


            // AddDeviceAnalogOut16bit
            .def("add_device_analog_out_16bit", &ControlLight_API::AddDeviceAnalogOut16bit,
                py::arg("sequencer"),
                py::arg("start_address"),
                py::arg("number_channels"),
                py::arg("signed_value"),
                py::arg("min_voltage"),
                py::arg("max_voltage"))

            // AddDeviceDigitalOut
            .def("add_device_digital_out", &ControlLight_API::AddDeviceDigitalOut,
                py::arg("sequencer"),
                py::arg("address"),
                py::arg("number_channels"))

            // AddDeviceAD9854
            .def("add_device_ad9854", &ControlLight_API::AddDeviceAD9854,
                py::arg("sequencer"),
                py::arg("address"),
                py::arg("version"),
                py::arg("external_clock_frequency"),
                py::arg("pll_reference_multiplier"),
                py::arg("frequency_multiplier"))

            // AddDeviceAD9858
            .def("add_device_ad9858", &ControlLight_API::AddDeviceAD9858,
                py::arg("sequencer"),
                py::arg("address"),
                py::arg("external_clock_frequency"),
                py::arg("frequency_multiplier"))

            // AddDeviceAD9958
            .def("add_device_ad9958", &ControlLight_API::AddDeviceAD9958,
                py::arg("sequencer"),
                py::arg("address"),
                py::arg("external_clock_frequency"),
                py::arg("frequency_multiplier"))

            // AddDeviceAnalogIn12bit
            .def("add_device_analog_in_12bit", &ControlLight_API::AddDeviceAnalogIn12bit,
                py::arg("sequencer"),
                py::arg("chip_select"),
                py::arg("signed_value"),
                py::arg("min_voltage"),
                py::arg("max_voltage"));

}


#endif

