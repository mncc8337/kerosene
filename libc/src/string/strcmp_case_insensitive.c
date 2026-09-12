#include <string.h>
#include <ctype.h>

int strcmp_case_insensitive(const char* str1, const char* str2) {
    while(*str1 && *str2 && toupper((unsigned char)*str1) == toupper((unsigned char)*str2)) {
        str1++;
        str2++;
    }
    return toupper((unsigned char)*str1) - toupper((unsigned char)*str2);
}

