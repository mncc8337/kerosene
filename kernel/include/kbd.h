#pragma once

#include <stdbool.h>
#include <stdint.h>

#define KBD_LED              0xed
#define KBD_ECHO             0xee
#define KBD_SCANCODE_SET     0xf0
#define KBD_IDENTIFY         0xf2
#define KBD_TYPEMATIC        0xf3
#define KBD_ENABLE_SCANNING  0xf4
#define KBD_DISABLE_SCANNING 0xf5
#define KBD_SET_DEFAULT      0xf6
#define KBD_RESEND           0xfe
#define KBD_RESET            0xff

#define KBD_KDETECT_ERROR1    0x00
#define KBD_SELF_TEST_PASSED  0xaa
#define KBD_ECHO              0xee
#define KBD_ACK               0xfa
#define KBD_SELF_TEST_FAILED1 0xfc
#define KBD_SELF_TEST_FAILED2 0xfd
#define KBD_RESEND            0xfe
#define KBD_KDETECT_ERROR2    0xff

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

#define KBD_KEYCODE_ESC ((0 << 4) | 0)
#define KBD_KEYCODE_F1  ((0 << 4) | 1)
#define KBD_KEYCODE_F2  ((0 << 4) | 2)
#define KBD_KEYCODE_F3  ((0 << 4) | 3)
#define KBD_KEYCODE_F4  ((0 << 4) | 4)
#define KBD_KEYCODE_F5  ((0 << 4) | 5)
#define KBD_KEYCODE_F6  ((0 << 4) | 6)
#define KBD_KEYCODE_F7  ((0 << 4) | 7)
#define KBD_KEYCODE_F8  ((0 << 4) | 8)
#define KBD_KEYCODE_F9  ((0 << 4) | 9)
#define KBD_KEYCODE_F10 ((0 << 4) | 10)
#define KBD_KEYCODE_F11 ((0 << 4) | 11)
#define KBD_KEYCODE_F12 ((0 << 4) | 12)

#define KBD_KEYCODE_BACKTICK  ((1 << 4) | 0)
#define KBD_KEYCODE_1         ((1 << 4) | 1)
#define KBD_KEYCODE_2         ((1 << 4) | 2)
#define KBD_KEYCODE_3         ((1 << 4) | 3)
#define KBD_KEYCODE_4         ((1 << 4) | 4)
#define KBD_KEYCODE_5         ((1 << 4) | 5)
#define KBD_KEYCODE_6         ((1 << 4) | 6)
#define KBD_KEYCODE_7         ((1 << 4) | 7)
#define KBD_KEYCODE_8         ((1 << 4) | 8)
#define KBD_KEYCODE_9         ((1 << 4) | 9)
#define KBD_KEYCODE_0         ((1 << 4) | 10)
#define KBD_KEYCODE_MINUS     ((1 << 4) | 11)
#define KBD_KEYCODE_EQUAL     ((1 << 4) | 12)
#define KBD_KEYCODE_BACKSPACE ((1 << 4) | 13)

#define KBD_KEYCODE_TAB           ((2 << 4) | 0)
#define KBD_KEYCODE_Q             ((2 << 4) | 1)
#define KBD_KEYCODE_W             ((2 << 4) | 2)
#define KBD_KEYCODE_E             ((2 << 4) | 3)
#define KBD_KEYCODE_R             ((2 << 4) | 4)
#define KBD_KEYCODE_T             ((2 << 4) | 5)
#define KBD_KEYCODE_Y             ((2 << 4) | 6)
#define KBD_KEYCODE_U             ((2 << 4) | 7)
#define KBD_KEYCODE_I             ((2 << 4) | 8)
#define KBD_KEYCODE_O             ((2 << 4) | 9)
#define KBD_KEYCODE_P             ((2 << 4) | 10)
#define KBD_KEYCODE_BRACKET_OPEN  ((2 << 4) | 11)
#define KBD_KEYCODE_BRACKET_CLOSE ((2 << 4) | 12)
#define KBD_KEYCODE_BACKSLASH     ((2 << 4) | 13)

