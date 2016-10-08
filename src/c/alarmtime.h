#pragma once
#include <pebble.h>

typedef void (*AlarmTimeCallBack)(int8_t day, uint8_t hour, uint8_t minute);

void show_alarmtime(int8_t day, uint8_t hour, uint8_t minute, AlarmTimeCallBack set_event);
void hide_alarmtime(void);