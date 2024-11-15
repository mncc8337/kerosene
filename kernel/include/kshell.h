#pragma once

#include "stdbool.h"

#define NODE_STACK_MAX_LENGTH 64

bool shell_init();
void shell_set_fs(int id);
void shell_process_prompt(char* prompts, unsigned prompts_len);
void shell_start();
