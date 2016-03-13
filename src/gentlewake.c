#include <pebble.h>
#include "mainwin.h"
#include "settings.h"
#include "common.h"
#include "konamicode.h"
#include "skipwin.h"
#include "msg.h"

// Main program unit
  
//#define DEBUG
  
#define WAKEUP_REASON_ALARM 0
#define WAKEUP_REASON_SNOOZE 1
#define WAKEUP_REASON_MONITOR 2
#define WAKEUP_REASON_DSTCHECK 4
#define WAKEUP_REASON_GOOB 5

#define MOVEMENT_THRESHOLD_LOW 10000
#define MOVEMENT_THRESHOLD_MID 15000
#define MOVEMENT_THRESHOLD_HIGH 20000

#define REST_MOVEMENT 300
#define NEXT_ALARM_NONE -1
#define NEXT_ALARM_SNOOZE -2
#define NEXT_ALARM_SKIPWEEK -3
#define NEXT_ALARM_ONETIME -4

#define ALARMS_KEY 0
#define SNOOZEDELAY_KEY 1
#define DYNAMICSNOOZE_KEY 2
#define SMARTALARM_KEY 3
#define MONITORPERIOD_KEY 4
#define ALARMSON_KEY 5
#define WAKEUPID_KEY 6
#define EASYLIGHT_KEY 9
#define MOVESENSITIVITY_KEY 12
#define DSTCHECKDAY_KEY 14
#define DSTCHECKHOUR_KEY 15
#define SKIPUNTIL_KEY 17
#define KONAMICODEON_KEY 18
#define VIBEPATTERN_KEY 19
#define ONETIMEALARM_KEY 20
#define AUTOCLOSETIMEOUT_KEY 21
#define GOOBMODE_KEY 22
#define GOOBPERIOD_KEY 23
#define WAKEUPGOOBID_KEY 24
#define GOOBALARMTIME_KEY 26
#define SETTINGS_KEY 50
#define STATE_KEY 51
#define SETTINGSVER_KEY 99

#define SETTINGS_VER 1

// Accelerometer smoothing constants (Numerator and Denominator - Num. divided by Den. must be less than 1. Higher = smoother, slower. Lower = faster, less smooth)
// #define FILTER_K_NUM 1
// #define FILTER_K_DEN 2  

static bool s_alarms_on = true;
static alarm s_alarms[7];
static char s_info[45];
static WakeupId s_wakeup_id;
static WakeupId s_wakeup_goob_id;
static time_t s_snooze_until;
static bool s_alarm_active;
static bool s_goob_active;
static time_t s_goob_time;
static time_t s_skip_until;
static uint8_t s_vibe_count;
static time_t s_last_easylight;
static bool s_accel_service_sub;
static int8_t s_next_alarm = -1;
static bool s_light_shown;
static int16_t s_last_x;
static int16_t s_last_y;
static int16_t s_last_z;
static int s_movement;
static bool s_loaded;
static bool s_dst_check_started;
static bool s_last_arm_swing_dir;
static uint8_t s_arm_swing_count;
static time_t s_arm_swing_start;
static int16_t s_x_filtered = -9999;
static int16_t s_y_filtered = -9999;

// Vibrate alarm paterns - 2nd dimension: [next vibe delay (sec), vibe segment index, vibe segment length]
static uint8_t vibe_patterns_orig[18][3] = {{3, 0, 1}, {3, 0, 1}, {4, 0, 3}, {4, 0, 3}, {4, 0, 5}, {4, 0, 5}, 
                                   {3, 1, 1}, {3, 1, 1}, {4, 1, 3}, {4, 1, 3}, {5, 1, 5}, {5, 1, 5}, 
                                   {3, 2, 1}, {3, 2, 1}, {4, 2, 3}, {4, 2, 3}, {5, 2, 5}, {5, 2, 5}};
static uint32_t vibe_segments_orig[3][5] = {{150, 500, 150, 500, 150}, {300, 500, 300, 500, 300}, {600, 500, 600, 500, 600}};

static uint8_t vibe_patterns_strong[24][3] = {{2, 0, 1}, {2, 0, 1}, {3, 0, 3}, {3, 0, 3}, {3, 0, 5}, {3, 0, 5}, 
                                   {2, 1, 1}, {2, 1, 1}, {3, 1, 3}, {3, 1, 3}, {4, 1, 5}, {4, 1, 5}, 
                                   {2, 2, 1}, {2, 2, 1}, {3, 2, 3}, {3, 2, 3}, {4, 2, 5}, {4, 2, 5}, 
                                   {2, 2, 5}, {2, 2, 5}, {3, 2, 5}, {3, 2, 5}, {4, 2, 5}, {4, 2, 5}};
static uint32_t vibe_segments_strong[3][5] = {{300, 250, 300, 250, 300}, {450, 250, 450, 250, 450}, {600, 250, 600, 250, 600}};

