#include <pebble.h>
#include "settings.h"
#ifndef PBL_PLATFORM_APLITE
#include "periodset.h"
#endif
#include "common.h"
#include "commonwin.h"
#include "alarmtime.h"

// Top-Level Settings Screen

#define NUM_MAIN_MENU_SECTIONS 5
#define NUM_ALARM_MENU_SECTIONS 1

#define NUM_MAIN_MENU_ALARM_ITEMS 1
#define NUM_MAIN_MENU_MISC_ITEMS 6
#define NUM_MAIN_MENU_SMART_ITEMS 4
#define NUM_MAIN_MENU_DST_ITEMS 2
#define NUM_MAIN_MENU_ABOUT_ITEMS 1
#define NUM_ALARM_MENU_ALARM_ITEMS 9

#define MAIN_MENU_ALARM_SECTION 0
#define MAIN_MENU_MISC_SECTION 1
#define MAIN_MENU_SMART_SECTION 2
#define MAIN_MENU_DST_SECTION 3
#define MAIN_MENU_ABOUT_SECTION 4

#define MAIN_MENU_ALARMS_ITEM 0

#define MAIN_MENU_SNOOZEDELAY_ITEM 0
#define MAIN_MENU_DYNAMICSNOOZE_ITEM 1
#define MAIN_MENU_EASYLIGHT_ITEM 2
#define MAIN_MENU_KONAMICODE_ITEM 3
#define MAIN_MENU_VIBEPATTERN_ITEM 4
#define MAIN_MENU_AUTOCLOSE_ITEM 5

#define MAIN_MENU_SMARTALARM_ITEM 0
#define MAIN_MENU_SMARTPERIOD_ITEM 1
#define MAIN_MENU_MOVESENSITIVITY_ITEM 2
#define MAIN_MENU_GOOB_ITEM 3

#define MAIN_MENU_DSTDAYCHECK_ITEM 0
#define MAIN_MENU_DSTDAYHOUR_ITEM 1
#define MAIN_MENU_VERSION_ITEM 0

static enum menulevel_e {
  ML_Main,
  ML_Alarms
} s_menulevel = ML_Main;

static alarm *s_alarms;
static struct Settings_st *s_settings;
static SettingsClosedCallBack s_settings_closed;
static GFont s_header_font;
  
static Window *s_window;
static MenuLayer *settings_layer;

static void initialise_ui(void) {
  GRect bounds;
  Layer *root_layer = NULL;
  s_window = window_create_fullscreen(&root_layer, &bounds);
  s_header_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  
  // settings_layer
  settings_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(settings_layer, s_window);
  IF_COLOR(menu_layer_set_normal_colors(settings_layer, GColorBlack, GColorWhite)); 
  IF_COLOR(menu_layer_set_highlight_colors(settings_layer, GColorBlueMoon, GColorWhite));
  layer_add_child(root_layer, (Layer *)settings_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(settings_layer);
#ifndef PBL_PLATFORM_APLITE
  unload_periodset();
#endif
  if (s_settings_closed != NULL) s_settings_closed();
}

// Set menu section count
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  switch (s_menulevel) {
    case ML_Main:
      return NUM_MAIN_MENU_SECTIONS;
    case ML_Alarms:
      return NUM_ALARM_MENU_SECTIONS;
    default:
      return 0;
  }
}

// Set menu section item counts
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (s_menulevel) {
    case ML_Main:
      switch (section_index) {
        case MAIN_MENU_ALARM_SECTION:
          return NUM_MAIN_MENU_ALARM_ITEMS;
        case MAIN_MENU_MISC_SECTION:
          return NUM_MAIN_MENU_MISC_ITEMS;
        case MAIN_MENU_SMART_SECTION:
          return NUM_MAIN_MENU_SMART_ITEMS;
        case MAIN_MENU_DST_SECTION:
          return NUM_MAIN_MENU_DST_ITEMS;
        case MAIN_MENU_ABOUT_SECTION:
          return NUM_MAIN_MENU_ABOUT_ITEMS;
        default:
          return 0;
      }
    case ML_Alarms:
      return NUM_ALARM_MENU_ALARM_ITEMS;
    default:
      return 0;
  }
}

