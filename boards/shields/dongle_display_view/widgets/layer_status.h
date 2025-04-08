/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#define CANVAS_SIZE 68

struct zmk_widget_layer_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_layer_stats_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_layer_stats_obj(struct zmk_widget_layer_status *widget);