static uint8_t vibe_patterns_goob[24][3] = {{2, 0, 5}, {2, 0, 5}, {2, 0, 5}, {2, 0, 5}, {2, 0, 5}, {2, 0, 5}, 
                                   {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, 
                                   {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
                                   {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}};
static uint32_t vibe_segments_goob[3][5] = {{150, 150, 150, 150, 500}, {150, 150, 150, 150, 1000}, {150, 150, 150, 150, 1500}};



static AppTimer *s_vibe_timer = NULL;

static struct Settings_st s_settings;

static struct State_st {
  uint8_t snooze_count;
  bool snoozing;
  bool monitoring;
  bool goob_monitoring;
  time_t last_reset_day;
} __attribute__((__packed__)) s_state ;

// Calculate which daily alarm (if any) will be next
// (Takes into account if the alarm for today was reset like when Smart Alarm is active and turned off
//  before the alarm time)
static int8_t get_next_alarm() {
  time_t next_date;
  int8_t next;
  
  if (!s_alarms_on) return NEXT_ALARM_NONE;
  
  // If the one-time alarm is enabled, that must be the next alarm
  if (s_settings.one_time_alarm.enabled) return NEXT_ALARM_ONETIME;
  
  // Get current time
  time_t utc = time(NULL);
  struct tm *t = localtime(&utc);
  
  // Save localtime details as the t struct gets stomped on by clock_to_timestamp
  uint8_t wday = t->tm_wday;
  uint8_t hour = t->tm_hour;
  uint8_t min = t->tm_min;
  
  // Get the 'skip until' time in UTC  
  time_t skip_utc = s_skip_until - get_UTC_offset(t);
  
  // If skipping more than 1 week of alarms, return a value indicating the s_skip_until time will
  // need to be used to calculate the next alarm
  if (day_diff(utc, skip_utc) >= 7) return NEXT_ALARM_SKIPWEEK;
  
  // Scan through alarms over the next 7 days (skipping today if we already had an alarm today)
  for (int d = wday + (strip_time(utc) == s_state.last_reset_day ? 1 : 0); d <= (wday + 7); d++) {
    next = d % 7;
    // Only look at alarms that are enabled and are after now
    if (s_alarms[next].enabled && (d > wday || s_alarms[next].hour > hour || 
                                   (s_alarms[next].hour == hour && s_alarms[next].minute > min))) {
      if (d == wday)
        // If alarm is today, strip time from current UTC time
        next_date = strip_time(utc);
      else
        // Else get UTC midnight for the next alarm
        next_date = strip_time(clock_to_timestamp(ad2wd(next), 0, 0));
      
      if (s_skip_until != 0 && day_diff(utc, next_date) >= 7)
        // If skipping and there are 7 days between now and then, show as skipping at least a week
        // so that today does not get confused with today next week
        return NEXT_ALARM_SKIPWEEK;
      else if (next_date >= strip_time(skip_utc))
        // Else if next date is >= skip date (skip date is 0 if not skipping) we have the next alarm index
        return next;
    }
  }
  
  // No alarms set
  return NEXT_ALARM_NONE;
}

// Gets a timestamp from the alarm index
static time_t alarm_to_timestamp(int8_t alarm) {
  time_t alarm_time = 0;
      
  // Get current time
  time_t curr_time = time(NULL);
  struct tm *t = localtime(&curr_time);
  
  WeekDay alarmday;
  
  if (alarm == NEXT_ALARM_ONETIME) {
    // Calculate whether the one-time alarm is today or tomorrow
    if (s_settings.one_time_alarm.hour > t->tm_hour ||
        (s_settings.one_time_alarm.hour == t->tm_hour &&
         s_settings.one_time_alarm.minute > t->tm_min))
      alarmday = TODAY; 
    else
      alarmday = ad2wd((t->tm_wday + 1) % 7); 
    
    // Get the time for the next alarm
    alarm_time = clock_to_timestamp(alarmday, s_settings.one_time_alarm.hour, 
                                    s_settings.one_time_alarm.minute);
    // Strip seconds
    alarm_time -= alarm_time % 60;
    
  } else if (alarm == NEXT_ALARM_SKIPWEEK) {
    // Calculate the next alarm after the 'skip until' date
    
    // First get skip time in UTC
    time_t skip_utc = s_skip_until - get_UTC_offset(NULL);
    
    // Then get a time struct in the local timezone
    struct tm *alarm_t = localtime(&skip_utc);
    int8_t next_alarm = 0;
    // Then find the next alarm on or after the skip date
    for (int8_t d = 0; d < 7; d++) {
      next_alarm = (alarm_t->tm_wday + d) % 7;
      if (s_alarms[next_alarm].enabled) {
        // Get the wakeup time in UTC
        alarm_time = skip_utc + (d * (24 * 60 * 60)) +
          (s_alarms[next_alarm].hour * (60 * 60)) + (s_alarms[next_alarm].minute * 60);
        break;
      }
    }
    
    if (alarm_time == 0) {
      // This should never happen, but we set the alarm time to something just in case
      alarm_time = skip_utc + (7 * 60 * 60);
    }
  } else {
    if (alarm == t->tm_wday && (s_alarms[alarm].hour > t->tm_hour || 
                                     (s_alarms[alarm].hour == t->tm_hour && 
                                      s_alarms[alarm].minute > t->tm_min))) {
      if (strip_time(curr_time) == s_state.last_reset_day)
        // If the alarm day is the same day as today, but the alarm was also reset today, the 
        // alarm must be for 1 week from now
        alarmday = ad2wd(alarm);
      else
        // If next alarm is today, use the TODAY enum
        alarmday = TODAY;
    } else
      // Else convert the day number to the WeekDay enum
      alarmday = ad2wd(alarm);
  
    // Get the time for the next alarm
    alarm_time = clock_to_timestamp(alarmday, s_alarms[alarm].hour, s_alarms[alarm].minute);
    // Strip seconds
    alarm_time -= alarm_time % 60;
  }
  
  return alarm_time;
}

// Generates the text to show what alarm is next
static void gen_info_str(int8_t next_alarm) {
  
  char day_str[9];
  char time_str[8];
  char timeto_str[20];
  
  if (next_alarm == NEXT_ALARM_NONE) {
    strncpy(s_info, "NO ALARMS SET", sizeof(s_info));
  } else if (next_alarm == NEXT_ALARM_SKIPWEEK) {
    // Get the 'skip until' time in UTC  
    time_t skip_utc = s_skip_until - get_UTC_offset(NULL);
    // Then use localtime to get a date struct back in the local timezone
    struct tm *skip_until = localtime(&skip_utc);
    strftime(s_info, sizeof(s_info), "Skip Until:%n%a, %b %d", skip_until);
  } else {    
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);
    
    if (next_alarm == NEXT_ALARM_ONETIME) {
      // Calculate whether the one-time alarm is today or tomorrow
      if (s_settings.one_time_alarm.hour > t->tm_hour ||
          (s_settings.one_time_alarm.hour == t->tm_hour &&
           s_settings.one_time_alarm.minute > t->tm_min))
        strncpy(day_str, "Today", sizeof(day_str));
      else
        strncpy(day_str, clock_is_24h_style() ? "Tomorrow" : "Tmrw", sizeof(day_str));
      
      gen_alarm_str(&(s_settings.one_time_alarm), time_str, sizeof(time_str));
    } else {
      
      if (next_alarm == t->tm_wday && (s_alarms[next_alarm].hour > t->tm_hour ||
                                       (s_alarms[next_alarm].hour == t->tm_hour &&
                                        s_alarms[next_alarm].minute > t->tm_min))) {
        if (strip_time(temp) == s_state.last_reset_day) {
          // If the alarm day is the same day as today, but the alarm was also reset today, the 
          // alarm must be for 1 week from now
          char day_name[4];
          daynameshort(next_alarm, day_name, sizeof(day_name));
          snprintf(day_str, sizeof(day_str), "Next %s", day_name);
        } else
          strncpy(day_str, "Today", sizeof(day_str));
      } else if (next_alarm == ((t->tm_wday+1)%7))
        strncpy(day_str, clock_is_24h_style() ? "Tomorrow" : "Tmrw", sizeof(day_str));
      else
        daynameshort(next_alarm, day_str, sizeof(day_str));
      
      gen_alarm_str(&s_alarms[next_alarm], time_str, sizeof(time_str));
    }
    
    time_t time_to = alarm_to_timestamp(next_alarm) - time(NULL);
    
    if (time_to < (10 * 60 * 60)) {
      // Add 'In X hrs, Y mins' text
      uint8_t hours = time_to / (3600);
      uint8_t mins = (time_to % (3600)) / 60;
      if (hours == 0) {
        if (mins == 0)
          strcpy(timeto_str, "\nIn <1 minute");
        else
          snprintf(timeto_str, sizeof(timeto_str), "\nIn %d minute%s", mins, (mins == 1) ? "" : "s");
      } else {
        if (mins == 0) {
          snprintf(timeto_str, sizeof(timeto_str), "\nIn %d hour%s", hours, (hours == 1) ? "" : "s");
        } else {
          snprintf(timeto_str, sizeof(timeto_str), "\nIn %d hr%s, %d min%s", hours, (hours == 1) ? "" : "s", mins, (mins == 1) ? "" : "s");
        }
      }
    } else {
      // Do not add any extra info
      timeto_str[0] = '\0';
    }
    
    snprintf(s_info, sizeof(s_info), "Next Alarm:%s\n%s %s", timeto_str, day_str, time_str);
  }
}

// Updates the displayed alarm time (and returns the next alarm day value)
static int8_t update_alarm_display() {
  int8_t next_alarm = get_next_alarm();
  gen_info_str(next_alarm);
  update_info(s_info);
  return next_alarm;
}

static void show_wakeup_error(WakeupId result, time_t wakeup_time, char *wakeup_type) {
  char msg[150];
          
  // If ID is still negative, show error message
  if (result == E_RANGE) {
    char daystr[10];
    char timestr[8];
    struct tm *wt = localtime(&wakeup_time);
    time_t curr_time = time(NULL);
    struct tm *ct = localtime(&curr_time);
    
    if (wt->tm_yday == ct->tm_yday)
      strcpy(daystr, "today");
    else
      dayname(wt->tm_wday, daystr, sizeof(daystr));
    gen_time_str(wt->tm_hour, wt->tm_min, timestr, sizeof(timestr));
    snprintf(msg, sizeof(msg), 
             "Unable to set %s alarm due to another alarm on %s at %s +/-5 minutes in another app. Please either change the alarm time here or the other app.", 
             wakeup_type, daystr, timestr);
  } else {
    snprintf(msg, sizeof(msg),
             "Unknown error while setting %s alarm (%d). A factory reset may be required if this happens again",
             wakeup_type, (int)result);
  }

  show_msg("OOPS", msg, 0, true);
}

// A slightly more robust wakeup scheduler that retries on certain errors and
// can retry for a different wakeup time offset by retry_diff up to retry_max times
static WakeupId wakeup_schedule_robust(time_t wakeup_time, int wakeup_reason, bool missed_alert, int8_t retry_diff, uint8_t retry_max) {
  WakeupId result = 0;
  uint8_t try_count = 0;
  
  result = wakeup_schedule(wakeup_time, wakeup_reason, missed_alert);
  
  if (result < 0) {
    // If result is negative, something went wrong, so make sure all wakeups for this app are cancelled and 
    // try again (don't cancel all for secondary alarms like GooB and DST Check)
    if (wakeup_reason != WAKEUP_REASON_GOOB && wakeup_reason != WAKEUP_REASON_DSTCHECK) wakeup_cancel_all();
    
    result = wakeup_schedule(wakeup_time, wakeup_reason, missed_alert);
    
    if (result == E_RANGE) {
      // E_RANGE means some other app has the same wakeup time +/- 1 minute, so try to set
      // the wakeup for a time up to (retry_diff * retry_max) minutes before/after.
      while (result == E_RANGE && try_count++ < retry_max) {
        wakeup_time += retry_diff;
  
        result = wakeup_schedule(wakeup_time, wakeup_reason, missed_alert);
      }
    }
  }
  
  return result;
}

static void save_state() {
  persist_write_data(STATE_KEY, &s_state, sizeof(s_state));
}

// Updates global Get Out Of Bed monitoring flag and alarm time and saves it in case of an exit
static void set_goob(bool monitoring, time_t goob_time) {
  s_state.goob_monitoring = monitoring;
  s_goob_time = goob_time;
  save_state();
  persist_write_data(GOOBALARMTIME_KEY, &s_goob_time, sizeof(s_goob_time));
}

// Timer handler that sets the wakeup time after a short delay
// (allows UI to refresh beforehand since this sometimes takes a second or 2 for some reason)
static void set_wakeup_delayed(void *data) {
  int8_t next_alarm = s_next_alarm;
  
  // Clear any previous wakeup time
  if (s_wakeup_id != 0 || s_wakeup_goob_id != 0) {
    wakeup_cancel_all();
    s_wakeup_id = 0;
    s_wakeup_goob_id = 0;
  }
  
  if (next_alarm != NEXT_ALARM_NONE) {
    // If there is a next alarm or snooze alarm (next_alarm == -2 when setting snooze wakeup)
    
    time_t curr_time = time(NULL);
    
    if (s_state.snoozing || s_alarm_active) {
      // if snoozing set a wakeup for the snooze period
      // (or even if the alarm is active set a snooze wakeup in case something happens during the alarm)
      
      uint16_t snooze_period = 0;
      
      if (s_settings.dynamic_snooze && s_state.snooze_count > 0) {
        // Shrink snooze time based on snooze count (down to a min. of 3 minutes)
        snooze_period = (s_settings.snooze_delay * 60) / s_state.snooze_count;
        if (snooze_period < 180) snooze_period = 180;
      } else {
        // Set normal snooze period
        snooze_period = s_settings.snooze_delay * 60;
      }
      
      s_snooze_until = curr_time + snooze_period;
      
      
      
      // Set snooze if GooB not enabled or snooze time is still before GooB time 
      if (s_snooze_until < s_goob_time) {
        // Show the snooze wakeup time
        if (s_state.snoozing) show_status(s_snooze_until, S_Snoozing);
        
        // Schedule the wakeup
        s_wakeup_id = wakeup_schedule_robust(s_snooze_until, WAKEUP_REASON_SNOOZE, true, 0, 0);
        
        if (s_wakeup_id < 0) {
          // If ID is still negative, show error message
          show_wakeup_error(s_wakeup_id, s_snooze_until, "snooze");
        }
      }
      
      // Setup GooB wakeup after/instead of snooze if enabled for after alarm time and is still in the future
      if (s_goob_time > curr_time) {
        s_wakeup_goob_id = wakeup_schedule_robust(s_goob_time, WAKEUP_REASON_GOOB, true, 60*((s_goob_time < curr_time + 360) ? 1 : -1), 5);
        
        if (s_wakeup_goob_id < 0)
          // If ID is still negative, show error message
          show_wakeup_error(s_wakeup_goob_id, s_goob_time, "Get Out Of Bed");
        else if (s_goob_time <= s_snooze_until && s_state.snoozing)
          show_status(s_goob_time, S_GooBMonitoring);
      }
    } else {
      // Set wakeup for next alarm
      
      // Get the time for the next alarm
      time_t alarm_time = alarm_to_timestamp(next_alarm);
      
      // If on, set Get Out Of Bed X min after alarm
      if (s_settings.goob_mode == GM_AfterAlarm)
        set_goob((curr_time >= alarm_time && curr_time < (alarm_time + (s_settings.goob_monitor_period * 60))), alarm_time + (s_settings.goob_monitor_period * 60));
      
      // If the smart alarm is on but not active, set the wakeup to the alarm time minus the monitor period
      if (s_settings.smart_alarm && !s_state.monitoring) 
        alarm_time -= s_settings.monitor_period == 0 ? 300 : s_settings.monitor_period * 60;
      
      // If the alarm time is in the past (setting a smart alarm for just a few minutes ahead for example)
      // then adjust it to be 5 seconds in the future
      if (alarm_time < curr_time) alarm_time = curr_time + 5;
      
      uint8_t wakeup_reason = (s_settings.smart_alarm && !s_state.monitoring) ? WAKEUP_REASON_MONITOR : WAKEUP_REASON_ALARM;
      
      // Schedule the wakeup
      s_wakeup_id = wakeup_schedule_robust(alarm_time, wakeup_reason, true, 60*((alarm_time < curr_time + 360) ? 1 : -1), 5);
        
      if (s_wakeup_id < 0) {
          // If ID is still negative, show error message
          show_wakeup_error(s_wakeup_id, alarm_time, "next");
      }
      
      // If smart alarm monitoring, update display with actual alarm time
      if (s_state.monitoring) show_status(alarm_time, S_SmartMonitoring);
      
      // Setup Get Out Of Bed Wakeup
      if (s_goob_time > curr_time) {
        s_wakeup_goob_id = wakeup_schedule_robust(s_goob_time, WAKEUP_REASON_GOOB, true, 60*((s_goob_time < curr_time + 360) ? 1 : -1), 5);
        
        if (s_wakeup_goob_id < 0)
          // If ID is still negative, show error message
          show_wakeup_error(s_wakeup_goob_id, s_goob_time, "Get Out Of Bed");
      }
    }
    
    if (s_settings.dst_check_day != 0) {
      // If DST check is on, set a wakeup for redoing alarms in case of a daylight savings time change
      time_t check_time = clock_to_timestamp(s_settings.dst_check_day, s_settings.dst_check_hour, 0);
      check_time -= check_time % 60;
      wakeup_schedule_robust(check_time, WAKEUP_REASON_DSTCHECK, false, 60, 10);
    }
  }
  
  // Always make sure wakeup ID is saved immediately
  persist_write_int(WAKEUPID_KEY, s_wakeup_id);
  persist_write_int(WAKEUPGOOBID_KEY, s_wakeup_goob_id);
  
  // If app was started for a DST check, close the app now that the wakeups have been redone.
  if (s_dst_check_started)
    window_stack_pop_all(false);
}

// Sets an app wakeup for the specified alarm day
// (next_alarm == -1 means no alarms, next_alarm == -2 means snooze wakeup)
static void set_wakeup(int8_t next_alarm) {
  s_next_alarm = next_alarm;
  // Delay actually setting the wakeup so the UI can update since it takes a second or 2 sometimes
  app_timer_register(250, set_wakeup_delayed, NULL);
}

// Updates global snoozing flag and saves it in case of an exit
static void set_snoozing(bool snoozing) {
  s_state.snoozing = snoozing;
  if (!snoozing) s_snooze_until = 0;
  save_state();
}

// Updates snooze count and saves it in case of an exit
static void set_snoozecount(uint8_t snooze_count) {
  s_state.snooze_count = snooze_count;
  save_state();
}

// Updates global monitoring flag and saves it in case of an exit
static void set_monitoring(bool monitoring) {
  s_state.monitoring = monitoring;
  save_state();
}

// Updates last reset day and saves it in case of an exit
static void set_lastresetday(time_t last_reset_day) {
  s_state.last_reset_day = strip_time(last_reset_day);
  save_state();
}

// Updates flag for skipping next alarm and saves it in case of an exit
static void set_skipuntil(time_t skip_until) {
  s_skip_until = skip_until;
  persist_write_data(SKIPUNTIL_KEY, &s_skip_until, sizeof(s_skip_until));
}

static void save_settings(void *data) {
  // Save all settings
  persist_write_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  persist_write_int(SETTINGSVER_KEY, SETTINGS_VER);
}

// Sets whether the one-time alarm is enabled and saves it in case of an exit
static void set_onetime_enabled(bool enabled) {
  s_settings.one_time_alarm.enabled = enabled;
  save_settings(NULL);
}

static void start_accel();

// Turns off an active alarm or cancels a snooze and sets wakeup for next alarm
static void reset_alarm() {
  int8_t next;
  
  s_alarm_active = false;
  set_snoozing(false);
  set_monitoring(false);
  set_goob(false, s_goob_time);
  set_snoozecount(0);
  s_arm_swing_start = 0;
  s_arm_swing_count = 0;
  s_last_arm_swing_dir = -1;
  
  if (!s_state.goob_monitoring && !s_goob_active && s_settings.goob_mode == GM_AfterStop) {
    time_t curr_time = time(NULL);
    set_goob(true, curr_time + (s_settings.goob_monitor_period * 60));
    s_wakeup_goob_id = wakeup_schedule_robust(s_goob_time, WAKEUP_REASON_GOOB, false, 60*((s_goob_time < curr_time+300) ? 1 : -1), 5);
    persist_write_int(WAKEUPGOOBID_KEY, s_wakeup_goob_id);
    if (s_wakeup_goob_id < 0)
      show_wakeup_error(s_wakeup_goob_id, s_goob_time, "Get Out Of Bed");
    else {
      show_status(s_goob_time, S_GooBMonitoring);
      start_accel();
    }
  } else {
    if (s_settings.one_time_alarm.enabled) set_onetime_enabled(false);
    set_lastresetday(time(NULL));
    
    // Update UI with next alarm details
    show_alarm_ui(false, false);
    next = update_alarm_display();
    
    // Set the next alarm wakeup
    set_wakeup(next);
  }
}

// Snoozes active alarm
static void snooze_alarm() {
  set_snoozing(true);
  set_snoozecount(s_state.snooze_count + 1);
  
  // Set snooze wakeup
  set_wakeup(NEXT_ALARM_SNOOZE);
}

static void vibe_alarm();

// Timer event to start another vibrate segment
static void handle_vibe_timer(void *data) {
  s_vibe_timer = NULL;
  vibe_alarm();
}

// Activate vibration for the alarm
static void vibe_alarm() {
  if (s_vibe_timer) {
    app_timer_cancel(s_vibe_timer);
    s_vibe_timer = NULL;
  }
  
  if ((s_alarm_active || s_goob_active) && ! s_state.snoozing) {
    // If still active and not snoozing
    
    uint8_t pattern_length = 0;
    uint8_t (*vibe_patterns)[3];
    uint32_t (*vibe_segments)[5];
    
    if (s_goob_active) {
      // Get Out Of Bed pattern
      pattern_length = (int)(sizeof(vibe_patterns_goob) / sizeof(vibe_patterns_goob[0]));
      vibe_patterns = vibe_patterns_goob;
      vibe_segments = vibe_segments_goob;
    } else {
      switch (s_settings.vibe_pattern) {
        case VP_NSG:
          // Not-So-Gentle Pattern
          pattern_length = (int)(sizeof(vibe_patterns_strong) / sizeof(vibe_patterns_strong[0]));
          vibe_patterns = vibe_patterns_strong;
          vibe_segments = vibe_segments_strong;
          break;
        case VP_NSG2Snooze:
          if (s_state.snooze_count >= 2) {
            // Not-So-Gentle Pattern after 2 snoozes
            pattern_length = (int)(sizeof(vibe_patterns_strong) / sizeof(vibe_patterns_strong[0]));
            vibe_patterns = vibe_patterns_strong;
            vibe_segments = vibe_segments_strong;
          } else {
            // Gentle Pattern before 2 snoozes
            pattern_length = (int)(sizeof(vibe_patterns_orig) / sizeof(vibe_patterns_orig[0]));
            vibe_patterns = vibe_patterns_orig;
            vibe_segments = vibe_segments_orig;
          }
          break;
        default:
          // Gentle pattern
          pattern_length = (int)(sizeof(vibe_patterns_orig) / sizeof(vibe_patterns_orig[0]));
          vibe_patterns = vibe_patterns_orig;
          vibe_segments = vibe_segments_orig;
          break;
      }
    }
    
    if (s_vibe_count >= pattern_length) {
      // If we've reach the end of the vibrate patterns...
      
      if (((s_settings.dynamic_snooze ? 3 : s_settings.snooze_delay) * s_state.snooze_count) > 60)
        // Reset alarm if snoozed for more than 1 hour
        reset_alarm();
      else
        // Auto-snooze if not turned off
        snooze_alarm();
    } else {
      // Make increasingly long vibrate patterns for the alarm
      
      // Setup timer event for next vibe using pattern array
      if (s_vibe_count < pattern_length) {
        s_vibe_timer = app_timer_register(vibe_patterns[s_vibe_count][0]*1000, handle_vibe_timer, NULL);
      }
      
      // Start current vibe using pattern and segment arrays
      VibePattern pat;
      pat.durations = vibe_segments[vibe_patterns[s_vibe_count][1]];
      pat.num_segments = vibe_patterns[s_vibe_count][2];
      vibes_enqueue_custom_pattern(pat);
      
      s_vibe_count++;
    }
  }
}

// Callback function to indicate when the settings have been closed
// so that various items can be updated
static void settings_update() {
  if (s_loaded) {
    // Reset the last reset day in case alarms were changed
    set_lastresetday(0);
    // Reset skip next too
    set_skipuntil(0);
    // Update the main window auto-close timeout
    update_autoclose_timeout(s_settings.autoclose_timeout);
    
    // Save settings after a delay to allow UI to update
    app_timer_register(500, save_settings, NULL);
  }
  
  // Update next alarm info
  int8_t next_alarm = update_alarm_display();
  
  // Set next wakeup in case alarms were changed
  if (s_loaded) set_wakeup(next_alarm);
}

// Shows the appropriate window for stopping the alarm based on the settings
static void show_stopwin() {
  if (s_settings.konamic_code_on)
    show_konamicode(reset_alarm);
  else
    show_msg("INSTRUCTIONS", "Hold Back button to exit app WITHOUT stopping alarm.\n\nDouble click ANY button to stop the alarm.", 10, false);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_state.snoozing && !s_state.monitoring) {
    // Disable Back button click when snoozing or monitoring sleep so we don't accidentally exit
    
    if (s_alarm_active || s_goob_active) {
      // Any button single click snoozes when alarm active
      snooze_alarm();
    } else {
      // If alarm not active, not snoozing, and not monitoring, exit app
      hide_mainwin();
    }
  } else {
    show_stopwin();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_state.snoozing && !s_state.monitoring) {
    // Disable Up button when snoozing or monitoring
    
    if (s_alarm_active || s_goob_active) {
      // Any button single click snoozes when alarm active
      snooze_alarm();
    } else {
      // Turn all alarms on or off
      s_alarms_on = !s_alarms_on;
      persist_write_bool(ALARMSON_KEY, s_alarms_on);
      update_onoff(s_alarms_on);
      // Reset skip
      set_skipuntil(0);
      // Reset one-time alarm
      if (s_settings.one_time_alarm.enabled) set_onetime_enabled(false);
      
      int8_t next_alarm = update_alarm_display();
      // Redo wakeup
      set_wakeup(s_alarms_on ? next_alarm : NEXT_ALARM_NONE);
    }
  } else {
    show_stopwin();
  }
}