#define KBD_KEYCODE_CAPSLOCK     ((3 << 4) | 0)
#define KBD_KEYCODE_A            ((3 << 4) | 1)
#define KBD_KEYCODE_S            ((3 << 4) | 2)
#define KBD_KEYCODE_D            ((3 << 4) | 3)
#define KBD_KEYCODE_F            ((3 << 4) | 4)
#define KBD_KEYCODE_G            ((3 << 4) | 5)
#define KBD_KEYCODE_H            ((3 << 4) | 6)
#define KBD_KEYCODE_J            ((3 << 4) | 7)
#define KBD_KEYCODE_K            ((3 << 4) | 8)
#define KBD_KEYCODE_L            ((3 << 4) | 9)
#define KBD_KEYCODE_SEMICOLON    ((3 << 4) | 10)
#define KBD_KEYCODE_SINGLE_QUOTE ((3 << 4) | 11)
#define KBD_KEYCODE_ENTER        ((3 << 4) | 12)

#define KBD_KEYCODE_LSHIFT ((4 << 4) | 0)
#define KBD_KEYCODE_Z      ((4 << 4) | 1)
#define KBD_KEYCODE_X      ((4 << 4) | 2)
#define KBD_KEYCODE_C      ((4 << 4) | 3)
#define KBD_KEYCODE_V      ((4 << 4) | 4)
#define KBD_KEYCODE_B      ((4 << 4) | 5)
#define KBD_KEYCODE_N      ((4 << 4) | 6)
#define KBD_KEYCODE_M      ((4 << 4) | 7)
#define KBD_KEYCODE_COMMA  ((4 << 4) | 8)
#define KBD_KEYCODE_DOT    ((4 << 4) | 9)
#define KBD_KEYCODE_SLASH  ((4 << 4) | 10)
#define KBD_KEYCODE_RSHIFT ((4 << 4) | 11)

#define KBD_KEYCODE_LCTRL  ((5 << 4) | 0)
#define KBD_KEYCODE_LGUI   ((5 << 4) | 1)
#define KBD_KEYCODE_LALT   ((5 << 4) | 2)
#define KBD_KEYCODE_SPACE  ((5 << 4) | 3)
#define KBD_KEYCODE_RALT   ((5 << 4) | 4)
#define KBD_KEYCODE_RGUI   ((5 << 4) | 5)
#define KBD_KEYCODE_APPS   ((5 << 4) | 6)
#define KBD_KEYCODE_RCTRL  ((5 << 4) | 7)

#define KBD_KEYCODE_PRINTSCREEN ((6 << 4) | 0)
#define KBD_KEYCODE_SCROLLLOCK  ((6 << 4) | 1)
#define KBD_KEYCODE_PAUSE       ((6 << 4) | 2)
#define KBD_KEYCODE_INSERT      ((6 << 4) | 3)
#define KBD_KEYCODE_HOME        ((6 << 4) | 4)
#define KBD_KEYCODE_PAGE_UP     ((6 << 4) | 5)
#define KBD_KEYCODE_DELETE      ((6 << 4) | 6)
#define KBD_KEYCODE_END         ((6 << 4) | 7)
#define KBD_KEYCODE_PAGE_DOWN   ((6 << 4) | 8)

#define KBD_KEYCODE_UP    ((7 << 4) | 0)
#define KBD_KEYCODE_LEFT  ((7 << 4) | 1)
#define KBD_KEYCODE_DOWN  ((7 << 4) | 2)
#define KBD_KEYCODE_RIGHT ((7 << 4) | 3)

#define KBD_KEYCODE(group, column) (((group) << 4) | (column))

typedef struct {
    uint8_t keycode;
    uint8_t mapped;
    bool released;
} key_t;

void kbd_set_led(bool scroll, bool num, bool caps);

void kbd_wait_key(key_t* key);

bool kbd_is_key_pressed(uint8_t keycode);

bool kbd_is_capslock_on(); 
bool kbd_is_scrolllock_on();
bool kbd_is_numlock_on();

void kbd_init();
