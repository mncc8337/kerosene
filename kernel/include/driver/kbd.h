#pragma once

#include "system.h"

void get_char(char* dest);
void get_string(char* dest, char end, void (*_callback)(char));
