#include <stdio.h>             // Biblioteca padrão para entrada/saída (printf, scanf, etc.)
#include <stdlib.h>            // Biblioteca padrão para funções utilitárias (malloc, free, rand, srand, etc.)
#include <string.h>            // Biblioteca para manipulação de strings (strcpy, strlen, etc.)
#include <math.h>              // Biblioteca para funções matemáticas (sqrt, cos, sin, etc.)
#include "pico/stdlib.h"       // Biblioteca padrão do SDK do Raspberry Pi Pico (inicialização, GPIO, etc.)
#include "hardware/adc.h"      // Biblioteca para uso do ADC (Conversor Analógico-Digital) no RP2040.
#include "hardware/dma.h"      // Biblioteca para uso do DMA (Acesso Direto à Memória) no RP2040.
#include "hardware/pwm.h"      // Biblioteca para controle do PWM (Modulação por Largura de Pulso) no RP2040.
#include "neopixel.c"          // Código de implementação para controle de LEDs NeoPixel.
#include "inc/ssd1306.h"       // Cabeçalho com funções e definições para o display SSD1306.
#include "hardware/i2c.h"      // Biblioteca para uso do protocolo I2C no RP2040.
#include "pico/time.h"         // Biblioteca para manipulação de tempo (delays, contadores, etc.) no Pico.
#include "hardware/uart.h"     // Biblioteca para utilização do protocolo UART (comunicação entre as 2 BitDogLabs)

#define MIC_CHANNEL 2                            // Define o canal do ADC que será usado para o microfone (canal 2).
#define MIC_PIN (26 + MIC_CHANNEL)               // Define o pino do microfone; os pinos analógicos começam no 26, somando o canal.
#define ADC_CLOCK 48000000                       // Define a frequência base do ADC (48 MHz) em Hertz.
#define ADC_DIV 24576.f                          // Define o divisor do clock do ADC para reduzir a frequência e atingir a taxa desejada.
#define SAMPLE_RATE (ADC_CLOCK / ADC_DIV)        // Calcula a taxa de amostragem real dividindo o clock base pelo divisor.
#define SAMPLES 4096                             // Define o número de amostras a serem coletadas (deve ser uma potência de 2, ideal para FFT).
#define LED_PIN 7                                // Define o pino de controle dos LEDs NeoPixel.
#define LED_COUNT 25                             // Define o número de LEDs NeoPixel que serão controlados.
#define BUZZER_A 21                              // Define o pino conectado ao buzzer A.
#define BUZZER_B 10                              // Define o pino conectado ao buzzer B.
#define NUM_NOTES (sizeof(notes) / sizeof(Note)) // Calcula o número de notas disponíveis com base no tamanho do array 'notes'.
#define I2C_SDA 14                               // Define o pino SDA para comunicação I2C.
#define I2C_SCL 15                               // Define o pino SCL para comunicação I2C.
#define TEXT_LINES 6                             // Define o número de linhas de texto que serão exibidas no display.
#define TEXT_LENGTH 16                           // Define o comprimento máximo de cada linha de texto.
#define BAR_WIDTH 40                             // Define a largura da abstração na forma de barra do eixo x do Joystick
#define ADC_MAX ((1 << 12) - 1)                  // Define o valor máximo do ADC para um conversor de 12 bits (2^12 - 1 = 4095).
#define JOYSTICK_BUTTON 22                       // Define o pino do botão do joystick.
#define A_BUTTON 5                               // Define o pino do botão A.
#define B_BUTTON 6                               // Define o pino do botão B.
#define UART_TX 0                                // Define o pino de transmissão (TX) da UART.
#define UART_RX 1                                // Define o pino de recepção (RX) da UART.
#define RED_PIN 13                               // Define o pino do LED vermelho.
#define GREEN_PIN 11                             // Define o pino do LED verde.
#define BLUE_PIN 12                              // Define o pino do LED azul.


// Variáveis globais
const float DIVISOR_CLK_PWM = 16.0;   // Divisor de clock para o PWM
uint16_t wrap_div_buzzer = 8;         // Valor do divisor de wrap do buzzer
int num_BitDogLabs = 2;               // Número de BitDoglabs conectadas
uint dma_channel;                     // Canal DMA
dma_channel_config dma_cfg;           // Configuração DMA
uint16_t adc_buffer[SAMPLES];         // Buffer de audio
char *note;                           // Nota musical
float diff;                           // Diferença entre a nota musical mais próxima e a nota detectada no microfone
float frequency;                      // Frequência predominante
char text[TEXT_LINES][TEXT_LENGTH];   // Variável de texto mostrada no display OLED
void print_draw_temp(int direction);  // Declara o protótipo de print_draw_temp (para usar antes de escrevê-la)
uint8_t idx1 = 0;                     // índice da nota 1 recebida por UART
uint8_t idx2 = 0;                     // índice da nota 2 recebida por UART
uint8_t idx3 = 0;                     // índice da nota 3 recebida por UART
uint8_t idx4 = 0;                     // índice da nota 4 recebida por UART
uint8_t correct_buzzer_uart_index = 0;// índice da nota correta recebida por UART

// Estrutura de dados

// Estrutura de número complexo
typedef struct {
    double real;  // Parte real
    double imag;  // Parte imaginária
} Complexo;

// Declaração de um vetor de números complexos para o cálculo da FFT
Complexo fft_in[SAMPLES]; 

// Estrutura de dados das notas musicais
typedef struct {
    const char *name; // Nomeclatura da nota
    float value;      // Valor associado (frequência ou wrap)
} Note;

