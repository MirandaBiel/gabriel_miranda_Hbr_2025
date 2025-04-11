#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

// Pinos I2C do display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

// Buffer e área do display
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;

// Funções auxiliares
void ssd1306_clear();
void ssd1306_show();
void update_display(uint x_val, uint y_val);

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando leitura do joystick...\n");

    // Inicialização do ADC
    adc_init();
    adc_gpio_init(26);  // ADC0 = Y
    adc_gpio_init(27);  // ADC1 = X

    // Inicialização do I2C
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000); // 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display
    ssd1306_init(I2C_PORT, OLED_ADDR, 128, 64);
    frame_area = (struct render_area){
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    ssd1306_clear();

    while (1) {
        // Leitura do eixo Y (ADC0)
        adc_select_input(0);
        uint y_val = adc_read();

        // Leitura do eixo X (ADC1)
        adc_select_input(1);
        uint x_val = adc_read();

        // Atualiza o terminal e o display
        printf("X: %4d | Y: %4d\n", x_val, y_val);
        update_display(x_val, y_val);

        sleep_ms(200);
    }

    return 0;
}

void update_display(uint x_val, uint y_val) {
    char buffer[32];
    ssd1306_clear();

    sprintf(buffer, "Joystick X: %4d", x_val);
    ssd1306_draw_string(ssd, 0, 0, buffer);

    sprintf(buffer, "Joystick Y: %4d", y_val);
    ssd1306_draw_string(ssd, 0, 16, buffer);

    ssd1306_show();
}

void ssd1306_clear() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void ssd1306_show() {
    render_on_display(ssd, &frame_area);
}
