#include <pebble.h>
#include "skipwin.h"
#include "common.h"

static SkipSetCallBack s_set_event;
static time_t s_skip_until = 0;
static char s_date[12];
  
static Window *s_window;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_okaction;
static GBitmap *s_res_img_downaction;
static GFont s_res_gothic_24;
static GFont s_res_gothic_28;
static ActionBarLayer *s_actionbarlayer;
static TextLayer *s_textlayer_info;
static TextLayer *s_textlayer_date;
static TextLayer *s_textlayer_status;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  IF_2(window_set_fullscreen(s_window, true));
  
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_okaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_OKACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  s_res_gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_res_gothic_28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorWhite);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, s_res_img_okaction);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, s_res_img_downaction);
  layer_set_frame(action_bar_layer_get_layer(s_actionbarlayer), GRect(124, 0, 20, 168));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(s_actionbarlayer), GRect(-5, 0, 30, 168)));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer);
  
  // s_textlayer_info
  s_textlayer_info = text_layer_create(GRect(7, 25, 117, 32));
  text_layer_set_background_color(s_textlayer_info, GColorClear);
  text_layer_set_text_color(s_textlayer_info, GColorWhite);
  text_layer_set_text(s_textlayer_info, "Skip Until:");
  text_layer_set_text_alignment(s_textlayer_info, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_info, s_res_gothic_24);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_info);
  
  // s_textlayer_date
  s_textlayer_date = text_layer_create(GRect(2, 64, 122, 32));
  text_layer_set_background_color(s_textlayer_date, GColorClear);
  text_layer_set_text_color(s_textlayer_date, GColorWhite);
  text_layer_set_text(s_textlayer_date, "Thu, Feb 22");
  text_layer_set_text_alignment(s_textlayer_date, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_date, s_res_gothic_28);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_date);
  
  // s_textlayer_status
  s_textlayer_status = text_layer_create(GRect(7, 100, 117, 32));
  text_layer_set_background_color(s_textlayer_status, GColorClear);
  text_layer_set_text_color(s_textlayer_status, GColorWhite);
  text_layer_set_text(s_textlayer_status, "(No skipping)");
  text_layer_set_text_alignment(s_textlayer_status, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_status, s_res_gothic_24);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_status);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer);
  text_layer_destroy(s_textlayer_info);
  text_layer_destroy(s_textlayer_date);
  text_layer_destroy(s_textlayer_status);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_okaction);
  gbitmap_destroy(s_res_img_downaction);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static time_t get_today() {
  return strip_time(time(NULL) + get_UTC_offset(NULL));
}

static void update_date_display() {
  struct tm *t;
  time_t skip_utc = s_skip_until - get_UTC_offset(NULL);
  t = localtime(&skip_utc);
  strftime(s_date, sizeof(s_date), "%a, %b %d", t);
  text_layer_set_text(s_textlayer_date, s_date);
  layer_set_hidden(text_layer_get_layer(s_textlayer_status), s_skip_until > get_today());
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Close this screen
  hide_skipwin();
  // Pass skip date back (skip date of today or earlier disables skip)
  s_set_event((s_skip_until <= get_today()) ? 0 : s_skip_until);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  time_t today = get_today();
  
  if (s_skip_until <= today)
      // If the skip date was already set to today, wrap around to 4 weeks from now
      s_skip_until = today + (28 * 24 * 60 * 60);
  else
      // Set the date to the previous date
      s_skip_until -= (24 * 60 * 60);
  
  update_date_display();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  time_t today = get_today();
  
  if (day_diff(today, s_skip_until) >= 28)
    // If the skip date is already set to 4 weeks from today, wrap around to today
    s_skip_until = today;
  else
    // Set the date to the next date
    s_skip_until += (24 * 60 * 60);
  
  update_date_display();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

void show_skipwin(time_t skip_until, SkipSetCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Store pointer to callback function
  s_set_event = set_event;
  
  // Trap action bar clicks
  window_set_click_config_provider(s_window, click_config_provider);
  
  time_t today = get_today();
  
  // Validate and store current skip date
  if (skip_until == 0)
    s_skip_until = today;
  else if (skip_until < today)
    s_skip_until = today;
  else
    s_skip_until = strip_time(skip_until);
  
  update_date_display();
  
  window_stack_push(s_window, true);
}

void hide_skipwin(void) {
  window_stack_remove(s_window, true);
}
