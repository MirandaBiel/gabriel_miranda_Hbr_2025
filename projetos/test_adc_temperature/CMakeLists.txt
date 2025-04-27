# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

project(TEST_ADC_TEMPERATURE C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detecta se estamos no HOST ou na PICO
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BUILDING_FOR_HOST TRUE)
else()
    set(BUILDING_FOR_HOST FALSE)
endif()

# Se NÃO for Host, importa SDK e inicializa Pico
if(NOT BUILDING_FOR_HOST)
    include(pico_sdk_import.cmake)
    pico_sdk_init()
endif()

# === Firmware principal para a Pico ===
if(NOT BUILDING_FOR_HOST)
    add_executable(test_adc_temperature
        src/test_adc_temperature.c
        src/adc_to_celsius.c
        inc/ssd1306_i2c.c
    )

    pico_set_program_name(test_adc_temperature "test_adc_temperature")
    pico_set_program_version(test_adc_temperature "0.1")

    pico_enable_stdio_uart(test_adc_temperature 0)
    pico_enable_stdio_usb(test_adc_temperature 1)

    target_link_libraries(test_adc_temperature
        pico_stdlib
        hardware_adc
        hardware_i2c
    )

    target_include_directories(test_adc_temperature PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/inc
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    pico_add_extra_outputs(test_adc_temperature)
endif()

# === Testes para rodar no PC (HOST) ===
# Sempre cria os testes independente da plataforma
add_executable(tests
    tests/test_adc_to_celsius.c
    src/adc_to_celsius.c
    lib/unity/src/unity.c
)

target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/unity/src
)

if(BUILDING_FOR_HOST)
    # Para Windows/Linux: Não linka nada de hardware
    target_link_libraries(tests
        m    # No Linux às vezes precisa da math lib
    )
else()
    # Para Pico: se quisesse rodar testes na Pico
    target_link_libraries(tests
        pico_stdlib
    )
    pico_enable_stdio_usb(tests 1)
    pico_enable_stdio_uart(tests 0)
    pico_add_extra_outputs(tests)
endif()
