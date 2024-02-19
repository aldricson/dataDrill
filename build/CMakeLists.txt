FILE (WRITE ../buildLog.txt "*** starting configuration ***\n")
FILE (APPEND ../buildLog.txt "specifying that the target system is Linux and the processor is x86_64.\n")
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
FILE (APPEND ../buildLog.txt "Setting the path for the toolchain.\n")
set(toolchainpath C:/build/20.x/x64/sysroots)
FILE (APPEND ../buildLog.txt "Setting the C and C++ compilers for N.i. Rt linux\n")
set(CMAKE_C_COMPILER ${toolchainpath}/i686-nilrtsdk-mingw32/usr/bin/x86_64-nilrt-linux/x86_64-nilrt-linux-gcc.exe)
set(CMAKE_CXX_COMPILER ${toolchainpath}/i686-nilrtsdk-mingw32/usr/bin/x86_64-nilrt-linux/x86_64-nilrt-linux-g++.exe)
FILE (APPEND ../buildLog.txt "Setting the system root directory for the toolchain.\n")
set(CMAKE_SYSROOT ${toolchainpath}/core2-64-nilrt-linux)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${toolchainpath}/core2-64-nilrtlinux/usr/include/c++/6.30 ${toolchainpath}/core2-64-nilrt-linux/usr/include/c++/6.3.0/x86_64-nilrtlinux)
set(CMAKE_CXX_FLAGS "-Wall -fmessage-length=0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

FILE (APPEND ../buildLog.txt "Setting the search behavior for various types of files.\n")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_CXX_STANDARD 20)
FILE (APPEND ../buildLog.txt "*** TOOL CHAIN AND COMPILER CONFIGURED ***\n\n")

FILE (APPEND ../buildLog.txt "*** Project specific ***\n")
cmake_minimum_required(VERSION 3.19)
set(PROJECT_NAME dataDrill)
FILE (APPEND ../buildLog.txt "project name is set to: ${PROJECT_NAME}\n")
project(${PROJECT_NAME})
FILE (APPEND ../buildLog.txt "loading sources and headers")
file(GLOB_RECURSE SOURCE_FILES "../src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "../src/*.h*")
FILE (APPEND ../buildLog.txt "\nList of Header Files:\n")
foreach(str ${HEADER_FILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()
FILE (APPEND ../buildLog.txt "List of Source Files:\n")
foreach(src ${SOURCE_FILES})
    FILE (APPEND ../buildLog.txt "${src}\n")
endforeach()
set(EXECUTABLE_OUTPUT_PATH bin)
FILE (APPEND ../buildLog.txt "Excutable output file set to ${EXECUTABLE_OUTPUT_PATH} bin\n")
set(CMAKE_BUILD_TYPE Debug)
FILE (APPEND ../buildLog.txt "build type is: ${CMAKE_BUILD_TYPE}\n")
set(THREADS_PREFER_PTHREAD_FLAG ON)
FILE (APPEND ../buildLog.txt "threading type is set to linux pthread library\n\n")

FILE (APPEND ../buildLog.txt "*** daqMx ***\n")
set(DAQMX_INCLUDE "C:/Program\ Files\ (x86)/National\ Instruments/NI-DAQ/DAQmx\ ANSI\ C\ Dev/include")
FILE (APPEND ../buildLog.txt "daqMx include path is set to: ${DAQMX_INCLUDE}\n")
file(GLOB_RECURSE DAQMX_HEADER_FILES "${DAQMX_INCLUDE}/*.h")
FILE (APPEND ../buildLog.txt "\nList of daqmx Headers:\n")
foreach(str ${DAQMX_HEADER_FILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()

# *** Add libmodbus Start ***
# ***  Locate libmodbus   ***
set(LIBMODBUS_PATH "C:/build/20.x/x64/sysroots/core2-64-nilrt-linux/usr/local/lib/libmodbus.so.5.1.0")
set(LIBMODBUS_INCLUDE_PATH "../src/Modbus/libmodbus/")

FILE (APPEND ../buildLog.txt "*** libModbus ***\n")
#file(GLOB_RECURSE LIBMODBUS_SOURCE_FILES "../src/Modbus/libmodbus/*.c")
file(GLOB_RECURSE LIBMODBUS_HEADER_FILES "../src/Modbus/libmodbus/*.h")
FILE (APPEND ../buildLog.txt "\nList of libmodbus Header Files:\n")
foreach(str ${LIBMODBUS_HEADER_FILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()
FILE (APPEND ../buildLog.txt "\n")
#FILE (APPEND ../buildLog.txt "List of libmodbus Source Files:\n")
#foreach(src ${LIBMODBUS_SOURCE_FILES})
#    FILE (APPEND ../buildLog.txt "${src}\n")
#endforeach()
#FILE (APPEND ../buildLog.txt "\n")
# *** Add libmodbus End ***



set (DAQMX_LIB "C:/Program\ Files\ (x86)/National\ Instruments/Shared/ExternalCompilerSupport/C/lib64/gcc")
FILE (APPEND ../buildLog.txt "linux librairies selected: ${DAQMX_LIB}\n")
file(GLOB_RECURSE DAQMX_LINUXLIB_FILES "${DAQMX_LIB}/*.so")
FILE (APPEND ../buildLog.txt "\nList of daqmx linux libraries:\n")
foreach(str ${DAQMX_LINUXLIB_FILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()

set (NISYSCFG_INCLUDE "C:/Program\ Files\ (x86)/National\ Instruments/Shared/ExternalCompilerSupport/C/include")
file(GLOB_RECURSE NISYSCFG_HEADER_FILES "${NISYSCFG_INCLUDE}/nisyscfg*.h")
FILE (APPEND ../buildLog.txt "\nList of nisyscfg linux headers:\n")
foreach(str ${NISYSCFG_HEADER_FILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()

set (OPENSSL_LIB_DIR "C:/build/20.x/x64/sysroots/core2-64-nilrt-linux/usr/lib")


set (OPENSSL_INCLUDE "C:/build/20.x/x64/sysroots/core2-64-nilrt-linux/usr/include/openssl")
file(GLOB_RECURSE OPENSSL_HEADERFILES "${OPENSSL_INCLUDE}/*.h")
FILE (APPEND ../buildLog.txt "\nList of openssl linux headers:\n")
foreach(str ${OPENSSL_HEADERFILES})
    FILE (APPEND ../buildLog.txt "${str}\n")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})


FILE (APPEND ../buildLog.txt "all files has been added to linux elf executable \n")



# Update include directories and linked libraries
target_include_directories(${PROJECT_NAME} PUBLIC ${DAQMX_INCLUDE} ${NISYSCFG_INCLUDE} ${LIBMODBUS_INCLUDE_PATH} ${OPENSSL_INCLUDE})
FILE (APPEND ../buildLog.txt "Nidaqmx, NiSysConfig, libmodbus, openssl headers directories are now added to the project\n")

target_link_libraries(${PROJECT_NAME} PUBLIC ${DAQMX_LINUXLIB_FILES} ${LIBMODBUS_PATH} ${OPENSSL_LIB_DIR}/libssl.so ${OPENSSL_LIB_DIR}/libcrypto.so pthread)
FILE (APPEND ../buildLog.txt "daqmx, libmodbus, and linux threading libraries are now linked to the project\n")

