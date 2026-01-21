# Copyright (c) 2022 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

# Set default path for Mbed OS library (no force-write for override)
set(MBED_OS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mbed-os CACHE INTERNAL "")

# Set default path for mbed_app.json5 (no force-write for override)
set(MBED_APP_JSON_PATH mbed_app.json5 CACHE INTERNAL "")

# Set default path for custom_targets.json5 (no force-write for override)
set(CUSTOM_TARGETS_PATH custom_targets CACHE INTERNAL "")
set(CUSTOM_TARGETS_JSON_PATH ${CUSTOM_TARGETS_PATH}/custom_targets.json5 CACHE INTERNAL "")

# Include Mbed toolchain setup file
include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)

set(APP_PROJECT MBED_CE_MCUBOOT_BLINKY)
set(APP_TARGET mbed-ce-mcuboot-blinky)

# Set up project name
project(${APP_PROJECT})

# Include Mbed project setup file
include(mbed_project_setup)

# Add for custom targets
add_subdirectory(${CUSTOM_TARGETS_PATH})

# Add Mbed OS library
add_subdirectory(${MBED_OS_PATH})

# Add other libraries
add_subdirectory(mcuboot/boot/bootutil)
add_subdirectory(mcuboot/boot/mbed)
if("COMPONENT_NUSD=1" IN_LIST MBED_TARGET_DEFINITIONS)
    add_subdirectory(drivers/COMPONENT_NUSD)
endif()

add_executable(${APP_TARGET})

# User-provided MBEDTLS_USER_CONFIG_FILE for mbedtls
target_include_directories(mbed-mbedtls
    PUBLIC
        .
)

target_sources(${APP_TARGET}
    PRIVATE
        main.cpp
        secondary_bd.cpp
        # Unneeded for mcuboot as library
        #signing_keys.c
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        mbed-os        
        mbed-mcuboot
        mbed-mbedtls
        $<$<IN_LIST:COMPONENT_DATAFLASH=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-dataflash>
        $<$<IN_LIST:DEVICE_FLASH=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-flashiap>
        $<$<IN_LIST:COMPONENT_I2CEE=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-i2cee>
        $<$<IN_LIST:COMPONENT_OSPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-ospif>
        $<$<IN_LIST:COMPONENT_QSPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-qspif>
        $<$<IN_LIST:COMPONENT_SD=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-sd>
        $<$<IN_LIST:COMPONENT_SPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-spif>
        $<$<IN_LIST:COMPONENT_NUSD=1,${MBED_TARGET_DEFINITIONS}>:nusd>
)

# Change flash image name for "ninja flash-<app>", where <app>_merged.hex
# is the combined image in post-build process
#
# "<app>.bin/hex" -> "<app>_merged.hex"
function(mbed_generate_upload_target target)
    gen_upload_target(${target} ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${target}>_merged.hex)
endfunction()

# Change flash image name for upload target, where <app>_merged.hex is the
# combined image in post-build process
#
# "load" -> "load <app>_merged.hex"
function(mbed_generate_upload_target target)
    # add upload target
    #
    # Ignore OUTPUT_EXT and fix to Intel Hex format
    gen_upload_target(${target} ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${target}>_merged.hex)

    # Make sure building the upload target causes the target to be built first
    if(TARGET flash-${target})
        add_dependencies(flash-${target} ${target})
    endif()

endfunction()

# Change flash image name for GDB debug, where <app>_merged.hex is the
# combined image in post-build process
#
# "load" -> "load <app>_merged.hex"
function(mbed_get_upload_launch_commands_for target result_var)

    set(${result_var} ${MBED_UPLOAD_LAUNCH_COMMANDS})

    # <app>_merged.hex for debug launch load
    set(HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${target}>_merged.hex)

    # GDB load command in MBED_UPLOAD_LAUNCH_COMMANDS?
    list(FIND MBED_UPLOAD_LAUNCH_COMMANDS "load" LOAD_INDEX)
    if(${LOAD_INDEX} GREATER_EQUAL 0)
        # "load" -> "load <app>.hex"
        #
        # GDB load command doesn't support binary format. Ignore OUTPUT_EXT
        # and fix to Intel Hex format.
        #
        # NOTE: The <app>.hex file name needs to be quoted (\") to pass along
        #       to gdb correctly.
        list(TRANSFORM ${result_var} APPEND " \"${HEX_FILE}\"" AT ${LOAD_INDEX})
    endif()

    # 'monitor program' command in MBED_UPLOAD_LAUNCH_COMMANDS?
    list(FIND MBED_UPLOAD_LAUNCH_COMMANDS "monitor program" MON_PROG_INDEX)
    if(${MON_PROG_INDEX} GREATER_EQUAL 0)
        list(TRANSFORM ${result_var} APPEND " \"${HEX_FILE}\"" AT ${MON_PROG_INDEX})
    endif()

    set(${result_var} ${${result_var}} PARENT_SCOPE)
endfunction(mbed_get_upload_launch_commands_for)

# Must call this for each target to set up bin file creation, code upload, etc
mbed_set_post_build(${APP_TARGET})
