#include <string.h>

int strncmp(const char* str1, const char* str2, size_t n) {
    while(n > 0) {
        if(*str1 != *str2) {
            return *(const unsigned char*)str1 - *(const unsigned char*)str2;
        }

        if(*str1 == '\0') return 0;
        
        str1++;
        str2++;
        n--;
    }
    
    return 0;
}
