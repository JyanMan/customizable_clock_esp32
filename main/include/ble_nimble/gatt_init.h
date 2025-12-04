#ifndef GATT_INIT_H
#define GATT_INIT_H

/* Library function declarations */
void ble_store_config_init(void);

/* Private function declarations */
void on_stack_reset(int reason);
void on_stack_sync(void);
void nimble_host_config_init(void);
void nimble_host_task(void *param);

#endif

