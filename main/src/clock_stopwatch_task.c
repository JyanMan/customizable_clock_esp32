#include <stdint.h>
#include <sys/lock.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock_stopwatch.h"
#include "sntp_setup.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gptimer.h"
#include "wifi_setup.h"

#define ONE_HOUR_IN_SEC 3600
#define CLOCK_STOPWATCH_TASK_PRIORITY 3
#define CLOCK_STOPWATCH_TASK_STACK_SIZE 3 * 1024

static volatile uint32_t local_time_s = 0;
static volatile uint16_t time_year = 0;
static volatile uint8_t time_month = 0;
static volatile uint8_t time_day = 0;

static const char *TAG = "clock stopwatch task";
static ClockStopwatchUiData ui_data;
static ClockStopwatchInfo stopwatch_info;
static SemaphoreHandle_t semaphore_stopwatch;

static void clock_stopwatch_sync_sntp_task(void *params);
static void clock_stopwatch_task(void *params);
static void send_read_queue_ui_data(ClockStopwatchInfo *stopwatch_info);
static void init_tasks();
static void stopwatch_increment_timer_init();
static void init_queues_and_semaphores();

QueueHandle_t ui_write_queue;
QueueHandle_t ui_read_queue;

/* PUBLIC FUNCTIONS */
ClockStopwatchInfo *get_stopwatch_info() { return &stopwatch_info; }

void clock_stopwatch_init() {
    init_queues_and_semaphores();
    stopwatch_increment_timer_init();
    init_tasks();
    send_read_queue_ui_data(&stopwatch_info);
}

void clock_stopwatch_update_time(struct tm* timeinfo) {
    local_time_s = timeinfo->tm_hour * 3600 + (timeinfo->tm_min * 60) + (timeinfo->tm_sec); 
    time_year = timeinfo->tm_year + 1900; // tm_year onnly returns 1900-year
    time_month = timeinfo->tm_mon;
    time_day = timeinfo->tm_mday;
}

/* PRIVATE FUNCTIONS */
static void send_read_queue_ui_data(ClockStopwatchInfo *stopwatch_info) {
    if (stopwatch_info == NULL) {
        ESP_LOGE(TAG, "received null stopwatch info");
        return;
    }
    
    _lock_acquire(&lvgl_api_lock);

    ui_data.timer_label_width = lv_obj_get_width(stopwatch_info->time_label);
    ui_data.timer_label_height = lv_obj_get_height(stopwatch_info->time_label);

    int32_t timer_label_x = lv_obj_get_x_aligned(stopwatch_info->time_label);
    int32_t timer_label_y = lv_obj_get_y_aligned(stopwatch_info->time_label);
    int32_t timer_label_pos = (timer_label_x << 16) | timer_label_y;
    ui_data.timer_label_pos = timer_label_pos;

    ESP_LOGI(TAG, "x_pos: %d", timer_label_x);
    ESP_LOGI(TAG, "y_pos: %d", timer_label_y);
    ESP_LOGI(TAG, "width: %d", ui_data.timer_label_width);
    ESP_LOGI(TAG, "height: %d", ui_data.timer_label_height);

    _lock_release(&lvgl_api_lock);

    ESP_LOGI(TAG, "sent data pos for ui_read_queue: %x", ui_data.timer_label_pos);

    xQueueSend(ui_read_queue, &ui_data, 0);
}

static bool stopwatch_increment_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
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
        .on_alarm = stopwatch_increment_timer_cb, // Call the user callback function when the alarm event occurs
    };
    // Register timer event callback functions, allowing user context to be carried
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    // Enable the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

static void init_queues_and_semaphores() {
    ESP_LOGI(TAG, "create queues");
    ui_read_queue = xQueueCreate(1, sizeof(uint32_t));
    ui_write_queue = xQueueCreate(10, sizeof(uint32_t));

    ESP_LOGI(TAG, "creating timer incrementor");
    semaphore_stopwatch = xSemaphoreCreateBinary();
    if (!semaphore_stopwatch) {
        ESP_LOGE(TAG, "insufficient heap memory for semaphore creation...");
    }
}

