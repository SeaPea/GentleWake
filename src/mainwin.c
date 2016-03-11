#include "mainwin.h"
#include "common.h"
#include "commonwin.h"

enum onoff_modes {
  MODE_OFF,
  MODE_ON,
  MODE_ACTIVE
};
  
static char current_time[] = "00:00";
static bool s_alarms_on;
static char s_info[45];
static char s_onoff_text[40];
static enum onoff_modes s_onoff_mode;
static uint8_t s_autoclose_timeout = 0;
static AppTimer *s_autoclose_timer = NULL;

static GBitmap *s_res_img_snooze;

static Window *s_window;
static GBitmap *s_res_img_standby;
static GBitmap *s_res_img_settings;
static GFont s_res_roboto_bold_subset_49;
static GFont s_res_gothic_18_bold;
static ActionBarLayer *action_layer;
static Layer *clock_layer;
static Layer *onoff_layer;
static Layer *info_layer;

static void draw_box(Layer *layer, GContext *ctx, GColor border_color, GColor back_color, GColor text_color, char *text) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, back_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), PBL_IF_RECT_ELSE(8, 0), GCornersAll);
  IF_3(graphics_context_set_stroke_width(ctx, 3)); 
  graphics_context_set_stroke_color(ctx, border_color);
  graphics_draw_round_rect(ctx, layer_get_bounds(layer), PBL_IF_RECT_ELSE(8, 0));
  graphics_context_set_text_color(ctx, text_color);
  GSize text_size = graphics_text_layout_get_content_size(text, s_res_gothic_18_bold, 
                                                          GRect(5, 5, bounds.size.w-10, bounds.size.h-2), 
                                                          GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  graphics_draw_text(ctx, text, s_res_gothic_18_bold, 
                     GRect(5, ((bounds.size.h-text_size.h)/2)-4, bounds.size.w-10, text_size.h), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_onoff(Layer *layer, GContext *ctx) {
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
  draw_box(layer, ctx, border_color, fill_color, COLOR_FALLBACK(GColorBlack, GColorWhite), s_onoff_text);
}

static void draw_clock(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
#ifdef PBL_RECT
  // Cover middle section of action bar to give more room for clock
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w-ACTION_BAR_WIDTH, 8, ACTION_BAR_WIDTH, 50), 0, GCornersAll);
#endif
  
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, current_time, s_res_roboto_bold_subset_49, bounds, 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_info(Layer *layer, GContext *ctx) {
  draw_box(layer, ctx, COLOR_FALLBACK(GColorBlueMoon, GColorWhite), COLOR_FALLBACK(GColorPictonBlue, GColorBlack),
          COLOR_FALLBACK(GColorBlack, GColorWhite), s_info);
}

static void initialise_ui(void) {
  
  Layer *root_layer = NULL;
  GRect bounds; 
  s_window = window_create_fullscreen(&root_layer, &bounds);
  
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
#ifdef PBL_RECT
  layer_set_frame(action_bar_layer_get_layer(action_layer), GRect(bounds.size.w-20, 0, 20, bounds.size.h));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(action_layer), GRect(-6, 0, 31, bounds.size.h)));
  // Put Action Bar underneath other layers on rectangular Pebbles
  layer_add_child(root_layer, action_bar_layer_get_layer(action_layer));
#endif
  
  // clock_layer
  clock_layer = layer_create_with_proc(root_layer, draw_clock,
                                       GRect(0 - PBL_IF_RECT_ELSE(0, ACTION_BAR_WIDTH/2), (bounds.size.h/2)-32-PBL_IF_ROUND_ELSE(2, 0), bounds.size.w, 65));
  
  // onoff_layer
  onoff_layer = layer_create_with_proc(root_layer, draw_onoff,
                                      PBL_IF_RECT_ELSE(GRect(2, (bounds.size.h/2)-82, 119, 56),
                                                   GRect(-10, (bounds.size.h/2)-82, bounds.size.w+11, 56)));
  
  // info_layer
  info_layer = layer_create_with_proc(root_layer, draw_info,
                                     PBL_IF_RECT_ELSE(GRect(2, (bounds.size.h/2)+26, 119, 56),
                                                 GRect(-10, (bounds.size.h/2)+24, bounds.size.w+11, 56)));
  
#ifdef PBL_ROUND
  // Put Action Bar on top for Pebble Round
  layer_add_child(root_layer, (Layer *)action_layer);
#endif
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_layer);
  layer_destroy(clock_layer);
  layer_destroy(onoff_layer);
  layer_destroy(info_layer);
  gbitmap_destroy(s_res_img_standby);
  gbitmap_destroy(s_res_img_settings);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
  gbitmap_destroy(s_res_img_snooze);
}