// Callback for when a 'skip until' date is set
static void update_skip(time_t skip_until) {
  set_skipuntil(skip_until);
  // Setting a 'skip until' date resets any one-time alarm
  if (skip_until > 0 && s_settings.one_time_alarm.enabled) set_onetime_enabled(false);
  
  // Update alarm display
  int8_t next_alarm = update_alarm_display();
  // Update wakeup
  set_wakeup(next_alarm);
}

static void up_longclick_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_alarm_active && !s_goob_active && !s_state.snoozing && !s_state.monitoring && !s_state.goob_monitoring) {
    // Disable long Up button press when alarm active, snoozing, or monitoring
    
    // Show window for setting 'skip until' date
    show_skipwin(s_skip_until, update_skip);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Single click snoozes when alarm active, Select button single click is disabled otherwise
  if (!s_state.snoozing && !s_state.monitoring) {
    if (s_alarm_active || s_goob_active) 
      snooze_alarm();
  } else {
    show_stopwin();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_state.snoozing && !s_state.monitoring) {
    // Disable Up button when snoozing or monitoring
    
    if (s_alarm_active || s_goob_active)
      // Any button single click snoozes when alarm active
      snooze_alarm();
    else
      // Show settings screen with current alarms and setings and a callback for when closed
      show_settings(s_alarms, &s_settings, settings_update);
  } else {
    show_stopwin();
  }
}

