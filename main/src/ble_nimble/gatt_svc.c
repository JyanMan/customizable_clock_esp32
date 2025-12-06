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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock_stopwatch.h"

#define READ_NUM_BYTES 5

/* Private function declarations */
static int clock_ui_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* button click service */
static const ble_uuid128_t clock_ui_svc_uuid = 
    BLE_UUID128_INIT(0xd2, 0xf2, 0xfa, 0xe7, 0x9f, 0xb9, 0x44, 0x99, 0x82, 
            0x13, 0x9d, 0xc6, 0x67, 0x47, 0x2e, 0x00);
// d2f2fae7-9fb9-4499-8213-9dc667472e00
static uint16_t clock_ui_chr_handle;
static const ble_uuid128_t btn_chr_uuid = 
    BLE_UUID128_INIT(0xa2, 0xda, 0xdb, 0x45, 0x08, 0xd9, 0x4e, 0x12, 0x97, 
            0xef, 0x57, 0x15, 0x58, 0x57, 0xf6, 0x46);
// a2dadb45-08d9-4e12-97ef-57155857f646

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* button click service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &clock_ui_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){/* LED characteristic */
                                        {.uuid = &btn_chr_uuid.u,
                                         .access_cb = clock_ui_chr_access,
                                         .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
                                         .val_handle = &clock_ui_chr_handle},
                                        {0}},
    },

    {
        0, /* No more services. */
    },
};

static int clock_ui_chr_access(
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
            if (attr_handle != clock_ui_chr_handle) {
                goto error;
            }
            /* Verify access buffer length */
            if (ctxt->om->om_len != READ_NUM_BYTES) {
                ESP_LOGI(TAG, "received data is not the right amount of bytes");
                return rc;
            }
            uint8_t *data = ctxt->om->om_data;
            uint8_t data_type = data[0]; 
            WriteData write_data;
            switch (data_type) {
                case 0x00:
                    write_data.data_type = WRITE_DATA_REQUESTDATA;
                    write_data.value.request_data = true;
                    ESP_LOGI(TAG, "data received is of type REQUESTDATA");
                    break;
                case 0x01:
                    /* receive data */
                    write_data.data_type = WRITE_DATA_POSITION;
                    write_data.value.pos.x= data[1] | (data[2] << 8);
                    write_data.value.pos.y = data[3] | (data[4] << 8);
                    /* put to queue to be read from the tasks */
                    break;
            }

            xQueueSend(ui_write_queue, &write_data, 100 / portTICK_PERIOD_MS);

            return rc;
        case BLE_GATT_ACCESS_OP_READ_CHR:
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            } else {
                ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                        attr_handle);
            }

            /* Verify attribute handle */
            if (attr_handle != clock_ui_chr_handle) {
                goto error;
            }

            ClockStopwatchUiData ui_data;
            if( xQueueReceive( ui_read_queue, &( ui_data ), 0 ) == pdPASS ) {

                int32_t timer_label_pos = ui_data.timer_label_pos;
                ESP_LOGI(TAG, "received queue as ui_data and its timer_label_pos: %x", timer_label_pos);
                
                /* send data via ble */
                uint8_t test = 0x01;
                
                os_mbuf_append(ctxt->om, &test, sizeof(uint8_t));
                os_mbuf_append(ctxt->om, &timer_label_pos, sizeof(int32_t));
                os_mbuf_append(ctxt->om, &ui_data.timer_label_height, sizeof(int32_t));
                rc = os_mbuf_append(ctxt->om, &ui_data.timer_label_width, sizeof(int32_t));
                ESP_LOGI(TAG, "received width and height: %d, %d", ui_data.timer_label_width, ui_data.timer_label_height);
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }

            ESP_LOGE(TAG, "ui_read_queue was empty");

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
