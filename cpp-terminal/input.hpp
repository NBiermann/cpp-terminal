#pragma once
#ifndef inPuT_heAderGUArD
#define inPuT_heAderGUArD

#include <map>
#include <string>

namespace Term {
enum Key {
    // Note that for A-Z in combination with CTRL,
    // the below numbers are used instead of CTRL | 'a' etc.
    // Similarly, Alt-Control-P is ALT | CTRL_P and not CTRL | ALT | 'P'
    CTRL_A = 1,
    CTRL_B,
    CTRL_C,
    CTRL_D,
    CTRL_E,
    CTRL_F,
    CTRL_G,
    CTRL_H,
    CTRL_I,
    CTRL_J,
    CTRL_K,
    CTRL_L,
    CTRL_M,
    CTRL_N,
    CTRL_O,
    CTRL_P,
    CTRL_Q,
    CTRL_R,
    CTRL_S,
    CTRL_T,
    CTRL_U,
    CTRL_V,
    CTRL_W,
    CTRL_X,
    CTRL_Y,
    CTRL_Z,

    BACKSPACE = 8,
    ENTER = 0xd,
    TAB = 9,
    ESC = 0x1b,

    SHIFT =    0x1000000u,
    ALT =      0x2000000u,
    CTRL =     0x4000000u,

    ARROW_UP = 0x8000000u,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    DEL,
    INSERT,
    NUMERIC_5, // "5" on the numeric block, but with "Num" disabled.
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    UNKNOWN = 0xfffffffu,
};

extern std::map<std::u32string, char32_t> sequences;

// Waits for a key press, translates escape codes
char32_t read_key();

// If there was a key press, returns the translated key from escape codes,
// otherwise returns 0. If the escape code is not supported, returns Key::UNKNWON_ESCAPE_CODE
// If a
//
char32_t read_key0();

std::u32string read_sequence();

std::u32string read_sequence0();


}  // namespace Term

#endif // inPuT_heAderGUArD
