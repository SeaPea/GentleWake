#include <pebble.h>
#include "settings.h"
#include "setalarms.h"
#include "periodset.h"
#include "common.h"

// Top-Level Settings Screen
  
#define NUM_MENU_SECTIONS 4
#define NUM_MENU_ALARM_ITEMS 1
#define NUM_MENU_MISC_ITEMS 5
#define NUM_MENU_SMART_ITEMS 3
#define NUM_MENU_DST_ITEMS 2
#define MENU_ALARM_SECTION 0
#define MENU_MISC_SECTION 1
#define MENU_SMART_SECTION 2
#define MENU_DST_SECTION 3
#define MENU_ALARMS_ITEM 0
#define MENU_SNOOZEDELAY_ITEM 0
#define MENU_DYNAMICSNOOZE_ITEM 1
#define MENU_EASYLIGHT_ITEM 2
#define MENU_KONAMICODE_ITEM 3
#define MENU_VIBEPATTERN_ITEM 4
#define MENU_SMARTALARM_ITEM 0
#define MENU_SMARTPERIOD_ITEM 1
#define MENU_MOVESENSITIVITY_ITEM 2
#define MENU_DSTDAYCHECK_ITEM 0
#define MENU_DSTDAYHOUR_ITEM 1
  
static alarm *s_alarms;
static struct Settings_st *s_settings;
static SettingsClosedCallBack s_settings_closed;
static GFont s_header_font;
  
static Window *s_window;
static MenuLayer *settings_layer;

static void initialise_ui(void) {
  s_window = window_create();
  IF_2(window_set_fullscreen(s_window, true));
  s_header_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  
  // settings_layer
  settings_layer = menu_layer_create(GRect(0, 0, 144, 168));
  menu_layer_set_click_config_onto_window(settings_layer, s_window);
  IF_3(menu_layer_set_normal_colors(settings_layer, GColorBlack, GColorWhite)); 
  IF_3(menu_layer_set_highlight_colors(settings_layer, GColorBlueMoon, GColorWhite));
  layer_add_child(window_get_root_layer(s_window), (Layer *)settings_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(settings_layer);
  if (s_settings_closed != NULL) s_settings_closed();
}

// Set menu section count
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Set menu section item counts
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case MENU_ALARM_SECTION:
      return NUM_MENU_ALARM_ITEMS;
    case MENU_MISC_SECTION:
      return NUM_MENU_MISC_ITEMS;
    case MENU_SMART_SECTION:
      return NUM_MENU_SMART_ITEMS;
    case MENU_DST_SECTION:
      return NUM_MENU_DST_ITEMS;
    default:
      return 0;
  }
}

// Set default menu item height
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case MENU_ALARM_SECTION:
      return 0;
    default:
      return MENU_CELL_BASIC_HEADER_HEIGHT;
  }
}