// Handles timer event when app has been idle for X minutes and auto-closes app
static void autoclose_handler(void *data) {
  if (s_autoclose_timer != NULL) {
    s_autoclose_timer = NULL;
    if (s_autoclose_timeout != 0 && window_stack_get_top_window() == s_window) {
      // If autoclose is still enabled and the main window is on top, then exit the app
      window_stack_pop_all(false);
    }
  } 
}

// Stops any active auto-close timer
static void stop_autoclose_timer() {
  if (s_autoclose_timer != NULL) {
    app_timer_cancel(s_autoclose_timer);
    s_autoclose_timer = NULL;
  }
}

// Starts or restarts any acive auto-close timer if auto-close is still enabled
static void restart_autoclose_timer() {
  if (s_autoclose_timeout == 0 || s_onoff_mode == MODE_ACTIVE)
    // Stop auto-close if it has been disabled or an alarm is active
    stop_autoclose_timer();
  else {
    // Start or restart auto-close timer (timeout is stored in minutes)
    if (s_autoclose_timer == NULL)
      s_autoclose_timer = app_timer_register(s_autoclose_timeout * 60 * 1000, autoclose_handler, NULL);
    else
      app_timer_reschedule(s_autoclose_timer, s_autoclose_timeout * 60 * 1000); 
  }
}

static void handle_window_appear(Window* window) {
  restart_autoclose_timer();
}

static void handle_window_disappear(Window* window) {
  stop_autoclose_timer();
}

static void set_onoff_text(const char *onoff_text) {
  strncpy(s_onoff_text, onoff_text, sizeof(s_onoff_text));
  layer_mark_dirty(onoff_layer);
}

// Updates the clock time
void update_clock() {
  clock_copy_time_string(current_time, sizeof(current_time));
  layer_mark_dirty(clock_layer);
}

void init_click_events(ClickConfigProvider click_config_provider) {
  window_set_click_config_provider(s_window, click_config_provider);
}

// Sets the alarms to show as Enabled or Disbaled
void update_onoff(bool on) {
  s_alarms_on = on;
  if (on) {
    s_onoff_mode = MODE_ON;
    set_onoff_text("Alarms Enabled");
    layer_set_hidden(info_layer, false);
  } else {
    s_onoff_mode = MODE_OFF;
    set_onoff_text("Alarms DISABLED");
    layer_set_hidden(info_layer, true);
  }
  restart_autoclose_timer();
}

// Updates the info at the bottom of the main window
void update_info(char* text) {
  strncpy(s_info, text, sizeof(s_info));
  layer_mark_dirty(info_layer);
}

// Updates the timeout period for the auto-close time and restarts the timer if appropriate
void update_autoclose_timeout(uint8_t timeout) {
  s_autoclose_timeout = timeout;
  restart_autoclose_timer();
}

// Updates the UI to show alarm as active or not
void show_alarm_ui(bool on, bool goob) {
  if (on) {
    s_onoff_mode = MODE_ACTIVE;
    stop_autoclose_timer();
    if (goob)
      set_onoff_text("GET UP!");
    else
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

// Update the main window to show snoozing, smart alarm monitoring, or Get Out Of Bed alarm monitoring
void show_status(time_t alarm_time, status_enum status) {
  s_onoff_mode = MODE_ACTIVE;
  stop_autoclose_timer();
  switch (status) {
    case S_Snoozing:
      set_onoff_text("SNOOZING");
      break;
    case S_SmartMonitoring:
      set_onoff_text("SMART ALARM\nACTIVE");
      break;
    case S_GooBMonitoring:
      set_onoff_text("GET OUT OF BED\nMONITORING");
      break;
  }
  
  struct tm *t = localtime(&alarm_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  char info[40];
  snprintf(info, sizeof(info), "%s: %s\n2 clicks to stop", (status == S_Snoozing ? "Until" : "Alarm"), time_str);
  update_info(info);
  
  action_bar_layer_set_icon(action_layer, BUTTON_ID_UP, s_res_img_snooze);
  action_bar_layer_set_icon(action_layer, BUTTON_ID_DOWN, s_res_img_snooze);
}

// Show the main application window
void show_mainwin(uint8_t autoclose_timeout) {
  initialise_ui();
  s_res_img_snooze = gbitmap_create_with_resource(RESOURCE_ID_IMG_SNOOZE);
  s_autoclose_timeout = autoclose_timeout;
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
    .appear = handle_window_appear,
    .disappear = handle_window_disappear
  });
  
  update_clock();
  
  window_stack_push(s_window, true);
}

// Close the main application window
void hide_mainwin(void) {
  window_stack_remove(s_window, true);
}
