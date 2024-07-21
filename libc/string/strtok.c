#include "string.h"

static char* old;

char* strtok(char* str, const char* delimiters) {
    if(str != NULL) old = str;
    else if(*old == '\0') return NULL;

    int pos = 0;
    while(old[pos] != '\0') {
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(delimiters[i] == old[pos]) {
                // skip duplicate delimiter
                int dup = 0;
                while(old[pos+dup] == delimiters[i]) dup++;
                old[pos] = '\0';
                old += pos + dup;
                return old - pos - 1;
            }
        }
        pos++;
    }

    // reached the end of the string
    old += pos;
    return old - pos;
}
