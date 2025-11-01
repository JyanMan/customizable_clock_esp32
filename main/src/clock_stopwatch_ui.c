/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#loader-with-arc

#include "lvgl.h"
#include "clock_stopwatch.h"

// static lv_obj_t * btn;
// static lv_display_rotation_t rotation = LV_DISP_ROTATION_0;
// 
// static void btn_cb(lv_event_t * e)
// {
//     lv_display_t *disp = lv_event_get_user_data(e);
//     rotation++;
//     if (rotation > LV_DISP_ROTATION_270) {
//         rotation = LV_DISP_ROTATION_0;
//     }
//     lv_disp_set_rotation(disp, rotation);
// }
// static void set_angle(void * obj, int32_t v)
// {
//     lv_arc_set_value(obj, v);
// }

void clock_countdown_lvgl_ui(lv_display_t *disp, lv_obj_t *label) {
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x282828), LV_PART_MAIN);

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
    lv_style_set_text_font(&label_style, &lv_font_montserrat_30);

    // lv_obj_set_style_text_font(lv_screen_active(), &lv_font_montserrat_30, LV_PART_MAIN);

    /*Create a white label, set its text and align it t&o the center*/
    // lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_obj_add_style(label, &label_style, LV_PART_MAIN);
    lv_label_set_text(label, "11:26:00");


    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x504945), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
} 