// Set default menu item height
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (s_menulevel) {
    case ML_Main:
      switch (section_index) {
        case MAIN_MENU_ALARM_SECTION:
          return 0;
        default:
          return MENU_CELL_BASIC_HEADER_HEIGHT;
      }
    case ML_Alarms:
      return 0;
    default:
      return 0;
  }
}

// Draw menu section headers
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (s_menulevel) {
    case ML_Main:
      graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorWhite));
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_fill_rect(ctx, layer_get_bounds(cell_layer), 0, GCornerNone);
      switch (section_index) {
        case MAIN_MENU_ALARM_SECTION:
          menu_cell_basic_header_draw(ctx, cell_layer, NULL);
          break;
        case MAIN_MENU_MISC_SECTION:
          graphics_draw_text(ctx, "Misc Settings", s_header_font, layer_get_bounds(cell_layer), 
                             GTextOverflowModeTrailingEllipsis, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
          //menu_cell_basic_header_draw(ctx, cell_layer, "Misc Settings");
          break;
        case MAIN_MENU_SMART_SECTION:
          graphics_draw_text(ctx, "Smart Alarm", s_header_font, layer_get_bounds(cell_layer), 
                             GTextOverflowModeTrailingEllipsis, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
          //menu_cell_basic_header_draw(ctx, cell_layer, "Smart Alarm");
          break;
        case MAIN_MENU_DST_SECTION:
          graphics_draw_text(ctx, "DST Check", s_header_font, layer_get_bounds(cell_layer), 
                             GTextOverflowModeTrailingEllipsis, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
          //menu_cell_basic_header_draw(ctx, cell_layer, "DST Check");
          break;
        case MAIN_MENU_ABOUT_SECTION:
          graphics_draw_text(ctx, "About", s_header_font, layer_get_bounds(cell_layer), 
                             GTextOverflowModeTrailingEllipsis, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
          //menu_cell_basic_header_draw(ctx, cell_layer, "About");
          break;
      }
      break;
    case ML_Alarms:
      // No headers for alarm menu
      menu_cell_basic_header_draw(ctx, cell_layer, NULL);;
      break;
  }
}


// Indicates if alarm times or on/off are mixed between days
static bool is_alarms_mixed() {
  for (uint8_t i = 0; i <= 5; i++) {
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
  char autoclose_str[17];
  char goob_str[27];
  
  char daystr[10];
  char alarmtimestr[8];
  char alarmstr[30];
  
  switch (s_menulevel) {
    case ML_Main:
      switch (cell_index->section) {
        case MAIN_MENU_ALARM_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_ALARMS_ITEM:
              // Alarms sub-menu
            
              // Determine if all alarms are off
              for (uint8_t i = 0; i <= 6; i++) {
                if (s_alarms[i].enabled) {
                  all_off = false;
                  break;
                }
              }
              if (all_off) {
                strncpy(alarm_summary, "All Off", sizeof(alarm_summary));
              } else {
                // Check if alarm times or on/off are mixed
                for (uint8_t i = 0; i <= 6; i++) {
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
                  for (uint8_t i = 0; i <= 6; i++) {
                    if (s_alarms[i].enabled && first_day == -1)
                      first_day = i;
                    if (first_day != -1 && last_day == -1 && !s_alarms[i].enabled)
                      last_day = i-1;
                  }
                  if (last_day == -1)
                    last_day = 6;
                  daynameshort(first_day, first_day_str, sizeof(first_day_str));
                  gen_alarm_str(&s_alarms[first_day], alarm_str, sizeof(alarm_str));
                  if (first_day == last_day)
                    snprintf(alarm_summary, sizeof(alarm_summary), "%s %s", first_day_str, alarm_str);
                  else {
                    daynameshort(last_day, last_day_str, sizeof(last_day_str));
                    // Show summary e.g. "Mon-Fri 7:00"
                    snprintf(alarm_summary, sizeof(alarm_summary), "%s-%s %s", first_day_str, last_day_str, alarm_str);
                  }
                }
              }
              menu_cell_basic_draw(ctx, cell_layer, "Set Alarms", alarm_summary, NULL);
              break;
          }
          break;
        
        case MAIN_MENU_MISC_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_SNOOZEDELAY_ITEM:
              // Set snooze time
              snprintf(snooze_str, sizeof(snooze_str), "%d minute(s)", s_settings->snooze_delay);
              menu_cell_basic_draw(ctx, cell_layer, "Max Snooze Delay", snooze_str, NULL);
              break;
    
            case MAIN_MENU_DYNAMICSNOOZE_ITEM:
              // Enable/Disable Dynamic Snooze
              menu_cell_basic_draw(ctx, cell_layer, "Dynamic Snooze", s_settings->dynamic_snooze ? "ON - Halves delay" : "OFF", NULL);
              break;
            
            case MAIN_MENU_EASYLIGHT_ITEM:
              // Enable/Disable Easy Light
              menu_cell_basic_draw(ctx, cell_layer, "Easy Light", s_settings->easy_light ? "ON - Hold up on alarm" : "OFF", NULL);
              break;
            
            case MAIN_MENU_KONAMICODE_ITEM:
              // Enable/Disable Konami Code
              menu_cell_basic_draw(ctx, cell_layer, "Stop Alarm", s_settings->konamic_code_on ? "Konami Code" : "Double click", NULL);
              break;
            
            case MAIN_MENU_VIBEPATTERN_ITEM:
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
            case MAIN_MENU_AUTOCLOSE_ITEM:
              // Show Auto-Close setting
              switch (s_settings->autoclose_timeout) {
                case 0:
                  strcpy(autoclose_str, "OFF");
                  break;
                case 1:
                  strcpy(autoclose_str, "After 1 Minute");
                  break;
                default:
                  snprintf(autoclose_str, sizeof(autoclose_str), "After %d Minutes", s_settings->autoclose_timeout);
                  break;
              }
              menu_cell_basic_draw(ctx, cell_layer, "Auto Close", autoclose_str, NULL);
              break;
          }
          break;
    
        case MAIN_MENU_SMART_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_SMARTALARM_ITEM:
              // Enable/Disable Smart Alarm
              menu_cell_basic_draw(ctx, cell_layer, "Smart Alarm", s_settings->smart_alarm ? "ON - Alarm on stirring" : "OFF", NULL);
              break;
            
            case MAIN_MENU_SMARTPERIOD_ITEM:
              // Set single day alarm
              snprintf(monitor_str, sizeof(monitor_str), "%d minute(s)", s_settings->monitor_period);
              menu_cell_basic_draw(ctx, cell_layer, "Monitor Period", monitor_str, NULL);
              break;
            
            case MAIN_MENU_MOVESENSITIVITY_ITEM:
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
            case MAIN_MENU_GOOB_ITEM:
              // Get Out Of Bed Setting
              switch (s_settings->goob_mode) {
                case GM_Off:
                  strcpy(goob_str, "Off");
                  break;
                case GM_AfterAlarm:
                  snprintf(goob_str, sizeof(goob_str), "%d min. after alarm start", s_settings->goob_monitor_period);
                  break;
                case GM_AfterStop:
                  snprintf(goob_str, sizeof(goob_str), "%d min. after stop alarm", s_settings->goob_monitor_period);
                  break;
              }
              menu_cell_basic_draw(ctx, cell_layer, "Get out of Bed Alm", goob_str, NULL);
              break;
          }
          break;
        
        case MAIN_MENU_DST_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_DSTDAYCHECK_ITEM:
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
            
            case MAIN_MENU_DSTDAYHOUR_ITEM:
              snprintf(dst_check_hour_str, sizeof(dst_check_hour_str), "%d AM", s_settings->dst_check_hour);
              menu_cell_basic_draw(ctx, cell_layer, "DST Check Hour", dst_check_hour_str, NULL);
              break;
          }
          break;
        
        case MAIN_MENU_ABOUT_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_VERSION_ITEM:
              menu_cell_basic_draw(ctx, cell_layer, "Version", VERSION, NULL);
              break;
          }
          break;
      }
      break;
    
    
    case ML_Alarms:
      switch (cell_index->row) {
        case 0:
          // Set One-Time alarm
          gen_alarm_str(&(s_settings->one_time_alarm), alarmtimestr, sizeof(alarmtimestr));
          if (s_settings->one_time_alarm.enabled)
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
          dayname(cell_index->row-2, daystr, sizeof(daystr));
          gen_alarm_str(&s_alarms[cell_index->row-2], alarmtimestr, sizeof(alarmtimestr));
          if (s_alarms[cell_index->row-2].enabled)
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn off", alarmtimestr);
          else
            snprintf(alarmstr, sizeof(alarmstr), "%s - Hold to turn on", alarmtimestr);
          menu_cell_basic_draw(ctx, cell_layer, daystr, alarmstr, NULL);
          break;
      }
      break;
  }
  
  
}

