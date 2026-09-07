#include "throttleControl_app.h"

#include "bleSlave.h"
#include "driver/gpio.h"
#include "potentioMeter.h"

// Change this to the GPIO connected to the controller potentiometer wiper.
static const gpio_num_t THROTTLE_POTENTIOMETER_GPIO = GPIO_NUM_34;
static Potentiometer throttlePotentiometer;

/**
 * Read the potentiometer and send its scaled value to the BLE master.
 */
static void readAndSendThrottle(void)
{
    const int throttle = potentiometerReadThrottle(&throttlePotentiometer);
    const uint8_t throttleByte = 0; // Send 0 for now. //(uint8_t)((throttle * UINT8_MAX) / 1000);
    bleSlave_sendThrottle(throttleByte);
}

/**
 * Execute one controller application cycle.
 */
void throttleControl_appCyclicEntryPoint(void)
{
    readAndSendThrottle();
}

/**
 * Initialize the controller potentiometer.
 */
void throttleControl_appInitAll(void)
{
    potentiometerInit(&throttlePotentiometer, THROTTLE_POTENTIOMETER_GPIO);
}