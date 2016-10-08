#pragma once
#include <pebble.h>

void show_msg(char *title, char *msg, uint8_t hide_after, bool vibe);
void hide_msg(void);