// Estrutura de dados para o display OLED
struct render_area frame_area = {
    start_column : 0,                 // Coluna inicial
    end_column : ssd1306_width - 1,   // Coluna final
    start_page : 0,                   // Página inicial
    end_page : ssd1306_n_pages - 1    // Página final
};

// Declaração de um dicionário de notas, os valores representam a frequência em Hz associada a respectiva nota musical
Note notes[] = {
    {"C3", 130.81}, {"D3", 146.83}, {"E3", 164.81}, {"F3", 174.61},
    {"G3", 196.00}, {"A3", 220.00}, {"B3", 246.94},
    {"C4", 261.63}, {"D4", 293.66}, {"E4", 329.63}, {"F4", 349.23},
    {"G4", 392.00}, {"A4", 440.00}, {"B4", 493.88},
    {"C5", 523.25}, {"D5", 587.33}, {"E5", 659.25}, {"F5", 698.46},
    {"G5", 783.99}, {"A5", 880.00}, {"B5", 987.77}
};

// Declaração de um dicionário de notas, os valores representam o wrap que produz a frequência PWM correspondente a nota musical
Note note_wraps[] = {
    {"C3", 59700},
    {"D3", 53200}, {"E3", 47403}, {"F3", 44750}, {"G3", 39812}, {"A3", 35511},
    {"B3", 31650}, {"C4", 29868}, {"D4", 26600}, {"E4", 23712}, {"F4", 22380},
    {"G4", 19928}, {"A4", 17757}, {"B4", 15825}, {"C5", 14941}, {"D5", 13300},
    {"E5", 11855}, {"F5", 11195}, {"G5", 9971},  {"A5", 8885},  {"B5", 7910}
};

// Declaração das telas do menu mostradas no display OLED

// Tela 1: Boas-vindas ou informações gerais
char tela1[7][TEXT_LENGTH + 1] = {
    "GABRIEL MIRANDA",
    "08/02/2025",
    "",
    "<-         ->",
    "",
    "Unidade 7",
    "EmbarcaTech"
};

// Tela 2: Tela para seleção do afinador
char tela2[7][TEXT_LENGTH + 1] = {
    "",
    "",
    "",
    "<- Afinador   ",
    "",
    "",
    ""
};

// Tela 3: Tela para seleção do treinamento de ouvido
char tela0[7][TEXT_LENGTH + 1] = {
    "",
    "",
    "",
    "    Treinar ->",
    "Ouvido",
    "",
    ""
};


//                                 Funções associadas ao afinador:


/*
 * find_closest_note:
 *   Dada uma frequência medida, encontra a nota musical cuja frequência está mais próxima.
 *   Parâmetros:
 *     - freq: frequência medida pelo microfone.
 *     - note_name: ponteiro para armazenar o endereço do nome da nota encontrada.
 *     - freq_diff: ponteiro para armazenar a diferença (erro) entre a frequência medida e a nota ideal.
 */
void find_closest_note(float freq, char **note_name, float *freq_diff) {

    float min_diff = fabs(notes[0].value - freq);
    int closest_index = 0;

    for (int i = 1; i < NUM_NOTES; i++) {
        float diff = fabs(notes[i].value - freq);
        if (diff < min_diff) {
            min_diff = diff;
            closest_index = i;
        }
    }

    *note_name = (char *)notes[closest_index].name;
    *freq_diff = freq - notes[closest_index].value;
}

/*
 * setup_adc_mic:
 *   Configura o ADC e o DMA para a coleta de amostras do microfone.
 *   Inicializa o pino correspondente, configura o FIFO do ADC e prepara o canal DMA.
 */
void setup_adc_mic() {

    adc_gpio_init(MIC_PIN);                     // Inicializa o pino do microfone como entrada analógica
    adc_init();                                 // Inicializa o ADC
    adc_select_input(MIC_CHANNEL);              // Seleciona o canal do ADC para o microfone
    adc_fifo_setup(true, true, 1, false, false); // Configura o FIFO do ADC (habilita o FIFO, threshold, etc.)
    adc_set_clkdiv(ADC_DIV);                     // Define o divisor de clock do ADC para atingir a taxa desejada

    // Configuração do DMA para transferir os dados do ADC para o buffer
    dma_channel = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16); // Cada transferência é de 16 bits
    channel_config_set_read_increment(&dma_cfg, false);           // Não incrementa o ponteiro de leitura (FIFO fixo)
    channel_config_set_write_increment(&dma_cfg, true);           // Incrementa o ponteiro de escrita (preenchendo o buffer)
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);                  // Sincroniza as transferências com as requisições do ADC
}

/*
 * sample_mic:
 *   Coleta SAMPLES amostras do microfone usando ADC + DMA.
 *   O ADC é iniciado e o DMA transfere os dados para o buffer adc_buffer.
 */
void sample_mic() {
    adc_fifo_drain();                           // Limpa o FIFO para evitar dados antigos
    adc_run(false);                             // Garante que o ADC esteja parado antes de configurar
    dma_channel_configure(dma_channel, &dma_cfg, adc_buffer, &(adc_hw->fifo), SAMPLES, true);
    adc_run(true);                              // Inicia o ADC para começar a amostragem
    dma_channel_wait_for_finish_blocking(dma_channel); // Aguarda até que o DMA conclua a transferência
    adc_run(false);                             // Para o ADC após a coleta
}

/*
 * fft:
 *   Implementação recursiva da Transformada Rápida de Fourier (FFT).
 *   Parâmetros:
 *     - x: vetor de números complexos contendo os dados (entrada/saída).
 *     - n: número de elementos no vetor.
 */
