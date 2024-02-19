# *** Starting Configuration ***
cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "dataDrill")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-nilrt-linux-g++")
set(CMAKE_C_COMPILER "/usr/bin/x86_64-nilrt-linux-gcc")

project(${PROJECT_NAME})

# *** Project Specific ***
file(GLOB_RECURSE SOURCE_FILES "../src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "../src/*.h*")

# Compiler Flags
set(CMAKE_CXX_FLAGS "-Wall -fmessage-length=0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 17)

# *** DAQmx ***
set(DAQMX_INCLUDE "../DAQMX_INCLUDE") # Update this path
file(GLOB_RECURSE DAQMX_HEADER_FILES "${DAQMX_INCLUDE}/*.h")

set(DAQMX_LIB "../DAQMX_LIB") # Update this path
file(GLOB_RECURSE DAQMX_LINUXLIB_FILES "${DAQMX_LIB}/*.so")

# *** NISYSCFG ***
set(NISYSCFG_INCLUDE "/path/to/NISYSCFG/include") # Update this path
file(GLOB_RECURSE NISYSCFG_HEADER_FILES "${NISYSCFG_INCLUDE}/nisyscfg*.h")

# *** Executable and Linking ***
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${DAQMX_HEADER_FILES} ${NISYSCFG_HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${DAQMX_INCLUDE} ${NISYSCFG_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC ${DAQMX_LINUXLIB_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
