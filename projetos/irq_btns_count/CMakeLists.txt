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
# Arquivo de projeto gerado para o Raspberry Pi Pico usando CMake

# Versão mínima do CMake requerida
cmake_minimum_required(VERSION 3.13)

# Definição dos padrões de linguagem C e C++
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Gera um arquivo compile_commands.json (útil para análise de código e IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicialização do pico_sdk a partir do local onde foi instalado
# (isso pode vir de variáveis de ambiente, cache do CMake, etc.)

# == NÃO MODIFIQUE AS LINHAS A SEGUIR se estiver usando a extensão do VS Code para o Raspberry Pi Pico ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})  # Caminho da pasta do usuário no Windows
else()
    set(USERHOME $ENV{HOME})         # Caminho da pasta do usuário em Unix/Linux
endif()

# Versões padrão do SDK, toolchain e utilitários
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)

# Caminho para o CMake personalizado usado pela extensão do VS Code
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

# Se o arquivo da extensão existir, inclui ele no projeto
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Define a placa que está sendo utilizada (pico_w = Raspberry Pi Pico W)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Importa o SDK da Raspberry Pi Pico
# (deve ser feito antes de definir o projeto)
include(pico_sdk_import.cmake)

# Define o nome do projeto e as linguagens usadas (C, C++, ASM)
project(irq_btns_count C CXX ASM)

# Inicializa o SDK da Raspberry Pi Pico
pico_sdk_init()

# Adiciona o executável ao projeto
# Os arquivos de código-fonte usados estão listados aqui
add_executable(irq_btns_count irq_btns_count.c inc/ssd1306_i2c.c)

# Define o nome do programa e sua versão
pico_set_program_name(irq_btns_count "irq_btns_count")
pico_set_program_version(irq_btns_count "0.1")

# Habilita/desabilita a saída padrão (stdio) via UART e USB
# (0 = desabilita, 1 = habilita)
pico_enable_stdio_uart(irq_btns_count 0)
pico_enable_stdio_usb(irq_btns_count 1)

# Adiciona a biblioteca padrão ao projeto
target_link_libraries(irq_btns_count
        pico_stdlib)

# Adiciona os diretórios de include padrão do projeto
target_include_directories(irq_btns_count PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Adiciona bibliotecas adicionais solicitadas pelo usuário
# Neste caso, a biblioteca de I2C do hardware (usada no OLED)
target_link_libraries(irq_btns_count 
        hardware_i2c
        )

# Gera arquivos extras de saída (como .uf2 e .bin)
pico_add_extra_outputs(irq_btns_count)
