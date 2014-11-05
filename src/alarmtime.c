#include "alarmtime.h"
#include <pebble.h>

enum part {HOUR, MINUTE};
  
static int m_day;
static int m_hour;
static int m_minute;
static char s_hour[3];
static char s_minute[3];
static AlarmTimeCallBack s_set_event;
static enum part s_selected = HOUR;
  
// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_nextaction;
static GBitmap *s_res_img_downaction;
static GFont s_res_gothic_18_bold;
static GFont s_res_bitham_30_black;
static ActionBarLayer *action_layer;
static TextLayer *day_layer;
static TextLayer *hour_layer;
static TextLayer *sep_layer;
static TextLayer *minute_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_UPACTION);
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWNACTION);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorBlack);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_upaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_SELECT, s_res_img_nextaction);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_downaction);
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
  
  // day_layer
  day_layer = text_layer_create(GRect(3, 11, 117, 37));
  text_layer_set_text(day_layer, "Wednesday Alarm");
  text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);
  text_layer_set_font(day_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)day_layer);
  
  // hour_layer
  hour_layer = text_layer_create(GRect(8, 58, 48, 36));
  text_layer_set_background_color(hour_layer, GColorBlack);
  text_layer_set_text_color(hour_layer, GColorWhite);
  text_layer_set_text(hour_layer, "22");
  text_layer_set_text_alignment(hour_layer, GTextAlignmentCenter);
  text_layer_set_font(hour_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)hour_layer);
  
  // sep_layer
  sep_layer = text_layer_create(GRect(56, 58, 12, 42));
  text_layer_set_background_color(sep_layer, GColorClear);
  text_layer_set_text(sep_layer, ":");
  text_layer_set_text_alignment(sep_layer, GTextAlignmentCenter);
  text_layer_set_font(sep_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)sep_layer);
  
  // minute_layer
  minute_layer = text_layer_create(GRect(67, 58, 48, 36));
  text_layer_set_text(minute_layer, "55");
  text_layer_set_text_alignment(minute_layer, GTextAlignmentCenter);
  text_layer_set_font(minute_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)minute_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_layer);
  text_layer_destroy(day_layer);
  text_layer_destroy(hour_layer);
  text_layer_destroy(sep_layer);
  text_layer_destroy(minute_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_nextaction);
  gbitmap_destroy(s_res_img_downaction);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void update_alarmtime() {
  
  snprintf(s_hour, sizeof(s_hour), "%d", m_hour);
  text_layer_set_text(hour_layer, s_hour);
  snprintf(s_minute, sizeof(s_minute), "%.2d", m_minute);
  text_layer_set_text(minute_layer, s_minute);
  
  if (s_selected == HOUR) {
    text_layer_set_background_color(hour_layer, GColorBlack);
    text_layer_set_text_color(hour_layer, GColorWhite);
    text_layer_set_background_color(minute_layer, GColorWhite);
    text_layer_set_text_color(minute_layer, GColorBlack);
  } else {
    text_layer_set_background_color(hour_layer, GColorWhite);
    text_layer_set_text_color(hour_layer, GColorBlack);
    text_layer_set_background_color(minute_layer, GColorBlack);
    text_layer_set_text_color(minute_layer, GColorWhite);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    s_selected = MINUTE;
    update_alarmtime();
  } else {
    hide_alarmtime();
    s_set_event(m_day, m_hour, m_minute);
  }
  
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    ++m_hour;
    m_hour %= 24;
  } else {
    ++m_minute;
    m_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    m_hour += 23;
    m_hour %= 24;
  } else {
    m_minute += 59;
    m_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

void show_alarmtime(int day, int hour, int minute, AlarmTimeCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_selected = HOUR;
  m_day = day;
  m_hour = hour;
  m_minute = minute;
  s_set_event = set_event;
  
  switch (day) {
    case -1:
      text_layer_set_text(day_layer, "Alarm Every Day");
      break;
    case 0:
      text_layer_set_text(day_layer, "Sunday Alarm");
      break;
    case 1:
      text_layer_set_text(day_layer, "Monday Alarm");
      break;
    case 2:
      text_layer_set_text(day_layer, "Tuesday Alarm");
      break;
    case 3:
      text_layer_set_text(day_layer, "Wednesday Alarm");
      break;
    case 4:
      text_layer_set_text(day_layer, "Thursday Alarm");
      break;
    case 5:
      text_layer_set_text(day_layer, "Friday Alarm");
      break;
    case 6:
      text_layer_set_text(day_layer, "Saturday Alarm");
      break;
    default:
      text_layer_set_text(day_layer, "");
  }
  
  update_alarmtime();
  
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_alarmtime(void) {
  window_stack_remove(s_window, true);
}
