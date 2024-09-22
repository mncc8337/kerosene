#include "kbd.h"
#include "system.h"
#include "ps2.h"
#include "locale.h"

#define KBD_EXTENDED_BYTE 0xe0

#define KBD_PRINTSCREEN_PRESSED_SCANCODE_2ND 0x2a
#define KBD_PRINTSCREEN_RELEASED_SCANCODE_2ND 0xb7
#define KBD_PAUSE_SCANCODE_1ST 0xe1

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

static bool key_pressed[175];

static bool capslock_on = false;
static bool scrolllock_on = false;
static bool numlock_on = false;

static key_t current_key;

static void set_scancode_set() {
    unsigned tries = 0;

try_again:
    if(tries == 3) return;

    // make sure to use scancode set 1
    ps2_wait_for_writing_data();
    ps2_write_data(KBD_SCANCODE_SET);
    ps2_wait_for_writing_data();
    // NOTE:
    // the osdev wiki said that
    // 1 is for scancode set 1 and 2 is for scancode set 2
    // but for some reason if i use 1 then the controller send scancode set 2
    // and if i use 2 the controller send scancode set 1
    // maybe it was sone translation stuff
    ps2_write_data(2);
    // wait for respond (ACK or RESEND)
    uint8_t respond = 0;
    while(respond != KBD_ACK && respond != KBD_RESEND) {
        ps2_wait_for_reading_data();
        respond = ps2_read_data();
    }

    // if we received RESEND then the previous command has failed
    if(respond == KBD_RESEND) {
        tries++;
        goto try_again;
    }
}

// predefined it here to be used in trash interrupt handler
static void kbd_handler(regs_t* r);

static uint8_t interrupt_progress_cnt = 0;
static uint8_t interrupt_loop_cnt = 0;
static void kbd_trash_int_handler() {
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

    // data must be ready at this point
    // so no need for waiting
    uint8_t scancode = ps2_read_data();

    if(scancode == KBD_EXTENDED_BYTE) {
        // turn on the flag 
        extended_byte = true;
        // skip
        return;
    }
    else if(scancode == KBD_PAUSE_SCANCODE_1ST) {
        interrupt_loop_cnt = 5;
        irq_install_handler(1, kbd_trash_int_handler);

        current_key.keycode = keycode_extended_byte[0x6f];
        current_key.mapped = 0;
        current_key.released = false;
        extended_byte = false;
        return;
    }

    if(extended_byte &&
            (scancode == KBD_PRINTSCREEN_PRESSED_SCANCODE_2ND
            || scancode == KBD_PRINTSCREEN_RELEASED_SCANCODE_2ND)) {
        key_pressed[0x6e] = (scancode == KBD_PRINTSCREEN_PRESSED_SCANCODE_2ND);
        interrupt_loop_cnt = 2;
        irq_install_handler(1, kbd_trash_int_handler);

        current_key.keycode = keycode_extended_byte[0x6e];
        current_key.mapped = 0;
        current_key.released = (scancode == KBD_PRINTSCREEN_RELEASED_SCANCODE_2ND);
        extended_byte = false;
        return;
    }

    bool released = (scancode & 0x80) == 0x80;
    scancode -= 0x80 * released;

    uint8_t kcode;
    if(extended_byte) kcode = keycode_extended_byte[scancode];
    else kcode = keycode[scancode];
   
    // pause do not interrupt when released
    // so it is a bad idea to set it to pressed forever
    if(kcode != KBD_KEYCODE_PAUSE) key_pressed[kcode] = !released;

    bool led_changed = false;
    if(kcode == KBD_KEYCODE_CAPSLOCK && !released) {
        capslock_on = !capslock_on;
        led_changed = true;
    }
    if(kcode == KBD_KEYCODE_SCROLLLOCK && !released) {
        scrolllock_on = !scrolllock_on;
        led_changed = true;
    }
    // if(kcode == KEYCODE_NUMLOCK && !released) {
    //     numlock_on = !numlock_on;
    //     led_changed = true;
    // }
    if(led_changed)
        kbd_set_led(scrolllock_on, numlock_on, capslock_on);

    char mapped = locale_map_key(kcode, false);
    if(key_pressed[KBD_KEYCODE_LSHIFT] || key_pressed[KBD_KEYCODE_RSHIFT]) {
        if(mapped < 0x61 || mapped > 0x7a)
            mapped = locale_map_key(kcode, true);
        else if(!capslock_on) mapped = locale_map_key(kcode, true);
    }
    // convert to uppercase if only capslock is on
    if(capslock_on && mapped >= 0x61
            && mapped <= 0x7a
            && !key_pressed[KBD_KEYCODE_LSHIFT]
            && !key_pressed[KBD_KEYCODE_RSHIFT])
        mapped -= 32;

    current_key.keycode = kcode;
    current_key.mapped = mapped;
    current_key.released = released;
    lastest_key_handled = false;
    
    // reset extended_byte status
    extended_byte = false;
}

// set keyboard LED indicators
void kbd_set_led(bool scroll, bool num, bool caps) {
    uint8_t bits = scroll | (num << 1) | (caps << 2);
    unsigned tries = 0;

try_again:
    if(tries == 3) return; // give up after 3 tries

    // send command byte
    ps2_wait_for_writing_data();
    ps2_write_data(KBD_LED);

    // send data byte
    ps2_wait_for_writing_data();
    ps2_write_data(bits);

    // wait for ACK or RESEND
    uint8_t respond = 0;
    while(respond != KBD_ACK && respond != KBD_RESEND) {
        ps2_wait_for_reading_data();
        respond = ps2_read_data();
    }

    // if we received RESEND then the previous command has failed
    if(respond == KBD_RESEND) {
        tries++;
        goto try_again;
    }
}

// wait until key event occur
void kbd_wait_key(key_t* k) {
    lastest_key_handled = true; // make sure to get a new key
    // wait until get a new key (unhandled)
    while(lastest_key_handled);
    lastest_key_handled = true;
    if(k) *k = current_key;
}

bool kbd_is_key_pressed(uint8_t keycode) {
    return key_pressed[keycode];
}

bool kbd_is_capslock_on() { return capslock_on; }
bool kbd_is_scrolllock_on() { return scrolllock_on; }
bool kbd_is_numlock_on() { return numlock_on; }

void kbd_init() {
    // make sure data port is empty
    ps2_read_data();

    // TODO: detect where is the PS/2 keyboard port located (first port or second port)
    // the code below assumes that keyboard is in the 1st port

    // ensure that the current scancode set is 1
    set_scancode_set();
    // initial LED state
    kbd_set_led(0, 0, 0);

    irq_install_handler(1, kbd_handler);
}
