#include "common.h"
#include "periodset.h"
#include <pebble.h>

// Screen for setting a time period in minutes
  
static int s_minutes = 0;
static int s_min_minutes = 0;
static int s_max_minutes = 60;
static char s_minute_str[3];
static PeriodSetCallBack s_set_event;

static Window *s_window;
static GFont s_res_bitham_30_black;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_24_bold;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_okaction;
static GBitmap *s_res_img_downaction;
static TextLayer *minutes_layer;
static TextLayer *unit_layer;
static TextLayer *title_layer;
static ActionBarLayer *action_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  IF_A(window_set_fullscreen(s_window, false));
  
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_okaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_OKACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  // minutes_layer
  minutes_layer = text_layer_create(GRect(40, 63, 42, 36));
  text_layer_set_background_color(minutes_layer, GColorClear);
  text_layer_set_text_color(minutes_layer, GColorWhite);
  text_layer_set_text(minutes_layer, "10");
  text_layer_set_text_alignment(minutes_layer, GTextAlignmentCenter);
  text_layer_set_font(minutes_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)minutes_layer);
  
  // unit_layer
  unit_layer = text_layer_create(GRect(22, 97, 79, 31));
  text_layer_set_background_color(unit_layer, GColorClear);
  text_layer_set_text_color(unit_layer, GColorWhite);
  text_layer_set_text(unit_layer, "Minutes");
  text_layer_set_text_alignment(unit_layer, GTextAlignmentCenter);
  text_layer_set_font(unit_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)unit_layer);
  
  // title_layer
  title_layer = text_layer_create(GRect(2, 12, 120, 49));
  text_layer_set_background_color(title_layer, GColorClear);
  text_layer_set_text_color(title_layer, GColorWhite);
  text_layer_set_text(title_layer, "Snooze Delay");
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_font(title_layer, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)title_layer);
  
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorWhite);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_SELECT, s_res_img_okaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_downaction);
  layer_set_frame(action_bar_layer_get_layer(action_layer), GRect(124, 0, 20, 168));
  IF_B(layer_set_bounds(action_bar_layer_get_layer(action_layer), GRect(-5, 0, 30, 168)));
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(minutes_layer);
  text_layer_destroy(unit_layer);
  text_layer_destroy(title_layer);
  action_bar_layer_destroy(action_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_okaction);
  gbitmap_destroy(s_res_img_downaction);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void update_minutes() {
  snprintf(s_minute_str, sizeof(s_minute_str), "%d", s_minutes);
  text_layer_set_text(minutes_layer, s_minute_str);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Close this screen
  hide_periodset();
  // Pass back minutes value
  s_set_event(s_minutes);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (++s_minutes > s_max_minutes) s_minutes = s_min_minutes;
  update_minutes();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (--s_minutes < s_min_minutes) s_minutes = s_max_minutes;
  update_minutes();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

// Show the time period screen with a title, current # of mininutes, min and max limits, 
// and a call back function that is called when the select button is pressed
void show_periodset(char *title, int minutes, int min_minutes, int max_minutes, PeriodSetCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Set the title (assumes char passed in is set statically)
  text_layer_set_text(title_layer, title);
  
  // Store all the parameters passed in and show the current value
  s_minutes = minutes;
  s_min_minutes = min_minutes;
  s_max_minutes = max_minutes;
  update_minutes();
  
  // Store pointer to callback function
  s_set_event = set_event;
  
  // Trap action bar clicks
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_periodset(void) {
  window_stack_remove(s_window, true);
}
