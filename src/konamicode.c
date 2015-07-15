#include <pebble.h>
#include "common.h"
#include "konamicode.h"

// Screen for displaying and receiving a random sequence of button presses like
// the 'Konami Code' for stopping an active alarm
  
static Window *s_window;
static GBitmap *s_res_img_nextaction;
static GBitmap *s_res_image_upaction2;
static GBitmap *s_res_image_downaction2;
static GBitmap *s_img_up_unsel;
static GBitmap *s_img_up_sel;
static GBitmap *s_img_right_unsel;
static GBitmap *s_img_right_sel;
static GBitmap *s_img_down_unsel;
static GBitmap *s_img_down_sel;
static GFont s_res_gothic_24;
static GFont s_res_gothic_14;
static ActionBarLayer *s_actionbarlayer;
static TextLayer *s_textlayer_backbutton;
static Layer *s_layer_border;
static TextLayer *s_textlayer_title;
static TextLayer *s_textlayer_instructions;
static BitmapLayer *s_bitmaplayer_1;
static BitmapLayer *s_bitmaplayer_2;
static BitmapLayer *s_bitmaplayer_3;
static BitmapLayer *s_bitmaplayer_4;
static BitmapLayer *s_bitmaplayer_5;

static AppTimer *s_tmr_close = NULL;
static CodeSuccessCallBack s_success_event = NULL;

enum KonamiCodes {
  KC_Up = 0,
  KC_Right = 1,
  KC_Down = 2
};

static enum KonamiCodes s_konami_sequence[5];
static uint8_t s_current_code = 0;

// Generates a random sequence of button presses and stores it in a static array
static void gen_konami_sequence() {
  
  srand(time(NULL));
  
  for (int i = 0; i < 5; i++) {
    s_konami_sequence[i] = rand() % 3;
  }
  
}

// Gets the matching bitmap for a button code and whether it should show as selected (successfully pressed)
static GBitmap* get_code_img(enum KonamiCodes code, bool selected) {
  switch (code) {
    case KC_Up:
      return (selected) ? s_img_up_sel : s_img_up_unsel;
      break;
    case KC_Right:
      return (selected) ? s_img_right_sel : s_img_right_unsel;
      break;
    case KC_Down:
      return (selected) ? s_img_down_sel : s_img_down_unsel;
      break;
    default:
      return NULL;
      break;
  }
}

// (Re)initialize the bitmaps that show which buttons to press
static void init_konami_bitmaps() {
  bitmap_layer_set_bitmap(s_bitmaplayer_1, get_code_img(s_konami_sequence[0], false));
  bitmap_layer_set_bitmap(s_bitmaplayer_2, get_code_img(s_konami_sequence[1], false));
  bitmap_layer_set_bitmap(s_bitmaplayer_3, get_code_img(s_konami_sequence[2], false));
  bitmap_layer_set_bitmap(s_bitmaplayer_4, get_code_img(s_konami_sequence[3], false));
  bitmap_layer_set_bitmap(s_bitmaplayer_5, get_code_img(s_konami_sequence[4], false));
}

