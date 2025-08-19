**Introduction**

A simple API for OpticsFoundry's control system, available as code for Visual Studio, as DLL for Qt, Visual Studio and as library for Python.

The library code is contained under [ControlLight](https://github.com/opticsfoundry/OpticsFoundry_ControlLight/tree/main/ControlLight).

The other folders contain example programs using various interfaces to the library.


**Overview**

&nbsp;

OpticsFoudry's control system can be accessed through three types of software, listed below. This repository contains ControlLightAPI. See See repository [OpticsFoundry_Control_AQuRA](https://github.com/opticsfoundry/OpticsFoundry_Control_AQuRA) for the other two versions.


&nbsp;

1. **ControlLightAPI**  
    A simple API giving the most direct access to the hardware. Tested with Python, Visual Studio C++ and Qt Creator (with MinGW).  
    ControlLightAPI can be configured through commands or through a json file.

2. **ControlAPI**  
    A fully featured API, having functions such as complex sequence assembly with "GoBackInTime", "Wavefunctions", and sequence cycling with individual command updates in the background. This is for example used for the [AQuRA clock](https://www.aquraclock.eu/).  
    This API's DLL has been tested with Visual Studio C++ and Qt Creator (with MinGW, see repository [OpticsFoundry_Control_Qt](https://github.com/opticsfoundry/OpticsFoundry_Control_Qt)). The API can also be accessed through TCP/IP, and we provide a Python and a Qt Creator example for that.  
    (If you need this API directly from Python, then tell us and we can create such a version within a few days.)  
    ControlAPI can be configured through ASCII files, a simple directory setting txt file, a hardware device configuration json file and a user output definition json file.

3. **Control.exe**

This is a fully featured experiment control system, based on ControlAPI.

Control.exe can be configured through code and/or through configuration files (the latter in the same manner as ControlAPI, on which Control.exe is based).

A somewhat outdated introduction and manual is available on [www.strontiumBEC.com](http://www.strontiumBEC.com) -> Control.

&nbsp;

&nbsp;

See Section "Installing Visual Studio 2022" below. Pay attention to always **use the same x64/Win32 Debug/Release settin**g for the DLL compiler and for the compiler that compiles the program that uses the DLL.

- **_x64_ or _Win32_** (i.e. 32bit or 64bit AMD/Intel code). For most situations x64 is better. The one situation in which Win32 is better is when editing the Control.exe software, as the compilation of Control.exe is vastly faster in Win32 than x64 mode.
- **_Debug_ or _Release_**: Release is quite a bit faster than Debug, both when compiling and when executing the program. Debug enables one to see Visual Studio debug information (even when using the DLL in Qt with MinGW).
- **_DLL or EXE:_** for ControlLightAPI you choose by modifying CMakeList.txt as explained in that file. For Control.dll vs Control.exe, in the Solution Explorer (in the Visual Studio GUI) you right click on "Control" (not " Solution Control" ) and select it under Configuration Properties -> General-> Configuration Type.

Control uses Microsoft Foundation Classes (MFC, or also called Afx). When installing Visual Studio C++, you need to select it as an extra option, otherwise it won't be installed.

&nbsp;

To install the Python library control_light_api systemwide via pip, use a command prompt, navigate to ControlLight_DLL_Test_Python_for_pip and run "pip install .". The DLL and PYD file need to be in the subdirectory "control_light_api" and the pyd file needs to be renamed \__init_\_pyd.

&nbsp;

&nbsp;

&nbsp;

&nbsp;

**ControlLightAPI interface**

&nbsp;

The best starting point is to study one of the simple example programs. For convenience here a copy of the Python test code:

&nbsp;

```py
import sys

main_path = ""
library_path = "lib"
sys.path.append(library_path)

import control_light_api

cla = control_light_api.ControlLightAPI()

print("Created:", cla.is_created())
try:
   cla.configure(display_errors = False)
   cla.load_from_json_file(main_path+r"ControlHardwareConfig.json")
except Exception as e:
   print("Error loading JSON file:", e)
   sys.exit(1)
try:
   cla.initialize()
   cla.switch_debug_mode(True, "DebugSequence")
   start_time = 0
   for i in range(10):
       cla.start_assembling_sequence()
       cla.sequencer_start_analog_in_acquisition(0,0,100, 0.1)
       for j in range(100):
           cla.set_voltage(0, 24, 10.0 \* j / 100.0)
           cla.wait_ms(0.1)
           frequency = 1000.0 \* j / 100.0
           cla.set_frequency(0, 232, frequency)
       cla.execute_sequence()
       input = cla.wait_till_end_of_sequence_then_get_input_data(timeout_in_s = 10)
       print("Input data length:", input\["length"\]," duration:", input\["end_time_of_cycle"\]-start_time)
       start_time = input\["end_time_of_cycle"\]

except Exception as e:
   print("Error:", e)
   sys.exit(1)
```

The API interface is documented in

ControlLight_docs\\html\\index.html

and therein in -> Files ->ControlAPI.h

(This document is created from the docstrings in ControlAPI.h using [Doxygen](https://doxygen.nl/). To obtain the latest version of the documentation, read the docstrings in ControlAPI.h directly, or load the Doxyfile in Control_Light_docs into Doxywizard and recompile.)

For convenience here a pdf with the relevant part:

&lt;<ControlLight_API_Documentation.pdf&gt;>

A good way to learn how the API works is to use the EXE mode in the ControlLight in Visual Studio 2022 in Debug mode. Then you can step through the example.

&nbsp;

Please note that all the Python command names are converted into lower case snake names,

e.g.

cla.load_from_json_file(main_path+r"ControlHardwareConfig.json")

instead of LoadFromJSONFile(). The details are in the binding description in  
ControlLight\\bindings\\ControlAPI_Python.cpp

&nbsp;

&nbsp;

&nbsp;

**ControlLightAPI hardware configuration JSON file**

&nbsp;

The hardware configuration file _ControlHardwareConfig.json_ contains lists of devices connected to the control system. This hardware configuration file is created by running the Python script

_.\\ConfigFileCreators\\ControlHardwareConfigFileCreator.py_

(Here . designates the control source code directory.)

If you want to read the json file, use an editor that understands json syntax, e.g. Visual Studio Code. I recommend regenerating the json file using the Python script, not manually editing it.

&nbsp;

Here an example Python hardware configuration file generator script:

&nbsp;
```py
if \__name__ == "\__main_\_":
   builder = ConfigBuilder()
   builder.RegisterSequencer(IP="192.168.0.109", Port=7, ClockFrequency=100000000, BusFrequency=2000000, DebugOn = False)
   analog_out_configs = \[
       (24, False, 0, 10),
       (28, True, -10, 10),
       (32, True, 0, 10),
       (36, True, -10, 10),
       (40, True, -10, 10)
   \]
   for addr, signed, minv, maxv in analog_out_configs:
       builder.RegisterAnalogOutBoard16bit(StartAddress=addr, Signed=signed, MinVoltage=minv, MaxVoltage=maxv)
   for addr in \[1, 2, 3, 4, 5, 6\]:
       builder.RegisterDigitalOutBoard(Address=addr)
   for addr in range(132, 172, 4):
       builder.RegisterDDSAD9854Board(Address=addr)
   for addr in range(52, 84, 4):
       builder.RegisterDDSAD9858Board(Address=addr)
   builder.RegisterDDSAD9958Board(Address=21)
   builder.Save()
```

&nbsp;

&nbsp;

**Rebuilding and using ControlLightAPI**

&nbsp;

The ControlLight API code resides in the folder "ControlLight", which is a Visual Studio C++ 2022 CMake project.

This API can be used in several ways:

1. Use it in Python, see example ControlLight_DLL_Test_Python.
2. Use it in Visual Studio, see example ControlLight_DLL_Test_VS_console.
3. Use it in Qt Creator MinGW, see examples ControlLight_DLL_Test_Qt_console and ControlLight_DLL_Test_Qt_GUI.
4. Compile it directly into your code. The examples are given directly in the ControlLight project, in the file ControlLight.cpp, and can be accessed by switching the project settings to EXE in the CMakeLists.txt file.

&nbsp;

You can also modify the API and recreated the DLL, LIB, PYD files as needed for VS, Qt, Python. For this you need to edit the code in the folder "ControlLight".

&nbsp;

Each of the examples do need access to a hardware configuration file. The path and filename is set in the LoadFromJSONFile() function in C or the cla.load_from_json_file() function in Python. In the demos it’s the ControlHardwareConfig.json file. The structure of this file is explained in the "Hardware configuration file" section below.

&nbsp;

If you need/want to update the library, then do the following

&nbsp;

1. Python:  
    \- In the ControlLight Visual Studio project, in CMakeLists.txt, choose the configuration PYD at the location marked with "<-- change this line to switch build…".

\- Safe the CMakeLists.txt file, which automatically will launch CMake and reconfigure Visual Studio.  
\- For safety, use Build -> Clean All.  
\- Build as x64 Release.  
\- Copy the files

control_light_api.cp310-win_amd64.pyd  
ControlLight.dll

from ControlLight\\out\\build\\x64-release to ControlLight_DLL_Test_Python\\lib

Installing the control_light_api systemwide using pip:  
Put the new pyd and dll into the ControlLight_DLL_Test_Python_for_pip\\control_light_api folder, rename the pyd file into \__init_\_.pyd, open a console, change to the ControlLight_DLL_Test_Python_for_pip folder, and run "pip install .".  
Uninstalling a pip installed control_light_api: run "pip uninstall control_light_api".

&nbsp;

1. Visual Studio

\- In the ControlLight Visual Studio project, in CMakeLists.txt, choose the configuration DLL at the location marked with "<-- change this line to switch build…".  
\- For the specific example given, check that the settings in the  
elseif(BUILD_MODE STREQUAL "DLL")  
branch are still

target_compile_definitions(ControlLight PRIVATE BUILDING_DLL)

target_compile_definitions(ControlLight PRIVATE C_API) target_compile_definitions(ControlLight PRIVATE \_AFXDLL)

# target_compile_definitions(ControlLight PRIVATE API_CLASS)

(note that the definition of API_CLASS is commented out).

\- Safe the CMakeLists.txt file, which automatically will launch CMake and reconfigure Visual Studio.  
\- For safety, use Build -> Clean All.  
\- Build as "x64 Debug" or "x64 Release". The release version is significantly faster, but doesn't provide Visual Studio debug information.  
\- Copy the file

ControlAPI.h

from ControlLight to ControlLight_DLL_Test_VS_console\\include and rename it to  
ControlLight.h  
Copy the files  
ControlLight.lib  
ControlLight.dll

For "x64 Debug":  
from ControlLight\\out\\build\\x64-debug to ControlLight_DLL_Test_VS_console\\lib and ControlLight_DLL_Test_VS_console\\bin, respectively.  
<br/>For "x64 Release":  
from ControlLight\\out\\build\\x64-release to ControlLight_DLL_Test_VS_console\\lib and ControlLight_DLL_Test_VS_console\\bin, respectively.  
<br/>

1. Qt  
    \- Recreate the DLL as explained under 2) Visual Studio in x64 release or debug mode.

&nbsp;

For release version:

ControlLight_DLL_Test_Qt_console\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Release\\release  
and

ControlLight_DLL_Test_Qt_GUI\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Release\\release  

For debug version:  
\- copy ControlLight.dll file from from ControlLight\\out\\build\\x64-debug to

ControlLight_DLL_Test_Qt_console\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Debug\\debug  
and

ControlLight_DLL_Test_Qt_GUI\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Debug\\debug  

&nbsp;

&nbsp;

&nbsp;

**Basic structure of ControlLightAPI**

&nbsp;

Each device is described in a class CDevice… derived from CDevice.  
The sequencer is also such a device, but with a somewhat special status, as it communicates with the control program over TCP/IP and sends signals to the devices attached to the FPGA sequencer over parallel bus or SPI.  

When the hardware configuration json file is read, the sequencers and their attached devices are defined, each as an instance of its class. The sequencers contain a list of all possible addresses, containing pointers to these instances. The pointers are nullptr if nothing is connected.

&nbsp;

When a command is sent to a device, attached to a sequencer with a certain id on a certain address, the software looks up the pointer to the instance of CDevice (or derived class) that exists under that sequencer id and address. If nullptr is obtained an error is registered.  
<br/>If not nullptr is obtained, the command is sent to the device. If the device is of the correct type, it will be able to handle the command. However, if the user confused an address, and sends a, let's say, SetVoltage command to a DDS, there is a problem. The CDeviceDDS class doesn't contain a SetVoltage command. Therefore the base classes SetVoltage command is called, i.e. CDevice::SetVoltage. All CDevice command functions are abstract, they just register an error.

&nbsp;

This means we use the C++ virtual function programing style to check if the commands sent by the user are ok.

&nbsp;

What does "registering an error" do?

If the user has defined cla.configure(True) an error message will be shown.

If Python is used or a C++ DLL compiled with #define THROW_EXCEPTIONS, then a CLA_Exception will be thrown.

If none of the above, the error message will be added to a list of error messages and a flag will be set showing that an error occurred. The user of the API can use DidErrorOccur() to querry if one occurred. If one occurred GetLastError will return all the ten last error messages as one string.

&nbsp;

&nbsp;

&nbsp;

&nbsp;

&nbsp;

1. **ControlAPI interface**

&nbsp;

&nbsp;

The large version of the ControlAPI interface can be accessed over TCP/IP (when running Control.exe), or be used as a DLL from Visual Studio 2022 C++, Qt Creator, or Python (can quickly be implemented if needed).

&nbsp;

The list of functions is described in [this Google Sheet](https://docs.google.com/spreadsheets/d/1ACSwebKKuk_bsI_3-E2YGZ4p2IvL3By_wfjo8vWTsmc/edit?usp=sharing).

&nbsp;

The best way to get to know this API is to study the demo code in Control_Firefly_AQuRA_Qt.

&nbsp;

The ControlAPI is basically Control.exe, just running without showing the GUI, as a DLL. This means it is configured like Control.exe. One difference is how one points the DLL to the folder in which the configuration files are. For ControlAPI, you specify this folder when you call "ConnectToLowLevelSoftware", see the definition of ParamFileDirectory in TestSequence.cpp. For example  

_const char\* ParamFileDirectory = "D:\\\\Florian\\\\Firefly\\\\FireflyControl\\\\Control_Firefly_AQuRA\\\\ConfigParams\\\\ControlParam";_

points to the configuration files

_ControlParam_ParamList.txt_

_ControlParam_SystemParamList.txt_

_ControlParam_UniMessList.dat_

_ControlParam_UtilityDialog.txt_

in the folder

_D:\\Florian\\Firefly\\FireflyControl\\Control_Firefly_AQuRA\\ConfigParams\\_

Note that ParamFileDirectory includes the folder name and the beginning of the configuration file names.

&nbsp;

At the same location, you should also specify a folder in which debug files will be stored, e.g.

_const QString DebugFileDirectory = "D:\\\\Florian\\\\Firefly\\\\FireflyControl\\\\DebugFiles";_

You can switch debug on or off in this codeline:

&nbsp;_if (!CA.ConnectToLowLevelSoftware(atelnet, ParamFileDirectory, /\*IP\*/ "192.168.0.103", /\*Debug\*/ true, DebugFileDirectory)) {_

&nbsp;

&nbsp;

You can use the API in TCPIP mode or in DLL mode. The TCPIP mode is useful for debugging, but otherwise the DLL mode is preferred. You can switch between TCPIP mode and DLL mode in

_ControlAPI.h_

By defining or not defining

_#define USE_CA_DLL_

If you use the TCPIP mode, search for "192.168." and adjust the IP adress to the one of the computer on which Control.exe is running. Make sure that Control.exe starts the ControlAPI sever, by enabling that option in the system parameter menus.

&nbsp;

In either mode, you need to select the IP address of the FPGA. You do this using a Python hardware configuration script, which is described in Section 3) Control.exe.

&nbsp;

To recompile Control.dll, open Control.sln in Visual Studio 2022. Select Debug, x64 in the toolbar:

&nbsp;

![](data:image/png;base64,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)

&nbsp;

To select "DLL", right-click on "Control" in the Solution explorer (not "Solution Control"). Select Properties. Select DLL under "General Properties -> Configuration Type".  
Build the solution. Look at the linker output. It tells you in which folder the DLL was created, e.g.

_D:\\Florian\\Firefly\\FireflyControl\\Control_Firefly_AQuRA\\x64\\Template_

&nbsp;

Copy _Control.dll_ to the folder in which Qt creates your executable, e.g.

_D:\\Florian\\Firefly\\FireflyControl\\Control_Firefly_Qt\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Debug\\debug_

&nbsp;

If you want to change the hardware configuration, do it by modifying the Configuration Python scripts and running them to create configuration json files, see Sec. 3) Control.exe.

&nbsp;

If you need a second name for an output, do that in Sequence.cpp (you can tell ChatGPT to create this table from the Google Sheet that we use for the AQuRA project:

&nbsp;

void CSequence::AddAlternativeCommandNames() {

ControlAPI.AddAlternativeCommandName("SetMOTCoilCurrent", "MOTCurrent", /\*Scale\*/2,/\*Offset\*/ 0);

ControlAPI.AddAlternativeCommandName("SetChillerSetpoint", "ChillerSetpoint");

_}_

