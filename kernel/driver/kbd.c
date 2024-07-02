#include "driver/kbd.h"

unsigned char keycode[] = {
    0, // nothing
    // escape
    (0 << 4) + 0,
    // row 1, no backtick
    (1 << 4) + 1,
    (1 << 4) + 2,
    (1 << 4) + 3,
    (1 << 4) + 4,
    (1 << 4) + 5,
    (1 << 4) + 6,
    (1 << 4) + 7,
    (1 << 4) + 8,
    (1 << 4) + 9,
    (1 << 4) + 10,
    (1 << 4) + 11,
    (1 << 4) + 12,
    (1 << 4) + 13,
    // row 2, no backslash
    (2 << 4) + 0,
    (2 << 4) + 1,
    (2 << 4) + 2,
    (2 << 4) + 3,
    (2 << 4) + 4,
    (2 << 4) + 5,
    (2 << 4) + 6,
    (2 << 4) + 7,
    (2 << 4) + 8,
    (2 << 4) + 9,
    (2 << 4) + 10,
    (2 << 4) + 11,
    (2 << 4) + 12,
    // enter
    (3 << 4) + 12,
    // lctrl
    (5 << 4) + 0,
    // row 3, no caps, enter
    (3 << 4) + 1,
    (3 << 4) + 2,
    (3 << 4) + 3,
    (3 << 4) + 4,
    (3 << 4) + 5,
    (3 << 4) + 6,
    (3 << 4) + 7,
    (3 << 4) + 8,
    (3 << 4) + 9,
    (3 << 4) + 10,
    (3 << 4) + 11,
    // backtick
    (1 << 4) + 0,
    // lshift
    (4 << 4) + 0,
    // backslash
    (2 << 4) + 13,
    // row 4, no lshift
    (4 << 4) + 1,
    (4 << 4) + 2,
    (4 << 4) + 3,
    (4 << 4) + 4,
    (4 << 4) + 5,
    (4 << 4) + 6,
    (4 << 4) + 7,
    (4 << 4) + 8,
    (4 << 4) + 9,
    (4 << 4) + 10,
    (4 << 4) + 11,
    // TODO: asterisk kpd
    0,
    // lalt
    (5 << 4) + 2,
    // spacebar
    (5 << 4) + 3,
    // capslock
    (3 << 4) + 0,
    // row 0, no esc, F11, F12
    (0 << 4) + 1,
    (0 << 4) + 2,
    (0 << 4) + 3,
    (0 << 4) + 4,
    (0 << 4) + 5,
    (0 << 4) + 6,
    (0 << 4) + 7,
    (0 << 4) + 8,
    (0 << 4) + 9,
    (0 << 4) + 10,
    // TODO: numlock kpd
    0,
    // scroll lock
    (6 << 4) + 1,
    // TODO: kpd
    // 7 8 9 -
    // 4 5 6 +
    // 1 2 3
    // 0 .
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0,
    0, 0,

    // null
    0, 0, 0,
    // F11, F12
    (0 << 4) + 11,
    (0 << 4) + 12
};

unsigned char keycode_extended_byte[] = {
    // null
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // TODO: prev track
    0,
    // null
    0, 0, 0, 0, 0, 0, 0, 0,
    // TODO: next track
    0,
    // null
    0, 0,
    // TODO: enter kpd
    0,
    // rctrl
    (5 << 4) + 7,
    // null
    0, 0,
    // TODO: mute, cal, play
    0, 0, 0,
    // null
    0,
    // TODO: stop
    0,
    // null
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    // TODO: volume down
    0,
    // null
    0,
    // TODO: volume up
    0,
    // null
    0,
    // TODO: WWW
    0,
    // null
    0, 0,
    // TODO: slash kpd
    0,
    // null
    0, 0,
    // ralt
    (5 << 4) + 4,
    // null
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // home
    (6 << 4) + 4,
    // arrow up
    (7 << 4) + 0,
    // page up
    (6 << 4) + 5,
    // null
    0,
    // arrow left
    (7 << 4) + 1,
    // null
    0,
    // arrow right
    (7 << 4) + 3,
    // null
    0,
    // end
    (6 << 4) + 7,
    // arrow down
    (7 << 4) + 2,
    // page down
    (6 << 4) + 8,
    // insert
    (6 << 4) + 3,
    // delete
    (6 << 4) + 6,
    // null,
    0, 0, 0, 0, 0, 0, 0,
    // lgui, rgui,
    (5 << 4) + 1,
    (5 << 4) + 5,
    // apps
    (5 << 4) + 6,
    // TODO: power, sleep
    0, 0,
    // null
    0, 0, 0,
    // TODO: wake
    0,
    // null
    0,
    // TODO: WWW search, favs, refresh, stop, forward, back
    0, 0, 0, 0, 0, 0,
    // TODO: my computer
    0,
    // TODO: email
    0,
    // TODO: media select
    0,
    // 0x6e: printscreen
    (6 << 4) + 0,
    // 0x6f: pause
    (6 << 4) + 2
};

