#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "adc_to_celsius.h"

// Pinos I2C do display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C
#define TEXT_LENGTH 16  

// Buffer e área do display
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;

// Funções auxiliares
void ssd1306_clear();
void ssd1306_show();
void update_display(float temperature_c);

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando leitura da temperatura interna...\n");

    // Inicialização do ADC
    adc_init();
    adc_set_temp_sensor_enabled(true); 
    adc_select_input(4); // Canal 4 = sensor interno

    // Inicialização do I2C
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
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
        uint16_t raw = adc_read();

        // Conversão usando a função nova
        float temperature_c = adc_to_celsius(raw);

        printf("Temperatura: %.2f °C\n", temperature_c);
        update_display(temperature_c);

        sleep_ms(500);
    }

    return 0;
}

void center_text(char *dest, const char *src, size_t width) {
    size_t len = strlen(src);
    size_t padding = (width > len) ? (width - len) / 2 : 0;
    snprintf(dest, width + 1, "%*s%s%*s", (int)padding, "", src, (int)padding, "");
}

void update_display(float temperature_c) {
    char buffer[TEXT_LENGTH];
    char line[TEXT_LENGTH + 1];

    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    center_text(line, "Temp Interna:", TEXT_LENGTH);
    ssd1306_draw_string(ssd, 5, 0, line);

    snprintf(buffer, TEXT_LENGTH, "%.2f C", temperature_c);
    center_text(line, buffer, TEXT_LENGTH);
    ssd1306_draw_string(ssd, 5, 16, line);

    render_on_display(ssd, &frame_area);
}

void ssd1306_clear() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void ssd1306_show() {
    render_on_display(ssd, &frame_area);
}
