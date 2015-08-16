#include <pebble.h>
#include "msg.h"
#include "common.h"

// Simple message window that can be set to auto-hide after a certain time
  
static char s_title[20];
static char s_msg[150];
static AppTimer *s_autohide = NULL;

static Window *s_window;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_18_bold;
static TextLayer *title_layer;
static TextLayer *msg_layer;

static void initialise_ui(void) {
  s_window = window_create();
  IF_2(window_set_fullscreen(s_window, true));
  
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  // title_layer
  title_layer = text_layer_create(GRect(5, -4, 134, 32));
  text_layer_set_text(title_layer, "title_layer");
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_font(title_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)title_layer);
  
  // msg_layer
  msg_layer = text_layer_create(GRect(3, 25, 138, 140));
  text_layer_set_background_color(msg_layer, GColorClear);
  text_layer_set_text(msg_layer, "msg_layer");
  text_layer_set_text_alignment(msg_layer, GTextAlignmentCenter);
  text_layer_set_font(msg_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)msg_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  s_window = NULL;
  text_layer_destroy(title_layer);
  text_layer_destroy(msg_layer);
}

static void cancel_autohide(void) {
  if (s_autohide != NULL) {
    app_timer_cancel(s_autohide);
    s_autohide = NULL;
  }
}

static void handle_window_unload(Window* window) {
  cancel_autohide();
  destroy_ui();
}

static void auto_hide(void *data) {
  s_autohide = NULL;
  hide_msg();
}

// Show or update window with the given title, message, and seconds to hide the message after (0 means no auto-hide)
void show_msg(char *title, char *msg, int hide_after, bool vibe) {
  
  if (s_window == NULL) {
    initialise_ui();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .unload = handle_window_unload,
    });
  }
  
  strncpy(s_title, title, sizeof(s_title));
  s_msg[sizeof(s_title)-1] = '\0';
  text_layer_set_text(title_layer, s_title);
  strncpy(s_msg, msg, sizeof(s_msg));
  s_msg[sizeof(s_msg)-1] = '\0';
  text_layer_set_text(msg_layer, s_msg);
  
  window_stack_push(s_window, true);
  
  if (vibe) vibes_long_pulse();
  
  // Set auto-hide timer
  if (hide_after == 0) {
    cancel_autohide();
  } else {
    if (s_autohide != NULL)
      app_timer_reschedule(s_autohide, hide_after * 1000);
    else
      app_timer_register(hide_after * 1000, auto_hide, NULL);
  }
}

void hide_msg(void) {
  cancel_autohide();
  if (s_window != NULL && window_stack_contains_window(s_window)) window_stack_remove(s_window, true);
}