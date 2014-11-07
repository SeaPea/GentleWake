#pragma once
#include <pebble.h>

typedef void (*AlarmChangeCallBack)();
  
typedef struct alarm {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
 } __attribute__((__packed__)) alarm;

struct Settings_st {
  int snooze_delay;
  bool dynamic_snooze;
  bool smart_alarm;
  int monitor_period;
};

void dayname(int day, char *daystr, int slen);
void daynameshort(int day, char *daystr, int slen);
void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen);