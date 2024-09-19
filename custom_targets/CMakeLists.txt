## CMakeLists which sets up the custom targets in this project

# Add Nuvoton CMake list files to CMake module search path
#
# Normally, this is added via add_subdirectory(mbed-os), but
# add_subdirectory(custom_targets) must place in front and may
# rely on these cmake list files beforehand.
list(APPEND CMAKE_MODULE_PATH
    ${MBED_SOURCE_DIR}/targets/TARGET_NUVOTON/scripts
)

# To create a custom target, you first need to figure out the CMake name of the target
# from its Mbed name.  This name is calculated as:
#
# 1. Convert target to lowercase
# 2. Replace all underscores (_) with hyphens (-)
# 3. add "mbed-" prefix.
#
# So, the target named 'NUMAKER_IOT_M467' will be called mbed-numaker-iot-m467 in CMake.
# The Mbed build system will look for a target of this name when configuring, so make sure to create it.

add_library(mbed-numaker-iot-m467-flashiap INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-flashiap INTERFACE mbed-numaker-iot-m467)

add_library(mbed-numaker-iot-m467-flashiap-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-flashiap-test INTERFACE mbed-numaker-iot-m467-flashiap)

add_library(mbed-numaker-iot-m467-flashiap-dualbank INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-flashiap-dualbank INTERFACE mbed-numaker-iot-m467)

add_library(mbed-numaker-iot-m467-flashiap-dualbank-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-flashiap-dualbank-test INTERFACE mbed-numaker-iot-m467-flashiap-dualbank)

add_library(mbed-numaker-iot-m467-spif INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-spif INTERFACE mbed-numaker-iot-m467)

add_library(mbed-numaker-iot-m467-spif-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-spif-test INTERFACE mbed-numaker-iot-m467-spif)

add_library(mbed-numaker-iot-m467-spif-boot80k INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-spif-boot80k INTERFACE mbed-numaker-iot-m467)

add_library(mbed-numaker-iot-m467-spif-boot80k-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-spif-boot80k-test INTERFACE mbed-numaker-iot-m467-spif-boot80k)

add_library(mbed-numaker-iot-m467-nusd INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-nusd INTERFACE mbed-numaker-iot-m467)

add_library(mbed-numaker-iot-m467-nusd-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m467-nusd-test INTERFACE mbed-numaker-iot-m467-nusd)

add_library(mbed-numaker-iot-m487-spif INTERFACE)
target_link_libraries(mbed-numaker-iot-m487-spif INTERFACE mbed-numaker-iot-m487)

add_library(mbed-numaker-iot-m487-spif-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m487-spif-test INTERFACE mbed-numaker-iot-m487-spif)

add_library(mbed-numaker-iot-m487-nusd INTERFACE)
target_link_libraries(mbed-numaker-iot-m487-nusd INTERFACE mbed-numaker-iot-m487)

add_library(mbed-numaker-iot-m487-nusd-test INTERFACE)
target_link_libraries(mbed-numaker-iot-m487-nusd-test INTERFACE mbed-numaker-iot-m487-nusd)
