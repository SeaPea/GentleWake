#include "setalarms.h"
#include "alarmtime.h"
#include "common.h"
#include <pebble.h>

// Screen to set alarm times
  
#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ALARM_ITEMS 9
  
static alarm *s_alarms;
static alarm *s_onetime;

static Window *s_window;
static MenuLayer *alarms_layer;

static void initialise_ui(void) {
  s_window = window_create();
  IF_2(window_set_fullscreen(s_window, true));
  
  // alarms_layer
  alarms_layer = menu_layer_create(GRect(0, 0, 144, 168));
  menu_layer_set_click_config_onto_window(alarms_layer, s_window);
  IF_3(menu_layer_set_normal_colors(alarms_layer, GColorBlack, GColorWhite)); 
  IF_3(menu_layer_set_highlight_colors(alarms_layer, GColorBlueMoon, GColorWhite));
  layer_add_child(window_get_root_layer(s_window), (Layer *)alarms_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(alarms_layer);
}


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

// Indicates if alarm times or on/off are mixed between days
static bool is_alarms_mixed() {
  for (int i = 0; i <= 5; i++) {
    if (s_alarms[i].enabled != s_alarms[i+1].enabled ||
        (s_alarms[i+1].enabled && 
         (s_alarms[i].hour != s_alarms[i+1].hour ||
          s_alarms[i].minute != s_alarms[i+1].minute))) {
      return true;
    }
  }
  
  return false;
}

// Draw menu items
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  
  char daystr[10];
  char alarmtimestr[8];
  char alarmstr[30];
  
  switch (cell_index->section) {
    case 0:
    
      switch (cell_index->row) {
        case 0:
          // Set One-Time alarm
          gen_alarm_str(s_onetime, alarmtimestr, sizeof(alarmtimestr));
          if (s_onetime->enabled)
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn off", alarmtimestr);
          else
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn on", alarmtimestr);
        
          menu_cell_basic_draw(ctx, cell_layer, "One-Time Alarm", alarmstr, NULL);
          break;
        
        case 1:
          // Set alarm time for all days
          if (is_alarms_mixed()) {
            strncpy(alarmstr, "Mixed - Hold to turn off", sizeof(alarmstr));
          } else {
            gen_alarm_str(&s_alarms[0], alarmtimestr, sizeof(alarmtimestr));
            if (s_alarms[0].enabled)
              snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn off", alarmtimestr);
            else
              snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn on", alarmtimestr);
          }
        
          menu_cell_basic_draw(ctx, cell_layer, "All Days", alarmstr, NULL);
          break;
        
        default:
          // Set single day alarm
          dayname(cell_index->row-1, daystr, sizeof(daystr));
          gen_alarm_str(&s_alarms[cell_index->row-1], alarmtimestr, sizeof(alarmtimestr));
          if (s_alarms[cell_index->row-1].enabled)
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn off", alarmtimestr);
          else
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn on", alarmtimestr);
          menu_cell_basic_draw(ctx, cell_layer, daystr, alarmstr, NULL);
      }
  }
}

// Call back from AlarmTime screen to set alarm
static void alarm_set(int day, int hour, int minute) {
  switch (day) {
    case -2:
      // Set One-Time alarm
      s_onetime->enabled = true;
      s_onetime->hour = hour;
      s_onetime->minute = minute;
      break;
    case -1:
      // Set all days the same
      for (int i = 0; i <= 6; i++) {
        s_alarms[i].enabled = true;
        s_alarms[i].hour = hour;
        s_alarms[i].minute = minute;
      }
      break;
    default:
      // Set individual day
      s_alarms[day].enabled = true;
      s_alarms[day].hour = hour;
      s_alarms[day].minute = minute;
      break;
  }
  layer_mark_dirty(menu_layer_get_layer(alarms_layer));
}

// Process menu item select clicks
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          show_alarmtime(-2, 
                         s_onetime->enabled ? s_onetime->hour : 7,
                         s_onetime->enabled ? s_onetime->minute : 0,
                         alarm_set);
          break;
        case 1:
          if (is_alarms_mixed())
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

// Process menu item long select clicks (Toggles alarms on/off)
static void menu_longselect_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        case 0:
          if (s_onetime->enabled) {
            s_onetime->enabled = false;  
          } else {
            s_onetime->enabled = true;
            s_onetime->hour = 7;
            s_onetime->minute = 0;
          }
          break;
        case 1:
          if (is_alarms_mixed()) {
            for (int i = 0; i <= 6; i++)
              s_alarms[i].enabled = false;
          } else {
            for (int i = 0; i <= 6; i++) {
              if (s_alarms[i].enabled) {
                s_alarms[i].enabled = false;
              } else {
                s_alarms[i].enabled = true;
                s_alarms[i].hour = 7;
                s_alarms[i].minute = 0;
              }
            }
          }
          break;
        default:
          if (s_alarms[cell_index->row-1].enabled) {
            s_alarms[cell_index->row-1].enabled = false;
          } else {
            s_alarms[cell_index->row-1].enabled = true;
            s_alarms[cell_index->row-1].hour = 7;
            s_alarms[cell_index->row-1].minute = 0;
          }
      }
      layer_mark_dirty(menu_layer_get_layer(alarms_layer));
      break;
  }

}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_setalarms(alarm *alarms, alarm *onetime) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Store pointer to alarms array and one-time alarm
  s_alarms = alarms;
  s_onetime = onetime;
  
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
