#include <string.h>
#include <stdbool.h>

char* strtok_r(char* str, const char* delimiters, char** token_ptr) {
    // dereference it to eliminate pointer nightmare
    // then only need to remember to sync it when return
    char* token = *token_ptr;
    char* tok_start = NULL;

    if(str != NULL) token = str;
    else if(!token || *token == '\0') goto ret;

    // skip leading delimiters
    while(token[0] != '\0') {
        bool delimiters_exist = false;
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(token[0] == delimiters[i]) {
                delimiters_exist = true;
                break;
            }
        }
        if(delimiters_exist) token++;
        else break;
    }
    // the string is full of delimiters
    if(token[0] == '\0') goto ret;

    int pos = 0;
    while(token[pos] != '\0') {
        for(int i = 0; delimiters[i] != '\0'; i++) {
            if(delimiters[i] != token[pos]) continue;
            token[pos] = '\0';
            tok_start = token;
            token += pos + 1;
            goto ret;
        }
        pos++;
    }

    // reached the end of the string
    tok_start = token;
    token += pos;

ret:
    *token_ptr = token;
    return tok_start;
}

