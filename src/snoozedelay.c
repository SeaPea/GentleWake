#include "snoozedelay.h"
#include <pebble.h>

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_bitham_30_black;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_24_bold;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_okaction;
static GBitmap *s_res_img_downaction;
static TextLayer *minutes_layer;
static TextLayer *s_textlayer_1;
static TextLayer *s_textlayer_2;
static ActionBarLayer *action_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, 0);
  
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_okaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_OKACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  // minutes_layer
  minutes_layer = text_layer_create(GRect(43, 63, 37, 36));
  text_layer_set_text(minutes_layer, "10");
  text_layer_set_text_alignment(minutes_layer, GTextAlignmentCenter);
  text_layer_set_font(minutes_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)minutes_layer);
  
  // s_textlayer_1
  s_textlayer_1 = text_layer_create(GRect(26, 97, 79, 31));
  text_layer_set_text(s_textlayer_1, "Minutes");
  text_layer_set_font(s_textlayer_1, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_1);
  
  // s_textlayer_2
  s_textlayer_2 = text_layer_create(GRect(10, 22, 102, 31));
  text_layer_set_text(s_textlayer_2, "Snooze Delay");
  text_layer_set_font(s_textlayer_2, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_2);
  
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorBlack);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_SELECT, s_res_img_okaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_downaction);
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(minutes_layer);
  text_layer_destroy(s_textlayer_1);
  text_layer_destroy(s_textlayer_2);
  action_bar_layer_destroy(action_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_okaction);
  gbitmap_destroy(s_res_img_downaction);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_snoozedelay(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
}

void hide_snoozedelay(void) {
  window_stack_remove(s_window, true);
}
