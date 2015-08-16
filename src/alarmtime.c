#include "alarmtime.h"
#include "common.h"
#include <pebble.h>

// Screen for setting alarm times

// Enum for indicating which alarm time part is selected
enum part {HOUR, MINUTE};
  
static int s_day;
static int s_hour;
static int s_minute;
static char s_hourstr[3];
static char s_minutestr[3];
static char s_alarmtitle[16];
static enum part s_selected = HOUR;
static AlarmTimeCallBack s_set_event;

static Window *s_window;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_nextaction;
static GBitmap *s_res_img_downaction;
static GFont s_res_gothic_18_bold;
static GFont s_res_bitham_30_black;
static ActionBarLayer *action_layer;
static Layer *time_layer;

static void draw_time(Layer *layer, GContext *ctx) {
  graphics_context_set_text_color(ctx, GColorWhite);
  // Draw title
  graphics_draw_text(ctx, s_alarmtitle, s_res_gothic_18_bold, GRect(3, 20, 117, 37), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Draw separater
  graphics_draw_text(ctx, ":", s_res_bitham_30_black, GRect(56, 68, 12, 42), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw AM/PM indicator
  if (!clock_is_24h_style())
    graphics_draw_text(ctx, s_hour >= 12 ? "PM" : "AM", s_res_bitham_30_black, GRect(36, 99, 52, 37), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Set highlighted component
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect((s_selected == HOUR) ? 8 : 67, 68, 48, 36), 0, GCornerNone);
  
  // Draw hour
  graphics_context_set_text_color(ctx, (s_selected == HOUR) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_hourstr, s_res_bitham_30_black, GRect(8, 68, 48, 36), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw minutes
  graphics_context_set_text_color(ctx, (s_selected == MINUTE) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_minutestr, s_res_bitham_30_black, GRect(67, 68, 48, 36), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
}

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  IF_2(window_set_fullscreen(s_window, true));
  
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorWhite);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_SELECT, s_res_img_nextaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_downaction);
  layer_set_frame(action_bar_layer_get_layer(action_layer), GRect(124, 0, 20, 168));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(action_layer), GRect(-5, 0, 30, 168)));
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
  
  time_layer = layer_create(GRect(0, 0, 124, 168));
  layer_set_update_proc(time_layer, draw_time); 
  layer_add_child(window_get_root_layer(s_window), time_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_layer);
  layer_destroy(time_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_nextaction);
  gbitmap_destroy(s_res_img_downaction);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

// Redraws the currently set alarm time
static void update_alarmtime() {
  
  if (clock_is_24h_style()) {
    snprintf(s_hourstr, sizeof(s_hourstr), "%d", s_hour);
  } else {
    snprintf(s_hourstr, sizeof(s_hourstr), "%d", s_hour > 12 ? s_hour - 12 : s_hour == 0 ? 12 : s_hour);
  }
  snprintf(s_minutestr, sizeof(s_minutestr), "%.2d", s_minute);
  
  layer_mark_dirty(time_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Move to the minute part
    s_selected = MINUTE;
    update_alarmtime();
  } else {
    // Close this screen
    hide_alarmtime();
    // Pass the alarm day and time back
    s_set_event(s_day, s_hour, s_minute);
  }
  
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Increment hour (wrap around)
    s_hour++;
    s_hour %= 24;
  } else {
    // Increment minute (wrap around)
    s_minute++;
    s_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Decrement hour (wrap around)
    s_hour += 23;
    s_hour %= 24;
  } else {
    // Decrement minute (wrap around)
    s_minute += 59;
    s_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

void show_alarmtime(int day, int hour, int minute, AlarmTimeCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_selected = HOUR;
  // Store the passed in parameters
  s_day = day;
  s_hour = hour;
  s_minute = minute;
  
  // Store pointer to callback for when done
  s_set_event = set_event;
  
  char daystr[10];
  
  // Generate alarm screen time
  switch (day) {
    case -2:
      strcpy(s_alarmtitle, "One-Time Alarm");
      break;
    case -1:
      strcpy(s_alarmtitle, "Alarm Every Day");
      break;
    default:
      dayname(day, daystr, sizeof(daystr));
      snprintf(s_alarmtitle, sizeof(s_alarmtitle), "%s Alarm", daystr);
  }
  
  update_alarmtime();
  
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_alarmtime(void) {
  window_stack_remove(s_window, true);
}
