# CMakeList.txt : CMake project for ControlLight, include source and define
# project specific logic here.
#
project(ControlLight)
project ("ControlLight")

cmake_minimum_required (VERSION 3.14)


# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MFC_FLAG 2)  # 1 = static, 2 = shared DLL

# Add custom option for build mode
set(BUILD_MODE "EXE" CACHE STRING "Choose build mode: EXE, DLL or PYD")
set_property(CACHE BUILD_MODE PROPERTY STRINGS EXE DLL PYD)


# Set the build mode here manually: EXE, DLL, or PYD
set(BUILD_MODE "EXE" CACHE STRING "Choose build mode" FORCE)  # <-- change this line to switch build modes  EXE, DLL, DLL_TEST or PYD, use Build -> Clean All if there is a problem with building the project


# Use dynamic (DLL) runtime libraries, needed for _AFXDLL
foreach(flag_var
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_MINSIZEREL)
  string(REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
endforeach()


# Conditional target generation
if(BUILD_MODE STREQUAL "EXE")
   

    # Enable Hot Reload for MSVC compilers if supported.
    if (POLICY CMP0141)
      cmake_policy(SET CMP0141 NEW)
      set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()

    #add all sources
    file(GLOB SOURCES "*.cpp" "*.h")

    #To create an executable for testing
    add_executable(ControlLight ${SOURCES})
  

    target_compile_definitions(ControlLight PRIVATE USING_DLL)
    target_compile_definitions(ControlLight PRIVATE _AFXDLL)
    target_compile_definitions(ControlLight PRIVATE C_API) #if you do not want a C++ API; comment out if you want C++ API
    #target_compile_definitions(ControlLight PRIVATE THROW_EXCEPTIONS) #Use this only for a C++ API; if you want to use C API, comment this line out
    #target_compile_definitions(ControlLight PRIVATE API_CLASS) #Use this only for a C++ API; if you want to use C API, comment this line out

    #set(CMAKE_BUILD_TYPE Debug)

    if (CMAKE_VERSION VERSION_GREATER 3.12)
        set_property(TARGET ControlLight PROPERTY CXX_STANDARD 20)
    endif()



elseif(BUILD_MODE STREQUAL "DLL")
   

    # Enable Hot Reload for MSVC compilers if supported.
    if (POLICY CMP0141)
      cmake_policy(SET CMP0141 NEW)
      set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()

    #add all sources
    file(GLOB SOURCES "*.cpp" "*.h")
    #To create DLL for C or C++
    add_library(ControlLight SHARED ${SOURCES})
    target_compile_definitions(ControlLight PRIVATE BUILDING_DLL)
    target_compile_definitions(ControlLight PRIVATE C_API) #if you do not want a C++ API; comment out if you want C++ API
    target_compile_definitions(ControlLight PRIVATE _AFXDLL)
    #target_compile_definitions(ControlLight PRIVATE API_CLASS) #Use this only for a C++ API; if you want to use C API, as needed for e.g. Qt, comment this line out

    
    #target_compile_definitions(ControlLight PRIVATE NAMESPACE_CLA) #only for C++ API, if you want to keep DLL functions in namespace CLA
    #if you use Namespace CLA, you need to put CLA:: in front of all DLL function names; you can then remove the CLA_ prefix from all functions


    #optional: move results to target folder
    #install(TARGETS ControlLight DESTINATION lib)
    #install(FILES ControlLight.h DESTINATION include)

    if (CMAKE_VERSION VERSION_GREATER 3.12)
        set_property(TARGET ControlLight PROPERTY CXX_STANDARD 20)
    endif()

elseif(BUILD_MODE STREQUAL "PYD")
    


    # Collect sources
    file(GLOB SOURCES "*.cpp" "*.h")

    # Add pybind11 from local source
    add_subdirectory(external/pybind11)

    # Add include path for json.hpp
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    file(GLOB_RECURSE CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
    )

    # Add ControlLight core library
    add_library(ControlLight SHARED ${SOURCES})
    target_compile_definitions(ControlLight PRIVATE PYTHON_API THROW_EXCEPTIONS API_CLASS _AFXDLL)
    target_include_directories(ControlLight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Add pybind11 (already downloaded manually)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11_build)

    # Add Python module
    pybind11_add_module(control_light_api bindings/ControlAPI_Python.cpp)
    target_link_libraries(control_light_api PRIVATE ControlLight)
    target_compile_definitions(control_light_api PRIVATE PYTHON_API THROW_EXCEPTIONS API_CLASS _AFXDLL)


else()
    message(FATAL_ERROR "Invalid BUILD_MODE: must be EXE, DLL, or PYD")
endif()

