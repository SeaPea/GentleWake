#include "errormsg.h"
#include <pebble.h>

static char s_msg[150];
  
// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_18_bold;
static TextLayer *title_layer;
static TextLayer *msg_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  // title_layer
  title_layer = text_layer_create(GRect(20, -4, 100, 29));
  text_layer_set_text(title_layer, "OOPS");
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_font(title_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)title_layer);
  
  // msg_layer
  msg_layer = text_layer_create(GRect(3, 22, 138, 128));
  text_layer_set_background_color(msg_layer, GColorClear);
  text_layer_set_text(msg_layer, "msg_layer");
  text_layer_set_text_alignment(msg_layer, GTextAlignmentCenter);
  text_layer_set_font(msg_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)msg_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(title_layer);
  text_layer_destroy(msg_layer);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_errormsg(char *msg) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  strncpy(s_msg, msg, sizeof(s_msg));
  text_layer_set_text(msg_layer, s_msg);
  
  window_stack_push(s_window, true);
  
  vibes_long_pulse();
}

void hide_errormsg(void) {
  window_stack_remove(s_window, true);
}
