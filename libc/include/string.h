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
int strcmp(const char* str1, const char* str2);
int strcmp_case_insensitive(const char* str1, const char* str2);
char* strtok(char* str, const char* delimiters, char** old);
