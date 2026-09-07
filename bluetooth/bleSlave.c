#include "bleSlave.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "bleSlave";

// These UUIDs must match the service and characteristic used by bleMaster.
static const ble_uuid128_t throttleServiceUuid = BLE_UUID128_INIT(
    0x2d, 0x55, 0xb1, 0x61, 0x4b, 0xf4, 0xef, 0x3f,
    0x22, 0x67, 0x6f, 0x41, 0x09, 0x57, 0x91, 0x8e);
static const ble_uuid128_t throttleCharacteristicUuid = BLE_UUID128_INIT(
    0x43, 0x55, 0xb1, 0x61, 0x4b, 0xf4, 0xef, 0x3f,
    0x22, 0x67, 0x6f, 0x41, 0x09, 0x57, 0x91, 0x87);

static uint16_t throttleValueHandle;
static uint16_t connectionHandle = BLE_HS_CONN_HANDLE_NONE;
static bool notificationsEnabled;

static void startAdvertising(void);
static int gapEvent(struct ble_gap_event *event, void *arg);

static const struct ble_gatt_svc_def gattServices[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &throttleServiceUuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &throttleCharacteristicUuid.u,
                .access_cb = NULL,
                .val_handle = &throttleValueHandle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 },
        },
    },
    { 0 },
};

/**
 * Start connectable advertising with the controller name and throttle UUID.
 */
static void startAdvertising(void)
{
    uint8_t ownAddressType;
    struct ble_hs_adv_fields fields;
    struct ble_gap_adv_params parameters;

    if (ble_hs_id_infer_auto(0, &ownAddressType) != 0) {
        ESP_LOGE(TAG, "Could not determine local BLE address type");
        return;
    }

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)BLE_SLAVE_DEVICE_NAME;
    fields.name_len = strlen(BLE_SLAVE_DEVICE_NAME);
    fields.name_is_complete = 1;
    fields.uuids128 = (ble_uuid128_t *)&throttleServiceUuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    if (ble_gap_adv_set_fields(&fields) != 0) {
        ESP_LOGE(TAG, "Could not set BLE advertisement fields");
        return;
    }

    memset(&parameters, 0, sizeof(parameters));
    parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    if (ble_gap_adv_start(ownAddressType, NULL, BLE_HS_FOREVER,
                          &parameters, gapEvent, NULL) != 0) {
        ESP_LOGE(TAG, "Could not start BLE advertising");
    }
}

/**
 * Handle connections, subscription changes, and restart advertising after
 * the master disconnects.
 */
static int gapEvent(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            connectionHandle = event->connect.conn_handle;
            notificationsEnabled = false;
            ESP_LOGI(TAG, "BLE master connected");
        } else {
            startAdvertising();
        }
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == throttleValueHandle) {
            notificationsEnabled = event->subscribe.cur_notify != 0;
            ESP_LOGI(TAG, "Throttle notifications %s",
                     notificationsEnabled ? "enabled" : "disabled");
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        connectionHandle = BLE_HS_CONN_HANDLE_NONE;
        notificationsEnabled = false;
        startAdvertising();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        startAdvertising();
        break;

    default:
        break;
    }

    return 0;
}

/**
 * Configure the NimBLE host after its controller synchronization callback.
 */
static void onHostSync(void)
{
    startAdvertising();
}

/**
 * Run the NimBLE host event loop in its FreeRTOS task.
 */
static void nimbleHostTask(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * Initialize NVS, the GATT database, and the NimBLE peripheral host.
 */
void bleSlave_init(void)
{
    esp_err_t nvsResult = nvs_flash_init();
    if (nvsResult == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvsResult == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsResult = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvsResult);

    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(BLE_SLAVE_DEVICE_NAME));
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gattServices));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gattServices));

    connectionHandle = BLE_HS_CONN_HANDLE_NONE;
    notificationsEnabled = false;
    ble_hs_cfg.sync_cb = onHostSync;
    nimble_port_freertos_init(nimbleHostTask);
}

/**
 * Send one throttle byte to the subscribed master.
 */
void bleSlave_sendThrottle(uint8_t throttle)
{
    if (!notificationsEnabled || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    struct os_mbuf *buffer = ble_hs_mbuf_from_flat(&throttle, sizeof(throttle));
    if (buffer == NULL) {
        ESP_LOGW(TAG, "Could not allocate throttle notification buffer");
        return;
    }

    const int rc = ble_gatts_notify_custom(connectionHandle,
                                            throttleValueHandle, buffer);
    if (rc != 0) {
        ESP_LOGW(TAG, "Could not send throttle notification: %d", rc);
        os_mbuf_free_chain(buffer);
    }
}

/**
 * Return whether the master has an active throttle notification subscription.
 */
bool bleSlave_isConnected(void)
{
    return notificationsEnabled;
}
