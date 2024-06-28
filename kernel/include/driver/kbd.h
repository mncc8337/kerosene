#pragma once

#include "system.h"

#define SCANCODE_ESCAPE               0x01
#define SCANCODE_1                    0x02
#define SCANCODE_2                    0x03
#define SCANCODE_3                    0x04
#define SCANCODE_4                    0x05
#define SCANCODE_5                    0x06
#define SCANCODE_6                    0x07
#define SCANCODE_7                    0x08
#define SCANCODE_8                    0x09
#define SCANCODE_9                    0x0a
#define SCANCODE_0                    0x0b
#define SCANCODE_MINUS                0x0c
#define SCANCODE_EQUAL                0x0d
#define SCANCODE_BACKSPACE            0x0e
#define SCANCODE_TAB                  0x0f
#define SCANCODE_Q                    0x10
#define SCANCODE_W                    0x11
#define SCANCODE_E                    0x12
#define SCANCODE_R                    0x13
#define SCANCODE_T                    0x14
#define SCANCODE_Y                    0x15
#define SCANCODE_U                    0x16
#define SCANCODE_I                    0x17
#define SCANCODE_O                    0x18
#define SCANCODE_P                    0x19
#define SCANCODE_SQUARE_BRACKET_OPEN  0x1a
#define SCANCODE_SQUARE_BRACKET_CLOSE 0x1b
#define SCANCODE_ENTER                0x1c
#define SCANCODE_LCTRL                0x1d
#define SCANCODE_A                    0x1e
#define SCANCODE_S                    0x1f
#define SCANCODE_D                    0x20
#define SCANCODE_F                    0x21
#define SCANCODE_G                    0x22
#define SCANCODE_H                    0x23
#define SCANCODE_J                    0x24
#define SCANCODE_K                    0x25
#define SCANCODE_L                    0x26
#define SCANCODE_SEMICOLON            0x27
#define SCANCODE_SINGLE_QUOTE         0x28
#define SCANCODE_BACKTICK             0x29
#define SCANCODE_LSHIFT               0x2a
#define SCANCODE_BACKSLASH            0x2b
#define SCANCODE_Z                    0x2c
#define SCANCODE_X                    0x2d
#define SCANCODE_C                    0x2e
#define SCANCODE_V                    0x2f
#define SCANCODE_B                    0x30
#define SCANCODE_N                    0x31
#define SCANCODE_M                    0x32
#define SCANCODE_COMMA                0x33
#define SCANCODE_DOT                  0x34
#define SCANCODE_SLASH                0x35
#define SCANCODE_RSHIFT               0x36
#define SCANCODE_ASTERICK_KPD         0x37
#define SCANCODE_LALT                 0x38
#define SCANCODE_SPACE                0x39
#define SCANCODE_CAPSLOCK             0x3a
#define SCANCODE_F1                   0x3b
#define SCANCODE_F2                   0x3c
#define SCANCODE_F3                   0x3d
#define SCANCODE_F4                   0x3e
#define SCANCODE_F5                   0x3f
#define SCANCODE_F6                   0x40
#define SCANCODE_F7                   0x41
#define SCANCODE_F8                   0x42
#define SCANCODE_F9                   0x43
#define SCANCODE_F10                  0x44
#define SCANCODE_NUMLOCK              0x45
#define SCANCODE_SCROLLLOCK           0x46
#define SCANCODE_7_KPD                0x47
#define SCANCODE_8_KPD                0x48
#define SCANCODE_9_KPD                0x49
#define SCANCODE_MINUS_KPD            0x4a
#define SCANCODE_4_KPD                0x4b
#define SCANCODE_5_KPD                0x4c
#define SCANCODE_6_KPD                0x4d
#define SCANCODE_PLUS_KPD             0x4e
#define SCANCODE_1_KPD                0x4f
#define SCANCODE_2_KPD                0x50
#define SCANCODE_3_KPD                0x51
#define SCANCODE_0_KPD                0x52
#define SCANCODE_DOT_KPD              0x53

#define SCANCODE_F11 0x57
#define SCANCODE_F12 0x58

#define EXTENDED_BYTE 0xe0

// from now on scancode are generated with EXTENDED_BYTE
#define SCANCODE_PREV_TRACK  0x10
#define SCANCODE_NEXT_TRACK  0x19

#define SCANCODE_ENTER_KPD   0x1c
#define SCANCODE_RCTRL       0x1d

#define SCANCODE_MUTE        0x20
#define SCANCODE_CALCULATOR  0x21
#define SCANCODE_PLAY        0x22
#define SCANCODE_STOP        0x24
#define SCANCODE_VOLUME_DOWN 0x2e
#define SCANCODE_VOLUME_UP   0x30
#define SCANCODE_WWW         0x32

#define SCANCODE_SLASH_KPD   0x35
#define SCANCODE_RALT        0x38

#define SCANCODE_HOME         0x47
#define SCANCODE_CURSOR_UP    0x48
#define SCANCODE_PAGE_UP      0x49
#define SCANCODE_CURSOR_LEFT  0x4b
#define SCANCODE_CURSOR_RIGHT 0x4d
#define SCANCODE_END          0x4f
#define SCANCODE_CURSOR_DOWN  0x50
#define SCANCODE_PAGE_DOWN    0x51
#define SCANCODE_INSERT       0x52
#define SCANCODE_DELETE       0x53
#define SCANCODE_LGUI         0x5b
#define SCANCODE_RGUI         0x5c
#define SCANCODE_APPS         0x5d
#define SCANCODE_POWER        0x5e
#define SCANCODE_SLEEP        0x5f
#define SCANCODE_WAKE         0x63
#define SCANCODE_WWW_SEARCH   0x65
#define SCANCODE_WWW_FAVS     0x66
#define SCANCODE_WWW_REF      0x67
#define SCANCODE_WWW_STOP     0x68
#define SCANCODE_WWW_FORDWARD 0x69
#define SCANCODE_WWW_BACK     0x6a
#define SCANCODE_MY_COMP      0x6b
#define SCANCODE_EMAIL        0x6c
#define SCANCODE_MEDIA_SEL    0x6d

// theyre just here as a note
#define SCANCODE_PRINTSCRN 0xe0, 0x2a, 0xe0, 0x37
#define SCANCODE_PAUSE     0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5

// total keys: 121

struct key {
    unsigned char scancode;
    unsigned char keycode;
    char mapped;
    bool released;
};

bool is_capslock_on(); 
bool is_scrolllock_on();
bool is_numlock_on();

void get_char(char* dest);
void get_string(char* dest, char end, void (*_callback)(struct key));

void kbd_init();