static void init_tasks() {
    ESP_LOGI(TAG, "Creating Stopwatch Update Tasks");
    xTaskCreate(clock_stopwatch_task, "CLOCK STOPWATCH", CLOCK_STOPWATCH_TASK_STACK_SIZE, 
        &stopwatch_info, CLOCK_STOPWATCH_TASK_PRIORITY, NULL);
    xTaskCreate(clock_stopwatch_sync_sntp_task, "CLOCK STOPWATCH SYNC SNTP", CLOCK_STOPWATCH_TASK_STACK_SIZE, 
        &stopwatch_info, CLOCK_STOPWATCH_TASK_PRIORITY, NULL);
}

static void clock_stopwatch_task(void *params) {
    for ( ;; ) {
        if ( xSemaphoreTake( semaphore_stopwatch, portMAX_DELAY) == pdFALSE ) {
            continue;
        }
        ClockStopwatchInfo *stopwatch_info = (ClockStopwatchInfo *)params;

        // increment time
        local_time_s += 1;

        if (stopwatch_info) {
            uint32_t signal_val;

            if( xQueueReceive( ui_write_queue, &( signal_val ), 0 ) == pdPASS ) {

                ESP_LOGI(TAG, "received point 1");
                int16_t x =  signal_val & 0xFFFF;         // automatically sign-extends
                int16_t y = (signal_val >> 16) & 0xFFFF;
                lv_obj_align(stopwatch_info->time_label, LV_ALIGN_TOP_LEFT, x, y);
                ESP_LOGI(TAG, "x: %d, y: %d, byte: (%x)", x, y, signal_val);
            }

            char local_time_str[16];
            char sec_str[4];

            uint8_t hour = local_time_s / 3600;
            uint8_t min = (local_time_s % 3600) / 60;
            uint8_t sec = local_time_s % 60;

            snprintf(local_time_str, 16, "%02d:%02d", hour, min);
            snprintf(sec_str, 4, "%02d", sec);

            _lock_acquire(&lvgl_api_lock);
            lv_label_set_text(stopwatch_info->time_label, local_time_str);
            lv_label_set_text(stopwatch_info->sec_label, sec_str);
            _lock_release(&lvgl_api_lock);
        }
    }
}

static esp_err_t sync_weather(ClockStopwatchInfo *stopwatch_info) {
    esp_err_t res = ESP_OK;

    return res;
}

static void update_date_label(ClockStopwatchInfo *stopwatch_info) {
    char date_str[32];

    char *time_month_str;

    switch (time_month) {
        case 0:  time_month_str = "Jan"; break;
        case 1:  time_month_str = "Feb"; break;
        case 2:  time_month_str = "Mar"; break;
        case 3:  time_month_str = "Apr"; break;
        case 4:  time_month_str = "May"; break;
        case 5:  time_month_str = "Jun"; break;
        case 6:  time_month_str = "Jul"; break;
        case 7:  time_month_str = "Aug"; break;
        case 8:  time_month_str = "Sep"; break;
        case 9:  time_month_str = "Oct"; break;
        case 10: time_month_str = "Nov"; break;
        case 11: time_month_str = "Dec"; break;
        default: time_month_str = "Invalid"; break;
    }

    snprintf(date_str, 32, "%s, %02d %04d", time_month_str, time_day, time_year);

    _lock_acquire(&lvgl_api_lock);
    lv_label_set_text(stopwatch_info->date_label, date_str);
    _lock_release(&lvgl_api_lock);
}

static void clock_stopwatch_sync_sntp_task(void *params) {

    static volatile uint8_t sync_retries = 0;
    static const uint8_t retry_sec = 5;

    ESP_ERROR_CHECK(wifi_full_init());

    // sync on boot
    if (sntp_sync() == ESP_ERR_TIMEOUT) {
        sync_retries++;
    }
    for ( ;; ) {

        ClockStopwatchInfo *stopwatch_info = (ClockStopwatchInfo *)params;
        update_date_label(stopwatch_info);

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

        uint8_t local_hours = local_time_s / ONE_HOUR_IN_SEC;
        if (local_hours == 0 || local_hours == 1) {
            if (sntp_sync() == ESP_ERR_TIMEOUT) {
                sync_retries++;
                continue;
            }
        }

        vTaskDelay((ONE_HOUR_IN_SEC * 1000) / portTICK_PERIOD_MS);
    }
}
