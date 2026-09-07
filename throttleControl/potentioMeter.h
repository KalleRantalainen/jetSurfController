#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t gpio;
    adc_unit_t unitId;
    adc_channel_t channel;
    adc_oneshot_unit_handle_t handle;
    bool initialized;
} Potentiometer;

// Initialize a potenitometer to a AIN capable pin
void potentiometerInit(Potentiometer *pot, gpio_num_t gpio);
// Read the raw input value 0...4095
int potentiometerReadRaw(Potentiometer *pot);
// Read the scaled potentiometer input 0...1000
int potentiometerReadThrottle(Potentiometer *pot);

#endif // POTENTIOMETER_H_