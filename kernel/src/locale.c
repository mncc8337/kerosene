#include "locale.h"

static int kbd_layout = -1;
static int timezone = TIMEZONE_NONE;

static char keymap[175];
static char keymap_shift[175];

static void set_keyboard_layout_US() {
    keymap[(1 << 4) + 0]  = '`';
    keymap[(1 << 4) + 1]  = '1';
    keymap[(1 << 4) + 2]  = '2';
    keymap[(1 << 4) + 3]  = '3';
    keymap[(1 << 4) + 4]  = '4';
    keymap[(1 << 4) + 5]  = '5';
    keymap[(1 << 4) + 6]  = '6';
    keymap[(1 << 4) + 7]  = '7';
    keymap[(1 << 4) + 8]  = '8';
    keymap[(1 << 4) + 9]  = '9';
    keymap[(1 << 4) + 10] = '0';
    keymap[(1 << 4) + 11] = '-';
    keymap[(1 << 4) + 12] = '=';
    keymap[(1 << 4) + 13] = '\b';

    keymap[(2 << 4) + 0]  = '\t';
    keymap[(2 << 4) + 1]  = 'q';
    keymap[(2 << 4) + 2]  = 'w';
    keymap[(2 << 4) + 3]  = 'e';
    keymap[(2 << 4) + 4]  = 'r';
    keymap[(2 << 4) + 5]  = 't';
    keymap[(2 << 4) + 6]  = 'y';
    keymap[(2 << 4) + 7]  = 'u';
    keymap[(2 << 4) + 8]  = 'i';
    keymap[(2 << 4) + 9]  = 'o';
    keymap[(2 << 4) + 10] = 'p';
    keymap[(2 << 4) + 11] = '[';
    keymap[(2 << 4) + 12] = ']';
    keymap[(2 << 4) + 13] = '\\';

    keymap[(3 << 4) + 1]  = 'a';
    keymap[(3 << 4) + 2]  = 's';
    keymap[(3 << 4) + 3]  = 'd';
    keymap[(3 << 4) + 4]  = 'f';
    keymap[(3 << 4) + 5]  = 'g';
    keymap[(3 << 4) + 6]  = 'h';
    keymap[(3 << 4) + 7]  = 'j';
    keymap[(3 << 4) + 8]  = 'k';
    keymap[(3 << 4) + 9]  = 'l';
    keymap[(3 << 4) + 10] = ';';
    keymap[(3 << 4) + 11] = '\'';
    keymap[(3 << 4) + 12] = '\n';

    keymap[(4 << 4) + 1]  = 'z';
    keymap[(4 << 4) + 2]  = 'x';
    keymap[(4 << 4) + 3]  = 'c';
    keymap[(4 << 4) + 4]  = 'v';
    keymap[(4 << 4) + 5]  = 'b';
    keymap[(4 << 4) + 6]  = 'n';
    keymap[(4 << 4) + 7]  = 'm';
    keymap[(4 << 4) + 8]  = ',';
    keymap[(4 << 4) + 9]  = '.';
    keymap[(4 << 4) + 10] = '/';

    keymap[(5 << 4) + 3] = ' ';

    keymap_shift[(1 << 4) + 0]  = '~';
    keymap_shift[(1 << 4) + 1]  = '!';
    keymap_shift[(1 << 4) + 2]  = '@';
    keymap_shift[(1 << 4) + 3]  = '#';
    keymap_shift[(1 << 4) + 4]  = '$';
    keymap_shift[(1 << 4) + 5]  = '%';
    keymap_shift[(1 << 4) + 6]  = '^';
    keymap_shift[(1 << 4) + 7]  = '&';
    keymap_shift[(1 << 4) + 8]  = '*';
    keymap_shift[(1 << 4) + 9]  = '(';
    keymap_shift[(1 << 4) + 10] = ')';
    keymap_shift[(1 << 4) + 11] = '_';
    keymap_shift[(1 << 4) + 12] = '+';

    keymap_shift[(2 << 4) + 1]  = 'Q';
    keymap_shift[(2 << 4) + 2]  = 'W';
    keymap_shift[(2 << 4) + 3]  = 'E';
    keymap_shift[(2 << 4) + 4]  = 'R';
    keymap_shift[(2 << 4) + 5]  = 'T';
    keymap_shift[(2 << 4) + 6]  = 'Y';
    keymap_shift[(2 << 4) + 7]  = 'U';
    keymap_shift[(2 << 4) + 8]  = 'I';
    keymap_shift[(2 << 4) + 9]  = 'O';
    keymap_shift[(2 << 4) + 10] = 'P';
    keymap_shift[(2 << 4) + 11] = '{';
    keymap_shift[(2 << 4) + 12] = '}';
    keymap_shift[(2 << 4) + 13] = '|';

    keymap_shift[(3 << 4) + 1]  = 'A';
    keymap_shift[(3 << 4) + 2]  = 'S';
    keymap_shift[(3 << 4) + 3]  = 'D';
    keymap_shift[(3 << 4) + 4]  = 'F';
    keymap_shift[(3 << 4) + 5]  = 'G';
    keymap_shift[(3 << 4) + 6]  = 'H';
    keymap_shift[(3 << 4) + 7]  = 'J';
    keymap_shift[(3 << 4) + 8]  = 'K';
    keymap_shift[(3 << 4) + 9]  = 'L';
    keymap_shift[(3 << 4) + 10] = ':';
    keymap_shift[(3 << 4) + 11] = '"';

    keymap_shift[(4 << 4) + 1]  = 'Z';
    keymap_shift[(4 << 4) + 2]  = 'X';
    keymap_shift[(4 << 4) + 3]  = 'C';
    keymap_shift[(4 << 4) + 4]  = 'V';
    keymap_shift[(4 << 4) + 5]  = 'B';
    keymap_shift[(4 << 4) + 6]  = 'N';
    keymap_shift[(4 << 4) + 7]  = 'M';
    keymap_shift[(4 << 4) + 8]  = '<';
    keymap_shift[(4 << 4) + 9]  = '>';
    keymap_shift[(4 << 4) + 10] = '?';
}

int locale_get_keyboard_layout() {
    return kbd_layout;
}
void locale_set_keyboard_layout(int layout) {
    kbd_layout = layout;

    switch(layout) {
        case KBD_LAYOUT_US:
            set_keyboard_layout_US();
            break;
        default:
            set_keyboard_layout_US();
    }
}

int locale_get_timezone() {
    return timezone;
}
void locale_set_timezone(int zone) {
    timezone = zone;
}

char locale_map_key(unsigned keycode, bool shift) {
    if(!shift) return keymap[keycode];
    return keymap_shift[keycode];
}

struct tm* locale_map_timezone(struct tm* time) {
    // TODO: implement timezone time process
    return time;
}
