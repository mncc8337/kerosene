#pragma once

#include "stdbool.h"
#include "time.h"

enum KEYBOARD_LAYOUT {
    KBD_LAYOUT_US
};

enum TIMEZONE {
    TIMEZONE_NONE,
    // this is future job
};

int locale_get_keyboard_layout();
void locale_set_keyboard_layout(int layout);
int locale_get_timezone();
void locale_set_timezone(int zone);
char locale_map_key(unsigned keycode, bool shift);
struct tm* locale_map_timezone(struct tm* time);
