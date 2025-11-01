#include <stdint.h>
#include <sys/lock.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock_stopwatch.h"
#include "lvgl.h"
#include "esp_log.h"

static uint32_t test_current_time = 22345;
// static _lock_t lvgl_api_lock;

void clock_stopwatch_task(void *params) {
    for ( ;; ) {
        test_current_time += 1;
        lv_obj_t **p_label = (lv_obj_t **)params;
        if (p_label) {
            ESP_LOGI("TEST LABEL", "label is received");
            
            char str[16];
            uint8_t hour = test_current_time / 3600;
            uint8_t min = (test_current_time % 3600) / 60;
            uint8_t sec = test_current_time % 60;
            snprintf(str, 16, "%02d:%02d:%02d", hour, min, sec);

            lv_obj_t *label = *p_label;
            _lock_acquire(&lvgl_api_lock);
            lv_label_set_text(label, str);
            _lock_release(&lvgl_api_lock);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