// Draw menu section headers
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorWhite));
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(cell_layer), 0, GCornerNone);
  switch (section_index) {
    case MENU_ALARM_SECTION:
      menu_cell_basic_header_draw(ctx, cell_layer, NULL);
      break;
    case MENU_MISC_SECTION:
      graphics_draw_text(ctx, "Misc Settings", s_header_font, layer_get_bounds(cell_layer), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      //menu_cell_basic_header_draw(ctx, cell_layer, "Misc Settings");
      break;
    case MENU_SMART_SECTION:
      graphics_draw_text(ctx, "Smart Alarm", s_header_font, layer_get_bounds(cell_layer), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      //menu_cell_basic_header_draw(ctx, cell_layer, "Smart Alarm");
      break;
    case MENU_DST_SECTION:
      graphics_draw_text(ctx, "DST Check", s_header_font, layer_get_bounds(cell_layer), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      //menu_cell_basic_header_draw(ctx, cell_layer, "DST Check");
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
  char dst_check_hour_str[6];
  
  switch (cell_index->section) {
    case MENU_ALARM_SECTION:
      switch (cell_index->row) {
        case MENU_ALARMS_ITEM:
          // Alarms sub-menu
        
          // Determine if all alarms are off
          for (int i = 0; i <= 6; i++) {
            if (s_alarms[i].enabled) {
              all_off = false;
              break;
            }
          }
          if (all_off) {
            strncpy(alarm_summary, "All Off", sizeof(alarm_summary));
          } else {
            // Check if alarm times or on/off are mixed
            for (int i = 0; i <= 6; i++) {
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
              // Else get the stretch of enabled alarm times and summarize it
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
              // Show summary e.g. "Mon-Fri 7:00"
              snprintf(alarm_summary, sizeof(alarm_summary), "%s-%s %s", first_day_str, last_day_str, alarm_str);
            }
          }
          menu_cell_basic_draw(ctx, cell_layer, "Set Alarms", alarm_summary, NULL);
          break;
      }
      break;
    
    case MENU_MISC_SECTION:
      switch (cell_index->row) {
        case MENU_SNOOZEDELAY_ITEM:
          // Set snooze time
          snprintf(snooze_str, sizeof(snooze_str), "%d minute(s)", s_settings->snooze_delay);
          menu_cell_basic_draw(ctx, cell_layer, "Max Snooze Delay", snooze_str, NULL);
          break;

        case MENU_DYNAMICSNOOZE_ITEM:
          // Enable/Disable Dynamic Snooze
          menu_cell_basic_draw(ctx, cell_layer, "Dynamic Snooze", s_settings->dynamic_snooze ? "ON - Halves delay" : "OFF", NULL);
          break;
        
        case MENU_EASYLIGHT_ITEM:
          // Enable/Disable Easy Light
          menu_cell_basic_draw(ctx, cell_layer, "Easy Light", s_settings->easy_light ? "ON - Hold up on alarm" : "OFF", NULL);
          break;
        
        case MENU_KONAMICODE_ITEM:
          // Enable/Disable Konami Code
          menu_cell_basic_draw(ctx, cell_layer, "Stop Alarm", s_settings->konamic_code_on ? "Konami Code" : "Double click", NULL);
          break;
        
        case MENU_VIBEPATTERN_ITEM:
          // Change the vibration level
          switch (s_settings->vibe_pattern) {
            case VP_Gentle:
              menu_cell_basic_draw(ctx, cell_layer, "Vibration Pattern", "Gentle (Original)", NULL);
              break;
            case VP_NSG:
              menu_cell_basic_draw(ctx, cell_layer, "Vibration Pattern", "Not-So-Gentle (NSG)", NULL);
              break;
            case VP_NSG2Snooze:
              menu_cell_basic_draw(ctx, cell_layer, "Vibration Pattern", "NSG After 2 Snoozes", NULL);
              break;
            default:
              menu_cell_basic_draw(ctx, cell_layer, "Vibration Pattern", "???", NULL);
              break;
          }
          break;
      }
      break;

    case MENU_SMART_SECTION:
      switch (cell_index->row) {
        case MENU_SMARTALARM_ITEM:
          // Enable/Disable Smart Alarm
          menu_cell_basic_draw(ctx, cell_layer, "Smart Alarm", s_settings->smart_alarm ? "ON - Alarm on stirring" : "OFF", NULL);
          break;
        
        case MENU_SMARTPERIOD_ITEM:
          // Set single day alarm
          snprintf(monitor_str, sizeof(monitor_str), "%d minute(s)", s_settings->monitor_period);
          menu_cell_basic_draw(ctx, cell_layer, "Monitor Period", monitor_str, NULL);
          break;
        
        case MENU_MOVESENSITIVITY_ITEM:
          // Adjust Smart Alarm movement sensitivity
          switch (s_settings->sensitivity) {
            case MS_LOW:
              menu_cell_basic_draw(ctx, cell_layer, "Sensitivity", "Low", NULL);
              break;
            case MS_MEDIUM:
              menu_cell_basic_draw(ctx, cell_layer, "Sensitivity", "Medium", NULL);
              break;
            case MS_HIGH:
              menu_cell_basic_draw(ctx, cell_layer, "Sensitivity", "High", NULL);
              break;
            default:
              menu_cell_basic_draw(ctx, cell_layer, "Sensitivity", "???", NULL);
              break;
          }
          break;
      }
      break;
    
    case MENU_DST_SECTION:
      switch (cell_index->row) {
        case MENU_DSTDAYCHECK_ITEM:
          switch (s_settings->dst_check_day) {
            case 0:
              menu_cell_basic_draw(ctx, cell_layer, "DST Check Day", "OFF", NULL);
              break;
            case TUESDAY:
              menu_cell_basic_draw(ctx, cell_layer, "DST Check Day", "Tuesday", NULL);
              break;
            case FRIDAY:
              menu_cell_basic_draw(ctx, cell_layer, "DST Check Day", "Friday", NULL);
              break;
            default:
              menu_cell_basic_draw(ctx, cell_layer, "DST Check Day", "Sunday", NULL);
              break;
          }
          break;
        
        case MENU_DSTDAYHOUR_ITEM:
          snprintf(dst_check_hour_str, sizeof(dst_check_hour_str), "%d AM", s_settings->dst_check_hour);
          menu_cell_basic_draw(ctx, cell_layer, "DST Check Hour", dst_check_hour_str, NULL);
          break;
      }
      break;
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
    case MENU_ALARM_SECTION:
      switch (cell_index->row) {
        case MENU_ALARMS_ITEM:
          show_setalarms(s_alarms);
      }
      break;
    
    case MENU_MISC_SECTION:
      switch (cell_index->row) {
        case MENU_SNOOZEDELAY_ITEM:
          show_periodset("Snooze Delay", s_settings->snooze_delay, 3, 20, snoozedelay_set);
          break;
        case MENU_DYNAMICSNOOZE_ITEM:
          s_settings->dynamic_snooze = !s_settings->dynamic_snooze;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case MENU_EASYLIGHT_ITEM:
          s_settings->easy_light = !s_settings->easy_light;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case MENU_KONAMICODE_ITEM:
          s_settings->konamic_code_on = !s_settings->konamic_code_on;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case MENU_VIBEPATTERN_ITEM:
          s_settings->vibe_pattern = (s_settings->vibe_pattern == VP_NSG2Snooze ? VP_Gentle : s_settings->vibe_pattern + 1);
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
      }
      break;
    
    case MENU_SMART_SECTION:
      switch (cell_index->row) {
        case MENU_SMARTALARM_ITEM:
          s_settings->smart_alarm = !s_settings->smart_alarm;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case MENU_SMARTPERIOD_ITEM:
          show_periodset("Smart Alarm Monitor Period", s_settings->monitor_period, 5, 60, monitorperiod_set);
          break;
        case MENU_MOVESENSITIVITY_ITEM:
          s_settings->sensitivity = (s_settings->sensitivity == MS_HIGH ? MS_LOW : s_settings->sensitivity + 1);
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
      }
      break;
    
    case MENU_DST_SECTION:
      switch (cell_index->row) {
        case MENU_DSTDAYCHECK_ITEM:
          // Cycle DST check day between Sunday, Tuesday, Friday, and OFF
          switch (s_settings->dst_check_day) {
            case SUNDAY:
              s_settings->dst_check_day = TUESDAY;
              break;
            case TUESDAY:
              s_settings->dst_check_day = FRIDAY;
              break;
            case FRIDAY:
              s_settings->dst_check_day = 0;
              break;
            default:
              s_settings->dst_check_day = SUNDAY;
              break;
          }
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
        case MENU_DSTDAYHOUR_ITEM:
          // Cycle DST check hour between 3AM and 9AM
          s_settings->dst_check_hour++;
          if (s_settings->dst_check_hour > 9) s_settings->dst_check_hour = 3;
          layer_mark_dirty(menu_layer_get_layer(settings_layer));
          break;
      }
      break;
  }

}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_settings(alarm *alarms, struct Settings_st *settings, SettingsClosedCallBack settings_closed) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_alarms = alarms;
  s_settings = settings;
  s_settings_closed = settings_closed;
  
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(settings_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback
  });
  
  window_stack_push(s_window, true);
}

void hide_settings(void) {
  window_stack_remove(s_window, true);
}
