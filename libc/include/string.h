#pragma once

#include <sys/cdefs.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void* memset(void* bufptr, int value, size_t size);
void* memmove(void* dstptr, const void* srcptr, size_t size);
int memcmp(const void* aptr, const void* bptr, size_t size);
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size);

size_t strlen(const char* str);
char* strrev(char* str);
bool strcmp(char* str1, char* str2);
char* strtok(char* str, const char* delimiters);