static void multiclick_handler(ClickRecognizerRef recognizer, void *context) {
  // If alarm is active (or snoozing) or smart alarm is active, double clicking
  // any button will reset the alarm
  if (s_alarm_active || s_state.monitoring || s_goob_active || s_state.goob_monitoring) {
    if (s_settings.konamic_code_on) {
      if (s_alarm_active || s_goob_active) snooze_alarm();
      show_konamicode(reset_alarm);
    } else
      reset_alarm();
  }
}

// Trap single and double clicks for ALL buttons
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_BACK, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 300, true, multiclick_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 1000, up_longclick_handler, NULL);
}

// Start the alarm, including vibrating the Pebble
static void start_alarm() {
  s_alarm_active = true;
  s_goob_active = false;
  set_snoozing(false);
  set_monitoring(false);
  show_alarm_ui(true, false);
  
  if (s_settings.goob_mode == GM_AfterAlarm) {
    // Start Get Out Of Bed monitoring if set to start after alarm start
    //set_goob(true, time(NULL) + (s_settings.goob_monitor_period * 60));
    set_goob(true, s_goob_time == 0 || s_goob_time < time(NULL) ? time(NULL) + (s_settings.goob_monitor_period * 60) : s_goob_time);
    start_accel();
  }
  
  // Set snooze wakeup in case app is closed with the alarm vibrating
  set_wakeup(NEXT_ALARM_SNOOZE);
  
  // Start alarm vibrate
  s_vibe_count = 0;
  vibe_alarm();
}

