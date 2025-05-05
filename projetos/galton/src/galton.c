// Inclusão de bibliotecas padrão e específicas do ambiente Pico e display OLED
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

// Define o tamanho da matriz de coordenadas e parâmetros de simulação
#define ROWS 57
#define COLS 13
#define MAX_OBJECTS 40
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_C 22

// Pinos I2C
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Variáveis globais de controle
int launched_count = 0;             // Contador de bolas lançadas
float fall_prob = 0.5;              // Probabilidade de cair para a esquerda (entre 0.0 e 1.0)
bool simulation_done = false;       // Flag para saber se terminou
bool step = true;                   // Alternância entre "step" e "transition"

// Fonte dos dígitos para exibição numérica
const uint8_t digit_font[10][5] = {
    {0x1F, 0x11, 0x11, 0x11, 0x1F}, // 0
    {0x00, 0x00, 0x1F, 0x00, 0x00}, // 1
    {0x1D, 0x15, 0x15, 0x15, 0x17}, // 2
    {0x11, 0x15, 0x15, 0x15, 0x1F}, // 3
    {0x07, 0x04, 0x04, 0x04, 0x1F}, // 4
    {0x17, 0x15, 0x15, 0x15, 0x1D}, // 5
    {0x1F, 0x15, 0x15, 0x15, 0x1D}, // 6
    {0x01, 0x01, 0x01, 0x01, 0x1F}, // 7
    {0x1F, 0x15, 0x15, 0x15, 0x1F}, // 8
    {0x17, 0x15, 0x15, 0x15, 0x1F}, // 9
};

// Estrutura para representar uma coordenada com posição física e tipo de forma
typedef struct {
    int x;
    int y;
    int shape; // 0 = bola, 1 = quadrado com cantos vazios
} Coord;

// Matriz de coordenadas usada para mapear a posição gráfica dos elementos
Coord coord_matrix[ROWS][COLS];

// Estrutura que representa cada bola na simulação
typedef struct {
    int i, j;             // Posição na matriz
    bool active;          // Se está em movimento
    bool released;        // Se já foi lançada
} FallingObject;

FallingObject objects[MAX_OBJECTS]; // Vetor com os objetos lançados
int cycle_counter = 0;              // Contador de ciclos para controle de tempo

// Inicializa o vetor de objetos (nenhum lançado ou ativo)
void init_objects() {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        objects[i].released = false;
        objects[i].active = false;
    }
}

// Matriz que controla as posições já ocupadas pelas bolas ao final do movimento
bool occupied[ROWS][COLS] = {false};

// Inicializa a matriz de coordenadas com posição e tipo de forma (desalinhamento em linhas ímpares)
void init_coord_matrix() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            Coord c;
            if (i % 2 == 0) {
                c.x = 2 + j * 5;
                c.y = 3 + i * 2;
                c.shape = 0;
            } else {
                Coord above = coord_matrix[i - 1][j];
                c.x = above.x + 1;
                c.y = above.y + 1;
                c.shape = 1;
            }
            coord_matrix[i][j] = c;
        }
    }
}

// Função auxiliar para desenhar pixels com rotação
void set_pixel_rotated(uint8_t *ssd, int x, int y, bool set) {
    int phys_x = ssd1306_width - 1 - y;
    int phys_y = x;
    if (phys_x >= 0 && phys_x < ssd1306_width && phys_y >= 0 && phys_y < ssd1306_height) {
        ssd1306_set_pixel(ssd, phys_x, phys_y, set);
    }
}

// Desenha uma bolinha com raio determinado na tela
void draw_ball(uint8_t *ssd, int x0, int y0, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                set_pixel_rotated(ssd, x0 + x, y0 + y, true);
            }
        }
    }
}

// Limpa a bolinha desenhada
void clear_ball(uint8_t *ssd, int x0, int y0, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                set_pixel_rotated(ssd, x0 + x, y0 + y, false);
            }
        }
    }
}

// Desenha quadrado com cantos vazios (representa obstáculos da pirâmide)
void draw_hollow_corner_square(uint8_t *ssd, int x0, int y0) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            bool is_corner = (x == 0 && y == 0) || (x == 3 && y == 0) ||
                             (x == 0 && y == 3) || (x == 3 && y == 3);
            if (!is_corner) {
                set_pixel_rotated(ssd, x0 + x, y0 + y, true);
            }
        }
    }
}

// Limpa o quadrado com cantos vazios
void clear_hollow_corner_square(uint8_t *ssd, int x0, int y0) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            bool is_corner = (x == 0 && y == 0) || (x == 3 && y == 0) ||
                             (x == 0 && y == 3) || (x == 3 && y == 3);
            if (!is_corner) {
                set_pixel_rotated(ssd, x0 + x, y0 + y, false);
            }
        }
    }
}

