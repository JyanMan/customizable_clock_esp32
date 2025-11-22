/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "ble_nimble/gatt_svc.h"
#include "ble_nimble/common.h"
#include "ble_nimble/global_vars.h"
#include "driver/gpio.h"

/* Private function declarations */
static int btn_click_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* button click service */
static const ble_uuid128_t btn_click_svc_uuid = 
    BLE_UUID128_INIT(0xd2, 0xf2, 0xfa, 0xe7, 0x9f, 0xb9, 0x44, 0x99, 0x82, 
            0x13, 0x9d, 0xc6, 0x67, 0x47, 0x2e, 0x00);
// d2f2fae7-9fb9-4499-8213-9dc667472e00
static uint16_t btn_chr_val_handle;
static const ble_uuid128_t btn_chr_uuid = 
    BLE_UUID128_INIT(0xa2, 0xda, 0xdb, 0x45, 0x08, 0xd9, 0x4e, 0x12, 0x97, 
            0xef, 0x57, 0x15, 0x58, 0x57, 0xf6, 0x46);
// a2dadb45-08d9-4e12-97ef-57155857f646

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* button click service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &btn_click_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){/* LED characteristic */
                                        {.uuid = &btn_chr_uuid.u,
                                         .access_cb = btn_click_chr_access,
                                         .flags = BLE_GATT_CHR_F_WRITE,
                                         .val_handle = &btn_chr_val_handle},
                                        {0}},
    },

    {
        0, /* No more services. */
    },
};
static int btn_click_chr_access(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg
) {
    int rc;
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                         conn_handle, attr_handle);
            } else {
                ESP_LOGI(TAG,
                         "characteristic write by nimble stack; attr_handle=%d",
                         attr_handle);
            }

            /* Verify attribute handle */
            if (attr_handle != btn_chr_val_handle) {
                goto error;
            }
            /* Verify access buffer length */
            if (ctxt->om->om_len != 1) {
                goto error;
            }
            /* Turn the LED on or off according to the operation bit */
            uint8_t value = ctxt->om->om_data[0];
            /* set the received data to global */
            global_data = value;
            switch (value) {
                case 0x01:
                    gpio_set_level(2, true);
                    ESP_LOGI(TAG, "led turned on!");
                    break;
                case 0x10:
                    gpio_set_level(2, false);
                    ESP_LOGI(TAG, "led turned off!");
                    break;
                default:
                    gpio_set_level(2, false);
                    break;
            }
            return rc;
        default:
            goto error;
    }
    error:
        ESP_LOGE(TAG,
                 "unexpected access operation to led characteristic, opcode: %d",
                 ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
}


/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
