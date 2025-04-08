/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

 #include <zephyr/kernel.h>

 #include <zephyr/logging/log.h>
 LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
 
 #include <zmk/display.h>
 #include <zmk/event_manager.h>
 #include <zmk/events/ble_active_profile_changed.h>
 #include <zmk/events/endpoint_changed.h>
 #include <zmk/events/usb_conn_state_changed.h>
 #include <zmk/usb.h>
 #include <zmk/ble.h>
 #include <zmk/endpoints.h>
 
 #include "output_status.h"
 
 static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
 
 LV_IMG_DECLARE(sym_usb);
 LV_IMG_DECLARE(sym_bt);
 LV_IMG_DECLARE(sym_ok);
 LV_IMG_DECLARE(sym_nok);
 LV_IMG_DECLARE(sym_open);
 LV_IMG_DECLARE(sym_1);
 LV_IMG_DECLARE(sym_2);
 LV_IMG_DECLARE(sym_3);
 LV_IMG_DECLARE(sym_4);
 LV_IMG_DECLARE(sym_5);
 
 const lv_img_dsc_t *sym_num[] = {
     &sym_1,
     &sym_2,
     &sym_3,
     &sym_4,
     &sym_5,
 };
 
 enum output_symbol {
     output_symbol_usb,
     output_symbol_usb_hid_status,
     output_symbol_bt,
     output_symbol_bt_number,
     output_symbol_bt_status,
     output_symbol_selection_line
 };
 
 enum selection_line_state {
     selection_line_state_usb,
     selection_line_state_bt
 } current_selection_line_state;
 
 lv_point_t selection_line_points[] = { {0, 0}, {13, 0} }; // will be replaced with lv_point_precise_t 
 
 struct output_status_state {
     struct zmk_endpoint_instance selected_endpoint;
     int active_profile_index;
     bool active_profile_connected;
     bool active_profile_bonded;
     bool usb_is_hid_ready;
 };
 
 static struct output_status_state get_state(const zmk_event_t *_eh) {
     return (struct output_status_state){
         .selected_endpoint = zmk_endpoints_selected(),
         .active_profile_index = zmk_ble_active_profile_index(),
         .active_profile_connected = zmk_ble_active_profile_is_connected(),
         .active_profile_bonded = !zmk_ble_active_profile_is_open(),
         .usb_is_hid_ready = zmk_usb_is_hid_ready()
     };
 }
 
 static void anim_x_cb(void * var, int32_t v) {
     lv_obj_set_x(var, v);
 }
 
 static void anim_size_cb(void * var, int32_t v) {
     selection_line_points[1].x = v;
 }
 
 static void move_object_x(void *obj, int32_t from, int32_t to) {
     lv_anim_t a;
     lv_anim_init(&a);
     lv_anim_set_var(&a, obj);
     lv_anim_set_time(&a, 200); // will be replaced with lv_anim_set_duration
     lv_anim_set_exec_cb(&a, anim_x_cb);
     lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
     lv_anim_set_values(&a, from, to);
     lv_anim_start(&a);
 }
 
 static void change_size_object(void *obj, int32_t from, int32_t to) {
     lv_anim_t a;
     lv_anim_init(&a);
     lv_anim_set_var(&a, obj);
     lv_anim_set_time(&a, 200); // will be replaced with lv_anim_set_duration
     lv_anim_set_exec_cb(&a, anim_size_cb);
     lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
     lv_anim_set_values(&a, from, to);
     lv_anim_start(&a);
 }
 
 static void set_status_symbol(lv_obj_t *widget, struct output_status_state state) {
     lv_obj_t *usb = lv_obj_get_child(widget, output_symbol_usb);
     lv_obj_t *usb_hid_status = lv_obj_get_child(widget, output_symbol_usb_hid_status);
     lv_obj_t *bt = lv_obj_get_child(widget, output_symbol_bt);
     lv_obj_t *bt_number = lv_obj_get_child(widget, output_symbol_bt_number);
     lv_obj_t *bt_status = lv_obj_get_child(widget, output_symbol_bt_status);
     lv_obj_t *selection_line = lv_obj_get_child(widget, output_symbol_selection_line);
 
     switch (state.selected_endpoint.transport) {
     case ZMK_TRANSPORT_USB:
         if (current_selection_line_state != selection_line_state_usb) {
             move_object_x(selection_line, lv_obj_get_x(bt) - 1, lv_obj_get_x(usb) - 1);
             change_size_object(selection_line, 18, 11);
             current_selection_line_state = selection_line_state_usb;
         }
         break;
     case ZMK_TRANSPORT_BLE:
         if (current_selection_line_state != selection_line_state_bt) {
             move_object_x(selection_line, lv_obj_get_x(usb) - 1, lv_obj_get_x(bt) - 1);
             change_size_object(selection_line, 11, 18);
             current_selection_line_state = selection_line_state_bt;
         }
         break;
     }
 
     if (state.usb_is_hid_ready) {
         lv_img_set_src(usb_hid_status, &sym_ok);
     } else {
         lv_img_set_src(usb_hid_status, &sym_nok);
     }
 
     if (state.active_profile_index < (sizeof(sym_num) / sizeof(lv_img_dsc_t *))) {
         lv_img_set_src(bt_number, sym_num[state.active_profile_index]);
     } else {
         lv_img_set_src(bt_number, &sym_nok);
     }
     
     if (state.active_profile_bonded) {
         if (state.active_profile_connected) {
             lv_img_set_src(bt_status, &sym_ok);
         } else {
             lv_img_set_src(bt_status, &sym_nok);
         }
     } else {
         lv_img_set_src(bt_status, &sym_open);
     }
 }
 
 static void output_status_update_cb(struct output_status_state state) {
     struct zmk_widget_output_status *widget;
     SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status_symbol(widget->obj, state); }
 }
 
 ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                             output_status_update_cb, get_state)
 ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
 ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
 ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
 
 int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
     static lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];  // キャンバスバッファ
 
     widget->obj = lv_obj_create(parent);
     lv_obj_set_size(widget->obj, CANVAS_SIZE, CANVAS_SIZE);
 
     // キャンバスの作成
     lv_obj_t *canvas = lv_canvas_create(widget->obj);
     lv_canvas_set_buffer(canvas, cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
     lv_obj_center(canvas);
     
     // キャンバスの背景を設定
     lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
 
     // USBアイコンの作成と配置
     lv_draw_img_dsc_t img_dsc;
     lv_draw_img_dsc_init(&img_dsc);
     
     // USBアイコンを描画
     lv_canvas_draw_img(canvas, 1, 4, &sym_usb, &img_dsc);
 
     // USBステータスアイコンを描画
     lv_canvas_draw_img(canvas, 3, 14, &sym_ok, &img_dsc);
 
     // Bluetoothアイコンを描画
     lv_canvas_draw_img(canvas, 18, 4, &sym_bt, &img_dsc);
 
     // Bluetooth番号を描画
     lv_canvas_draw_img(canvas, 32, 11, sym_num[0], &img_dsc);
 
     // Bluetoothステータスを描画
     lv_canvas_draw_img(canvas, 32, 5, &sym_ok, &img_dsc);
 
     // 選択ラインを描画
     lv_draw_line_dsc_t line_dsc;
     init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);
     // lv_canvas_draw_line(canvas, 0, 0, 13, 0, &line_dsc);
 
     // キャンバスを回転
     rotate_canvas(canvas, cbuf);
 
     sys_slist_append(&widgets, &widget->node);
     widget_output_status_init();
     return 0;
 }
 
 lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
     return widget->obj;
 }
 