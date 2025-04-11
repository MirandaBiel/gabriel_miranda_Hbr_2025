#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "inc/ssd1306.h"

#define BUTTON_A 5
#define BUTTON_B 6

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

// Variáveis globais
volatile int counter = 0;
volatile int n_clicks = 0;
volatile int debounce_count = 0;
volatile bool counting = false;
volatile bool a_pressed = false;

static int last_counter = -1;
static int last_clicks = -1;
static int last_debounce_count = -1;
static absolute_time_t last_b_press = {0};

uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;

repeating_timer_t countdown_timer;
repeating_timer_t display_timer;

// Protótipos
bool countdown_callback(repeating_timer_t *rt);
bool display_callback(repeating_timer_t *rt);
void gpio_callback(uint gpio, uint32_t events);
void update_display();
void ssd1306_clear();
void ssd1306_show();

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Sistema iniciado...\n");

    // GPIO botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    // I2C e OLED
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(I2C_PORT, OLED_ADDR, 128, 64);
    frame_area = (struct render_area){0, ssd1306_width - 1, 0, ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&frame_area);
    ssd1306_clear();

    // Timer de display (300ms)
    add_repeating_timer_ms(-300, display_callback, NULL, &display_timer);

    // Prioridade de interrupções
    irq_set_priority(IO_IRQ_BANK0, 0);    // GPIO (mais alta)
    irq_set_priority(TIMER_IRQ_0, 1);     // Timer (mais baixa)

    while (1) {
        if (a_pressed) {
            a_pressed = false;
            counter = 9;
            n_clicks = 0;
            debounce_count = 0;
            counting = true;
            last_b_press = get_absolute_time();

            add_repeating_timer_ms(-1000, countdown_callback, NULL, &countdown_timer);
        }

        tight_loop_contents();  // Loop principal sem bloqueio
    }

    return 0;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A && !counting) {
        a_pressed = true;
    } else if (gpio == BUTTON_B && counting) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_b_press, now) > 200000) {  // 200ms debounce
            n_clicks++;
            last_b_press = now;
        } else {
            debounce_count++;
        }
    }
}

bool countdown_callback(repeating_timer_t *rt) {
    if (counting && counter > 0) {
        counter--;
        if (counter == 0) {
            counting = false;
            cancel_repeating_timer(rt);
        }
    }
    return counting;
}

bool display_callback(repeating_timer_t *rt) {
    update_display();
    return true;
}

void update_display() {
    if (counter != last_counter || n_clicks != last_clicks || debounce_count != last_debounce_count) {
        char buffer[32];
        ssd1306_clear();

        sprintf(buffer, "Contador: %d", counter);
        ssd1306_draw_string(ssd, 0, 0, buffer);

        sprintf(buffer, "Clicks: %d", n_clicks);
        ssd1306_draw_string(ssd, 0, 16, buffer);

        sprintf(buffer, "Ignorados: %d", debounce_count);
        ssd1306_draw_string(ssd, 0, 32, buffer);

        ssd1306_show();

        last_counter = counter;
        last_clicks = n_clicks;
        last_debounce_count = debounce_count;
    }
}

void ssd1306_clear() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void ssd1306_show() {
    render_on_display(ssd, &frame_area);
}
