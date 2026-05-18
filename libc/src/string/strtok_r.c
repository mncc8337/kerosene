#include <string.h>
#include <stdbool.h>

char* strtok_r(char* str, const char* delimiters, char** old_ptr) {
    // dereference it to eliminate pointer nightmare
    // then only need to remember to sync it when return
    char* old = *old_ptr;
    char* tok_start = NULL;

    if(str != NULL) old = str;
    else if(!old || *old == '\0') goto ret;

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
    if(old[0] == '\0') goto ret;

    int pos = 0;
    while(old[pos] != '\0') {
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(delimiters[i] != old[pos]) continue;
            old[pos] = '\0';
            tok_start = old;
            old += pos + 1;
            goto ret;
        }
        pos++;
    }

    // reached the end of the string
    tok_start = old;
    old += pos;

ret:
    *old_ptr = old;
    return tok_start;
}

