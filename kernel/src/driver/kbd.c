#include "kbd.h"
#include "system.h"
#include "ps2.h"

static uint8_t keycode[] = {
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

static uint8_t keycode_extended_byte[] = {
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

static char keymap[175];
static char keymap_shift[175];

static bool key_pressed[175];

static bool capslock_on = false;
static bool scrolllock_on = false;
static bool numlock_on = false;

static key_t current_key;
static void (*key_listener)(key_t) = 0;

// predefined it here to be used in trash interrupt handler
static void kbd_handler(regs_t* r);

static uint8_t interrupt_progress_cnt = 0;
static uint8_t interrupt_loop_cnt = 0;
static void kbd_trash_int_handler() {
    ps2_wait_for_reading_data();
    ps2_read_data();
    interrupt_progress_cnt++;
    // loop for sometime to discard useless interrupts
    if(interrupt_progress_cnt == interrupt_loop_cnt) {
        interrupt_progress_cnt = 0;
        // reinstall the default handler
        irq_install_handler(1, kbd_handler);
    }
}

static volatile bool lastest_key_handled = true;
static bool extended_byte = false;
static void kbd_handler(regs_t* r) {
    (void)(r); // avoid unused arg

    ps2_wait_for_reading_data();
    uint8_t scancode = ps2_read_data();

    if(scancode == EXTENDED_BYTE) {
        // turn on the flag 
        extended_byte = true;
        // skip
        return;
    }
    else if(scancode == PAUSE_SCANCODE_1ST) {
        interrupt_loop_cnt = 5;
        irq_install_handler(1, kbd_trash_int_handler);

        current_key.keycode = keycode_extended_byte[0x6f];
        current_key.mapped = 0;
        current_key.released = false;
        goto call_key_listener;
    }

    if(extended_byte &&
       (scancode == PRINTSCREEN_PRESSED_SCANCODE_2ND || scancode == PRINTSCREEN_RELEASED_SCANCODE_2ND)) {
        key_pressed[0x6e] = (scancode == PRINTSCREEN_PRESSED_SCANCODE_2ND);
        interrupt_loop_cnt = 2;
        irq_install_handler(1, kbd_trash_int_handler);

        current_key.keycode = keycode_extended_byte[0x6e];
        current_key.mapped = 0;
        current_key.released = (scancode == PRINTSCREEN_RELEASED_SCANCODE_2ND);
        goto call_key_listener;
    }

    bool released = (scancode & 0x80) == 0x80;
    scancode -= 0x80 * released;

    uint8_t kcode;
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

    char mapped = keymap[kcode];
    if(key_pressed[KEYCODE_LSHIFT] || key_pressed[KEYCODE_RSHIFT]) {
        if(mapped < 0x61 || mapped > 0x7a)
            mapped = keymap_shift[kcode];
        else if(!capslock_on) mapped = keymap_shift[kcode];
    }
    // convert to uppercase if only capslock is on
    if(capslock_on && mapped >= 0x61
            && mapped <= 0x7a
            && !key_pressed[KEYCODE_LSHIFT]
            && !key_pressed[KEYCODE_RSHIFT])
        mapped -= 32;

    current_key.keycode = kcode;
    current_key.mapped = mapped;
    current_key.released = released;
    lastest_key_handled = false;
    
call_key_listener:
    if(key_listener) key_listener(current_key);

    // reset extended_byte status
    extended_byte = false;
}

// wait until key event occurr
void kbd_wait_key(key_t* k) {
    lastest_key_handled = true; // make sure to get a new key
    // wait until get a new key (unhandled)
    while(lastest_key_handled)
        asm volatile("sti; hlt; cli");
    asm volatile("sti");
    lastest_key_handled = true;
    if(k) *k = current_key;
}

uint8_t kbd_get_keycode(uint8_t group, uint8_t no) {
    return (group << 4) + no;
}

uint8_t kbd_key_map(uint8_t keycode, unsigned layout) {
    switch(layout) {
        case KBL_US:
            return keymap[keycode];
        default:
            return keymap[keycode];
    }
}
uint8_t kbd_key_shift_map(uint8_t keycode, unsigned layout) {
    switch(layout) {
        case KBL_US:
            return keymap_shift[keycode];
        default:
            return keymap_shift[keycode];
    }
}

bool kbd_is_key_pressed(uint8_t keycode) {
    return key_pressed[keycode];
}

bool kbd_is_capslock_on() { return capslock_on; }
bool kbd_is_scrolllock_on() { return scrolllock_on; }
bool kbd_is_numlock_on() { return numlock_on; }

void kbd_install_key_listener(void (*klis)(key_t)) {
    key_listener = klis;
}
void kbd_uninstall_key_listener() {
    key_listener = 0;
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
