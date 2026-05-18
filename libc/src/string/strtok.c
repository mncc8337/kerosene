#include <string.h>
#include <stdbool.h>

static char* old = NULL;

char* strtok(char* str, const char* delimiters) {
    if(str != NULL) old = str;
    else if(!old || *old == '\0') return NULL;

    // skip leading delimiters
    while(old[0] != '\0') {
        bool delimiters_exist = false;
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(old[0] == delimiters[i]) {
                delimiters_exist = true;
                break;
            }
        }
        if(delimiters_exist) old++;
        else break;
    }
    // the string is full of delimiters
    if(old[0] == '\0') return NULL;

    char* tok_start;

    int pos = 0;
    while(old[pos] != '\0') {
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(delimiters[i] != old[pos]) continue;
            old[pos] = '\0';
            tok_start = old;
            old += pos + 1;
            return tok_start;
        }
        pos++;
    }

    // reached the end of the string
    tok_start = old;
    old += pos;
    return tok_start;
}
