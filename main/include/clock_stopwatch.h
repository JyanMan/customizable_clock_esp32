#ifndef CLOCK_STOPWATCH_H
#define CLOCK_STOPWATCH_H

#include "lvgl.h"
#include "sys/lock.h"

LV_FONT_DECLARE(FontAwesome);
#define LV_SYMBOL_SUN "\xEF\x86\x85"

typedef struct ClockStopwatchInfo {
    lv_obj_t *time_label;
    lv_obj_t *sec_label;
    lv_obj_t *weather_label;
} ClockStopwatchInfo;

static _lock_t lvgl_api_lock;

void clock_stopwatch_info_init(ClockStopwatchInfo *csi);

void clock_countdown_lvgl_ui(lv_display_t *disp, ClockStopwatchInfo *stopwatch_info);

void clock_stopwatch_update_time(uint32_t new_time_s);
void clock_stopwatch_task(void *params);
void clock_stopwatch_sync_sntp_task(void *params);

#endif
