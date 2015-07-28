#pragma once
#include <pebble.h>
  
#ifdef PBL_COLOR
#define IF_COLOR(statement)   (statement)
#define IF_BW(statement)
#define IF_COLORBW(color, bw) (color)
#define COLOR_SCREEN 1
#else
#define IF_COLOR(statement)
#define IF_BW(statement)    (statement)
#define IF_COLORBW(color, bw) (bw)
#define COLOR_SCREEN 0
#endif

#ifdef PBL_SDK_3
#define IF_32(sdk3, sdk2) (sdk3)
#define IF_3(sdk3) (sdk3)
#define IF_2(sdk2)
#else
#define IF_32(sdk3, sdk2) (sdk2)
#define IF_3(sdk3)
#define IF_2(sdk2) (sdk2)
#endif
  
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

typedef enum VibePatterns {
  VP_Gentle = 0,
  VP_NSG = 1, // Not-So-Gentle
  VP_NSG2Snooze = 2
} VibePatterns;

struct Settings_st {
  uint8_t snooze_delay;
  bool dynamic_snooze;
  bool easy_light;
  bool smart_alarm;
  uint8_t monitor_period;
  MoveSensitivty sensitivity;
  WeekDay dst_check_day;
  uint8_t dst_check_hour;
  bool konamic_code_on;
  VibePatterns vibe_pattern;
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
time_t strip_time(time_t timestamp);
int64_t day_diff(time_t date1, time_t date2);
time_t get_UTC_offset(struct tm *t);
WeekDay ad2wd(AlarmDay alarmday);