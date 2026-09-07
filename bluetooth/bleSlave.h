#ifndef BLE_SLAVE_H_
#define BLE_SLAVE_H_

#include <stdbool.h>
#include <stdint.h>

// Name used by the jetSurf master while scanning for the controller.
#define BLE_SLAVE_DEVICE_NAME "jetSurfRadioController"

// Initialize the NimBLE peripheral and begin advertising.
void bleSlave_init(void);

// Notify the connected master of the latest one-byte throttle value.
void bleSlave_sendThrottle(uint8_t throttle);

// Return true when a master is subscribed to throttle notifications.
bool bleSlave_isConnected(void);

#endif // BLE_SLAVE_H_
