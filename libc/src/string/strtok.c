#include <string.h>

char* strtok(char* str, const char* delimiters) {
    static char* p = NULL;
    return strtok_r(str, delimiters, &p);
}