void fft(Complexo *x, int n) {
    if (n <= 1) return;  // Caso base: FFT de um único elemento é o próprio elemento

    // Aloca os vetores para as partes pares e ímpares
    Complexo *par = (Complexo *)malloc(n / 2 * sizeof(Complexo));
    Complexo *impar = (Complexo *)malloc(n / 2 * sizeof(Complexo));

    // Separa os elementos pares e ímpares do vetor x
    for (int i = 0; i < n / 2; i++) {
        par[i] = x[i * 2];
        impar[i] = x[i * 2 + 1];
    }

    // Recursivamente aplica FFT para as partes pares e ímpares
    fft(par, n / 2);
    fft(impar, n / 2);

    // Combina os resultados utilizando os coeficientes complexos (raízes da unidade)
    for (int k = 0; k < n / 2; k++) {
        double teta = -2 * M_PI * k / n;
        Complexo t;
        t.real = cos(teta) * impar[k].real - sin(teta) * impar[k].imag;
        t.imag = sin(teta) * impar[k].real + cos(teta) * impar[k].imag;

        // Combina as partes para formar os coeficientes da FFT
        x[k].real = par[k].real + t.real;
        x[k].imag = par[k].imag + t.imag;
        x[k + n / 2].real = par[k].real - t.real;
        x[k + n / 2].imag = par[k].imag - t.imag;
    }

    // Libera a memória alocada para os vetores auxiliares
    free(par);
    free(impar);
}

/*
 * get_dominant_freq:
 *   Processa os dados amostrados (armazenados em adc_buffer) para determinar a frequência dominante.
 *   Utiliza a FFT para identificar o pico no domínio da frequência.
 *   Retorna a frequência dominante calculada.
 */
float get_dominant_freq() {
    // Preparação dos dados de entrada
    for (int i = 0; i < SAMPLES; i++) {
        fft_in[i].real = (double)adc_buffer[i] - 2048; // Centraliza os valores em torno de zero
        fft_in[i].imag = 0.0;
    }

    // Executa a FFT
    fft(fft_in, SAMPLES);

    // Determina a frequência dominante
    float max_magnitude = 0.0;
    int max_index = 0;
    for (int i = 1; i < SAMPLES / 2; i++) {
        float magnitude = sqrt(fft_in[i].real * fft_in[i].real + fft_in[i].imag * fft_in[i].imag);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    // Calcula a frequência correspondente
    float dominant_freq = (float)max_index * SAMPLE_RATE / SAMPLES;
    return dominant_freq;
}


//                                 Funções associadas ao treinamento de ouvido:


/*
 * setup_buzzers:
 *   Configura os pinos dos buzzers para a função PWM e define o divisor de clock para cada um.
 */
void setup_buzzers() {
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);          // Configura o pino para função PWM
    uint slice_num_a = pwm_gpio_to_slice_num(BUZZER_A);
    pwm_set_clkdiv(slice_num_a, DIVISOR_CLK_PWM);        // Define o divisor de clock para o PWM do buzzer A

    gpio_set_function(BUZZER_B, GPIO_FUNC_PWM);          // Configura o pino para função PWM
    uint slice_num_b = pwm_gpio_to_slice_num(BUZZER_B);
    pwm_set_clkdiv(slice_num_b, DIVISOR_CLK_PWM);        // Define o divisor de clock para o PWM do buzzer B
}

/*
 * play_note:
 *   Toca uma nota em um buzzer específico.
 *   Parâmetros:
 *     - pin: pino do buzzer a ser acionado.
 *     - wrap: valor de wrap que define a frequência (via PWM) da nota.
 */
void play_note(uint pin, uint16_t wrap)
{
  int slice = pwm_gpio_to_slice_num(pin);                // Obtém o slice PWM correspondente ao pino
  pwm_set_wrap(slice, wrap);                             // Define o valor de wrap para o PWM
  pwm_set_gpio_level(pin, wrap / wrap_div_buzzer);       // Ajusta o nível PWM com base no wrap (Duty Cycle)
  pwm_set_enabled(slice, true);                          // Habilita o PWM no slice correspondente
}

/*
 * play_rest:
 *   Silencia o buzzer desabilitando o PWM.
 *   Parâmetro:
 *     - pin: pino do buzzer que deve ser silenciado.
 */
void play_rest(uint pin)
{
  int slice = pwm_gpio_to_slice_num(pin);                // Obtém o slice PWM correspondente ao pino
  pwm_set_enabled(slice, false);                         // Desabilita o PWM, silenciando o buzzer
}

/*
 * get_user_guess:
 *   Aguarda até que o usuário pressione um dos botões (A ou B) para indicar seu palpite.
 *   Retorna 0 se o botão A for pressionado e 1 se o botão B for pressionado.
 */
int get_user_guess() {
    int guess = -1;
    while (guess == -1) {
        // Verifica se o botão A foi pressionado (botão ativo em nível baixo)
        if (!gpio_get(A_BUTTON)) {
            guess = 0;
            sleep_ms(300); // Delay para debounce
        }
        // Verifica se o botão B foi pressionado
        else if (!gpio_get(B_BUTTON)) {
            guess = 1;
            sleep_ms(300); // Delay para debounce
        }
    }
    return guess;
}

/*
 * evaluate_response:
 *   Processa o palpite do usuário comparando com o buzzer correto e atualiza o display e os LEDs.
 *   Parâmetros:
 *     - correctBuzzer: índice do buzzer que tocou a nota correta (0 ou 1).
 *     - userGuess: palpite do usuário (0 para botão A ou 1 para botão B).
 *     - ssd: buffer do display OLED.
 */
