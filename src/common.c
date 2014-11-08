#include <pebble.h>
#include "common.h"

void dayname(int day, char *daystr, int slen) {
  switch (day) {
    case 0:
      strncpy(daystr, "Sunday", slen);
      break;
    case 1:
      strncpy(daystr, "Monday", slen);
      break;
    case 2:
      strncpy(daystr, "Tuesday", slen);
      break;
    case 3:
      strncpy(daystr, "Wednesday", slen);
      break;
    case 4:
      strncpy(daystr, "Thursday", slen);
      break;
    case 5:
      strncpy(daystr, "Friday", slen);
      break;
    case 6:
      strncpy(daystr, "Saturday", slen);
      break;
    default:
      strncpy(daystr, "", slen);
  }
}

void daynameshort(int day, char *daystr, int slen) {
  switch (day) {
    case 0:
      strncpy(daystr, "Sun", slen);
      break;
    case 1:
      strncpy(daystr, "Mon", slen);
      break;
    case 2:
      strncpy(daystr, "Tue", slen);
      break;
    case 3:
      strncpy(daystr, "Wed", slen);
      break;
    case 4:
      strncpy(daystr, "Thu", slen);
      break;
    case 5:
      strncpy(daystr, "Fri", slen);
      break;
    case 6:
      strncpy(daystr, "Sat", slen);
      break;
    default:
      strncpy(daystr, "", slen);
  }
}

void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen) {
  if (alarmtime->enabled) {
    if (clock_is_24h_style())
      snprintf(alarmstr, slen, "%d:%.2d", alarmtime->hour, alarmtime->minute);
    else
      snprintf(alarmstr, slen, "%d:%.2d%s", alarmtime->hour > 12 ? alarmtime->hour - 12 : 
                                               alarmtime->hour == 0 ? 12 : alarmtime->hour, 
               alarmtime->minute, alarmtime->hour > 12 ? "PM" : "AM");
  } else {
    strncpy(alarmstr, "OFF", slen);
  }
}

WeekDay ad2wd(AlarmDay alarmday) {
  switch (alarmday) {
    case A_SUNDAY:
      return SUNDAY;
    case A_MONDAY:
      return MONDAY;
    case A_TUESDAY:
      return TUESDAY;
    case A_WEDNESDAY:
      return WEDNESDAY;
    case A_THURSDAY:
      return THURSDAY;
    case A_FRIDAY:
      return FRIDAY;
    case A_SATURDAY:
      return SATURDAY;
    default:
      return TODAY;
  }
}