unsigned char keymap[175];
unsigned char keymap_shift[175];

bool key_pressed[175];

bool capslock_on = false;
bool scrolllock_on = false;
bool numlock_on = false;

bool input_end = false;
char* kbd_input;
unsigned int kbd_input_len = 0;
char endding_char = 0;
bool has_job = false;
void (*callback)(key);

void emit_end() {
    // reset
    kbd_input = 0;
    kbd_input_len = 0;
    endding_char = 0;
    callback = 0;
    // toggle flag
    input_end = true;
}

// TODO: handle print screen and pause
void kbd_handler(regs* r) {
    ps2_wait_for_reading_data();
    unsigned char scancode = ps2_read_data();
    bool extended_byte = false;
    if(scancode == EXTENDED_BYTE) {
        ps2_wait_for_reading_data();
        scancode = ps2_read_data();
        extended_byte = true;

        // if printscreen pressed or released
        if(scancode == PRINTSCREEN_PRESSED_SCANCODE_2ND) {
            // trash other interrupt
            ps2_wait_for_reading_data(); ps2_read_data();
            ps2_wait_for_reading_data(); ps2_read_data();
            // change to 0x6e so that it is matched
            // to the defined position in keycode_extended_byte
            scancode = 0x6e;
        }
        else if(scancode == PRINTSCREEN_RELEASED_SCANCODE_2ND) {
            // trash other interrupt
            ps2_wait_for_reading_data(); ps2_read_data();
            ps2_wait_for_reading_data(); ps2_read_data();
            // add 0x80 because it is released
            scancode = 0x6e + 0x80;
        }
    }
    // if pause
    else if(scancode == PAUSE_SCANCODE_1ST) {
        extended_byte = true;
        // trash other interrupts
        ps2_wait_for_reading_data(); ps2_read_data();
        ps2_wait_for_reading_data(); ps2_read_data();
        ps2_wait_for_reading_data(); ps2_read_data();
        ps2_wait_for_reading_data(); ps2_read_data();
        ps2_wait_for_reading_data(); ps2_read_data();
        scancode = 0x6f;
    }

    bool released = (scancode & 0x80) == 0x80;
    scancode -= 0x80 * released;

    unsigned char kcode;
    if(extended_byte) kcode = keycode_extended_byte[scancode];
    else kcode = keycode[scancode];
   
    // pause do not interrupt when released so it is a bad idea
    // to set it to pressed forever
    if(kcode != KEYCODE_PAUSE) key_pressed[kcode] = !released;

    if(kcode == KEYCODE_CAPSLOCK && !released)
        capslock_on = !capslock_on;
    if(kcode == KEYCODE_SCROLLLOCK && !released)
        scrolllock_on = !scrolllock_on;
    // if(kcode == KEYCODE_NUMLOCK && !released)
    //     numlock_on = !numlock_on;

    if(input_end) return;

    char mapped = keymap[kcode];
    if((key_pressed[KEYCODE_LSHIFT] || key_pressed[KEYCODE_RSHIFT]) && !capslock_on)
        mapped = keymap_shift[kcode];
    // convert to uppercase if only capslock is on
    else if(capslock_on && mapped >= 0x61 && mapped <= 0x7a)
        mapped -= 32;

    key k;
    k.keycode = kcode;
    k.released = released;

    if(!released) {
        if(mapped != endding_char) {
            if(mapped != '\b' || endding_char == 0) {
                kbd_input[kbd_input_len] = mapped;
                kbd_input_len++;

                 // if get_char then end
                if(endding_char == 0) emit_end();
                else callback(k);
            }
            else if(kbd_input_len > 0) {
                kbd_input_len--;
                kbd_input[kbd_input_len] = ' ';
                callback(k);
            }
            // dont send callback when cannot erase more
        }
        else {
            // add null terminator
            kbd_input[kbd_input_len] = '\0';
            emit_end();
        }
    }
}

unsigned char get_keycode(unsigned char group, unsigned char no) {
    return (group << 4) + no;
}

unsigned char key_map(unsigned char keycode, unsigned int layout) {
    switch(layout) {
        case KBL_US:
            return keymap[keycode];
        default:
            return keymap[keycode];
    }
}

bool is_key_pressed(unsigned char keycode) {
    return key_pressed[keycode];
}

bool is_capslock_on() { return capslock_on; }
bool is_scrolllock_on() { return scrolllock_on; }
bool is_numlock_on() { return numlock_on; }

void get_char(char* dest) {
    input_end = false;
    kbd_input = dest;
    endding_char = 0;

    // block process until done
    while(!input_end) SYS_SLEEP;
}
void get_string(char* dest, char _endding_char, void (*_callback)(key)) {
    input_end = false;
    kbd_input = dest;
    endding_char = _endding_char;
    callback = _callback;

    // block process until done
    while(!input_end) SYS_SLEEP;
}

void kbd_init() {
    // init the keymap
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

    irq_install_handler(1, kbd_handler);
}