void evaluate_response(int correctBuzzer, int userGuess, uint8_t *ssd) {
    // Limpa o buffer do display
    memset(ssd, 0, ssd1306_buffer_length);

    // (Opcional) Impressão para depuração: mostra os valores de correctBuzzer e userGuess
    printf("correctBuzzer: %d \n userGuess: %d.\n", correctBuzzer, userGuess);

    // Mostra no display OLED se o usuário acertou ou não
    if (userGuess == correctBuzzer) {
        ssd1306_draw_string(ssd, 5, 0, "Acertou!");
    } else {
        ssd1306_draw_string(ssd, 5, 0, "Errou!");
    }
    render_on_display(ssd, &frame_area);

    // Exibe na matriz de LEDs um padrão de feedback visual
    if (userGuess == correctBuzzer) {
        // Se acertou, mostra padrão “verde”
        print_draw_temp(correctBuzzer + 4);
        print_draw_temp(correctBuzzer + 4);
        print_draw_temp(correctBuzzer + 4);
    } else {
        // Se errou, mostra padrão “vermelho”
        print_draw_temp(correctBuzzer);
        print_draw_temp(correctBuzzer);
        print_draw_temp(correctBuzzer);
    }
}

/*
 * get_user_guess_uart:
 *   Monitora continuamente a UART e os botões físicos (A e B).
 *   Retorna um valor quando recebe uma entrada válida:
 *     - Retorna 0 se a UART receber o caractere 'a'.
 *     - Retorna 1 se a UART receber o caractere 'b'.
 *     - Retorna 2 se o botão A for pressionado (e envia 'a' pela UART).
 *     - Retorna 3 se o botão B for pressionado (e envia 'b' pela UART).
 */
int get_user_guess_uart() {
    int guess = -1;
    
    while (guess == -1) {
        // Verifica se há dados disponíveis na UART
        if (uart_is_readable(uart0)) {
            char received = uart_getc(uart0); // Lê um caractere da UART
            if (received == 'a') {
                return 0; // Se recebeu 'a', retorna 0
            } else if (received == 'b') {
                return 1; // Se recebeu 'b', retorna 1
            }
        }

        // Verifica se o botão A foi pressionado (botão ativo em nível baixo)
        if (!gpio_get(A_BUTTON)) {
            guess = 2; // Define o palpite como 2 se o botão A foi pressionado
            uart_putc(uart0, 'a'); // Envia 'a' pela UART para sinalizar o evento
            //sleep_ms(300); // Opcional: delay para debounce do botão
        }
        // Verifica se o botão B foi pressionado
        else if (!gpio_get(B_BUTTON)) {
            guess = 3; // Define o palpite como 3 se o botão B foi pressionado
            uart_putc(uart0, 'b'); // Envia 'b' pela UART para sinalizar o evento
            //sleep_ms(300); // Opcional: delay para debounce do botão
        }
    }

    return guess; // Retorna o valor correspondente ao evento capturado
}


//                                 Funções associadas ao Joystick e botões:


/*
 * setup_adc_joystick:
 *   Configura o ADC para ler a posição do Joystick (utilizando um pino analógico).
 */
void setup_adc_joystick(){
    adc_init();               // Inicializa o ADC
    adc_gpio_init(27);        // Inicializa o pino associado ao Joystick
    adc_select_input(1);      // Seleciona o canal correspondente (neste caso, canal 1)
}

/*
 * setup_buttons:
 *   Inicializa e configura os pinos dos botões (Joystick, A e B) como entradas com resistor de pull-up.
 */
void setup_buttons(){

    // Botão do Joystick
    gpio_init(JOYSTICK_BUTTON);             
    gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN); 
    gpio_pull_up(JOYSTICK_BUTTON);

    // Botão A
    gpio_init(A_BUTTON);             
    gpio_set_dir(A_BUTTON, GPIO_IN); 
    gpio_pull_up(A_BUTTON);

    // Botão B
    gpio_init(B_BUTTON);             
    gpio_set_dir(B_BUTTON, GPIO_IN);
    gpio_pull_up(B_BUTTON);
}


//                                 Funções associadas a matriz de LEDs RBG:


/*
 * print_draw_temp:
 *   Exibe desenhos temporários (apagam ao final da função) na matriz de LEDs de acordo com o parâmetro 'direction'.
 *   Estes desenhos são usados para dar feedback visual (ex.: indicar palpite correto/errado ou chamar atenção).
 */
