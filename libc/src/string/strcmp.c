#include <string.h>

int strcmp(const char* str1, const char* str2) {
    while(*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

// TODO: implement ctype.h
static char toupper(char chr) {
    bool is_lower = (chr >= 'a' && chr <= 'z');
    chr -= 32 * is_lower;
    return chr;
}

int strcmp_case_insensitive(const char* str1, const char* str2) {
    while(*str1 && *str2 && toupper((unsigned char)*str1) == toupper((unsigned char)*str2)) {
        str1++;
        str2++;
    }
    return toupper((unsigned char)*str1) - toupper((unsigned char)*str2);
}
