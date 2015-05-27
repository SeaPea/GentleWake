#include "mainwin.h"
#include "common.h"

enum onoff_modes {
  MODE_OFF,
  MODE_ON,
  MODE_ACTIVE
};
  
static char current_time[] = "00:00";
static bool s_alarms_on;
static char s_info[40];
static char s_onoff_text[40];
static enum onoff_modes s_onoff_mode;

static GBitmap *s_res_img_snooze;

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

static void draw_onoff(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor border_color;
  GColor fill_color;
#ifdef PBL_COLOR
  switch (s_onoff_mode) {
    case MODE_OFF:
      border_color = GColorRed;
      fill_color = GColorMelon;
      break;
    case MODE_ON:
      border_color = GColorJaegerGreen;
      fill_color = GColorMintGreen;
      break;
    case MODE_ACTIVE:
      border_color = GColorChromeYellow;
      fill_color = GColorPastelYellow;
      break;
  }
#else
  border_color = GColorWhite;
  fill_color = GColorBlack;
#endif
  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 8, GCornersAll);
  IF_B(graphics_context_set_stroke_width(ctx, 3)); 
  graphics_context_set_stroke_color(ctx, border_color);
  graphics_draw_round_rect(ctx, layer_get_bounds(layer), 8);
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(GColorBlack, GColorWhite));
  GSize text_size = graphics_text_layout_get_content_size(s_onoff_text, s_res_gothic_18_bold, 
                                                          GRect(5, 5, bounds.size.w-10, bounds.size.h-10), 
                                                          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  graphics_draw_text(ctx, s_onoff_text, s_res_gothic_18_bold, 
                     GRect(5, ((bounds.size.h-text_size.h)/2)-4, bounds.size.w-10, text_size.h), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_info(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorPictonBlue, GColorBlack));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 8, GCornersAll);
  IF_B(graphics_context_set_stroke_width(ctx, 3)); 
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorBlueMoon, GColorWhite));
  graphics_draw_round_rect(ctx, layer_get_bounds(layer), 8);
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(GColorBlack, GColorWhite));
  GSize text_size = graphics_text_layout_get_content_size(s_info, s_res_gothic_18_bold, 
                                                          GRect(5, 5, bounds.size.w-10, bounds.size.h-10), 
                                                          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  graphics_draw_text(ctx, s_info, s_res_gothic_18_bold, 
                     GRect(5, ((bounds.size.h-text_size.h)/2)-4, bounds.size.w-10, text_size.h), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  IF_A(window_set_fullscreen(s_window, true));
  
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
  layer_set_frame(action_bar_layer_get_layer(action_layer), GRect(124, 0, 20, 168));
  IF_B(layer_set_bounds(action_bar_layer_get_layer(action_layer), GRect(-5, 0, 30, 168)));
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_layer);
  
  // clockbg_layer
  clockbg_layer = text_layer_create(GRect(0, 60, 144, 50));
  text_layer_set_background_color(clockbg_layer, GColorBlack);
  text_layer_set_text_color(clockbg_layer, GColorClear);
  text_layer_set_text(clockbg_layer, "    ");
  layer_add_child(window_get_root_layer(s_window), (Layer *)clockbg_layer);
  
  // clock_layer
  clock_layer = text_layer_create(GRect(0, 52, 144, 65));
  text_layer_set_background_color(clock_layer, GColorClear);
  text_layer_set_text_color(clock_layer, GColorWhite);
  text_layer_set_text(clock_layer, "23:55");
  text_layer_set_text_alignment(clock_layer, GTextAlignmentCenter);
  text_layer_set_font(clock_layer, s_res_roboto_bold_subset_49);
  layer_add_child(window_get_root_layer(s_window), (Layer *)clock_layer);
  
  // onoff_layer
  onoff_layer = text_layer_create(GRect(2, 2, 119, 56));
  layer_set_update_proc(text_layer_get_layer(onoff_layer), draw_onoff); 
  layer_add_child(window_get_root_layer(s_window), (Layer *)onoff_layer);
  
  // info_layer
  info_layer = text_layer_create(GRect(2, 110, 119, 56));
  layer_set_update_proc(text_layer_get_layer(info_layer), draw_info); 
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

static void set_onoff_text(const char *onoff_text) {
  strncpy(s_onoff_text, onoff_text, sizeof(s_onoff_text));
  layer_mark_dirty(text_layer_get_layer(onoff_layer));
}

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
    s_onoff_mode = MODE_ON;
    set_onoff_text("Alarms Enabled");
    layer_set_hidden(text_layer_get_layer(info_layer), false);
  } else {
    s_onoff_mode = MODE_OFF;
    set_onoff_text("Alarms DISABLED");
    layer_set_hidden(text_layer_get_layer(info_layer), true);
  }
}

void update_info(char* text) {
  strncpy(s_info, text, sizeof(s_info));
  layer_mark_dirty(text_layer_get_layer(info_layer));
}

void show_alarm_ui(bool on) {
  if (on) {
    s_onoff_mode = MODE_ACTIVE;
    set_onoff_text("WAKEY! WAKEY!");
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
  s_onoff_mode = MODE_ACTIVE;
  set_onoff_text("SNOOZING");
  
  struct tm *t = localtime(&snooze_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  char info[40];
  snprintf(info, sizeof(info), "Until: %s\n2 clicks to stop", time_str);
  update_info(info);
  
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_snooze);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_snooze);
}

void show_monitoring(time_t alarm_time) {
  s_onoff_mode = MODE_ACTIVE;
  set_onoff_text("SMART ALARM ACTIVE");
  
  struct tm *t = localtime(&alarm_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  char info[40];
  snprintf(info, sizeof(info), "Alarm: %s\n2 clicks to stop", time_str);
  update_info(info);
  
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