void print_draw_temp(int direction){
    switch (direction) {
        // Seta vermelha para esquerda (Botão A - BitDogLab 1 - Palpite errado)
        case 0:                       
            npSetLED(14, 100, 0, 0);
            npSetLED(13, 100, 0, 0);
            npSetLED(12, 100, 0, 0);
            npSetLED(11, 100, 0, 0);
            npSetLED(10, 100, 0, 0);
            npSetLED(16, 100, 0, 0);
            npSetLED(22, 100, 0, 0);
            npSetLED(6, 100, 0, 0);
            npSetLED(2, 100, 0, 0);
            sleep_ms(500);
            npWrite();
            sleep_ms(500);
            break; 
        // Seta vermelha para direita (Botão B - BitDogLab 1 - Palpite errado)   
        case 1:
            npSetLED(14, 100, 0, 0);
            npSetLED(13, 100, 0, 0);
            npSetLED(12, 100, 0, 0);
            npSetLED(11, 100, 0, 0);
            npSetLED(10, 100, 0, 0);
            npSetLED(8, 100, 0, 0);
            npSetLED(2, 100, 0, 0);
            npSetLED(18, 100, 0, 0);
            npSetLED(22, 100, 0, 0);
            sleep_ms(500);
            npWrite();
            sleep_ms(500);
            break;
        // Seta verde para esquerda (Botão A - BitDogLab 1 - Palpite certo)
        case 2:
            npSetLED(14, 0, 100, 0);
            npSetLED(13, 0, 100, 0);
            npSetLED(12, 0, 100, 0);
            npSetLED(11, 0, 100, 0);
            npSetLED(10, 0, 100, 0);
            npSetLED(16, 0, 100, 0);
            npSetLED(22, 0, 100, 0);
            npSetLED(6, 0, 100, 0);
            npSetLED(2, 0, 100, 0);
            sleep_ms(500);
            npWrite();
            sleep_ms(500);
            break;
        // Seta verde para direita (Botão B - BitDogLab 1 - Palpite certo)    
        case 3:
            npSetLED(14, 0, 100, 0);
            npSetLED(13, 0, 100, 0);
            npSetLED(12, 0, 100, 0);
            npSetLED(11, 0, 100, 0);
            npSetLED(10, 0, 100, 0);
            npSetLED(8, 0, 100, 0);
            npSetLED(2, 0, 100, 0);
            npSetLED(18, 0, 100, 0);
            npSetLED(22, 0, 100, 0);
            sleep_ms(500);
            npWrite();
            sleep_ms(500);
            break;
        // Chama atenção para começar a emitir o som nos buzzers
        case 4:
            // Fase 1
            npSetLED(12, 100, 100, 0);
            sleep_ms(50);
            npWrite();
            sleep_ms(100);

            // Fase 2
            npSetLED(12, 0, 0, 0);

            npSetLED(11, 100, 100, 0);
            npSetLED(13, 100, 100, 0);
            npSetLED(18, 100, 100, 0);
            npSetLED(17, 100, 100, 0);
            npSetLED(16, 100, 100, 0);
            npSetLED(8, 100, 100, 0);
            npSetLED(7, 100, 100, 0);
            npSetLED(6, 100, 100, 0);
            sleep_ms(50);
            npWrite();
            sleep_ms(100);

            // Fase 3
            npSetLED(11, 0, 0, 0);
            npSetLED(13, 0, 0, 0);
            npSetLED(18, 0, 0, 0);
            npSetLED(17, 0, 0, 0);
            npSetLED(16, 0, 0, 0);
            npSetLED(8, 0, 0, 0);
            npSetLED(7, 0, 0, 0);
            npSetLED(6, 0, 0, 0);

            npSetLED(0, 100, 100, 0);
            npSetLED(1, 100, 100, 0);
            npSetLED(2, 100, 100, 0);
            npSetLED(3, 100, 100, 0);
            npSetLED(4, 100, 100, 0);
            npSetLED(5, 100, 100, 0);
            npSetLED(14, 100, 100, 0);
            npSetLED(15, 100, 100, 0);
            npSetLED(24, 100, 100, 0);
            npSetLED(23, 100, 100, 0);
            npSetLED(22, 100, 100, 0);
            npSetLED(21, 100, 100, 0);
            npSetLED(20, 100, 100, 0);
            npSetLED(19, 100, 100, 0);
            npSetLED(10, 100, 100, 0);
            npSetLED(9, 100, 100, 0);

            sleep_ms(50);
            npWrite();
            sleep_ms(100);

            // Fase 4
            npSetLED(11, 100, 100, 0);
            npSetLED(13, 100, 100, 0);
            npSetLED(18, 100, 100, 0);
            npSetLED(17, 100, 100, 0);
            npSetLED(16, 100, 100, 0);
            npSetLED(8, 100, 100, 0);
            npSetLED(7, 100, 100, 0);
            npSetLED(6, 100, 100, 0);

            npSetLED(0, 0, 0, 0);
            npSetLED(1, 0, 0, 0);
            npSetLED(2, 0, 0, 0);
            npSetLED(3, 0, 0, 0);
            npSetLED(4, 0, 0, 0);
            npSetLED(5, 0, 0, 0);
            npSetLED(14, 0, 0, 0);
            npSetLED(15, 0, 0, 0);
            npSetLED(24, 0, 0, 0);
            npSetLED(23, 0, 0, 0);
            npSetLED(22, 0, 0, 0);
            npSetLED(21, 0, 0, 0);
            npSetLED(20, 0, 0, 0);
            npSetLED(19, 0, 0, 0);
            npSetLED(10, 0, 0, 0);
            npSetLED(9, 0, 0, 0);

            sleep_ms(50);
            npWrite();
            sleep_ms(100);

            // Fase 5
            npSetLED(12, 100, 100, 0);

            npSetLED(11, 0, 0, 0);
            npSetLED(13, 0, 0, 0);
            npSetLED(18, 0, 0, 0);
            npSetLED(17, 0, 0, 0);
            npSetLED(16, 0, 0, 0);
            npSetLED(8, 0, 0, 0);
            npSetLED(7, 0, 0, 0);
            npSetLED(6, 0, 0, 0);
            sleep_ms(50);
            npWrite();
            sleep_ms(100);

            break;
    }

    // Limpa a Matriz de LEDs ao final da exibição
    npClear();
    npWrite();
}

/*
 * print_draw_fix:
 *   Exibe desenhos “fixos” (permanecem após a função) na matriz de LEDs,
 *   geralmente para indicar qual o buzzer está com o som ativo.
 */