// Desenha o elemento correspondente (bola ou obstáculo) da matriz
void draw_matrix_element(uint8_t *ssd, struct render_area *frame_area, int i, int j) {
    if (i < 0 || i >= ROWS || j < 0 || j >= COLS) return;
    Coord c = coord_matrix[i][j];
    if (c.shape == 0) draw_ball(ssd, c.x, c.y, 2);
    else if (c.shape == 1) draw_hollow_corner_square(ssd, c.x, c.y);
}

// Limpa o elemento da tela
void clear_matrix_element(uint8_t *ssd, struct render_area *frame_area, int i, int j) {
    if (i < 0 || i >= ROWS || j < 0 || j >= COLS) return;

    Coord c = coord_matrix[i][j];

    if (c.shape == 0) {
        clear_ball(ssd, c.x, c.y, 2);
    } else if (c.shape == 1) {
        clear_hollow_corner_square(ssd, c.x, c.y);
    }

    //render_on_display(ssd, frame_area);
}

// Desenha a pirâmide central com obstáculos
void draw_pyramid(uint8_t *ssd, struct render_area *frame_area) {
    int pyramid_height = 6; // número de linhas da pirâmide
    int base_y = 4;         // y inicial
    int x_center = COLS / 2; // centro da pirâmide na matriz

    for (int i = 0; i < pyramid_height; i++) {
        int y = base_y + i * 2;
        for (int dx = -i; dx <= i; dx += 2) {
            int x = x_center + dx;
            if (x >= 0 && x < COLS && y < ROWS) {
                draw_matrix_element(ssd, frame_area, y, x);
                occupied[y][x] = true;
            }
        }
    }
}

// Desenha um único dígito numérico
void draw_digit(uint8_t *ssd, int x, int y, int digit) {
    if (digit < 0 || digit > 9) return;

    for (int col = 0; col < 5; col++) {
        uint8_t line = digit_font[digit][col];
        for (int row = 0; row < 5; row++) {
            // Inverte a linha: espelha verticalmente
            bool set = (line >> row) & 0x01;
            set_pixel_rotated(ssd, x + col, y + row, set);
        }
    }
}

// Exibe a probabilidade atual no canto superior
void draw_probability_display(uint8_t *ssd, float prob) {
    // Converte para inteiro entre 0–99
    int percent = (int)(prob * 100.0f);
    if (percent < 0) percent = 0;
    if (percent > 99) percent = 99;

    int tens = percent / 10;
    int units = percent % 10;

    // canto superior esquerdo (inverso por rotação)
    int x_tens = 53;
    int y_tens = 2;
    int x_units = 59;
    int y_units = 2;

    draw_digit(ssd, x_tens, y_tens, tens);
    draw_digit(ssd, x_units, y_units, units);
}

// Mostra o número de bolas lançadas
void draw_ball_counter(uint8_t *ssd, int count) {
    int tens = (count / 10) % 10;
    int units = count % 10;

    // canto superior direito rotacionado (posição invertida)
    // x = linhas, y = colunas
    int x_tens = 2;
    int y_tens = 2;
    int x_units = 8;
    int y_units = 2;

    draw_digit(ssd, x_tens, y_tens, tens);
    draw_digit(ssd, x_units, y_units, units);
}

// Lança uma nova bola no centro superior
void launch_object(uint8_t *ssd, struct render_area *frame_area) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].released) {
            objects[i].i = 2;              // linha
            objects[i].j = 6;              // coluna
            objects[i].active = true;
            objects[i].released = true;

            draw_matrix_element(ssd, frame_area, objects[i].i, objects[i].j);
            launched_count++;
            draw_ball_counter(ssd, launched_count);
            render_on_display(ssd, frame_area);
            break;
        }
    }
}

