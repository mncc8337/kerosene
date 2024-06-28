#include "system.h"

int string_len(char* str) {
    int len = 0;
    while(str[len] != '\0')
        len++;
    return len;
}

char* to_string(int num) {
    char* res;

    if(num == 0) {
        res[0] = '0';
        res[1] = '\0';
        return res;
    }

    unsigned int cnt = 0;

    bool _signed = num < 0;
    if(_signed) {
        num *= -1;
        res[0] = '-';
        cnt++;
    }

    while(num > 0) {
        res[cnt] = num % 10 + '0';
        num /= 10;
        cnt++;
    }
    res[cnt] = '\0';

    // reverse the string
    char tmp ;
    for(int i = _signed; i < (cnt - _signed)/2; i++) {
        tmp = res[i];
        res[i] = res[cnt - i - 1 + _signed];
        res[cnt - i - 1 + _signed] = tmp;
    }

    return res;
}

bool string_cmp(char* str1, char* str2) {
    int t = 0;
    while(str1[t] == str2[t]) {
        if(str1[t] == '\0' || str2[t] == '\0')
            break;
        t++;
    }
    if(str1[t] == '\0' && str2[t] == '\0')
        return true;
    return false;
}

char* substr(char* str, unsigned int p1, unsigned int p2) {
    char* ret;

    int p = 0;
    for(int i = p1; i < p2; i++) {
        ret[p] = str[i];
        p++;
    }
    ret[p] = '\0';

    return ret;
}

// return an array fill with startpoint and endpoint of tokens separated by sep
void tokenize(char* string, char sep, int* token_pos, int* token_count) {
    int string_pos = 0;
    *token_count = 0;

    while(string[string_pos] != '\0') {
        if(string[string_pos] == sep) {
            // repeat until meeting a different char (including null)
            while(string[string_pos] == sep) string_pos++;
        }
        else {
            token_pos[(*token_count) * 2] = string_pos;
            // increase until meeting a sep or null
            while(string[string_pos] != sep && string[string_pos] != '\0')
                string_pos++;
            token_pos[(*token_count) * 2 + 1] = string_pos - 1;
            (*token_count)++;
        }
    }
}
