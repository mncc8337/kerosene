#pragma once

#include "system.h"
#include "ps2.h"

#define EXTENDED_BYTE 0xe0

// i know it's long
#define PRINTSCREEN_PRESSED_SCANCODE_2ND 0x2a
#define PRINTSCREEN_RELEASED_SCANCODE_2ND 0xb7
#define PAUSE_SCANCODE_1ST 0xe1

// keycodes
// 4 upperbits: group (row)
// 4 lowerbits: index (column)
//
// these are based on my keyboard layout
// group | name        | keynum | keys
// ------+-------------+--------+----------------------------------------------------------
// 0     | Fn row      | 13     | ESC, F1 - F12
// 1     | number row  | 14     | `, 1 - 0, -, +, BS
// 2     | top row     | 14     | tab, qwerty, [, ], '\'
// 3     | primary row | 13     | caps, asdf, ;, ', enter
// 4     | bottow row  | 12     | lshift, zxcv, ,, ., /, rshift
// 5     | spacebar    | 8      | lctrl, lsuper, lalt, spacebar, ralt, rsuper, menu, rctrl
// 6     | util        | 9      | prntscrn, scrlk, pause, ins, home, pgup, del, end, pgdn
// 7     | arrow       | 4      | up, left, down, right
// 8     | keypad      |        | be added later
// 9     | media       |        | be added later
// 10    | APCI        |        | be added later

#define KEYCODE_ESC (0 << 4) + 0
#define KEYCODE_F1  (0 << 4) + 1
#define KEYCODE_F2  (0 << 4) + 2
#define KEYCODE_F3  (0 << 4) + 3
#define KEYCODE_F4  (0 << 4) + 4
#define KEYCODE_F5  (0 << 4) + 5
#define KEYCODE_F6  (0 << 4) + 6
#define KEYCODE_F7  (0 << 4) + 7
#define KEYCODE_F8  (0 << 4) + 8
#define KEYCODE_F9  (0 << 4) + 9
#define KEYCODE_F10 (0 << 4) + 10
#define KEYCODE_F11 (0 << 4) + 11
#define KEYCODE_F12 (0 << 4) + 12

#define KEYCODE_BACKTICK  (1 << 4) + 0
#define KEYCODE_1         (1 << 4) + 1
#define KEYCODE_2         (1 << 4) + 2
#define KEYCODE_3         (1 << 4) + 3
#define KEYCODE_4         (1 << 4) + 4
#define KEYCODE_5         (1 << 4) + 5
#define KEYCODE_6         (1 << 4) + 6
#define KEYCODE_7         (1 << 4) + 7
#define KEYCODE_8         (1 << 4) + 8
#define KEYCODE_9         (1 << 4) + 9
#define KEYCODE_0         (1 << 4) + 10
#define KEYCODE_MINUS     (1 << 4) + 11
#define KEYCODE_EQUAL     (1 << 4) + 12
#define KEYCODE_BACKSPACE (1 << 4) + 13

#define KEYCODE_TAB           (2 << 4) + 0
#define KEYCODE_Q             (2 << 4) + 1
#define KEYCODE_W             (2 << 4) + 2
#define KEYCODE_E             (2 << 4) + 3
#define KEYCODE_R             (2 << 4) + 4
#define KEYCODE_T             (2 << 4) + 5
#define KEYCODE_Y             (2 << 4) + 6
#define KEYCODE_U             (2 << 4) + 7
#define KEYCODE_I             (2 << 4) + 8
#define KEYCODE_O             (2 << 4) + 9
#define KEYCODE_P             (2 << 4) + 10
#define KEYCODE_BRACKET_OPEN  (2 << 4) + 11
#define KEYCODE_BRACKET_CLOSE (2 << 4) + 12
#define KEYCODE_BACKSLASH     (2 << 4) + 13

#define KEYCODE_CAPSLOCK     (3 << 4) + 0
#define KEYCODE_A            (3 << 4) + 1
#define KEYCODE_S            (3 << 4) + 2
#define KEYCODE_D            (3 << 4) + 3
#define KEYCODE_F            (3 << 4) + 4
#define KEYCODE_G            (3 << 4) + 5
#define KEYCODE_H            (3 << 4) + 6
#define KEYCODE_J            (3 << 4) + 7
#define KEYCODE_K            (3 << 4) + 8
#define KEYCODE_L            (3 << 4) + 9
#define KEYCODE_SEMICOLON    (3 << 4) + 10
#define KEYCODE_SINGLE_QUOTE (3 << 4) + 11
#define KEYCODE_ENTER        (3 << 4) + 12

#define KEYCODE_LSHIFT (4 << 4) + 0
#define KEYCODE_Z      (4 << 4) + 1
#define KEYCODE_X      (4 << 4) + 2
#define KEYCODE_C      (4 << 4) + 3
#define KEYCODE_V      (4 << 4) + 4
#define KEYCODE_B      (4 << 4) + 5
#define KEYCODE_N      (4 << 4) + 6
#define KEYCODE_M      (4 << 4) + 7
#define KEYCODE_COMMA  (4 << 4) + 8
#define KEYCODE_DOT    (4 << 4) + 9
#define KEYCODE_SLASH  (4 << 4) + 10
#define KEYCODE_RSHIFT (4 << 4) + 11

#define KEYCODE_LCTRL  (5 << 4) + 0
#define KEYCODE_LGUI   (5 << 4) + 1
#define KEYCODE_LALT   (5 << 4) + 2
#define KEYCODE_SPACE  (5 << 4) + 3
#define KEYCODE_RALT   (5 << 4) + 4
#define KEYCODE_RGUI   (5 << 4) + 5
#define KEYCODE_APPS   (5 << 4) + 6
#define KEYCODE_RCTRL  (5 << 4) + 7

#define KEYCODE_PRINTSCREEN (6 << 4) + 0
#define KEYCODE_SCROLLLOCK  (6 << 4) + 1
#define KEYCODE_PAUSE       (6 << 4) + 2
#define KEYCODE_INSERT      (6 << 4) + 3
#define KEYCODE_HOME        (6 << 4) + 4
#define KEYCODE_PAGE_UP     (6 << 4) + 5
#define KEYCODE_DELETE      (6 << 4) + 6
#define KEYCODE_END         (6 << 4) + 7
#define KEYCODE_PAGE_DOWN   (6 << 4) + 8

#define KEYCODE_UP    (7 << 4) + 0
#define KEYCODE_LEFT  (7 << 4) + 1
#define KEYCODE_DOWN  (7 << 4) + 2
#define KEYCODE_RIGHT (7 << 4) + 3

typedef struct {
    unsigned char keycode;
    unsigned char mapped;
    bool released;
} key_t;

enum KEYBOARD_LAYOUT {
    KBL_US
};

unsigned char get_keycode(unsigned char group, unsigned char no);
unsigned char key_shift_map(unsigned char keycode, unsigned int layout);
unsigned char key_map(unsigned char keycode, unsigned int layout);

bool is_key_pressed(unsigned char keycode);

bool is_capslock_on(); 
bool is_scrolllock_on();
bool is_numlock_on();

void set_key_listener(void (*klis)(key_t));

void kbd_init();
