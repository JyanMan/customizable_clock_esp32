/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#loader-with-arc

#include "lvgl.h"
#include "clock_stopwatch.h"

static void clock_time_label(lv_obj_t *label) {
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_radius(&label_style, 5);
    lv_style_set_bg_opa(&label_style, LV_OPA_COVER);
    lv_style_set_bg_color(&label_style, lv_color_hex(0x458588));
    lv_style_set_border_width(&label_style, 2);
    lv_style_set_border_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_pad_all(&label_style, 10);

    lv_style_set_text_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_text_letter_space(&label_style, 5);
    lv_style_set_text_line_space(&label_style, 20);
    lv_style_set_text_decor(&label_style, LV_TEXT_DECOR_UNDERLINE);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_48);

    lv_obj_add_style(label, &label_style, LV_PART_MAIN);
    lv_label_set_text(label, "00:00");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x504945), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, -50, 0);
}

static void clock_sec_label(lv_obj_t *label) {
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_radius(&label_style, 5);
    lv_style_set_bg_opa(&label_style, LV_OPA_COVER);
    lv_style_set_bg_color(&label_style, lv_color_hex(0x458588));
    lv_style_set_border_width(&label_style, 2);
    lv_style_set_border_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_pad_all(&label_style, 10);

    lv_style_set_text_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_text_letter_space(&label_style, 5);
    lv_style_set_text_line_space(&label_style, 20);
    lv_style_set_text_decor(&label_style, LV_TEXT_DECOR_UNDERLINE);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_20);

    lv_obj_add_style(label, &label_style, LV_PART_MAIN);
    lv_label_set_text(label, "00");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x504945), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 100, 20);
}

static void clock_weather_label(lv_obj_t *label) {
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_radius(&label_style, 5);
    lv_style_set_bg_opa(&label_style, LV_OPA_COVER);
    lv_style_set_bg_color(&label_style, lv_color_hex(0x458588));
    lv_style_set_border_width(&label_style, 2);
    lv_style_set_border_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_pad_all(&label_style, 10);

    lv_style_set_text_color(&label_style, lv_color_hex(0xebdbb2));
    lv_style_set_text_letter_space(&label_style, 5);
    lv_style_set_text_line_space(&label_style, 20);
    lv_style_set_text_decor(&label_style, LV_TEXT_DECOR_UNDERLINE);
    lv_style_set_text_font(&label_style, &FontAwesome);

    lv_obj_add_style(label, &label_style, LV_PART_MAIN);
    lv_label_set_text(label, LV_SYMBOL_CLOUD_SUN_RAIN);
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x504945), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 100, -25);
}

void clock_countdown_lvgl_ui(lv_display_t *disp, ClockStopwatchInfo *stopwatch_info) {
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x282828), LV_PART_MAIN);

    clock_time_label(stopwatch_info->time_label);
    clock_sec_label(stopwatch_info->sec_label);
    clock_weather_label(stopwatch_info->weather_label);

} 
