# Projeto: Piscar LED da Raspberry Pi Pico W com Arquitetura Modular

Este projeto demonstra como estruturar de forma modular um programa simples para a **Raspberry Pi Pico W**: fazer o LED embutido piscar usando camadas de **Driver**, **HAL** e **App**.

---

## 📂 Estrutura de Pastas

```
projeto/
├── app/
│   └── main.c            # Lógica principal da aplicação
├── drivers/
│   └── led_embutido.c     # Controle direto do hardware (LED embutido)
├── hal/
│   └── hal_led.c          # Camada de abstração do LED
├── include/
│   ├── led_embutido.h     # Header do driver
│   └── hal_led.h          # Header da HAL
└── CMakeLists.txt         # Arquivo de build (compilação)
```

---

## 🛠️ Componentes do Projeto

- **drivers/led_embutido.c**  
  Implementa o controle direto do LED usando a API do Wi-Fi chip (`cyw43_arch`).
  
- **hal/hal_led.c**  
  Fornece funções de alto nível (`hal_led_init()`, `hal_led_toggle()`) para trabalhar com o LED de maneira independente do hardware.

- **app/main.c**  
  Controla o fluxo principal da aplicação, chamando funções da HAL.

- **include/**  
  Contém os headers (`.h`) que definem as funções públicas usadas entre módulos.

- **CMakeLists.txt**  
  Define como compilar o projeto, organiza includes e bibliotecas necessárias.

---

## 🔧 Como Compilar e Carregar

**Pré-requisitos:**
- Raspberry Pi Pico SDK instalado
- CMake 3.13+ instalado
- Compilador ARM (ex: arm-none-eabi-gcc)
- Ferramentas `ninja` ou `make`
- VSCode (opcional, mas recomendado)

---

### Passos para compilar:

1. Abra um terminal na raiz do projeto.
2. Crie a pasta de build:
   ```bash
   mkdir build
   cd build
   ```
3. Gere os arquivos de build:
   ```bash
   cmake ..
   ```
4. Compile o projeto:
   ```bash
   make
   ```
   ou
   ```bash
   ninja
   ```
5. O binário gerado (`modular_picow_blink.uf2`) estará na pasta `build/`.
6. Para carregar na Pico W:
   - Segure o botão BOOTSEL da placa, conecte via USB.
   - A Pico aparecerá como um disco removível.
   - Copie o `.uf2` para o dispositivo.

---

## ✨ O que o programa faz?

- Inicializa o LED embutido da Raspberry Pi Pico W.
- Pisca o LED: ligado 500 ms, desligado 500 ms, repetidamente.

---

## 📋 Observações importantes

- **HAL** = Hardware Abstraction Layer → facilita mudar o hardware no futuro sem mudar toda a aplicação.
- **Driver** acessa diretamente o hardware específico.
- **App** apenas usa funções da HAL, não se preocupa com o hardware real.

Essa separação é muito útil em projetos maiores e profissionais!

---

## 💬 Contato

Projeto feito para estudos de programação embarcada com Raspberry Pi Pico W.  
Dúvidas ou melhorias? Me procure! 🚀