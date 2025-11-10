#include "lvgl.h"
#include "clock_stopwatch.h"

void clock_stopwatch_info_init(ClockStopwatchInfo *csi) {
    csi->time_label = lv_label_create(lv_screen_active());
    csi->sec_label = lv_label_create(lv_screen_active());
    csi->weather_label = lv_label_create(lv_screen_active());
    csi->date_label = lv_label_create(lv_screen_active());
}
