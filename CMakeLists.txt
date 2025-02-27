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
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Musica_2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Musica_2 Musica_2.c inc/ssd1306.c)

pico_set_program_name(Musica_2 "Musica_2")
pico_set_program_version(Musica_2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Musica_2 1)
pico_enable_stdio_usb(Musica_2 1)

pico_generate_pio_header(Musica_2 ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

# Add the standard library to the build
target_link_libraries(Musica_2
        pico_stdlib
        hardware_pio
        hardware_pwm
        hardware_adc
        )

# Add the standard include files to the build
target_include_directories(Musica_2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(Musica_2 
        hardware_i2c
        )

target_sources(Musica_2 PRIVATE
       inc/ssd1306.c
    )

pico_add_extra_outputs(Musica_2)

