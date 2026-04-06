
# disable compiler checks
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# setup cross compiler toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING 1)

# TOOLCHAIN_ROOT is expected to point to the directory containing arm-none-eabi-* binaries.
# If not provided, prefer the repo toolchain, then fall back to PATH.
set(_repo_toolchain_bin "${CMAKE_CURRENT_LIST_DIR}/../tools/gcc-arm-none-eabi/bin")
if(NOT DEFINED TOOLCHAIN_ROOT OR TOOLCHAIN_ROOT STREQUAL "")
    if(EXISTS "${_repo_toolchain_bin}/arm-none-eabi-gcc")
        set(TOOLCHAIN_ROOT "${_repo_toolchain_bin}")
    elseif(DEFINED ENV{TOOLCHAIN_ROOT} AND NOT "$ENV{TOOLCHAIN_ROOT}" STREQUAL "")
        set(TOOLCHAIN_ROOT "$ENV{TOOLCHAIN_ROOT}")
    endif()
endif()

if(DEFINED TOOLCHAIN_ROOT AND NOT TOOLCHAIN_ROOT STREQUAL "")
    file(TO_CMAKE_PATH "${TOOLCHAIN_ROOT}" TOOLCHAIN_ROOT)
    set(_arm_gcc "${TOOLCHAIN_ROOT}/arm-none-eabi-gcc")
    set(_arm_gxx "${TOOLCHAIN_ROOT}/arm-none-eabi-g++")
    set(_arm_ar "${TOOLCHAIN_ROOT}/arm-none-eabi-ar")
    set(_arm_ranlib "${TOOLCHAIN_ROOT}/arm-none-eabi-ranlib")
    set(_arm_ld "${TOOLCHAIN_ROOT}/arm-none-eabi-ld")

    if(NOT EXISTS "${_arm_gcc}" OR NOT EXISTS "${_arm_gxx}" OR NOT EXISTS "${_arm_ld}")
        message(FATAL_ERROR "TOOLCHAIN_ROOT='${TOOLCHAIN_ROOT}' must contain arm-none-eabi-gcc, arm-none-eabi-g++, and arm-none-eabi-ld")
    endif()
else()
    find_program(_arm_gcc arm-none-eabi-gcc)
    find_program(_arm_gxx arm-none-eabi-g++)
    find_program(_arm_ar arm-none-eabi-ar)
    find_program(_arm_ranlib arm-none-eabi-ranlib)
    find_program(_arm_ld arm-none-eabi-ld)

    if(NOT _arm_gcc OR NOT _arm_gxx OR NOT _arm_ld)
        message(FATAL_ERROR "arm-none-eabi toolchain not found. Set TOOLCHAIN_ROOT (directory with arm-none-eabi-* binaries) or ensure toolchain is in PATH.")
    endif()
endif()

set(CMAKE_C_COMPILER "${_arm_gcc}")
set(CMAKE_CXX_COMPILER "${_arm_gxx}")
if(_arm_ar)
    set(CMAKE_AR "${_arm_ar}")
endif()
if(_arm_ranlib)
    set(CMAKE_RANLIB "${_arm_ranlib}")
endif()
set(CMAKE_LINKER "${_arm_ld}")
# Some STM32 targets in this project still consume LINKER explicitly.
set(LINKER "${_arm_ld}" CACHE FILEPATH "ARM linker executable" FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
