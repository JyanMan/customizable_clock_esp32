#include "ble_nimble/ble_nimble_setup.h"
#include "ble_nimble/common.h"
#include "ble_nimble/gatt_init.h"
#include "ble_nimble/gatt_svc.h"
#include "ble_nimble/common.h"
#include "ble_nimble/gap.h"

#define NIMBLE_STACK_SIZE 4*1024
#define NIMBLE_TASK_PRIORITY 5

esp_err_t ble_nimble_setup() {
    nimble_host_config_init();

    /* NimBLE stack initialization */
    esp_err_t rc = nimble_port_init();
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 rc);
        return rc;
    }

    /* GAP service initialization */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return rc;
    }

    /* GATT server initialization */
    rc = gatt_svc_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
        return rc;
    }

    /* Start NimBLE host task thread and return */
    xTaskCreate(nimble_host_task, "NIMBLE HOST TASK", NIMBLE_STACK_SIZE, NULL, NIMBLE_TASK_PRIORITY, NULL);

    return rc;
}
