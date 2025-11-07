#include <stdint.h>
#include <sys/lock.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock_stopwatch.h"
#include "sntp_setup.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gptimer.h"

#define ONE_HOUR_IN_SEC 3600

static volatile uint32_t time_since_last_sntp_update = 22345;
static volatile uint32_t time_diff_since_last_update = 0;

static const char *TAG = "clock stopwatch task";

void clock_stopwatch_update_time(uint32_t new_time_s) {
    time_since_last_sntp_update = new_time_s;
    time_diff_since_last_update = 0;
}

SemaphoreHandle_t semaphore_stopwatch;

static bool example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semaphore_stopwatch, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
    return false;
}

static void stopwatch_increment_timer_init() {

    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };
    // Create a timer instance
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,      // When the alarm event occurs, the timer will automatically reload to 0
        .alarm_count = 1000000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
        .flags.auto_reload_on_alarm = true, // Enable auto-reload function
    };
    // Set the timer's alarm action
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb, // Call the user callback function when the alarm event occurs
    };
    // Register timer event callback functions, allowing user context to be carried
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    // Enable the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void clock_stopwatch_task(void *params) {

    semaphore_stopwatch = xSemaphoreCreateBinary();
    if (!semaphore_stopwatch) {
        ESP_LOGE(TAG, "insufficient heap memory for semaphore creation...");
    }

    stopwatch_increment_timer_init();

    for ( ;; ) {
        if ( xSemaphoreTake( semaphore_stopwatch, portMAX_DELAY) == pdFALSE ) {
            continue;
        }
        // uint64_t curr_time = boot_time + (esp_timer_get_time() / 1000000ULL);
        lv_obj_t **p_label = (lv_obj_t **)params;
        time_diff_since_last_update += 1;
        if (p_label) {
            uint32_t local_time_s  = time_since_last_sntp_update + time_diff_since_last_update;
            char str[16];
            uint8_t hour = local_time_s / 3600;
            uint8_t min = (local_time_s % 3600) / 60;
            uint8_t sec = local_time_s % 60;
            snprintf(str, 16, "%02d:%02d:%02d", hour, min, sec);

            lv_obj_t *label = *p_label;
            _lock_acquire(&lvgl_api_lock);
            lv_label_set_text(label, str);
            _lock_release(&lvgl_api_lock);
        }
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void clock_stopwatch_sync_sntp_task(void *params) {
    static volatile uint8_t sync_retries = 0;
    static const uint8_t retry_sec = 5;
    // sync on boot
    if (sntp_sync() == ESP_ERR_TIMEOUT) {
        sync_retries++;
    }
    for ( ;; ) {
        uint8_t hours_since_update = time_diff_since_last_update / ONE_HOUR_IN_SEC;
        if (sync_retries >= 5) {
            ESP_LOGE(TAG, "unable to sync after %d retries", retry_sec);
            sync_retries = 0;
        }
        if (sync_retries != 0) {
            ESP_LOGI(TAG, "unable to sync sntp... retrying in %d sec", retry_sec);
            vTaskDelay((retry_sec * 1000) / portTICK_PERIOD_MS);
            if (sntp_sync() == ESP_ERR_TIMEOUT) {
                continue;
            }
            else {
                sync_retries = 0;
            }
        }
        if (hours_since_update >= 24) {
            if (sntp_sync() == ESP_ERR_TIMEOUT) {
                sync_retries++;
                continue;
            }
        }
        vTaskDelay((ONE_HOUR_IN_SEC * 1000) / portTICK_PERIOD_MS);
    }
}