// Atualiza o movimento das bolas, dependendo do modo (step ou transition)
void update_objects(uint8_t *ssd, struct render_area *frame_area, bool step) {
    // Momento de decisão
    if (step) {
        for (int i = 0; i < MAX_OBJECTS; i++) {
            // Pulo objetos não lançados ou não ativos
            if (!objects[i].active || !objects[i].released) continue;

            int ci = objects[i].i;
            int cj = objects[i].j;

            // Se o objeto já saiu da pirâmide
            if(ci >= 14){
                if(ci + 2 > ROWS - 1){ // Atingiu a base

                    occupied[ci][cj] = true;
                    objects[i].active = false;

                } else if(occupied[ci + 2][cj]){ // Atingiu outro objeto

                    occupied[ci][cj] = true;
                    objects[i].active = false;

                }else{ // Tem espaço para cair mais

                    objects[i].i += 2;
                }
            }else{ // Se o objeto está na pirâmide

                int direction = ((float)rand() / RAND_MAX) < fall_prob ? 0 : 1;

                if (direction == 0 && cj > 0 && ci + 2 < ROWS) {
                    // esquerda
                    objects[i].i += 1;
                    objects[i].j -= 1;
                } else if (cj < COLS && ci + 2 < ROWS) {
                    // direita
                    objects[i].i += 1;
                } else {
                    // trava se não puder ir
                    objects[i].active = false;
                }
                
            }

            clear_matrix_element(ssd, frame_area, ci, cj);
            draw_matrix_element(ssd, frame_area, objects[i].i, objects[i].j);
        }

    }else{ // Momento de transição

        for (int i = 0; i < MAX_OBJECTS; i++) {
            // Pulo objetos não lançados ou não ativos
            if (!objects[i].active || !objects[i].released) continue;

            int ci = objects[i].i;
            int cj = objects[i].j;

            // Se o objeto já saiu da pirâmide
            if(ci >= 14){

                if(ci + 2 > ROWS - 1){ // Atingiu a base

                    occupied[ci][cj] = true;
                    objects[i].active = false;

                } else if(occupied[ci + 2][cj]){ // Atingiu outro objeto

                    occupied[ci][cj] = true;
                    objects[i].active = false;

                }else{ // Tem espaço para cair mais

                    objects[i].i += 2;
                }

            }else{ // Se o objeto está na pirâmide

                if (!occupied[ci+1][cj]) {
                    // esquerda
                    objects[i].i += 1;
                } else {
                    // direita
                    objects[i].i += 1;
                    objects[i].j += 1;
                }
                
            }
            clear_matrix_element(ssd, frame_area, ci, cj);
            draw_matrix_element(ssd, frame_area, objects[i].i, objects[i].j);
        }

    }

    render_on_display(ssd, frame_area);
}

// Verifica se todas as bolas pararam
bool all_objects_inactive() {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].active || !objects[i].released) return false;
    }
    return true;
}

// Reinicia a simulação após encerramento
void reset_simulation(uint8_t *ssd, struct render_area *frame_area) {
    launched_count = 0;
    memset(ssd, 0, ssd1306_buffer_length); // limpa display

    // Limpa estado interno
    memset(occupied, 0, sizeof(occupied));
    init_objects();
    init_coord_matrix();

    // Redesenha tudo
    draw_pyramid(ssd, frame_area);
    draw_probability_display(ssd, fall_prob);
    draw_ball_counter(ssd, launched_count);
    render_on_display(ssd, frame_area);

    simulation_done = false;
    cycle_counter = 0;
    step = true;
}

// Ajusta a probabilidade usando os botões A e B, finaliza com C
void adjust_fall_prob(uint8_t *ssd, struct render_area *frame_area) {
    while (true) {
        bool button_a = !gpio_get(BUTTON_A); // pressionado = LOW
        bool button_b = !gpio_get(BUTTON_B);
        bool button_c = !gpio_get(BUTTON_C);

        if (button_a) {
            fall_prob -= 0.05f;
            if (fall_prob < 0.0f) fall_prob = 0.0f;
            draw_probability_display(ssd, fall_prob);
            render_on_display(ssd, frame_area);
            sleep_ms(200); // debounce
        }

        if (button_b) {
            fall_prob += 0.05f;
            if (fall_prob > 1.0f) fall_prob = 1.0f;
            draw_probability_display(ssd, fall_prob);
            render_on_display(ssd, frame_area);
            sleep_ms(200); // debounce
        }

        if (button_c) {
            break; // Sai do ajuste
        }

        sleep_ms(10); // pequena espera para evitar polling agressivo
    }
}

// Função principal
int main() {
    stdio_init_all();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUTTON_C);
    gpio_set_dir(BUTTON_C, GPIO_IN);
    gpio_pull_up(BUTTON_C);

    ssd1306_init();

    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    sleep_ms(1000);

    init_coord_matrix();

    draw_pyramid(ssd, &frame_area);
    draw_probability_display(ssd, fall_prob);
    render_on_display(ssd, &frame_area);

    init_objects();
    srand(to_us_since_boot(get_absolute_time())); // semente para rand()

    while (true) {
        if (!simulation_done) {
            if (cycle_counter % 4 == 0) {
                launch_object(ssd, &frame_area);
            }

            update_objects(ssd, &frame_area, step);
            step = !step;
            cycle_counter++;

            // Verifica se acabou
            if (all_objects_inactive()) {
                simulation_done = true;
            }

        } else {
            adjust_fall_prob(ssd, &frame_area);
            reset_simulation(ssd, &frame_area);
        }
        
        //sleep_ms(100); // para controlar a velocidade
    }

    return 0;
}
