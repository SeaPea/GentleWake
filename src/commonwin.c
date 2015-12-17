#include "commonwin.h"
#include "common.h"

Window* window_create_fullscreen(Layer **root_layer, GRect *bounds) {
  Window *win = window_create();
  IF_2(window_set_fullscreen(win, true));
  *root_layer = window_get_root_layer(win);
  *bounds = layer_get_bounds(*root_layer); 
  window_set_background_color(win, GColorBlack);
  IF_2(bounds.size.h += 16);
  return win;
}

Layer* layer_create_with_proc(Layer *root_layer, LayerUpdateProc proc, GRect bounds) {
  Layer *l = layer_create(bounds);
  layer_set_update_proc(l, proc); 
  layer_add_child(root_layer, l);
  return l;
}

ActionBarLayer* actionbar_create(Window *win, Layer *root_layer, const GRect *bounds, GBitmap *bmp_up, GBitmap *bmp_sel, GBitmap *bmp_down) {
  ActionBarLayer *actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(actionbarlayer, win);
  action_bar_layer_set_background_color(actionbarlayer, GColorWhite);
  action_bar_layer_set_icon(actionbarlayer, BUTTON_ID_UP, bmp_up);
  action_bar_layer_set_icon(actionbarlayer, BUTTON_ID_SELECT, bmp_sel);
  action_bar_layer_set_icon(actionbarlayer, BUTTON_ID_DOWN, bmp_down);
#ifdef PBL_RECT
  layer_set_frame(action_bar_layer_get_layer(actionbarlayer), GRect(bounds->size.w-20, 0, 20, bounds->size.h));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(actionbarlayer), GRect(-5, 0, 30, bounds->size.h)));
#endif
  layer_add_child(root_layer, action_bar_layer_get_layer(actionbarlayer));
  return actionbarlayer;
}