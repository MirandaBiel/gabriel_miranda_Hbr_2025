## ✅ **README.md – Projeto Galton Board Digital com Raspberry Pi Pico W**

### 📌 Descrição

Este projeto implementa uma simulação visual de uma **Galton Board (também conhecida como Quincunx)** em uma **Raspberry Pi Pico W** com display **OLED I2C SSD1306**, utilizando a biblioteca `ssd1306` da BitDogLab. O sistema simula o comportamento probabilístico de várias esferas caindo por uma pirâmide de obstáculos, exibindo graficamente o resultado e permitindo controle interativo da **probabilidade de queda para a esquerda**.

---

### 🧰 Requisitos

- Raspberry Pi Pico W  
- Display OLED 128x64 I2C (controlador SSD1306)  
- BitDogLab / Plataforma com botões A, B e C nos pinos GPIO 5, 6 e 22  
- Biblioteca `ssd1306` para o display (BitDogLab ou equivalente)  
- Compilador C para Pico (CMake + SDK)  
- Fonte de alimentação USB  

---

### 🔧 Instalação e Compilação

1. Clone este repositório e configure o SDK da Raspberry Pi Pico.
2. Compile usando CMake:

```bash
mkdir build && cd build
cmake ..
make
```

3. Grave o `.uf2` na placa Pico via modo de bootloader (pressionando BOOTSEL ao conectar via USB).

---

### 🎮 Controles

| Botão     | Função                                                                 |
|-----------|------------------------------------------------------------------------|
| A (GPIO 5) | Diminui a probabilidade de cair para a esquerda (-5%)                 |
| B (GPIO 6) | Aumenta a probabilidade de cair para a esquerda (+5%)                 |
| C (GPIO 22)| Confirma a probabilidade e reinicia a simulação                       |

---

### 📊 Funcionamento

- A simulação inicia com uma pirâmide de obstáculos no centro.
- A cada 4 ciclos, uma nova bola é lançada no centro do topo.
- Cada bola se move para a esquerda ou direita com base na probabilidade configurada.
- Quando todas as bolas finalizam seu trajeto, o usuário pode ajustar a probabilidade e reiniciar.

---

### 🧠 Lógica de Simulação

- As bolas alternam entre duas fases de movimento: **step** (descida) e **transition** (desvio).
- A posição de cada bola é atualizada em uma matriz coordenada `coord_matrix`.
- Bolas são desenhadas com `draw_ball`, obstáculos com `draw_hollow_corner_square`.

---

### 📷 Exibição no OLED

- As bolas e os obstáculos são representadas como círculos.
- O número de bolas lançadas (ajustável) e a probabilidade configurada são mostrados no canto superior do display.

---

### 📁 Estrutura de Arquivos

```
/src
  - main.c                  <- Código principal com a simulação
  - inc/ssd1306.h           <- Header da biblioteca OLED
  - inc/font_digits.h       <- Fonte numérica (customizada)
```

---

### 📃 Licença

MIT License © 2025 — Desenvolvido para fins acadêmicos.

---

## 📚 **Manual Técnico – Galton Board Digital**

### 1. **Objetivo do Sistema**

Simular uma Galton Board digital em um display OLED, permitindo visualização do comportamento probabilístico de múltiplas partículas caindo e interagindo com obstáculos fixos, com controle dinâmico da probabilidade de desvio.

---

### 2. **Arquitetura do Sistema**

- **Microcontrolador**: Raspberry Pi Pico W
- **Interface**: Display OLED SSD1306 (I2C)
- **Entradas**: Três botões físicos (GPIOs)
- **Saídas**: Gráfico no display OLED

---

### 3. **Componentes Principais**

| Módulo                  | Responsabilidade                                                    |
|------------------------|----------------------------------------------------------------------|
| `coord_matrix`         | Matriz de coordenadas físicas e lógicas da Galton Board             |
| `FallingObject`        | Estrutura que representa cada bola com estado ativo/inativo         |
| `ssd1306`              | Biblioteca de controle gráfico para display OLED                    |
| `digit_font`           | Representação visual de dígitos 0-9 no display                      |
| `occupied`             | Matriz booleana de ocupação de espaços após parada das bolas        |

---

### 4. **Fluxo de Execução**

1. Inicializa display, botões e estruturas de controle.
2. Desenha a pirâmide central de obstáculos.
3. A cada 4 ciclos (ajustável), uma bola é lançada no centro superior.
4. A bola percorre seu caminho, desviando com probabilidade `fall_prob`.
5. Quando todas as bolas terminam, usuário pode ajustar `fall_prob` e reiniciar.

---

### 5. **Tratamento de Movimento**

- O movimento alterna entre:
  - **Step**: deslocamento para baixo (linha `i+1`)
  - **Transition**: desvio para esquerda ou direita (`j±1`) com base em `fall_prob`
- A colisão com obstáculos ou bolas anteriores impede movimento adicional.

---

### 6. **Interface Gráfica no OLED**

- Cada elemento é desenhado em coordenadas físicas convertidas da matriz lógica.
- A exibição é rotacionada para melhor encaixe no display 128x64.
- O contador de bolas e probabilidade são exibidos como números grandes nos cantos.

---

### 7. **Parâmetros Ajustáveis**

| Parâmetro      | Intervalo   | Controle       |
|----------------|-------------|----------------|
| `fall_prob`    | 0.00 a 1.00 | Botões A e B   |
| `MAX_OBJECTS`  | até 40      | Constante C    |
| `COLS`, `ROWS` | layout      | Constantes     |