// Start the Get Out Of Bed alarm, including vibrating the Pebble
static void start_goob_alarm() {
  s_alarm_active = false;
  s_goob_active = true;
  set_snoozing(false);
  set_monitoring(false);
  show_alarm_ui(true, true);
  // Set snooze wakeup in case app is closed with the alarm vibrating
  set_wakeup(NEXT_ALARM_SNOOZE);

  // Start alarm vibrate
  s_vibe_count = 0;
  vibe_alarm();
}

// Timer event to unsubscribe the accelerometer service after a delay
// (without the delay it could be called during the service callback, which crashes the app)
static void unsub_accel_delay(void *data) {
  if (s_accel_service_sub && !s_state.monitoring && !s_state.goob_monitoring && ((!s_alarm_active && !s_goob_active && !s_state.snoozing) || !s_settings.easy_light)) {
    accel_data_service_unsubscribe();
    s_accel_service_sub = false;
  }
}

// Integer division with rounding
// int16_t divide(int16_t n, int16_t d)
// {
//   return ((n < 0) ^ (d < 0)) ? ((n - d/2) / d) : ((n + d/2) / d);
// }

// Handle accelerometer data while smart alarm is active or alarm is active/snoozing
// to detect stirring or lifting watch for Easy Light respectively
static void accel_handler(AccelData *data, uint32_t num_samples) {
  if (s_alarm_active || s_goob_active || s_state.snoozing || s_state.monitoring || s_state.goob_monitoring) {
    if (s_alarm_active || s_goob_active || s_state.snoozing) {
      if (s_settings.easy_light) {
        for (uint32_t i = 0; i < num_samples; i++) {
          // If watch screen is held vertically (as if looking at the time) while alarm is on or snoozing,
          // turn the light on for a few seconds
          if (!data[i].did_vibrate) {
            if ((data[i].x > -1250 && data[i].x < -750) || (data[i].x > 750 && data[i].x < 1250) ||
                (data[i].y > -1250 && data[i].y < -750)) {
              if (!s_light_shown && (data[i].timestamp - s_last_easylight) > 3000) {
                // Record when light was last shown and has been shown in this position
                // so it doesn't keep coming on
                s_last_easylight = data[i].timestamp;
                s_light_shown = true;
                light_enable_interaction();
                break;
              } 
            } else {
              // When watch is lowered, reset this flag
              s_light_shown = false;
            }
          }
        }
      }
    } else if (s_state.monitoring) {
      // Smart Alarm is active, so check for an accumultaive amount of movement, which may indicate stirring
      
      // Initialize last x, y, z readings
      if (s_last_x == 0) s_last_x = data[0].x;
      if (s_last_y == 0) s_last_y = data[0].y;
      if (s_last_z == 0) s_last_z = data[0].z;
      
      int diff;
      
      // Get the accel difference for each direction for the last sample period and as positive values
      // and add to the movement counter
      for (uint32_t i = 0; i < num_samples; i++) {
        if (!data[i].did_vibrate) {
          diff = s_last_x - data[i].x;
          s_movement += (diff > 0 ? diff : -diff);
          diff = s_last_y - data[i].y;
          s_movement += (diff > 0 ? diff : -diff);
          diff = s_last_z - data[i].z;
          s_movement += (diff > 0 ? diff : -diff);
          s_last_x = data[i].x;
          s_last_y = data[i].y;
          s_last_z = data[i].z;
        }
      }
      
      // At rest, movement value can accumulate by about 200, so subtract X on every call so
      // that sustained movement is required to trigger the alarm
      s_movement -= REST_MOVEMENT;
      
      if (s_movement < 0)
        // Movement counter cannot be negative
        s_movement = 0;
      else if ((s_settings.sensitivity == MS_LOW && s_movement > MOVEMENT_THRESHOLD_HIGH) ||
               (s_settings.sensitivity == MS_MEDIUM && s_movement > MOVEMENT_THRESHOLD_MID) ||
               (s_settings.sensitivity == MS_HIGH && s_movement > MOVEMENT_THRESHOLD_LOW))
        // If movement counter is over the threshold, activate alarm
        start_alarm();
      
#ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Movement: %d", s_movement);
#endif
    }
    
    if (s_state.goob_monitoring && ((!s_alarm_active && !s_goob_active) || (s_state.snoozing && s_goob_time <= s_snooze_until))) {
      // Monitor for movement that will cancel the Get Out Of Bed alarm
      // (5 arm swings with no more than 2 seconds between swings will cancel alarm)
      
      if (time(NULL) - s_arm_swing_start > 2) {
        // Reset arm swing stats if more than 2 seconds have passed since last registered swing
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Resetting arm swing stats");
        s_arm_swing_start = time(NULL);
        s_arm_swing_count = 0;
        s_last_arm_swing_dir = -1;
        s_x_filtered = data[0].x;
        s_y_filtered = data[0].y;
      }
      
      for (uint32_t i = 0; i < num_samples; i++) {
        if (!data[i].did_vibrate) {
          // Perform single pass IIR filter on accelerometer values to get smoother motion
//           s_x_filtered = (divide(s_x_filtered * FILTER_K_NUM, FILTER_K_DEN)) + divide((FILTER_K_DEN - FILTER_K_NUM) * data[i].x, FILTER_K_DEN);
//           s_y_filtered = (divide(s_y_filtered * FILTER_K_NUM, FILTER_K_DEN)) + divide((FILTER_K_DEN - FILTER_K_NUM) * data[i].y, FILTER_K_DEN);
          s_x_filtered = (s_x_filtered >> 1) + (data[i].x >> 1);
          s_y_filtered = (s_y_filtered >> 1) + (data[i].y >> 1);;
          
          // Very simplistic arm swing detection
          if (s_x_filtered <= -500 || s_x_filtered >= 500) {
            // Arm is probably somewhat vertical
            if ((s_y_filtered >= 350 && !s_last_arm_swing_dir) ||
                (s_y_filtered <= 350 && s_last_arm_swing_dir)) {
              // Arm probably changing direction, so count as a swing every other time
              s_last_arm_swing_dir ^= true;
              if (s_last_arm_swing_dir) {
                s_arm_swing_count++;
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "Arm swing count: %d", s_arm_swing_count);
                // Restart idle countdown
                s_arm_swing_start = time(NULL);
              }
            }
          }
        }
      }
      
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "GooB Monitoring - x: %d, y: %d", s_x_filtered, s_y_filtered);
      
      if (s_arm_swing_count >= 5) reset_alarm();
    }
  } else if (s_accel_service_sub) {
    // Stop monitoring for movement if nothing is active
    // (Delayed by 250ms since Pebble doesn't like the accel service being unsubscribed during this call)
    app_timer_register(250, unsub_accel_delay, NULL);
  }
}

