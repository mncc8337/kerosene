#include <ctype.h>

int isprint(const int c) {
    return (c >= ' ' && c <= '~');
}
