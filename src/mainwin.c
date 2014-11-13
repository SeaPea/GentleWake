#include "mainwin.h"
#include "common.h"
  
static char current_time[] = "00:00";
static bool s_alarms_on;
static char s_info[40];

static GBitmap *s_res_img_snooze;

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_img_standby;
static GBitmap *s_res_img_settings;
static GFont s_res_roboto_bold_subset_49;
static GFont s_res_gothic_18_bold;
static ActionBarLayer *action_layer;
static TextLayer *clockbg_layer;
static TextLayer *clock_layer;
static TextLayer *onoff_layer;
static TextLayer *info_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_fullscreen(s_window, 0);
  
  s_res_img_standby = gbitmap_create_with_resource(RESOURCE_ID_IMG_STANDBY);
  s_res_img_settings = gbitmap_create_with_resource(RESOURCE_ID_IMG_SETTINGS);
  s_res_roboto_bold_subset_49 = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorWhite);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_standby);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_settings);
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
  
  // clockbg_layer
  clockbg_layer = text_layer_create(GRect(2, 52, 143, 46));
  text_layer_set_background_color(clockbg_layer, GColorBlack);
  text_layer_set_text_color(clockbg_layer, GColorClear);
  text_layer_set_text(clockbg_layer, "    ");
  layer_add_child(window_get_root_layer(s_window), (Layer *)clockbg_layer);
  
  // clock_layer
  clock_layer = text_layer_create(GRect(0, 42, 144, 65));
  text_layer_set_background_color(clock_layer, GColorClear);
  text_layer_set_text_color(clock_layer, GColorWhite);
  text_layer_set_text(clock_layer, "23:55");
  text_layer_set_text_alignment(clock_layer, GTextAlignmentCenter);
  text_layer_set_font(clock_layer, s_res_roboto_bold_subset_49);
  layer_add_child(window_get_root_layer(s_window), (Layer *)clock_layer);
  
  // onoff_layer
  onoff_layer = text_layer_create(GRect(3, 17, 119, 36));
  text_layer_set_background_color(onoff_layer, GColorBlack);
  text_layer_set_text_color(onoff_layer, GColorWhite);
  text_layer_set_text(onoff_layer, "SMART ALARM ACTIVE");
  text_layer_set_text_alignment(onoff_layer, GTextAlignmentCenter);
  text_layer_set_font(onoff_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)onoff_layer);
  
  // info_layer
  info_layer = text_layer_create(GRect(0, 102, 121, 43));
  text_layer_set_background_color(info_layer, GColorBlack);
  text_layer_set_text_color(info_layer, GColorWhite);
  text_layer_set_text(info_layer, "Click to snooze Hold any to stop");
  text_layer_set_text_alignment(info_layer, GTextAlignmentCenter);
  text_layer_set_font(info_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)info_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_layer);
  text_layer_destroy(clockbg_layer);
  text_layer_destroy(clock_layer);
  text_layer_destroy(onoff_layer);
  text_layer_destroy(info_layer);
  gbitmap_destroy(s_res_img_standby);
  gbitmap_destroy(s_res_img_settings);
}
// END AUTO-GENERATED UI CODE

void update_clock() {
  clock_copy_time_string(current_time, sizeof(current_time));
  text_layer_set_text(clock_layer, current_time);
}

void init_click_events(ClickConfigProvider click_config_provider) {
  window_set_click_config_provider(s_window, click_config_provider);
}

void update_onoff(bool on) {
  s_alarms_on = on;
  if (on) {
    text_layer_set_text(onoff_layer, "Alarms Enabled");
    layer_set_hidden(text_layer_get_layer(info_layer), false);
  } else {
    text_layer_set_text(onoff_layer, "Alarms DISABLED");
    layer_set_hidden(text_layer_get_layer(info_layer), true);
  }
}

void update_info(char* text) {
  text_layer_set_text(info_layer, text);
}

void show_alarm_ui(bool on) {
  if (on) {
    text_layer_set_text(onoff_layer, "WAKEY! WAKEY!");
    update_info("Click to snooze\n2 clicks to stop ");
    action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_snooze);
    action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_snooze);
  } else {
    update_onoff(s_alarms_on);
    action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_standby);
    action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_settings);
  }
}

void show_snooze(time_t snooze_time) {
  text_layer_set_text(onoff_layer, "SNOOZING");
  
  struct tm *t = localtime(&snooze_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  snprintf(s_info, sizeof(s_info), "Until: %s\n2 clicks to stop", time_str);
  update_info(s_info);
  
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_snooze);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_snooze);
}

void show_monitoring(time_t alarm_time) {
  text_layer_set_text(onoff_layer, "SMART ALARM ACTIVE");
  
  struct tm *t = localtime(&alarm_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  snprintf(s_info, sizeof(s_info), "Alarm: %s\n2 clicks to stop", time_str);
  update_info(s_info);
  
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_snooze);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_snooze);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
  gbitmap_destroy(s_res_img_snooze);
}

void show_mainwin(void) {
  initialise_ui();
  s_res_img_snooze = gbitmap_create_with_resource(RESOURCE_ID_IMG_SNOOZE);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  update_clock();
  
  window_stack_push(s_window, true);
}

void hide_mainwin(void) {
  window_stack_remove(s_window, true);
}
