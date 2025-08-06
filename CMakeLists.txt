# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(airsoft-display C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(airsoft-display 
    src/main.cpp 
    lib/sh1107-pico/src/sh1107_driver.cpp
)
    # add_executable(pico_examples pico_examples.cpp)

pico_set_program_name(airsoft-display "airsoft-display")
    # pico_set_program_name(pico_examples "pico_examples")
pico_set_program_version(airsoft-display "0.1")
    # pico_set_program_version(pico_examples "0.1")

# Generate PIO header
pico_generate_pio_header(airsoft-display ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
    # pico_generate_pio_header(pico_examples ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(airsoft-display 0)
pico_enable_stdio_usb(airsoft-display 0)
    # pico_enable_stdio_uart(pico_examples 0)
    # pico_enable_stdio_usb(pico_examples 0)

# Add the standard library to the build
target_link_libraries(airsoft-display
        pico_stdlib)
    # target_link_libraries(pico_examples
    #         pico_stdlib)

target_include_directories(airsoft-display PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        src
        lib/sh1107-pico/src
)
    # target_include_directories(pico_examples PRIVATE
    #         ${CMAKE_CURRENT_LIST_DIR}
    #         ${CMAKE_CURRENT_BINARY_DIR}
    # )

# Add any user requested libraries
target_link_libraries(airsoft-display 
        hardware_spi
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )
    # target_link_libraries(pico_examples 
    #         hardware_spi
    #         hardware_dma
    #         hardware_pio
    #         hardware_interp
    #         hardware_timer
    #         hardware_watchdog
    #         hardware_clocks
    #         )

pico_add_extra_outputs(airsoft-display)
    # pico_add_extra_outputs(pico_examples)