void print_draw_fix(int direction){
    npClear();
    switch (direction) {
        // Seta amarela para esquerda (Botão A tocando - BitDogLab 1)
        case 0:
            npSetLED(14, 100, 100, 0);
            npSetLED(13, 100, 100, 0);
            npSetLED(12, 100, 100, 0);
            npSetLED(11, 100, 100, 0);
            npSetLED(10, 100, 100, 0);
            npSetLED(16, 100, 100, 0);
            npSetLED(22, 100, 100, 0);
            npSetLED(6, 100, 100, 0);
            npSetLED(2, 100, 100, 0);
            sleep_ms(200);
            npWrite();
            break;
        // Seta amarela para direita (Botão B tocando - BitDogLab 1)
        case 1:
            npSetLED(14, 100, 100, 0);
            npSetLED(13, 100, 100, 0);
            npSetLED(12, 100, 100, 0);
            npSetLED(11, 100, 100, 0);
            npSetLED(10, 100, 100, 0);
            npSetLED(8, 100, 100, 0);
            npSetLED(2, 100, 100, 0);
            npSetLED(18, 100, 100, 0);
            npSetLED(22, 100, 100, 0);
            sleep_ms(200);
            npWrite();
            break;
    }
}

//                                 Funções associadas ao dispaly OLED:


/*
 * setup_oled:
 *   Inicializa a comunicação I2C e configura o display OLED.
 */
void setup_oled(){
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);   // Inicializa o I2C com a frequência definida
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();                             // Inicializa o display OLED
}

/*
 * center_text:
 *   Recebe uma string de origem e centraliza-a em uma string de destino com largura definida.
 *   Parâmetros:
 *     - dest: buffer de destino onde a string centralizada será armazenada.
 *     - src: string original a ser centralizada.
 *     - width: largura total para centralização.
 */
void center_text(char *dest, const char *src, size_t width) {
    size_t len = strlen(src);
    size_t padding = (width > len) ? (width - len) / 2 : 0;
    snprintf(dest, width + 1, "%*s%s%*s", (int)padding, "", src, (int)padding, "");
}

/*
 * update_texts_tuner:
 *   Atualiza o buffer de texto (array 'text') com as informações do afinador:
 *     - Frequência medida
 *     - Nota musical mais próxima
 *     - Erro (diferença em Hz)
 */
void update_texts_tuner(float frequency, const char *nota, float erro) {
    center_text(text[0], "", TEXT_LENGTH);
    
    char buffer[TEXT_LENGTH];
    snprintf(buffer, TEXT_LENGTH, "Freq: %.0f HZ", frequency);
    center_text(text[1], buffer, TEXT_LENGTH);
    
    center_text(text[2], "", TEXT_LENGTH);

    snprintf(buffer, TEXT_LENGTH, "Nota: %s", nota);
    center_text(text[3], buffer, TEXT_LENGTH);

    center_text(text[4], "", TEXT_LENGTH);

    snprintf(buffer, TEXT_LENGTH, "Erro: %.0f HZ", erro);
    center_text(text[5], buffer, TEXT_LENGTH);
}

/*
 * update_texts_training:
 *   Atualiza o buffer de texto com informações para o modo de treinamento de ouvido.
 *   Exibe a mensagem “ACERTE O BOTAO DA NOTA” e mostra a frequência e a nota a serem adivinhadas.
 */
void update_texts_training(float frequency, const char *nota) {
    
    center_text(text[0], "ACERTE O BOTAO", TEXT_LENGTH);

    center_text(text[1], "DA NOTA", TEXT_LENGTH);

    center_text(text[2], "", TEXT_LENGTH);

    char buffer[TEXT_LENGTH];
    snprintf(buffer, TEXT_LENGTH, "Nota: %s", nota);
    center_text(text[3], buffer, TEXT_LENGTH);

    center_text(text[4], "", TEXT_LENGTH);

    snprintf(buffer, TEXT_LENGTH, "Freq: %.0f HZ", frequency);
    center_text(text[5], buffer, TEXT_LENGTH);
}

/*
 * update_texts_uart:
 *   Atualiza o conteúdo das linhas de texto exibidas no display OLED, formatando valores recebidos via UART.
 *   O intuito dessa função é debugar o código utilizando o display OLED
 *   Parâmetros:
 *     - uart_wrap1: Valor do primeiro wrap recebido pela UART.
 *     - uart_wrap2: Valor do segundo wrap recebido pela UART.
 *     - correct_buzzer_uart: Índice do buzzer correto recebido pela UART.
 */
void update_texts_uart(int uart_wrap1, int uart_wrap2, int correct_buzzer_uart) {
    char buffer[TEXT_LENGTH]; // Buffer temporário para formatar as strings

    // Formata e exibe o primeiro wrap recebido pela UART
    snprintf(buffer, TEXT_LENGTH, "wrap1: %d", uart_wrap1);
    center_text(text[0], buffer, TEXT_LENGTH);

    // Linha vazia para melhor espaçamento visual no display
    center_text(text[1], "", TEXT_LENGTH);
    
    // Formata e exibe o segundo wrap recebido pela UART
    snprintf(buffer, TEXT_LENGTH, "wrap2: %d", uart_wrap2);
    center_text(text[2], buffer, TEXT_LENGTH);

    // Outra linha vazia para espaçamento
    center_text(text[3], "", TEXT_LENGTH);

    // Formata e exibe o índice do buzzer correto
    snprintf(buffer, TEXT_LENGTH, "cb: %d", correct_buzzer_uart);
    center_text(text[4], buffer, TEXT_LENGTH);

    // Última linha vazia para manter o alinhamento do display
    center_text(text[5], "", TEXT_LENGTH);
}

/*
 * display_texts:
 *   Exibe cada linha do buffer de texto no display OLED.
 */
