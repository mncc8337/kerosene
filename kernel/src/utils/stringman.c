#include "utils.h"

// return an array fill with startpoint and endpoint of tokens separated by sep
int tokenize(char* string, char sep, int* token_pos) {
    int string_pos = 0;
    int token_count = 0;

    while(string[string_pos] != '\0') {
        if(string[string_pos] == sep) {
            // repeat until meeting a different char (including null)
            while(string[string_pos] == sep) string_pos++;
        }
        else {
            token_pos[token_count * 2] = string_pos;
            // increase until meeting a sep or null
            while(string[string_pos] != sep && string[string_pos] != '\0')
                string_pos++;
            token_pos[token_count * 2 + 1] = string_pos - 1;
            token_count++;
        }
    }
    return token_count;
}
