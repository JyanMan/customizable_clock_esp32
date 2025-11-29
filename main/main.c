#include "lcd_lvgl_setup.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_setup.h"
#include "clock_stopwatch.h"
#include "ble_nimble/ble_nimble_setup.h"

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    lcd_lvgl_setup();
    clock_stopwatch_init();
    
    ESP_ERROR_CHECK(ble_nimble_setup());
}