// Start monitoring the accelerometer for either the Smart Alarm or Easy Light
static void start_accel() {
  if (!s_accel_service_sub) {
    accel_data_service_subscribe(5, accel_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    s_accel_service_sub = true;
  }
}

// Handler for when the wakeup time occurs
static void wakeup_handler(WakeupId id, int32_t reason) {
  if (reason == WAKEUP_REASON_DSTCHECK) {
    // If wakeup was for Daylight Savings Time check and we're not in the middle
    // of an active alarm or monitoring (which will update the wakeup times anyway), then
    // redo the alarm wakeups
    if (!s_alarm_active && !s_state.snoozing && !s_state.monitoring)
      set_wakeup(s_alarms_on ? get_next_alarm() : -1);
  } else {
    // Clear last reset day since either normal alarm or smart alarm is now active 
    set_lastresetday(0);
    // Also reset skip
    set_skipuntil(0);
    
    if (reason != WAKEUP_REASON_SNOOZE) {
      set_snoozecount(0);
    }
    
    if (reason == WAKEUP_REASON_MONITOR) {
      // Start monitoring activity for stirring
      set_monitoring(true);
      s_last_x = 0;
      s_last_y = 0;
      s_last_z = 0;
      s_movement = 0;
      // Set wakeup for the actual alarm time in case we're dead to the world or something goes wrong during monitoring
      set_wakeup(get_next_alarm());
    } else if (reason == WAKEUP_REASON_GOOB) {
      start_goob_alarm();
    } else {
      // Activate the alarm
      start_alarm();
    }
    
    // Monitor movement for either Smart Alarm or Easy Light
    if (s_state.monitoring || s_settings.easy_light) {
      start_accel();
    }
  }
}

// Handle clock change events
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Show the current time on the main screen
  if ((units_changed & MINUTE_UNIT) != 0) {
    update_clock();
    if (!s_alarm_active && !s_state.snoozing && !s_state.monitoring) update_alarm_display();
  }
}

