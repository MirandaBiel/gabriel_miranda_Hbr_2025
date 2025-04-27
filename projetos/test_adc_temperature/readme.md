# 📚 `test_adc_temperature`

Projeto de testes para conversão de ADC em temperatura utilizando Raspberry Pi Pico W + Unity para testes unitários.

---

## 🚀 Objetivo

- Desenvolver e testar a função de conversão de valores de ADC para temperatura em °C.
- Utilizar **Unity** (framework de testes) para validar o código.
- Permitir **rodar testes no PC** (Windows/Linux) e **firmware na Pico** separadamente.

---

## 📂 Organização do Projeto

```text
/
├── src/                # Código fonte principal
├── inc/                # Headers (.h) do projeto
├── tests/              # Testes unitários usando Unity
├── lib/unity/          # Biblioteca Unity (clonada automaticamente)
├── build/              # Build para a Pico (gera .uf2)
├── build-host/         # Build para o PC (gera tests.exe)
├── CMakeLists.txt      # Configurações de build
└── README.md           # Este arquivo
```

---

## 🛠️ Compilação

### 🔹 Compilar para Raspberry Pi Pico (firmware `.uf2`)

1. Abra um terminal no projeto.
2. Crie uma pasta de build:
   ```bash
   mkdir build
   cd build
   ```
3. Rode o CMake normalmente:
   ```bash
   cmake ..
   ```
4. Compile:
   ```bash
   cmake --build .
   ```
5. Resultado: O arquivo `.uf2` será gerado para gravar na Pico.

---

### 🔹 Compilar testes para o PC (gerar `tests.exe`)

1. **Importante:** Apague a pasta `build-host/` antiga, se existir:
   ```powershell
   Remove-Item -Recurse -Force .\build-host\
   ```
2. Crie a nova pasta:
   ```powershell
   mkdir build-host
   cd build-host
   ```
3. Rode o CMake com MinGW Makefiles:
   ```powershell
   cmake .. -G "MinGW Makefiles"
   ```
4. Compile os testes:
   ```powershell
   cmake --build . --target tests
   ```
5. Execute:
   ```powershell
   .\tests.exe
   ```

**Obs.:** Certifique-se de ter o **MinGW** instalado e no `PATH`.

---

## 🔧 Requisitos

- CMake ≥ 3.13
- Raspberry Pi Pico SDK (para build firmware)
- Compilador ARM (`arm-none-eabi-gcc`) para Pico
- MinGW (`gcc`) para build no Windows

---

## 📦 Bibliotecas utilizadas

- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity) – Biblioteca C para testes unitários.

---

## ⚡ Notas importantes

- Separar builds (`build/` para Pico, `build-host/` para PC) evita conflitos de toolchain.
- Testes no PC são mais rápidos e práticos para validar lógica antes de carregar na placa.
- O projeto clona automaticamente o Unity se não encontrar a pasta `lib/unity/`.

---

## 🧠 Autor

- **Seu Nome** – [Perfil GitHub ou LinkedIn se quiser colocar]

---

# 🛠️ TODOs futuros

- [ ] Automatizar o clone da Unity apenas na build de testes.
- [ ] Adicionar testes de integração para comunicação I2C.
- [ ] Criar presets de CMake para facilitar builds (`CMakePresets.json`).

---

# 📸 Imagem ilustrativa (opcional)

> *(Aqui você poderia colocar, se quiser depois, um fluxograma ou esquema do projeto!)*

---


