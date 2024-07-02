#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// string.c
size_t strlen(char* str);
char* strrev(char* str);
char* itoa(int num, char* buff, int radix);
bool strcmp(char* str1, char* str2);
char* substr(char* str, unsigned int p1, unsigned int p2);
int tokenize(char* string, char sep, int* token_pos);
