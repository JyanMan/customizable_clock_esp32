#include "lvgl.h"
#include "sys/lock.h"

static _lock_t lvgl_api_lock;

void clock_stopwatch_update_time(uint32_t new_time_s);
void clock_countdown_lvgl_ui(lv_display_t *disp, lv_obj_t *label);
void clock_stopwatch_task(void *params);
void clock_stopwatch_sync_sntp_task(void *params);