#ifndef PBL_PLATFORM_APLITE
static void snoozedelay_set(uint8_t minutes) {
  s_settings->snooze_delay = minutes;
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}

static void monitorperiod_set(uint8_t minutes) {
  s_settings->monitor_period = minutes;
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}
#endif


// Call back from AlarmTime screen to set alarm
static void alarm_set(int8_t day, uint8_t hour, uint8_t minute) {
  switch (day) {
    case -2:
      // Set One-Time alarm
      s_settings->one_time_alarm.enabled = true;
      s_settings->one_time_alarm.hour = hour;
      s_settings->one_time_alarm.minute = minute;
      break;
    case -1:
      // Set all days the same
      for (uint8_t i = 0; i <= 6; i++) {
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
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}

// Process menu item select clicks
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  
  switch (s_menulevel) {
    case ML_Main:
      switch (cell_index->section) {
        case MAIN_MENU_ALARM_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_ALARMS_ITEM:
    #ifndef PBL_PLATFORM_APLITE
              unload_periodset();
    #endif
              s_menulevel = ML_Alarms;
              menu_layer_reload_data(settings_layer);
              break;
          }
          break;
        
        case MAIN_MENU_MISC_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_SNOOZEDELAY_ITEM:
    #ifdef PBL_PLATFORM_APLITE
              // Save memory on Aplite watches by just incrementing in the menu
              s_settings->snooze_delay++;
              if (s_settings->snooze_delay > 20) s_settings->snooze_delay = 3;
    #else
              show_periodset("Snooze Delay", s_settings->snooze_delay, 3, 20, snoozedelay_set);
    #endif
              break;
            case MAIN_MENU_DYNAMICSNOOZE_ITEM:
              s_settings->dynamic_snooze = !s_settings->dynamic_snooze;
              break;
            case MAIN_MENU_EASYLIGHT_ITEM:
              s_settings->easy_light = !s_settings->easy_light;
              break;
            case MAIN_MENU_KONAMICODE_ITEM:
              s_settings->konamic_code_on = !s_settings->konamic_code_on;
              break;
            case MAIN_MENU_VIBEPATTERN_ITEM:
              s_settings->vibe_pattern = (s_settings->vibe_pattern == VP_NSG2Snooze ? VP_Gentle : s_settings->vibe_pattern + 1);
              break;
            case MAIN_MENU_AUTOCLOSE_ITEM:
              s_settings->autoclose_timeout = (s_settings->autoclose_timeout + 1) % 11;
          }
          break;
        
        case MAIN_MENU_SMART_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_SMARTALARM_ITEM:
              s_settings->smart_alarm = !s_settings->smart_alarm;
              break;
            case MAIN_MENU_SMARTPERIOD_ITEM:
    #ifdef PBL_PLATFORM_APLITE
              s_settings->monitor_period += 5;
              if (s_settings->monitor_period > 60) s_settings->monitor_period = 5;
    #else
              show_periodset("Smart Alarm Monitor Period", s_settings->monitor_period, 5, 60, monitorperiod_set);
    #endif
              break;
            case MAIN_MENU_MOVESENSITIVITY_ITEM:
              s_settings->sensitivity = (s_settings->sensitivity == MS_HIGH ? MS_LOW : s_settings->sensitivity + 1);
              break;
            case MAIN_MENU_GOOB_ITEM:
              if (s_settings->goob_mode == GM_AfterStop && s_settings->goob_monitor_period == 30) {
                s_settings->goob_mode = GM_Off;
                s_settings->goob_monitor_period = 5;
              } else if (s_settings->goob_mode == GM_Off) {
                s_settings->goob_mode = GM_AfterAlarm;
                s_settings->goob_monitor_period = 5;
              } else if (s_settings->goob_mode == GM_AfterAlarm) {
                s_settings->goob_mode = GM_AfterStop;
              } else {
                s_settings->goob_mode = GM_AfterAlarm;
                s_settings->goob_monitor_period += 5;
              }
              break;
          }
          break;
        
        case MAIN_MENU_DST_SECTION:
          switch (cell_index->row) {
            case MAIN_MENU_DSTDAYCHECK_ITEM:
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
              break;
            case MAIN_MENU_DSTDAYHOUR_ITEM:
              // Cycle DST check hour between 3AM and 9AM
              s_settings->dst_check_hour++;
              if (s_settings->dst_check_hour > 9) s_settings->dst_check_hour = 3;
              break;
          }
          break;
      }
      break;
    
    
    case ML_Alarms:
      switch (cell_index->row) {
        case 0:
          // One-Time Alarm
          show_alarmtime(-2, 
                         s_settings->one_time_alarm.enabled ? s_settings->one_time_alarm.hour : 7,
                         s_settings->one_time_alarm.enabled ? s_settings->one_time_alarm.minute : 0,
                         alarm_set);
          break;
        case 1:
          // All-Days setting
          if (is_alarms_mixed())
            show_alarmtime(-1, 7, 0, alarm_set);
          else
            show_alarmtime(-1, 
                           s_alarms[0].enabled ? s_alarms[0].hour : 7, 
                           s_alarms[0].enabled ? s_alarms[0].minute : 0, 
                           alarm_set);
      
          break;
        default:
          // Individual day alarms
          show_alarmtime(cell_index->row-2, 
                         s_alarms[cell_index->row-2].enabled ? s_alarms[cell_index->row-2].hour : 7, 
                         s_alarms[cell_index->row-2].enabled ? s_alarms[cell_index->row-2].minute : 0, 
                         alarm_set);
          break;
      }
      break;
  }
  
  layer_mark_dirty(menu_layer_get_layer(settings_layer));
}


