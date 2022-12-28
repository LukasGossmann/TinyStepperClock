# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/lukas/RaspberryPiPico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(TinyStepperClock C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(TinyStepperClock
  TinyStepperClock.c
  PWM.c
  Stepper.c
  RTC.c
  WS2812.c
)

pico_set_program_name(TinyStepperClock "TinyStepperClock")
pico_set_program_version(TinyStepperClock "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(TinyStepperClock no_flash)

pico_generate_pio_header(TinyStepperClock ${CMAKE_CURRENT_LIST_DIR}/WS2812.pio)

pico_enable_stdio_uart(TinyStepperClock 0)
pico_enable_stdio_usb(TinyStepperClock 1)

# Add the standard library to the build
target_link_libraries(TinyStepperClock
        pico_stdlib
        hardware_pio
        hardware_pwm
        hardware_rtc)

# Add the standard include files to the build
target_include_directories(TinyStepperClock PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(TinyStepperClock
        )

pico_add_extra_outputs(TinyStepperClock)

