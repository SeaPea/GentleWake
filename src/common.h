#pragma once
#include <pebble.h>

typedef struct alarm {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
 } __attribute__((__packed__)) alarm;

void dayname(int day, char *daystr, int slen);
void daynameshort(int day, char *daystr, int slen);
void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen);