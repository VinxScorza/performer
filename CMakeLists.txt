cmake_minimum_required(VERSION 3.10...3.30)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/src/platform/stm32/libs/libopencm3/README.md)
message(STATUS "" )
message(STATUS "------------------------------------------------------------" )
message(STATUS "" )
message(STATUS "             !!! submodules not found !!!" )
message(STATUS "" )
message(STATUS "     fix with: git submodule update --init --recursive" )
message(STATUS "" )
message(STATUS "------------------------------------------------------------" )
message(STATUS "" )
message(FATAL_ERROR "libopencm3 submodule not found!")
endif()

project(sequencer)
enable_testing()

# Fix per pybind11
if(NOT CMAKE_SYSTEM_NAME MATCHES "Emscripten")
set(PYBIND11_FINDPYTHON ON)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
endif()

add_subdirectory(src)
