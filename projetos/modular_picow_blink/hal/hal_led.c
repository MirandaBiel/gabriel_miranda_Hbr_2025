#include "hal_led.h"
#include "led_embutido.h"
#include "pico/stdlib.h"

void hal_led_init() {
    if (led_embutido_init() != 0) {
        // Se necessário, pode adicionar tratamento de erro
        while (1);
    }
}

void hal_led_toggle() {
    static bool led_on = false;

    if (led_on) {
        led_embutido_off();
    } else {
        led_embutido_on();
    }
    led_on = !led_on;
}