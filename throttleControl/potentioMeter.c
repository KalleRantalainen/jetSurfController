#include "potentioMeter.h"

/**
 * Initialize a potentiometer to some AIN pin of ESP32
 * @param pot potentiometer instance
 * @param gpio some gpio pin capable of AIN
 */
void potentiometerInit(Potentiometer *pot, gpio_num_t gpio)
{
    if (pot == NULL) {
        return;
    }

    // Store the selected ADC pin and reset state before configuring the unit.
    pot->gpio = gpio;
    pot->handle = NULL;
    pot->initialized = false;

    // Create a one-shot ADC unit for the potentiometer measurement.
    // = take a single measurement upon request rather than
    //   sampling constantly.
    adc_oneshot_unit_init_cfg_t initConfig = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    if (adc_oneshot_new_unit(&initConfig, &pot->handle) != ESP_OK) {
        return;
    }

    // Map the GPIO pin to the ADC unit and channel used by the ESP32.
    adc_unit_t unit = ADC_UNIT_1;
    adc_channel_t channel = ADC_CHANNEL_0;
    if (adc_oneshot_io_to_channel(gpio, &unit, &channel) != ESP_OK) {
        adc_oneshot_del_unit(pot->handle);
        pot->handle = NULL;
        return;
    }

    // Save the resolved ADC unit and channel on the potentiometer instance.
    pot->unitId = unit;
    pot->channel = channel;

    // Configure the ADC channel with wide input range and 12-bit resolution.
    adc_oneshot_chan_cfg_t channelConfig = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    if (adc_oneshot_config_channel(pot->handle, pot->channel, &channelConfig) != ESP_OK) {
        adc_oneshot_del_unit(pot->handle);
        pot->handle = NULL;
        return;
    }

    // Mark the potentiometer as ready for reads.
    pot->initialized = true;
}

/**
 * Read the raw ADC value from the potentiometer input.
 * @param pot the potentiometer instance to read
 * @return int, the raw potentiometer value
 */
int potentiometerReadRaw(Potentiometer *pot)
{
    // Guard against uninitialized or invalid potentiometer instances.
    if (pot == NULL || !pot->initialized || pot->handle == NULL) {
        return 0;
    }

    // Read the ADC conversion result into a local variable.
    int rawValue = 0;
    if (adc_oneshot_read(pot->handle, pot->channel, &rawValue) != ESP_OK) {
        return 0;
    }

    // Return the full ADC reading from the potentiometer.
    return rawValue;
}

/**
 * Convert the raw ADC reading into a throttle value in the 0..1000 range.
 * @param pot the potentiometer instance to read
 * @return int, the scaled potentiometer value i.e. throttle input
 */
int potentiometerReadThrottle(Potentiometer *pot)
{
    // Fetch the raw ADC value and scale it to the throttle range.
    const int rawValue = potentiometerReadRaw(pot);
    return (rawValue * 1000) / 4095;
}