#pragma once
#include <pebble.h>

Window* window_create_fullscreen(Layer **root_layer, GRect *bounds);
Layer* layer_create_with_proc(Layer *root_layer, LayerUpdateProc proc, GRect bounds);
ActionBarLayer* actionbar_create(Window *win, Layer *root_layer, const GRect *bounds, GBitmap *bmp_up, GBitmap *bmp_sel, GBitmap *bmp_down);
