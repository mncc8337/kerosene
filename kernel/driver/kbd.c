#include "driver/kbd.h"

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
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

bool input_end = false;
char* kbd_input;
unsigned int kbd_input_len = 0;
char endding_char = 0;
void (*callback)(char);

void emit_end() {
    // uninstall itself
    irq_uninstall_handler(1);
    // reset
    kbd_input = 0;
    kbd_input_len = 0;
    endding_char = 0;
    callback = 0;
    // toggle flag
    input_end = true;
}

void kbd_handler(struct regs* r) {
    unsigned char scancode = port_inb(0x60);

    bool released = scancode & 0x80;

    char mapped = kbd_us[scancode];

    if(!released) {
        if(mapped != endding_char) {
            kbd_input[kbd_input_len] = mapped;
            kbd_input_len++;

            // get_char
            if(endding_char == 0) emit_end(); // get_char doesnt have callback
            else callback(mapped);
        }
        else {
            // add null terminator
            kbd_input[kbd_input_len] = '\0';
            emit_end();
        }
    }
}

void get_char(char* dest) {
    kbd_input = dest;
    endding_char = 0;
    irq_install_handler(1, kbd_handler);

    // block process until done
    while(!input_end)
        asm volatile("sti; hlt; cli"); // sleep mode

    input_end = false;
}
void get_string(char* dest, char _endding_char, void (*_callback)(char)) {
    kbd_input = dest;
    endding_char = _endding_char;
    callback = _callback;
    irq_install_handler(1, kbd_handler);

    // block process until done
    while(!input_end)
        asm volatile("sti; hlt; cli"); // sleep mode

    input_end = false;
}