// Process menu item long select clicks (Toggles alarms on/off)
static void menu_longselect_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  
  switch (s_menulevel) {
    case ML_Main:
      // No long select on main menu
      break;
    
    case ML_Alarms:
      switch (cell_index->row) {
        case 0:
          // One-Time Alarm
          if (s_settings->one_time_alarm.enabled) {
            s_settings->one_time_alarm.enabled = false;  
          } else {
            s_settings->one_time_alarm.enabled = true;
            s_settings->one_time_alarm.hour = 7;
            s_settings->one_time_alarm.minute = 0;
          }
          break;
        case 1:
          // All-Days Setting
          if (is_alarms_mixed()) {
            for (uint8_t i = 0; i <= 6; i++)
              s_alarms[i].enabled = false;
          } else {
            for (uint8_t i = 0; i <= 6; i++) {
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
          // Individual day alarms
          if (s_alarms[cell_index->row-2].enabled) {
            s_alarms[cell_index->row-2].enabled = false;
          } else {
            s_alarms[cell_index->row-2].enabled = true;
            s_alarms[cell_index->row-2].hour = 7;
            s_alarms[cell_index->row-2].minute = 0;
          }
          break;
      }
      layer_mark_dirty(menu_layer_get_layer(settings_layer));
      break;
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (s_menulevel) {
    case ML_Main:
      hide_settings();
      break;
    case ML_Alarms:
      s_menulevel = ML_Main;
      menu_layer_reload_data(settings_layer);
      MenuIndex idx = {.section = MAIN_MENU_ALARM_SECTION, .row = MAIN_MENU_ALARMS_ITEM};
      menu_layer_set_selected_index(settings_layer, idx, MenuRowAlignCenter, false);
      layer_mark_dirty(menu_layer_get_layer(settings_layer));
      break;
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex idx = menu_layer_get_selected_index(settings_layer);
  menu_select_callback(settings_layer, &idx, NULL);
}

static void longselect_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex idx = menu_layer_get_selected_index(settings_layer);
  menu_longselect_callback(settings_layer, &idx, NULL);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  menu_layer_set_selected_next(settings_layer, true, MenuRowAlignCenter, true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  menu_layer_set_selected_next(settings_layer, false, MenuRowAlignCenter, true);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  // Have to reimplement the menu up/down/select clicks in order to override window back click as well
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, longselect_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 250, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 250, down_click_handler);
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
  });
  
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_settings(void) {
  window_stack_remove(s_window, true);
}
