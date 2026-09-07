#include <stdio.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bleSlave.h"
#include "throttleControl_app.h"

// The BLE connection interval is six 1.25 ms units, or 7.5 ms.
static const int APPLICATION_CYCLE_US = 7500;

/**
 * Read the controller input and publish one throttle byte over BLE.
 */
static void applicationTimerCallback(void *arg)
{
	(void)arg;
	throttleControl_appCyclicEntryPoint();
}

/**
 * Initialize the BLE peripheral and potentiometer application.
 */
static void applicationInit(void)
{
	bleSlave_init();
	throttleControl_appInitAll();
}

/**
 * Start the controller application and keep the main task alive.
 */
void app_main(void)
{
	applicationInit();

	const esp_timer_create_args_t timerArgs = {
		.callback = applicationTimerCallback,
		.name = "throttlePublisher"
	};
	esp_timer_handle_t applicationTimer;
	ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &applicationTimer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(applicationTimer, APPLICATION_CYCLE_US));

	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
