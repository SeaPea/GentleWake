#pragma once
#include <pebble.h>

typedef void (*SettingsClosedCallBack)();
  
typedef struct alarm {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
 } __attribute__((__packed__)) alarm;

typedef enum MoveSensitivity {
  MS_LOW = 1,
  MS_MEDIUM = 2,
  MS_HIGH = 3
} MoveSensitivty;

struct Settings_st {
  uint8_t snooze_delay;
  bool dynamic_snooze;
  bool easy_light;
  bool smart_alarm;
  uint8_t monitor_period;
  MoveSensitivty sensitivity;
  WeekDay dst_check_day;
  uint8_t dst_check_hour;
};

typedef enum AlarmDay {
  A_SUNDAY = 0,
  A_MONDAY = 1,
  A_TUESDAY = 2,
  A_WEDNESDAY = 3,
  A_THURSDAY = 4,
  A_FRIDAY = 5,
  A_SATURDAY = 6
} AlarmDay;

void dayname(int day, char *daystr, int slen);
void daynameshort(int day, char *daystr, int slen);
void gen_time_str(int hour, int min, char *timestr, int slen);
void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen);
WeekDay ad2wd(AlarmDay alarmday);