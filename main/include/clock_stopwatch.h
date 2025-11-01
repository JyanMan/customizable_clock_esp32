#include "lvgl.h"
#include "sys/lock.h"

static _lock_t lvgl_api_lock;

void clock_countdown_lvgl_ui(lv_display_t *disp, lv_obj_t *label);
void clock_stopwatch_task(void *params);