void display_texts(uint8_t *ssd) {
    
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    
    int y = 0;
    for (uint i = 0; i < TEXT_LINES; i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);  // Desenha a string no display
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

/*
 * display_screen:
 *   Exibe uma tela pré-definida (ex.: tela1, tela2, tela0) no display OLED.
 *   Parâmetros:
 *     - screen: matriz de strings que contém as linhas da tela.
 *     - num_lines: número de linhas na tela.
 *     - ssd: buffer do display OLED.
 */
void display_screen(const char screen[][TEXT_LENGTH + 1], unsigned int num_lines, uint8_t *ssd) {
    
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    char line[TEXT_LENGTH + 1];
    int y = 0;
    for (unsigned int i = 0; i < num_lines; i++) {
        center_text(line, screen[i], TEXT_LENGTH);
        ssd1306_draw_string(ssd, 5, y, line);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}


//                                             Funções relacionadas à comunicação UART

/*
 * setup_uart:
 *   Inicializa a comunicação UART0 com um baud rate de 115200.
 *   Configura os pinos TX e RX e habilita o FIFO para melhorar a eficiência da transmissão.
 */
void setup_uart(){
    uart_init(uart0, 115200); // Inicializa a UART0 com baud rate de 115200
    gpio_set_function(UART_TX, GPIO_FUNC_UART); // Configura pino 0 como TX
    gpio_set_function(UART_RX, GPIO_FUNC_UART); // Configura pino 1 como RX
    
    // Habilita o FIFO para evitar sobrecarga de buffer
    uart_set_fifo_enabled(uart0, true);
}

/*
 * test_uart_connection_device1:
 *   Envia o caractere 's' via UART e espera, por até 500 ms, receber o mesmo caractere de volta.
 *   Se o caractere for recebido dentro do prazo, incrementa o contador success_count.
 *   Após cada teste, aguarda 200 ms antes de repetir o processo.
 */
void test_uart_connection_device1() {
    // Acende os LEDs para indicar que o teste está em execução.
    gpio_put(RED_PIN, 1);
    gpio_put(GREEN_PIN, 1);

    while (true) {
        // Envia o caractere 's' via UART.
        uart_putc(uart0, 's');

        // Define o prazo para aguardar 500 ms (500000 microsegundos).
        absolute_time_t deadline = delayed_by_us(get_absolute_time(), 500 * 1000);
        bool received = false;

        // Aguarda até que o prazo seja alcançado ou o caractere 's' seja recebido.
        while (!time_reached(deadline)) {
            if (uart_is_readable(uart0)) {
                char received_char = uart_getc(uart0);
                if (received_char == 's') {
                    received = true;
                    break;
                }
            }
            sleep_ms(1); // Pequena pausa para reduzir o uso da CPU.
        }

    }
}

/*
 * test_uart_connection_device2:
 *   Fica aguardando indefinidamente a chegada de um caractere pela UART.
 *   Quando o caractere recebido for 's', a função envia 's' de volta via UART
 *   e, em seguida, retorna (sai da função).
 */
void test_uart_connection_device2() {
    gpio_put(RED_PIN, 1);
    gpio_put(GREEN_PIN, 1);

    while (true) {
        // Verifica se há dados disponíveis na UART
        if (uart_is_readable(uart0)) {
            char received = uart_getc(uart0);
            // Se o caractere recebido for 's'
            if (received == 's') {
                // Envia 's' de volta via UART
                uart_putc(uart0, 's');
                // Sai da função
                gpio_put(RED_PIN, 0);
                return;
            }
        }
        // Pausa de 1 ms para reduzir o uso da CPU durante a espera
        sleep_ms(1);
    }
}

/*
 * setup_rbg:
 *   Inicializa os pinos dos LED RGB e os configura como saída.
 *   Define o estado inicial do LED como desligado (0).
 */
void setup_rbg() {
    // Inicializa os pinos dos LEDs RGB
    gpio_init(RED_PIN);
    gpio_init(GREEN_PIN);
    gpio_init(BLUE_PIN);

    // Configura os pinos como saída para controlar os LEDs
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    // Garante que os LEDs iniciem desligados
    gpio_put(RED_PIN, 0);   // LED vermelho desligado
    gpio_put(GREEN_PIN, 0); // LED verde desligado
    gpio_put(BLUE_PIN, 0);  // LED azul desligado
}

/*
 * uart_send_uint8_as_char:
 *   Envia um valor uint8_t (convertido para caractere) via UART.
 *
 * Parâmetros:
 *   - uart: Instância da UART (por exemplo, uart0).
 *   - value: Valor do tipo uint8_t a ser enviado.
 */
void uart_send_uint8_as_char(uart_inst_t *uart, uint8_t value) {
    uart_putc(uart, (char)value);
}

/*
 * uart_wait_for_char:
 *   Fica monitorando a UART indefinidamente até que um caractere seja recebido.
 *   Retorna o caractere recebido (do tipo uint8_t).
 */
uint8_t uart_wait_for_char(uart_inst_t *uart) {
    while (!uart_is_readable(uart)) {
        sleep_ms(1);  // Pequena pausa para reduzir o uso da CPU
    }
    return uart_getc(uart);
}

//                                             Função principal

int main() {
    // Inicialização seguida de uma espera
    stdio_init_all();
    sleep_ms(3000);

    // Inicialização da matriz de LEDs
    npInit(LED_PIN, LED_COUNT);

    // Inicialização do display OLED
    setup_oled();
    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    update_texts_uart(idx1, idx2, correct_buzzer_uart_index);
    display_texts(ssd);

    // Inicialização dos Botões
    setup_buttons();
    bool button_a_pressed;
    bool button_b_pressed;
    bool button_js_pressed;

    // Inicializa a interface de comunicação UART
    setup_uart();

    // Inicializa o LED RGB que irá indicar os status de conexão entre as BitDogLabs
    setup_rbg();

    // Configura os buzzers
    setup_buzzers();

    // Testa se há um dispositivo conectado
    test_uart_connection_device2();

    // Variável de controle para saber se a conexão entre as BitDogLabs será mantida ou não
    uint8_t repeat;

    // Loop de execução
    while (true) {

        // Recebe do primeiro dispositivo os índices correspondentes às notas e o número do buzzer com a nota correta
        uint8_t idx1 = uart_wait_for_char(uart0);
        uint8_t idx2 = uart_wait_for_char(uart0);
        uint8_t idx3 = uart_wait_for_char(uart0);
        uint8_t idx4 = uart_wait_for_char(uart0);
        uint8_t correct_buzzer_uart_index = uart_wait_for_char(uart0);

        // Obtém os valores de frequência e os nomes das notas escolhidas
        float freq1 = notes[idx1].value;
        float freq2 = notes[idx2].value;
        float freq3 = notes[idx3].value;
        float freq4 = notes[idx4].value;
        const char *note1 = notes[idx1].name;
        const char *note2 = notes[idx2].name;
        const char *note3 = notes[idx3].name;
        const char *note4 = notes[idx4].name;

        // Determina qual o valor de frequência e a nota deverão ser mostradas no display com base em correctBuzzer
        float displayedFreq;
        const char *displayedNote;
        switch (correct_buzzer_uart_index)
        {
        case 0:
            displayedFreq = freq2;
            displayedNote = note2;
            break;
        case 1:
            displayedFreq = freq1;
            displayedNote = note1;
            break;
        case 2:
            displayedFreq = freq4;
            displayedNote = note4;
            break;
        case 3:
            displayedFreq = freq3;
            displayedNote = note3;
            break;
        default:
            break;
        }
        
        // Busca os valores de wrap associados as notas
        int wrap3 = 0, wrap4 = 0;
        int num_wraps = sizeof(note_wraps) / sizeof(note_wraps[0]);
        for (int j = 0; j < num_wraps; j++) {
            if (strcmp(note_wraps[j].name, note3) == 0) {
                wrap3 = note_wraps[j].value;
            }
            if (strcmp(note_wraps[j].name, note4) == 0) {
                wrap4 = note_wraps[j].value;
            }
        }

        // Aguarda um sinal de sincronização do primeiro dispositivo
        uart_wait_for_char(uart0);

        // Exibe as informações no display OLED
        update_texts_training(displayedFreq, displayedNote);
        display_texts(ssd);
        sleep_ms(500);

        // Chama atenção para a exibição dos sons
        print_draw_temp(4);

        // Aguarda o primeiro dispositivo tocar os buzzers
        sleep_ms(3000);

        // Aguarda um sinal de sincronização do primeiro dispositivo
        uart_wait_for_char(uart0);

        // Toca a primeira nota no Buzzer A
        print_draw_fix(0);
        play_note(BUZZER_A, wrap4);
        sleep_ms(2000);
        play_rest(BUZZER_A);
        
        // Toca a segunda nota no Buzzer B
        print_draw_fix(1);
        play_note(BUZZER_B, wrap3);
        sleep_ms(2000);
        play_rest(BUZZER_B);

        // Limpa a matriz de LEDs
        npClear();
        npWrite();

        // Manda um caractere para a sincronização entre os 2 dispositivos
        uart_send_uint8_as_char(uart0, 's');

        // Obtém o palpite do usuário sobre qual buzzer tocou a nota certa
        int userGuess = get_user_guess_uart();

        // Apaga o conteúdo mostrado no display OLED
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, &frame_area);

        // Analisa e mostra se o usuário acertou ou não por meio do display OLED e da matriz de LEDs
        switch (correct_buzzer_uart_index)
        {
        case 0:
            sleep_ms(2000);
            uart_wait_for_char(uart0);
            break;
        case 1:
            sleep_ms(2000);
            uart_wait_for_char(uart0);
            break;
        case 2:
            if(userGuess == 2){
                ssd1306_draw_string(ssd, 5, 0, "Acertou");
                render_on_display(ssd, &frame_area);
                print_draw_temp(2);
                print_draw_temp(2);
                print_draw_temp(2);
                uart_send_uint8_as_char(uart0, 's');
            }else{
                ssd1306_draw_string(ssd, 5, 0, "Errou");
                render_on_display(ssd, &frame_area);
                print_draw_temp(0);
                print_draw_temp(0);
                print_draw_temp(0);
                uart_send_uint8_as_char(uart0, 's');
            }
            break;
        case 3:
            if(userGuess == 3){
                ssd1306_draw_string(ssd, 5, 0, "Acertou");
                render_on_display(ssd, &frame_area);
                print_draw_temp(3);
                print_draw_temp(3);
                print_draw_temp(3);
                uart_send_uint8_as_char(uart0, 's');
            }else{
                ssd1306_draw_string(ssd, 5, 0, "Errou");
                render_on_display(ssd, &frame_area);
                print_draw_temp(1);
                print_draw_temp(1);
                print_draw_temp(1);
                uart_send_uint8_as_char(uart0, 's');
            }
            break;
        default:
            break;
        }

        // Obtém um sinal do primeiro dispositivo que determina a continuidade da conexão UART
        repeat = uart_wait_for_char(uart0);
        if(!repeat){
            test_uart_connection_device2();
        }

    }
}
