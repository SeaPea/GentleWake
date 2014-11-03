#include "settings.h"
#include "snoozedelay.h"
#include <pebble.h>

#define NUM_MENU_SECTIONS 2
#define NUM_MENU_MISC_ITEMS 2
#define NUM_MENU_ALARM_ITEMS 8
  
// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static MenuLayer *settings_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  // settings_layer
  settings_layer = menu_layer_create(GRect(0, 0, 144, 152));
  menu_layer_set_click_config_onto_window(settings_layer, s_window);
  layer_add_child(window_get_root_layer(s_window), (Layer *)settings_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(settings_layer);
}
// END AUTO-GENERATED UI CODE

// Set menu section count
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Set menu section item counts
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_MENU_MISC_ITEMS;
    case 1:
      return NUM_MENU_ALARM_ITEMS;
    default:
      return 0;
  }
}

// Set default menu item height
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Draw menu section headers
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Misc Settings");
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Alarms");
      break;
  }
}

// Draw menu items
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          // Set snooze time
          menu_cell_basic_draw(ctx, cell_layer, "Snooze Delay", "9 minutes", NULL);
          break;

        case 1:
          // Enable/Disable Smart Alarm
          menu_cell_basic_draw(ctx, cell_layer, "Smart Alarm", "OFF", NULL);
          break;
      }
      break;

    case 1:
      switch (cell_index->row) {
        case 0:
          // Set alarm time for all days
          menu_cell_basic_draw(ctx, cell_layer, "All Days", "7:00", NULL);
          break;
        case 1:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Sunday", "OFF", NULL);
          break;
        case 2:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Monday", "7:30", NULL);
          break;
        case 3:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Tuesday", "7:30", NULL);
          break;
        case 4:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Wednesday", "7:30", NULL);
          break;
        case 5:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Thursday", "7:30", NULL);
          break;
        case 6:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Friday", "7:30", NULL);
          break;
        case 7:
          // Set single day alarm
          menu_cell_basic_draw(ctx, cell_layer, "Saturday", "OFF", NULL);
          break;
      }
  }
}

// Process menu item select clicks
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          show_snoozedelay();
      }
      // Cycle the icon
      //current_icon = (current_icon + 1) % NUM_MENU_ICONS;
      // After changing the icon, mark the layer to have it updated
      //layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
  }

}

// Process menu item long select clicks
void menu_longselect_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 1:
      // Cycle the icon
      //current_icon = (current_icon + 1) % NUM_MENU_ICONS;
      // After changing the icon, mark the layer to have it updated
      //layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
  }

}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_settings(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(settings_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_longselect_callback
  });
  
  window_stack_push(s_window, true);
}

void hide_settings(void) {
  window_stack_remove(s_window, true);
}
