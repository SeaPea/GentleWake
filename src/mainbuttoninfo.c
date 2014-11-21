#include "mainbuttoninfo.h"
#include <pebble.h>

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_gothic_14;
static TextLayer *s_textlayer_1;
static TextLayer *s_textlayer_2;
static TextLayer *s_textlayer_3;
static TextLayer *s_textlayer_4;
static TextLayer *s_textlayer_5;
static TextLayer *s_textlayer_6;
static TextLayer *s_textlayer_7;
static TextLayer *s_textlayer_8;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_fullscreen(s_window, true);
  
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // s_textlayer_1
  s_textlayer_1 = text_layer_create(GRect(3, 38, 67, 19));
  text_layer_set_background_color(s_textlayer_1, GColorClear);
  text_layer_set_text_color(s_textlayer_1, GColorWhite);
  text_layer_set_text(s_textlayer_1, "<-- Exit App");
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_1);
  
  // s_textlayer_2
  s_textlayer_2 = text_layer_create(GRect(2, 52, 66, 36));
  text_layer_set_background_color(s_textlayer_2, GColorClear);
  text_layer_set_text_color(s_textlayer_2, GColorWhite);
  text_layer_set_text(s_textlayer_2, "(Alarms will still work)");
  text_layer_set_font(s_textlayer_2, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_2);
  
  // s_textlayer_3
  s_textlayer_3 = text_layer_create(GRect(112, 2, 30, 17));
  text_layer_set_background_color(s_textlayer_3, GColorClear);
  text_layer_set_text_color(s_textlayer_3, GColorWhite);
  text_layer_set_text(s_textlayer_3, "-->");
  text_layer_set_text_alignment(s_textlayer_3, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_3);
  
  // s_textlayer_4
  s_textlayer_4 = text_layer_create(GRect(67, 13, 75, 17));
  text_layer_set_background_color(s_textlayer_4, GColorClear);
  text_layer_set_text_color(s_textlayer_4, GColorWhite);
  text_layer_set_text(s_textlayer_4, "Single Click:");
  text_layer_set_text_alignment(s_textlayer_4, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_4);
  
  // s_textlayer_5
  s_textlayer_5 = text_layer_create(GRect(63, 25, 82, 19));
  text_layer_set_background_color(s_textlayer_5, GColorClear);
  text_layer_set_text_color(s_textlayer_5, GColorWhite);
  text_layer_set_text(s_textlayer_5, "Alarms On/Off ");
  text_layer_set_text_alignment(s_textlayer_5, GTextAlignmentRight);
  text_layer_set_font(s_textlayer_5, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_5);
  
  // s_textlayer_6
  s_textlayer_6 = text_layer_create(GRect(94, 38, 48, 18));
  text_layer_set_background_color(s_textlayer_6, GColorClear);
  text_layer_set_text_color(s_textlayer_6, GColorWhite);
  text_layer_set_text(s_textlayer_6, "Hold:");
  text_layer_set_text_alignment(s_textlayer_6, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_6);
  
  // s_textlayer_7
  s_textlayer_7 = text_layer_create(GRect(83, 50, 62, 32));
  text_layer_set_background_color(s_textlayer_7, GColorClear);
  text_layer_set_text_color(s_textlayer_7, GColorWhite);
  text_layer_set_text(s_textlayer_7, "Skip Next Alarm ");
  text_layer_set_text_alignment(s_textlayer_7, GTextAlignmentRight);
  text_layer_set_font(s_textlayer_7, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_7);
  
  // s_textlayer_8
  s_textlayer_8 = text_layer_create(GRect(61, 149, 81, 16));
  text_layer_set_background_color(s_textlayer_8, GColorClear);
  text_layer_set_text_color(s_textlayer_8, GColorWhite);
  text_layer_set_text(s_textlayer_8, "Settings -->");
  text_layer_set_text_alignment(s_textlayer_8, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_8);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_textlayer_1);
  text_layer_destroy(s_textlayer_2);
  text_layer_destroy(s_textlayer_3);
  text_layer_destroy(s_textlayer_4);
  text_layer_destroy(s_textlayer_5);
  text_layer_destroy(s_textlayer_6);
  text_layer_destroy(s_textlayer_7);
  text_layer_destroy(s_textlayer_8);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void hide_delay(void *data) {
  hide_mainbuttoninfo();
}

void show_mainbuttoninfo(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  
  app_timer_register(10000, hide_delay, NULL);
}

void hide_mainbuttoninfo(void) {
  window_stack_remove(s_window, true);
}
