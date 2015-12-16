#include "common.h"
#include "periodset.h"
#include <pebble.h>

// Screen for setting a time period in minutes

static PeriodSetCallBack s_set_event;

#ifdef PBL_SDK_2 
// Have to use the built-in NumberWindow for Aplite to reduce the binary size
// (would use it for Basalt with more memory, except it is broken in OS 3)
static NumberWindow *s_num_window = NULL;

static void selected_handler(struct NumberWindow *number_window, void *context) {
  int minutes = number_window_get_value(s_num_window);
  // Close this screen
  hide_periodset();
  // Pass back minutes value
  s_set_event(minutes);
}

// Show the time period screen with a title, current # of mininutes, min and max limits, 
// and a call back function that is called when the select button is pressed
void show_periodset(char *title, int minutes, int min_minutes, int max_minutes, PeriodSetCallBack set_event) {
  if (s_num_window == NULL) {
    s_num_window = number_window_create(title, (NumberWindowCallbacks) {
      .selected = selected_handler
    }, NULL);
  } else {
    number_window_set_label(s_num_window, title);
  }
  
  number_window_set_min(s_num_window, min_minutes);
  number_window_set_max(s_num_window, max_minutes);
  number_window_set_value(s_num_window, minutes);
  number_window_set_step_size(s_num_window, 1);
  
  // Store pointer to callback function
  s_set_event = set_event;
  
  window_stack_push(number_window_get_window(s_num_window), true);
}

void hide_periodset(void) {
  window_stack_remove(number_window_get_window(s_num_window), true);
  unload_periodset();
}

void unload_periodset(void) {
  if (s_num_window != NULL) {
    number_window_destroy(s_num_window);
    s_num_window = NULL;
  }
}
#else
// Create our own NumberWindow for Pebbles as the 
// built-in NumberWindow is broken in OS 3

#define MAX_TITLE 30
#define LEN_PERIOD 3

static char *s_title;
static uint8_t s_minutes = 0;
static uint8_t s_min_minutes = 0;
static uint8_t s_max_minutes = 60;
static char s_minute_str[LEN_PERIOD];

static Window *s_window;
static GFont s_res_bitham_30_black;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_24_bold;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_okaction;
static GBitmap *s_res_img_downaction;
//static TextLayer *minutes_layer;
//static TextLayer *unit_layer;
//static TextLayer *title_layer;
static ActionBarLayer *action_layer;
static Layer *s_period_layer;

static void draw_period(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  
  graphics_context_set_text_color(ctx, GColorWhite);
  // Draw title
  graphics_draw_text(ctx, s_title, s_res_gothic_24_bold, 
                     GRect(2+PBL_IF_ROUND_ELSE(ACTION_BAR_WIDTH/2, 0), (bounds.size.h/2)-70, bounds.size.w-ACTION_BAR_WIDTH-4, 49), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Draw period
  graphics_draw_text(ctx, s_minute_str, s_res_bitham_30_black, 
                     GRect(((bounds.size.w-PBL_IF_RECT_ELSE(ACTION_BAR_WIDTH,0))/2)-21, (bounds.size.h/2)-19, 42, 36), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw units
  graphics_draw_text(ctx, "Minutes", s_res_gothic_28_bold, 
                     GRect(((bounds.size.w-PBL_IF_RECT_ELSE(ACTION_BAR_WIDTH,0))/2)-40, (bounds.size.h/2)+15, 80, 31), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void initialise_ui(void) {
  s_title = malloc(MAX_TITLE);
  
  s_window = window_create();
  IF_2(window_set_fullscreen(s_window, true));
  Layer *root_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(root_layer); 
  window_set_background_color(s_window, GColorBlack);
  IF_2(bounds.size.h += 16);
  
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_okaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_OKACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  
  s_period_layer = layer_create(bounds);
  layer_set_update_proc(s_period_layer, draw_period); 
  layer_add_child(root_layer, s_period_layer);
  
  // minutes_layer
  /*minutes_layer = text_layer_create(GRect(((bounds.size.w-PBL_IF_RECT_ELSE(ACTION_BAR_WIDTH,0))/2)-21, (bounds.size.h/2)-19, 42, 36));
  text_layer_set_background_color(minutes_layer, GColorClear);
  text_layer_set_text_color(minutes_layer, GColorWhite);
  text_layer_set_text(minutes_layer, "10");
  text_layer_set_text_alignment(minutes_layer, GTextAlignmentCenter);
  text_layer_set_font(minutes_layer, s_res_bitham_30_black);
  layer_add_child(root_layer, (Layer *)minutes_layer);
  
  // unit_layer
  unit_layer = text_layer_create(GRect(((bounds.size.w-PBL_IF_RECT_ELSE(ACTION_BAR_WIDTH,0))/2)-40, (bounds.size.h/2)+15, 80, 31));
  text_layer_set_background_color(unit_layer, GColorClear);
  text_layer_set_text_color(unit_layer, GColorWhite);
  text_layer_set_text(unit_layer, "Minutes");
  text_layer_set_text_alignment(unit_layer, GTextAlignmentCenter);
  text_layer_set_font(unit_layer, s_res_gothic_28_bold);
  layer_add_child(root_layer, (Layer *)unit_layer);
  
  // title_layer
  title_layer = text_layer_create(GRect(2+PBL_IF_ROUND_ELSE(ACTION_BAR_WIDTH/2, 0), (bounds.size.h/2)-70, bounds.size.w-ACTION_BAR_WIDTH-4, 49));
  text_layer_set_background_color(title_layer, GColorClear);
  text_layer_set_text_color(title_layer, GColorWhite);
  text_layer_set_text(title_layer, "Snooze Delay");
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_font(title_layer, s_res_gothic_24_bold);
  layer_add_child(root_layer, (Layer *)title_layer);*/
  
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorWhite);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_SELECT, s_res_img_okaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_downaction);
#ifdef PBL_RECT
  layer_set_frame(action_bar_layer_get_layer(action_layer), GRect(bounds.size.w-20, 0, 20, bounds.size.h));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(action_layer), GRect(-5, 0, 30, bounds.size.h)));
#endif
  layer_add_child(root_layer, (Layer *)action_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  layer_destroy(s_period_layer);
  //text_layer_destroy(minutes_layer);
  //text_layer_destroy(unit_layer);
  //text_layer_destroy(title_layer);
  action_bar_layer_destroy(action_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_okaction);
  gbitmap_destroy(s_res_img_downaction);
  
  free(s_title);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void update_minutes() {
  snprintf(s_minute_str, LEN_PERIOD, "%d", s_minutes);
  layer_mark_dirty(s_period_layer);
  //text_layer_set_text(minutes_layer, s_minute_str);
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
void show_periodset(char *title, uint8_t minutes, uint8_t min_minutes, uint8_t max_minutes, PeriodSetCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Set the title (assumes char passed in is set statically)
  //text_layer_set_text(title_layer, title);
  strncpy(s_title, title, MAX_TITLE);
  
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

void unload_periodset(void) {
  // do nothing
}
#endif
