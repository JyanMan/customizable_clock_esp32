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

static uint32_t time_since_last_sntp_update = 22345;
static uint32_t time_diff_since_last_update = 0;
static uint8_t start_sntp_sync = 0;

void clock_stopwatch_update_time(uint32_t new_time_s) {
    time_since_last_sntp_update = new_time_s;
    time_diff_since_last_update = 0;
}

void clock_stopwatch_task(void *params) {
    for ( ;; ) {
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
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static bool sync_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {

    uint8_t *sync_retries = (uint8_t *)user_ctx;
    // sync_sntp_retry_timer();
    // if (sntp_sync() == ESP_ERR_TIMEOUT && *sync_retries < 10) {
    if (*sync_retries < 10) {
        *sync_retries += 1;
        // sync_sntp_retry_timer();
    }
    else {
        *sync_retries = 0;
        gptimer_stop(timer);
    }
    return false;
}

static void sync_sntp_retry_timer(uint8_t *retries) {
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 10 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };
    // Create a timer instance
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1000000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
        .flags.auto_reload_on_alarm = true // Disable auto-reload function
    };
    // Set the timer's alarm action
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = sync_alarm_callback, // Call the user callback function when the alarm event occurs
    };
    // Register timer event callback functions, allowing user context to be carried
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, retries));
    // Enable the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void clock_stopwatch_sync_sntp_task(void *params) {
    static uint8_t sync_retries = 0;
    // sync on boot
    static const uint8_t retry_sec = 5;
    if (sntp_sync() == ESP_ERR_TIMEOUT) {
        sync_retries++;
    }
    for ( ;; ) {
        uint8_t hours_since_update = time_diff_since_last_update / ONE_HOUR_IN_SEC;
        if (sync_retries >= 5) {
            ESP_LOGE("sntp sync task", "unable to sync after %d retries", retry_sec);
            sync_retries = 0;
        }
        if (sync_retries != 0) {
            ESP_LOGI("sntp sync task", "unable to sync sntp... retrying in %d sec", retry_sec);
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
