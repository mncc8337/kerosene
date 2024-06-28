#include "driver/kbd.h"

unsigned char kbd_us[128] = {
    0,  27, // escape
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, // ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, // left shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, // right shift
    '*',
    0, // alt
    ' ',
    0, // capslock
    // F1 to F10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, // numlock
    0, // scrollLock
    0, // home
    0, // up arrow
    0, // page up
    '-',
    0, // left arrow
    0,
    0, // right arrow
    '+',
    0, // end key
    0, // down arrow
    0, // page down
    0, // insert Key
    0, // delete Key
    0,   0,   0,
    0, // F11
    0, // F12
    0 // all other keys are undefined
};

unsigned char kbd_shift_us[128] = {
    0,  27, // escape
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, // ctrl
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0, // left shift
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, // right shift
    '*',
    0, // alt
    ' ',
    0, // capslock
    // F1 to F10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, // numlock
    0, // scrollLock
    0, // home
    0, // up arrow
    0, // page up
    '-',
    0, // left arrow
    0,
    0, // right arrow
    '+',
    0, // end key
    0, // down arrow
    0, // page down
    0, // insert Key
    0, // delete Key
    0,   0,   0,
    0, // F11
    0, // F12
    0 // all other keys are undefined
};

bool key_pressed[128];

bool capslock_on = false;
bool scrolllock_on = false;
bool numlock_on = false;

bool input_end = false;
char* kbd_input;
unsigned int kbd_input_len = 0;
char endding_char = 0;
bool has_job = false;
void (*callback)(struct key);

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
void kbd_handler(struct regs* r) {
    unsigned char scancode = port_inb(0x60);
    bool released = (scancode & 0x80) == 0x80;
    scancode -= 0x80 * released;
    
    key_pressed[scancode] = !released;
    if(scancode == SCANCODE_CAPSLOCK && !released)
        capslock_on = !capslock_on;
    if(scancode == SCANCODE_SCROLLLOCK && !released)
        scrolllock_on = !scrolllock_on;
    if(scancode == SCANCODE_NUMLOCK && !released)
        numlock_on = !numlock_on;

    if(input_end) return;

    char mapped = kbd_us[scancode];
    if(key_pressed[SCANCODE_LSHIFT] || key_pressed[SCANCODE_RSHIFT])
        if(capslock_on) mapped = kbd_us[scancode];
        else mapped = kbd_shift_us[scancode];
    // convert to uppercase if only capslock is on
    else if(capslock_on && mapped >= 0x61 && mapped <= 0x7a)
        mapped -= 32;

    struct key k;
    k.scancode = scancode;
    k.mapped = mapped;
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
    while(!input_end)
        asm volatile("sti; hlt; cli"); // sleep mode
}
void get_string(char* dest, char _endding_char, void (*_callback)(struct key)) {
    input_end = false;
    kbd_input = dest;
    endding_char = _endding_char;
    callback = _callback;

    // block process until done
    while(!input_end)
        asm volatile("sti; hlt; cli"); // sleep mode
}

void kbd_init() {
    irq_install_handler(1, kbd_handler);
}
