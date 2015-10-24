#include <pebble.h>
#include "common.h"
#include "konamicode.h"

// Screen for displaying and receiving a random sequence of button presses like
// the 'Konami Code' for stopping an active alarm
  
static Window *s_window;
static GBitmap *s_res_img_nextaction;
static GBitmap *s_res_image_upaction2;
static GBitmap *s_res_image_downaction2;
static GFont s_res_gothic_24;
static GFont s_res_gothic_14;
static ActionBarLayer *s_actionbarlayer;
static TextLayer *s_textlayer_backbutton;
static Layer *s_layer_code;

static AppTimer *s_tmr_close = NULL;
static CodeSuccessCallBack s_success_event = NULL;

enum KonamiCodes {
  KC_Up = 0,
  KC_Right = 1,
  KC_Down = 2,
  KC_Max = 3
};

static enum KonamiCodes s_konami_sequence[5];
static uint8_t s_current_code = 0;

// Generates a random sequence of button presses and stores it in a static array
static void gen_konami_sequence() {
  
  srand(time(NULL));
  
  // Generate the first code
  s_konami_sequence[0] = rand() % KC_Max;
  
  for (int i = 1; i < 5; i++) {
    // Generate the next code that is always different from the previous
    s_konami_sequence[i] = rand() % (KC_Max - 1);
    if (s_konami_sequence[i] >= s_konami_sequence[i-1]) s_konami_sequence[i]++;
  }
}

// Gets the matching bitmap for a button code and whether it should show as selected (successfully pressed)
static GBitmap* get_code_img(enum KonamiCodes code, bool selected) {
  switch (code) {
    case KC_Up:
      if (selected)
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_SEL);
      else
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_UNSEL);
      break;
    case KC_Right:
      if (selected)
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RIGHT_SEL);
      else
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RIGHT_UNSEL);
      break;
    case KC_Down:
      if (selected)
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_SEL);
      else
        return gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_UNSEL);
      break;
    default:
      return NULL;
      break;
  }
}

// Closes the window after a period of inactivity
static void close_timeout(void *data) {
  s_tmr_close = NULL;
  hide_konamicode();
}

// Resets the timer that closes the window after 10 seconds of inactivity
static void reset_close_timer() {
  if (s_tmr_close == NULL)
    s_tmr_close = app_timer_register(10000, close_timeout, NULL);
  else
    app_timer_reschedule(s_tmr_close, 10000);
}

// Cancels the timer that auto-closes the window
static void cancel_close_timer() {
  if (s_tmr_close != NULL) {
    app_timer_cancel(s_tmr_close);
    s_tmr_close = NULL;
  }
}

// Draw the random Konami Code with instructions
static void draw_code(Layer *layer, GContext *ctx) {
  // Draw border
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GRect layer_rect = layer_get_bounds(layer);
  graphics_draw_round_rect(ctx, GRect(0, 0, layer_rect.size.w, layer_rect.size.h), 8);
  
  graphics_context_set_text_color(ctx, GColorWhite);
  
  // Draw title
  graphics_draw_text(ctx, "Random Konami Code", s_res_gothic_24, GRect(1, -5, 119, 49), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw instructions
  graphics_draw_text(ctx, "Press the buttons in the order below to stop the alarm", s_res_gothic_14, GRect(3, 44, 114, 43), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw codes
  for (uint8_t i = 0; i < 5; i++) {
    GBitmap *img = get_code_img(s_konami_sequence[i], (i < s_current_code));
    if (img != NULL) {
      graphics_draw_bitmap_in_rect(ctx, img, GRect(7 + (i * 23), 91, 18, 18));
      gbitmap_destroy(img);
    }
  }
}

// Initialize all the window UI elements
static void initialise_ui(void) {
  s_window = window_create();
  IF_2(window_set_fullscreen(s_window, true));
  Layer *root_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(root_layer);  
  IF_2(bounds.size.h += 16);
  window_set_background_color(s_window, GColorBlack);
  
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_image_upaction2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPACTION2);
  s_res_image_downaction2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWNACTION2);
  s_res_gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  
  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorBlack);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, s_res_image_upaction2);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, s_res_img_nextaction);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, s_res_image_downaction2);
#ifdef PBL_RECT
  layer_set_frame(action_bar_layer_get_layer(s_actionbarlayer), GRect(bounds.size.w-20, 0, 20, bounds.size.h));
  IF_3(layer_set_bounds(action_bar_layer_get_layer(s_actionbarlayer), GRect(-5, 0, 30, bounds.size.h)));
#endif
  layer_add_child(root_layer, (Layer *)s_actionbarlayer);
  
  // s_textlayer_backbutton
  s_textlayer_backbutton = text_layer_create(GRect(1, 4, bounds.size.w-PBL_IF_RECT_ELSE(ACTION_BAR_WIDTH+1, 0), 45));
  text_layer_set_background_color(s_textlayer_backbutton, GColorClear);
  text_layer_set_text_color(s_textlayer_backbutton, GColorWhite);
  text_layer_set_text(s_textlayer_backbutton, "<- HOLD Back button to exit without stopping alarm");
  layer_add_child(root_layer, (Layer *)s_textlayer_backbutton);
#ifndef PBL_RECT
  text_layer_set_text_alignment(s_textlayer_backbutton, GTextAlignmentCenter);
  text_layer_enable_screen_text_flow_and_paging(s_textlayer_backbutton, 1);
#endif
  
  // Setup random konami code sequence
  s_current_code = 0;
  gen_konami_sequence();
  
  // s_layer_code
  s_layer_code = layer_create(GRect(2+PBL_IF_ROUND_ELSE(25, 0), 50, 120, bounds.size.h-56));
  layer_add_child(root_layer, s_layer_code);
  layer_set_update_proc(s_layer_code, draw_code);
}

// Free memory from all the UI elements
static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer);
  text_layer_destroy(s_textlayer_backbutton);
  layer_destroy(s_layer_code);
  gbitmap_destroy(s_res_img_nextaction);
  gbitmap_destroy(s_res_image_upaction2);
  gbitmap_destroy(s_res_image_downaction2);
}

// Process a button click to determine if it was successful or not
static void process_click(enum KonamiCodes code) {
  if (s_konami_sequence[s_current_code] == code) {
    if (s_current_code == 4) {
      // Successfully entered entire konami code, call callback and close window
      if (s_success_event != NULL) s_success_event();
      vibes_double_pulse();
      hide_konamicode();
    } else {
      // One more successfully entered, move to next code
      s_current_code++;
      layer_mark_dirty(s_layer_code);
      // Reset inactivity timer
      reset_close_timer();
    }
  } else {
    // Wrong code entered, reset everything to go back to the start
    s_current_code = 0;
    layer_mark_dirty(s_layer_code);
    vibes_long_pulse();
    reset_close_timer();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  process_click(KC_Up);
}

static void right_click_handler(ClickRecognizerRef recognizer, void *context) {
  process_click(KC_Right);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  process_click(KC_Down);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, right_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void handle_window_unload(Window* window) {
  cancel_close_timer();
  destroy_ui();
}

// Show the Konami Code window with a method pointer that is called when the entire Konami Code is 
// successfully entered
void show_konamicode(CodeSuccessCallBack callback) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  
  // Store callback for calling later
  s_success_event = callback;
  // Register button click events
  action_bar_layer_set_click_config_provider(s_actionbarlayer, click_config_provider);
  // Start timer to close window after a few seconds of inactivity
  reset_close_timer();
}

// Hide the Konami Code window
void hide_konamicode(void) {
  cancel_close_timer();
  window_stack_remove(s_window, true);
}

