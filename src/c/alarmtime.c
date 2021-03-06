#include "alarmtime.h"
#include "common.h"
#include "commonwin.h"
#include <pebble.h>

// Screen for setting alarm times

#define LEN_HOUR 3
#define LEN_MIN 3
#define MAX_TITLE 16

// Enum for indicating which alarm time part is selected
enum part {HOUR, MINUTE};
  
static int8_t s_day;
static uint8_t s_hour;
static uint8_t s_minute;
static char s_hourstr[LEN_HOUR];
static char s_minutestr[LEN_MIN];
static char s_alarmtitle[MAX_TITLE];
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
  GRect bounds = layer_get_bounds(layer); 
  
  graphics_context_set_text_color(ctx, GColorWhite);
  // Draw title
  graphics_draw_text(ctx, s_alarmtitle, s_res_gothic_18_bold, GRect(3+PBL_IF_ROUND_ELSE(10, 0), (bounds.size.h/2)-56, bounds.size.w-6, 37), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Draw separater
  graphics_draw_text(ctx, ":", s_res_bitham_30_black, GRect((bounds.size.w/2)-6, (bounds.size.h/2)-16, 12, 42), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw AM/PM indicator
  if (!clock_is_24h_style())
    graphics_draw_text(ctx, s_hour >= 12 ? "PM" : "AM", s_res_bitham_30_black, GRect((bounds.size.w/2)-26, (bounds.size.h/2)+25, 52, 37), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Set highlighted component
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect((bounds.size.w/2)-((s_selected == HOUR) ? 54 : -5), (bounds.size.h/2)-16, 48, 36), 0, GCornerNone);
  
  // Draw hour
  graphics_context_set_text_color(ctx, (s_selected == HOUR) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_hourstr, s_res_bitham_30_black, GRect((bounds.size.w/2)-54, (bounds.size.h/2)-16, 48, 36), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw minutes
  graphics_context_set_text_color(ctx, (s_selected == MINUTE) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_minutestr, s_res_bitham_30_black, GRect((bounds.size.w/2)+5, (bounds.size.h/2)-16, 48, 36), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
}

static void initialise_ui(void) {
  
  GRect bounds;
  Layer *root_layer = NULL;
  s_window = window_create_fullscreen(&root_layer, &bounds);
  
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPACTION2);
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWNACTION2);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  // action_layer
  action_layer = actionbar_create(s_window, root_layer, &bounds, s_res_img_upaction, s_res_img_nextaction, s_res_img_downaction);
  
  time_layer = layer_create_with_proc(root_layer, draw_time, 
                                     GRect(0, 0, bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
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
    snprintf(s_hourstr, LEN_HOUR, "%d", s_hour);
  } else {
    snprintf(s_hourstr, LEN_HOUR, "%d", s_hour > 12 ? s_hour - 12 : s_hour == 0 ? 12 : s_hour);
  }
  snprintf(s_minutestr, LEN_MIN, "%.2d", s_minute);
  
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

void show_alarmtime(int8_t day, uint8_t hour, uint8_t minute, AlarmTimeCallBack set_event) {
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
      strncpy(s_alarmtitle, "One-Time Alarm", MAX_TITLE);
      break;
    case -1:
      strncpy(s_alarmtitle, "Alarm Every Day", MAX_TITLE);
      break;
    default:
      dayname(day, daystr, 10);
      snprintf(s_alarmtitle, MAX_TITLE, "%s Alarm", daystr);
      //strncpy(s_alarmtitle, "Alarm", MAX_TITLE);
      break;
  }
  
  update_alarmtime();
  
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_alarmtime(void) {
  window_stack_remove(s_window, true);
}
