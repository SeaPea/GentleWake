#include "common.h"
#include "stopinstructions.h"
#include <pebble.h>

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_gothic_24_bold;
static TextLayer *s_textlayer_2;
static TextLayer *s_textlayer_1;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  IF_2(window_set_fullscreen(s_window, true));
  
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  // s_textlayer_2
  s_textlayer_2 = text_layer_create(GRect(2, 5, 139, 60));
  text_layer_set_background_color(s_textlayer_2, GColorBlack);
  text_layer_set_text_color(s_textlayer_2, GColorWhite);
  text_layer_set_text(s_textlayer_2, "<-- Hold Back button to exit app WITHOUT stopping alarm (Will stop sleep monitoring)");
  text_layer_set_text_alignment(s_textlayer_2, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_2);
  
  // s_textlayer_1
  s_textlayer_1 = text_layer_create(GRect(5, 73, 136, 74));
  text_layer_set_background_color(s_textlayer_1, GColorBlack);
  text_layer_set_text_color(s_textlayer_1, GColorWhite);
  text_layer_set_text(s_textlayer_1, "Double click ANY button to stop the alarm");
  text_layer_set_text_alignment(s_textlayer_1, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_1, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_1);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_textlayer_2);
  text_layer_destroy(s_textlayer_1);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void autohide(void *data) {
  hide_stopinstructions();
}

void show_stopinstructions(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  
  app_timer_register(10000, autohide, NULL);
}

void hide_stopinstructions(void) {
  window_stack_remove(s_window, true);
}
