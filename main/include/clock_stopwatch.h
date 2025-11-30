#ifndef CLOCK_STOPWATCH_H
#define CLOCK_STOPWATCH_H

#include <time.h>
#include "lvgl.h"
#include "sys/lock.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

LV_FONT_DECLARE(FontAwesome);
/* 0xf0e9, 0xf73d, 0xf743, 0xf185, 0xf72e, 0xf743, 0xf73c, 0xf75a, 0xf740 */
/* LVSYMBOL are in HEX UTF8 bytes */
#define LV_SYMBOL_SUN "\xEF\x86\x85" // 0xf185
#define LV_SYMBOL_CLOUD_SHOWERS_HEAVY "\xEF\x9D\x80" // 0xf740
#define LV_SYMBOL_CLOUD_SUN_RAIN "\xEF\x9D\x83" // 0xf743
#define LV_SYMBOL_CLOUD_MOON_RAIN "\xEF\x9C\xBC" // 0xf73c
#define LV_SYMBOL_POO_STORM "\xEF\x9D\x9A" // 0xf75a
#define LV_SYMBOL_CLOUD_RAIN "\xEF\x9C\xBD" // 0xf73d
#define LV_SYMBOL_WIND "\xEF\x9C\xAE" // 0xf72e

typedef struct ClockStopwatchInfo {
    lv_obj_t *time_label;
    lv_obj_t *sec_label;
    lv_obj_t *weather_label;
    lv_obj_t *date_label;
} ClockStopwatchInfo;

typedef struct ClockStopwatchUiData {
    int32_t timer_label_pos;
    int32_t timer_label_width;
    int32_t timer_label_height;
} ClockStopwatchUiData;

typedef struct WriteData {
    int16_t timer_label_x;
    int16_t timer_label_y;
} WriteData;

/* data queue for ui position changes */
extern QueueHandle_t ui_write_queue;

extern QueueHandle_t ui_read_queue;

ClockStopwatchInfo *get_stopwatch_info();
void clock_stopwatch_init();
void clock_stopwatch_info_init(ClockStopwatchInfo *csi);
void clock_countdown_lvgl_ui(lv_display_t *disp, ClockStopwatchInfo *stopwatch_info);
void clock_stopwatch_update_time(struct tm* timeinfo);
// void clock_stopwatch_task(void *params);
// void clock_stopwatch_sync_sntp_task(void *params);

#endif
