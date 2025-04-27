#include "adc_to_celsius.h"

float adc_to_celsius(uint16_t adc_val) {
    float voltage = adc_val * 3.3f / 4095.0f;
    float temperature_c = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temperature_c;
}