// Gets int from persist if exists, else returns default value
static int persist_int(const uint32_t persist_key, int default_val) {
  if (persist_exists(persist_key))
    return persist_read_int(persist_key);
  else
    return default_val;
}

// Gets bool from persist if exists, else returns default value
static bool persist_bool(const uint32_t persist_key, bool default_val) {
  if (persist_exists(persist_key))
    return persist_read_bool(persist_key);
  else
    return default_val;
}

static void init(void) {
  
  // Load all the settings
  persist_read_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  
  if (persist_exists(SETTINGS_KEY)) {
    switch (persist_int(SETTINGSVER_KEY, 1)) {
      case 1:
      default:
        persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
        break;
    }
  } else {
    s_settings.snooze_delay = persist_int(SNOOZEDELAY_KEY, 9);
    s_settings.dynamic_snooze = persist_bool(DYNAMICSNOOZE_KEY, true);
    s_settings.easy_light = persist_bool(EASYLIGHT_KEY, true);
    s_settings.smart_alarm = persist_bool(SMARTALARM_KEY, true);
    s_settings.monitor_period = persist_int(MONITORPERIOD_KEY, 30);
    s_settings.sensitivity = persist_int(MOVESENSITIVITY_KEY, MS_MEDIUM);
    s_settings.dst_check_day = persist_int(DSTCHECKDAY_KEY, SUNDAY);
    s_settings.dst_check_hour = persist_int(DSTCHECKHOUR_KEY, 4);
    s_settings.konamic_code_on = persist_read_bool(KONAMICODEON_KEY);
    s_settings.vibe_pattern = persist_int(VIBEPATTERN_KEY, VP_Gentle);
    if (persist_exists(ONETIMEALARM_KEY))
      persist_read_data(ONETIMEALARM_KEY, &(s_settings.one_time_alarm), sizeof(s_settings.one_time_alarm));
    else
      s_settings.one_time_alarm.enabled = false;
    s_settings.autoclose_timeout = persist_int(AUTOCLOSETIMEOUT_KEY, 0);
    s_settings.goob_mode = persist_int(GOOBMODE_KEY, GM_Off);
    s_settings.goob_monitor_period = persist_int(GOOBPERIOD_KEY, 5);  
  }
   
  // Restore state
  s_alarms_on = persist_bool(ALARMSON_KEY, true);
  s_wakeup_id = persist_int(WAKEUPID_KEY, 0);
  s_wakeup_goob_id = persist_int(WAKEUPGOOBID_KEY, 0);
//   persist_read_data(LASTRESETDATE_KEY, &s_last_reset_day, sizeof(s_last_reset_day));
//   s_snooze_count = persist_int(SNOOZECOUNT_KEY, 0);
//   s_snoozing = persist_read_bool(SNOOZINGON_KEY);
//   s_monitoring = persist_read_bool(MONITORINGON_KEY);
//   s_goob_monitoring = persist_read_bool(GOOBMONITORINGON_KEY);
  persist_read_data(STATE_KEY, &s_state, sizeof(s_state));
  persist_read_data(SKIPUNTIL_KEY, &s_skip_until, sizeof(s_skip_until));
  persist_read_data(GOOBALARMTIME_KEY, &s_goob_time, sizeof(s_goob_time));
  
  // Show the main screen and update the UI
  show_mainwin(s_settings.autoclose_timeout);
  init_click_events(click_config_provider);
  update_onoff(s_alarms_on);
  settings_update();
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  wakeup_service_subscribe(wakeup_handler);
  
  if (s_alarms_on) {
    if(launch_reason() == APP_LAUNCH_WAKEUP) {
      // The app was started by a wakeup event
    
      // Get details and handle the event
      WakeupId id = 0;
      int32_t reason = 0;
      wakeup_get_launch_event(&id, &reason);
      
      // It app was started for a DST check, exit once the wakeups have been redone (happens on a timer)
      if (reason == WAKEUP_REASON_DSTCHECK) s_dst_check_started = true;
      
      wakeup_handler(id, reason);
       
    } else {
      // Make sure a wakeup event is set if needed
      time_t wakeuptime = 0;
      time_t wakeupgoobtime = 0;
      if ((s_wakeup_id == 0 || !wakeup_query(s_wakeup_id, &wakeuptime)) &&
          (s_wakeup_goob_id == 0 || !wakeup_query(s_wakeup_goob_id, &wakeupgoobtime)))
        set_wakeup(get_next_alarm());
      else if (s_wakeup_id != 0 || s_wakeup_goob_id != 0) {
        // Else if recovering from a crash or forced exit, restart any snoozing/monitoring
        if (s_state.goob_monitoring) {
          show_status(wakeupgoobtime, S_GooBMonitoring);
          start_accel();
        } else if (s_state.snoozing) {
          s_alarm_active = true;
          s_snooze_until = wakeuptime;
          show_status(wakeuptime, S_Snoozing);
          if (s_settings.easy_light) start_accel();
        } else if (s_state.monitoring) {
          show_status(wakeuptime, S_SmartMonitoring);
          start_accel();
        }
      }
    }
  }
  
  s_loaded = true;
}

static void deinit(void) {
  
  if (s_accel_service_sub) accel_data_service_unsubscribe();
  
  hide_mainwin();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}