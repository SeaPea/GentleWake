#include <pebble.h>
#include "common.h"

void dayname(int day, char *daystr) {
  switch (day) {
    case 0:
      strncpy(daystr, "Sunday", 10);
      break;
    case 1:
      strncpy(daystr, "Monday", 10);
      break;
    case 2:
      strncpy(daystr, "Tuesday", 10);
      break;
    case 3:
      strncpy(daystr, "Wednesday", 10);
      break;
    case 4:
      strncpy(daystr, "Thursday", 10);
      break;
    case 5:
      strncpy(daystr, "Friday", 10);
      break;
    case 6:
      strncpy(daystr, "Saturday", 10);
      break;
    default:
      strncpy(daystr, "", 10);
  }
}