// Changes a code (button press) in the sequence to show as selected/succesfully pressed
static void set_code_selected(uint8_t code_idx) {
  switch (code_idx) {
    case 0:
      bitmap_layer_set_bitmap(s_bitmaplayer_1, get_code_img(s_konami_sequence[0], true));
      break;
    case 1:
      bitmap_layer_set_bitmap(s_bitmaplayer_2, get_code_img(s_konami_sequence[1], true));
      break;
    case 2:
      bitmap_layer_set_bitmap(s_bitmaplayer_3, get_code_img(s_konami_sequence[2], true));
      break;
    case 3:
      bitmap_layer_set_bitmap(s_bitmaplayer_4, get_code_img(s_konami_sequence[3], true));
      break;
    case 4:
      bitmap_layer_set_bitmap(s_bitmaplayer_5, get_code_img(s_konami_sequence[4], true));
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

// Draw a border around the Konami Code instructions
static void draw_border(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  GRect layer_rect = layer_get_bounds(layer);
  graphics_draw_round_rect(ctx, GRect(0, 0, layer_rect.size.w, layer_rect.size.h), 8);
}

// Initialize all the window UI elements
static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, true);
  #endif
  
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_image_upaction2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPACTION2);
  s_res_image_downaction2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWNACTION2);
  s_img_up_unsel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_UNSEL);
  s_img_up_sel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_SEL);
  s_img_right_unsel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RIGHT_UNSEL);
  s_img_right_sel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RIGHT_SEL);
  s_img_down_unsel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_UNSEL);
  s_img_down_sel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_SEL);
  s_res_gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorBlack);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, s_res_image_upaction2);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, s_res_img_nextaction);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, s_res_image_downaction2);
  layer_set_frame(action_bar_layer_get_layer(s_actionbarlayer), GRect(124, 0, 20, 168));
  IF_B(layer_set_bounds(action_bar_layer_get_layer(s_actionbarlayer), GRect(-5, 0, 30, 168)));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer);
  
  // s_textlayer_backbutton
  s_textlayer_backbutton = text_layer_create(GRect(1, 4, 121, 45));
  text_layer_set_background_color(s_textlayer_backbutton, GColorClear);
  text_layer_set_text_color(s_textlayer_backbutton, GColorWhite);
  text_layer_set_text(s_textlayer_backbutton, "<- HOLD Back button to exit WITHOUT stopping alarm");
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_backbutton);
  
  // s_layer_border
  s_layer_border = layer_create(GRect(2, 50, 120, 115));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_layer_border);
  layer_set_update_proc(s_layer_border, draw_border);
  
  // s_textlayer_title
  s_textlayer_title = text_layer_create(GRect(3, 45, 119, 49));
  text_layer_set_background_color(s_textlayer_title, GColorClear);
  text_layer_set_text_color(s_textlayer_title, GColorWhite);
  text_layer_set_text(s_textlayer_title, "Random Konami Code");
  text_layer_set_text_alignment(s_textlayer_title, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_title, s_res_gothic_24);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_title);
  
  // s_textlayer_instructions
  s_textlayer_instructions = text_layer_create(GRect(5, 94, 114, 43));
  text_layer_set_background_color(s_textlayer_instructions, GColorClear);
  text_layer_set_text_color(s_textlayer_instructions, GColorWhite);
  text_layer_set_text(s_textlayer_instructions, "Press the buttons in the order below to stop the alarm");
  text_layer_set_text_alignment(s_textlayer_instructions, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_instructions, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_instructions);
  
  // s_bitmaplayer_1
  s_bitmaplayer_1 = bitmap_layer_create(GRect(7, 141, 18, 18));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmaplayer_1);
  
  // s_bitmaplayer_2
  s_bitmaplayer_2 = bitmap_layer_create(GRect(30, 141, 18, 18));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmaplayer_2);
  
  // s_bitmaplayer_3
  s_bitmaplayer_3 = bitmap_layer_create(GRect(53, 141, 18, 18));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmaplayer_3);
  
  // s_bitmaplayer_4
  s_bitmaplayer_4 = bitmap_layer_create(GRect(76, 141, 18, 18));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmaplayer_4);
  
  // s_bitmaplayer_5
  s_bitmaplayer_5 = bitmap_layer_create(GRect(99, 141, 18, 18));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmaplayer_5);
  
  // Setup random konami code sequence
  s_current_code = 0;
  gen_konami_sequence();
  init_konami_bitmaps();
}

// Free memory from all the UI elements
static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer);
  text_layer_destroy(s_textlayer_backbutton);
  layer_destroy(s_layer_border);
  text_layer_destroy(s_textlayer_title);
  text_layer_destroy(s_textlayer_instructions);
  bitmap_layer_destroy(s_bitmaplayer_1);
  bitmap_layer_destroy(s_bitmaplayer_2);
  bitmap_layer_destroy(s_bitmaplayer_3);
  bitmap_layer_destroy(s_bitmaplayer_4);
  bitmap_layer_destroy(s_bitmaplayer_5);
  gbitmap_destroy(s_res_img_nextaction);
  gbitmap_destroy(s_res_image_upaction2);
  gbitmap_destroy(s_res_image_downaction2);
  gbitmap_destroy(s_img_up_unsel);
  gbitmap_destroy(s_img_up_sel);
  gbitmap_destroy(s_img_right_unsel);
  gbitmap_destroy(s_img_right_sel);
  gbitmap_destroy(s_img_down_unsel);
  gbitmap_destroy(s_img_down_sel);
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
      set_code_selected(s_current_code);
      s_current_code++;
      // Reset inactivity timer
      reset_close_timer();
    }
  } else {
    // Wrong code entered, reset everything to go back to the start
    s_current_code = 0;
    vibes_long_pulse();
    init_konami_bitmaps();
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
  if (s_tmr_close != NULL) {
    app_timer_cancel(s_tmr_close);
    s_tmr_close = NULL;
  }
  window_stack_remove(s_window, true);
}

