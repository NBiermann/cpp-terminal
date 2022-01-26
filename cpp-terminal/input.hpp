#pragma once

#include <map>
#include <string>

namespace Term {
enum Key : char32_t {
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
    CTRL_H = 8,
    CTRL_I = 9,
    CTRL_J = 0xa,
    CTRL_K,
    CTRL_L,
    CTRL_M = 0xd,
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
    CTRL_Z, // = 0x1a
    CTRL_4 = 0x1c,
    CTRL_5,
    CTRL_6,
    CTRL_7,

    BACKSPACE = 8, // same as CTRL_H
    TAB = 9,       // same as CTRL_I
    LF = 0xa,      // same as CTRL_J
    CR = 0xd,      // same as CTRL_M
    ENTER = 0xd,   // same as CTRL_M and CR, for compatibility
    ESC = 0x1b,
    DEL = 0x7f,

    SHIFT =    0x1000000u, // flag
    ALT =      0x2000000u, // flag
    CTRL =     0x4000000u, // flag

    NON_PRINTABLES_FLOOR = 0x8000000u,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    INSERT,
    NUMERIC_5, // "5" on the numeric block with "Num" disabled. Linux-only.
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

    UNKNOWN = 0xfffffffu
};

extern std::map<std::u32string, char32_t> sequences;

// Waits for a key press, translates escape codes, decodes utf8
char32_t read_key();

// If there was a key press, returns the translated key from escape codes
// resp. the decoded key from utf8, otherwise returns 0.
// If the escape code is not supported, returns Key::UNKNWON
char32_t read_key0();

namespace Private {

std::u32string read_sequence();
std::u32string read_sequence0();
char32_t decode_sequence(const std::u32string&);

} // namespace Term::Private

} // namespace Term
