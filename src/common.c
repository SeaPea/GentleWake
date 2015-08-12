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

void gen_time_str(int hour, int min, char *timestr, int slen) {
  if (clock_is_24h_style())
      snprintf(timestr, slen, "%d:%.2d", hour, min);
    else
      snprintf(timestr, slen, "%d:%.2d%s", hour > 12 ? hour - 12 : hour == 0 ? 12 : hour, min,
               hour >= 12 ? "PM" : "AM");
}

void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen) {
  if (alarmtime->enabled) {
    gen_time_str(alarmtime->hour, alarmtime->minute, alarmstr, slen);
  } else {
    strncpy(alarmstr, "OFF", slen);
  }
}

// Strip time component from a timestamp (leaving the date part)
time_t strip_time(time_t timestamp) {
  return timestamp - (timestamp % (60*60*24));
}

// Calculates the number of days (not 24 hour periods) between 2 dates where date1 is older
// than date2 (if date2 is older a negative number will be returned)
int64_t day_diff(time_t date1, time_t date2) {
  return ((strip_time(date2) - strip_time(date1)) / (60*60*24));
}

// Gets the UTC offset of the local time in seconds 
// (pass in an existing localtime struct tm to save creating another one, or else pass NULL)
time_t get_UTC_offset(struct tm *t) {
#ifdef PBL_SDK_2
  // SDK2 uses localtime instead of UTC for all time functions so always return 0
  return 0; 
#else
  if (t == NULL) {
    time_t temp;
    temp = time(NULL);
    t = localtime(&temp);
  }
  
  return t->tm_gmtoff + ((t->tm_isdst > 0) ? 3600 : 0);
#endif 
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