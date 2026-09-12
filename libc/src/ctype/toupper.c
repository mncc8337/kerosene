#include <ctype.h>

char toupper(char chr) {
    int is_lower = (chr >= 'a' && chr <= 'z');
    chr -= 32 * is_lower;
    return chr;
}
