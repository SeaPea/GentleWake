#include "settings.h"
#include "setalarms.h"
#include "periodset.h"
#include "common.h"
#include <pebble.h>

#define NUM_MENU_SECTIONS 3
#define NUM_MENU_ALARM_ITEMS 1
#define NUM_MENU_MISC_ITEMS 2
#define NUM_MENU_SMART_ITEMS 2
  
static alarm *s_alarms;
static struct Settings_st *s_settings;
static AlarmChangeCallBack s_alarm_changed;
  
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
  if (s_alarm_changed != NULL) s_alarm_changed();
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
      return NUM_MENU_ALARM_ITEMS;
    case 1:
      return NUM_MENU_MISC_ITEMS;
    case 2:
      return NUM_MENU_SMART_ITEMS;
    default:
      return 0;
  }
}

// Set default menu item height
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return 0;
    default:
      return MENU_CELL_BASIC_HEADER_HEIGHT;
  }
}

// Draw menu section headers
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, NULL);
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Snooze Settings");
      break;
    case 2:
      menu_cell_basic_header_draw(ctx, cell_layer, "Smart Alarm");
      break;
  }
}

// Draw menu items
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char alarm_summary[15];
  bool is_mixed = false;
  bool all_off = true;
  int first_day = -1;
  int last_day = -1;
  char first_day_str[4];
  char last_day_str[4];
  char alarm_str[8];
  char snooze_str[15];
  char monitor_str[15];
  
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          // Alarms sub-menu
          for (int i = 0; i <= 6; i++) {
            if (s_alarms[i].enabled) {
              all_off = false;
              break;
            }
          }
          if (all_off) {
            strncpy(alarm_summary, "All Off", sizeof(alarm_summary));
          } else {
            for (int i = 0; i < 6; i++) {
              if (s_alarms[i].enabled) {
                if (first_day == -1) {
                  first_day = i;
                } else if (s_alarms[i].hour != s_alarms[first_day].hour ||
                           s_alarms[i].minute != s_alarms[first_day].minute) {
                  is_mixed = true;
                  break;
                }
              }
            }
            if (is_mixed) {
              strncpy(alarm_summary, "Mixed", sizeof(alarm_summary));
            } else {
              first_day = -1;
              for (int i = 0; i <= 6; i++) {
                if (s_alarms[i].enabled && first_day == -1)
                  first_day = i;
                if (first_day != -1 && last_day == -1 && !s_alarms[i].enabled)
                  last_day = i-1;
              }
              if (last_day == -1)
                last_day = 6;
              daynameshort(first_day, first_day_str, sizeof(first_day_str));
              daynameshort(last_day, last_day_str, sizeof(last_day_str));
              gen_alarm_str(&s_alarms[first_day], alarm_str, sizeof(alarm_str));
              snprintf(alarm_summary, sizeof(alarm_summary), "%s-%s %s", first_day_str, last_day_str, alarm_str);
            }
          }
          menu_cell_basic_draw(ctx, cell_layer, "Set Alarms", alarm_summary, NULL);
          break;
      }
      break;
    
    case 1:
      switch (cell_index->row) {
        case 0:
          // Set snooze time
          snprintf(snooze_str, sizeof(snooze_str), "%d minute(s)", s_settings->snooze_delay);
          menu_cell_basic_draw(ctx, cell_layer, "Max Snooze Delay", snooze_str, NULL);
          break;

        case 1:
          // Enable/Disable Dynamic Snooze
          menu_cell_basic_draw(ctx, cell_layer, "Dynamic Snooze", s_settings->dynamic_snooze ? "ON" : "OFF", NULL);
          break;
      }
      break;

    case 2:
      switch (cell_index->row) {
        case 0:
          // Enable/Disable Smart Alarm
          menu_cell_basic_draw(ctx, cell_layer, "Smart Alarm", s_settings->smart_alarm ? "ON" : "OFF", NULL);
          break;
        case 1:
          // Set single day alarm
          snprintf(monitor_str, sizeof(monitor_str), "%d minute(s)", s_settings->monitor_period);
          menu_cell_basic_draw(ctx, cell_layer, "Monitor Period", monitor_str, NULL);
          break;
      }
  }
}

static void snoozedelay_set(int minutes) {
  s_settings->snooze_delay = minutes;
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}

static void monitorperiod_set(int minutes) {
  s_settings->monitor_period = minutes;
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}

// Process menu item select clicks
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          show_setalarms(s_alarms);
      }
      break;
    case 1:
      switch (cell_index->row) {
        case 0:
          show_periodset("Snooze Delay", s_settings->snooze_delay, 20, snoozedelay_set);
          break;
        case 1:
          s_settings->dynamic_snooze = !s_settings->dynamic_snooze;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
      }
      break;
    case 2:
      switch (cell_index->row) {
        case 0:
          s_settings->smart_alarm = !s_settings->smart_alarm;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case 1:
          show_periodset("Smart Alarm Monitor Period", s_settings->monitor_period, 60, monitorperiod_set);
          break;
      }
      break;
  }

}

// Process menu item long select clicks
static void menu_longselect_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 1:
      
      //layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
  }

}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_settings(alarm *alarms, struct Settings_st *settings, AlarmChangeCallBack alarm_changed) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_alarms = alarms;
  s_settings = settings;
  s_alarm_changed = alarm_changed;
  
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
