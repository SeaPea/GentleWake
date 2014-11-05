#include "setalarms.h"
#include "alarmtime.h"
#include "common.h"
#include <pebble.h>

#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ALARM_ITEMS 8
  
static alarm *s_alarms;
  
// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static MenuLayer *alarms_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  // alarms_layer
  alarms_layer = menu_layer_create(GRect(0, 0, 144, 152));
  menu_layer_set_click_config_onto_window(alarms_layer, s_window);
  layer_add_child(window_get_root_layer(s_window), (Layer *)alarms_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(alarms_layer);
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
    default:
      return 0;
  }
}

static void gen_alarm_str(alarm *alarmtime, char *alarmstr) {
  if (alarmtime->enabled)
    snprintf(alarmstr, 6, "%d:%.2d", alarmtime->hour, alarmtime->minute);
  else
    strncpy(alarmstr, "OFF", 6);
}

// Draw menu items
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  
  bool mixed = false;
  char daystr[10];
  char alarmstr[6];
  
  switch (cell_index->section) {
    case 0:
    
      switch (cell_index->row) {
        case 0:
          // Set alarm time for all days
          for (int i = 0; i <= 5; i++) {
            if (s_alarms[i].enabled != s_alarms[i+1].enabled ||
                s_alarms[i].hour != s_alarms[i+1].hour ||
                s_alarms[i].minute != s_alarms[i+1].minute) {
              mixed = true;
              break;
            }
          }
          
          if (mixed)
            strncpy(alarmstr, "Mixed", 6);
          else
            gen_alarm_str(&s_alarms[0], alarmstr);
        
          menu_cell_basic_draw(ctx, cell_layer, "All Days", alarmstr, NULL);
          break;
        default:
          // Set single day alarm
          dayname(cell_index->row-1, daystr);
          gen_alarm_str(&s_alarms[cell_index->row-1], alarmstr);
          menu_cell_basic_draw(ctx, cell_layer, daystr, alarmstr, NULL);
      }
  }
}

static void alarm_set(int day, int hour, int minute) {
  if (day == -1) {
    for (int i = 0; i <= 6; i++) {
      s_alarms[i].enabled = true;
      s_alarms[i].hour = hour;
      s_alarms[i].minute = minute;
    }
  } else {
    s_alarms[day].enabled = true;
    s_alarms[day].hour = hour;
    s_alarms[day].minute = minute;
  }
  layer_mark_dirty(menu_layer_get_layer(alarms_layer));
}

// Process menu item select clicks
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  bool mixed = false;
  
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          for (int i = 0; i <= 5; i++) {
            if (s_alarms[i].enabled != s_alarms[i+1].enabled ||
                s_alarms[i].hour != s_alarms[i+1].hour ||
                s_alarms[i].minute != s_alarms[i+1].minute) {
              mixed = true;
              break;
            }
          }
        
          if (mixed)
            show_alarmtime(-1, 7, 0, alarm_set);
          else
            show_alarmtime(-1, 
                           s_alarms[0].enabled ? s_alarms[0].hour : 7, 
                           s_alarms[0].enabled ? s_alarms[0].minute : 0, 
                           alarm_set);
            
          break;
        default:
          show_alarmtime(cell_index->row-1, 
                         s_alarms[cell_index->row-1].enabled ? s_alarms[cell_index->row-1].hour : 7, 
                         s_alarms[cell_index->row-1].enabled ? s_alarms[cell_index->row-1].minute : 0, 
                         alarm_set);
      }
      break;
  }

}

// Process menu item long select clicks
static void menu_longselect_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 1:
      
      break;
  }

}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_setalarms(alarm *alarms) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_alarms = alarms;
  
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(alarms_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = NULL,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_longselect_callback
  });
  
  window_stack_push(s_window, true);
}

void hide_setalarms(void) {
  window_stack_remove(s_window, true);
}
