#include "utils.h"

size_t strlen(char* str) {
    unsigned int len = 0;
    while(str[len] != '\0')
        len++;
    return len;
}

char* strrev(char* str) {
    unsigned int len = strlen(str);

    for(unsigned int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }

    return str;
}

char* itoa(int num, char* buff, int radix) {
    int i;
    unsigned int n;

    if(num == 0) {
        buff[0] = '0';
        buff[1] = '\0';
        return buff;
    }

    int sign = (radix == 10 && num < 0);    
    if(sign) n = -num;
    else n = (unsigned)num;

    int cnt = 0;
    while(n) {
        i = n % radix;
        n /= radix;
        if(i < 10) buff[cnt] = i + '0';
        else buff[cnt] = i + 'a' - 10;
        cnt++;
    }

    if(sign) {
        buff[cnt] = '-';
        cnt++;
    }
    buff[cnt] = '\0';

    strrev(buff);

    return buff;
}

bool strcmp(char* str1, char* str2) {
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
    char* ret = 0;

    int p = 0;
    for(int i = p1; i < p2; i++) {
        ret[p++] = str[i];
    }
    ret[p] = '\0';

    return ret;
}

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
