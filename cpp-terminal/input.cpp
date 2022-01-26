#include "input.hpp"
#include "platform.hpp"
#include <chrono>
#include <thread>

using namespace std;

map<u32string, char32_t> Term::sequences = {
    {U"\x1b[A", ARROW_UP},
    {U"\x1b[1;2A", SHIFT | ARROW_UP},
    {U"\x1b[1;3A", ALT | ARROW_UP},
    {U"\x1b[1;4A", SHIFT | ALT | ARROW_UP},
    {U"\x1b[1;5A", CTRL | ARROW_UP},
    {U"\x1b[1;6A", SHIFT | CTRL | ARROW_UP},
    {U"\x1b[1;7A", CTRL | ALT | ARROW_UP},
    {U"\x1b[1;8A", SHIFT | CTRL | ALT | ARROW_UP},

    {U"\x1b[B", ARROW_DOWN},
    {U"\x1b[1;2B", SHIFT | ARROW_DOWN},
    {U"\x1b[1;3B", ALT | ARROW_DOWN},
    {U"\x1b[1;4B", SHIFT | ALT | ARROW_DOWN},
    {U"\x1b[1;5B", CTRL | ARROW_DOWN},
    {U"\x1b[1;6B", SHIFT | CTRL | ARROW_DOWN},
    {U"\x1b[1;7B", CTRL | ALT | ARROW_DOWN},
    {U"\x1b[1;8B", SHIFT | CTRL | ALT | ARROW_DOWN},

    {U"\x1b[C", ARROW_RIGHT},
    {U"\x1b[1;2C", SHIFT | ARROW_RIGHT},
    {U"\x1b[1;3C", ALT | ARROW_RIGHT},
    {U"\x1b[1;4C", SHIFT | ALT | ARROW_RIGHT},
    {U"\x1b[1;5C", CTRL | ARROW_RIGHT},
    {U"\x1b[1;6C", SHIFT | CTRL | ARROW_RIGHT},
    {U"\x1b[1;7C", CTRL | ALT | ARROW_RIGHT},
    {U"\x1b[1;8C", SHIFT | CTRL | ALT | ARROW_RIGHT},

    {U"\x1b[D", ARROW_LEFT},
    {U"\x1b[1;2D", SHIFT | ARROW_LEFT},
    {U"\x1b[1;3D", ALT | ARROW_LEFT},
    {U"\x1b[1;4D", SHIFT | ALT | ARROW_LEFT},
    {U"\x1b[1;5D", CTRL | ARROW_LEFT},
    {U"\x1b[1;6D", SHIFT | CTRL | ARROW_LEFT},
    {U"\x1b[1;7D", CTRL | ALT | ARROW_LEFT},
    {U"\x1b[1;8D", SHIFT | CTRL | ALT | ARROW_LEFT},

    {U"\x1b[5~", PAGE_UP},
    {U"\x1b[5;2~", SHIFT | PAGE_UP},
    {U"\x1b[5;3~", ALT | PAGE_UP},
    {U"\x1b[5;4~", SHIFT | ALT | PAGE_UP},
    {U"\x1b[5;5~", CTRL | PAGE_UP},
    {U"\x1b[5;6~", SHIFT | CTRL | PAGE_UP},
    {U"\x1b[5;7~", CTRL | ALT | PAGE_UP},
    {U"\x1b[5;8~", SHIFT | CTRL | ALT | PAGE_UP},

    {U"\x1b[6~", PAGE_DOWN},
    {U"\x1b[6;2~", SHIFT | PAGE_DOWN},
    {U"\x1b[6;3~", ALT | PAGE_DOWN},
    {U"\x1b[6;4~", SHIFT | ALT | PAGE_DOWN},
    {U"\x1b[6;5~", CTRL | PAGE_DOWN},
    {U"\x1b[6;6~", SHIFT | CTRL | PAGE_DOWN},
    {U"\x1b[6;7~", CTRL | ALT | PAGE_DOWN},
    {U"\x1b[6;8~", SHIFT | CTRL | ALT | PAGE_DOWN},

    {U"\x1b[H", HOME},
    {U"\x1b[1;2H", SHIFT | HOME},
    {U"\x1b[1;3H", ALT | HOME},
    {U"\x1b[1;4H", SHIFT | ALT | HOME},
    {U"\x1b[1;5H", CTRL | HOME},
    {U"\x1b[1;6H", SHIFT | CTRL | HOME},
    {U"\x1b[1;7H", CTRL | ALT | HOME},
    {U"\x1b[1;8H", SHIFT | CTRL | ALT | HOME},

    {U"\x1b[F", END},
    {U"\x1b[1;2F", SHIFT | END},
    {U"\x1b[1;3F", ALT | END},
    {U"\x1b[1;4F", SHIFT | ALT | END},
    {U"\x1b[1;5F", CTRL | END},
    {U"\x1b[1;6F", SHIFT | CTRL | END},
    {U"\x1b[1;7F", CTRL | ALT | END},
    {U"\x1b[1;8F", SHIFT | CTRL | ALT | END},

    {U"\x1b[3~", DEL},
    {U"\x1b[3;2~", SHIFT | DEL},
    {U"\x1b[3;3~", ALT | DEL},
    {U"\x1b[3;4~", SHIFT | ALT | DEL},
    {U"\x1b[3;5~", CTRL | DEL},
    {U"\x1b[3;6~", SHIFT | CTRL | DEL},
    {U"\x1b[3;7~", CTRL | ALT | DEL},
    {U"\x1b[3;8~", SHIFT | CTRL | ALT | DEL},

    {U"\x1b[2~", INSERT},
    {U"\x1b[2;2~", SHIFT | INSERT},
    {U"\x1b[2;3~", ALT | INSERT},
    {U"\x1b[2;4~", SHIFT | ALT | INSERT},
    {U"\x1b[2;5~", CTRL | INSERT},
    {U"\x1b[2;6~", SHIFT | CTRL | INSERT},
    {U"\x1b[2;7~", CTRL | ALT | INSERT},
    {U"\x1b[2;8~", SHIFT | CTRL | ALT | INSERT},

    // NUMERIC_5 means the '5' on the numeric block, but
    // with "Num" disabled. On Windows, these ANSI sequences
    // are not triggered, so this is Linux-only!
    {U"\x1b[E", NUMERIC_5}, //
    {U"\x1b[1;2E", SHIFT | NUMERIC_5},
    {U"\x1b[1;3E", ALT | NUMERIC_5},
    {U"\x1b[1;4E", SHIFT | ALT | NUMERIC_5},
    {U"\x1b[1;5E", CTRL | NUMERIC_5},
    {U"\x1b[1;6E", SHIFT | CTRL | NUMERIC_5},
    {U"\x1b[1;7E", CTRL | ALT | NUMERIC_5},
    {U"\x1b[1;8E", SHIFT | CTRL | ALT | NUMERIC_5},

    {U"\x1bOP", F1},
    {U"\x1b[1;2P", SHIFT | F1},
    {U"\x1b[1;3P", ALT | F1},
    {U"\x1b[1;4P", SHIFT | ALT | F1},
    {U"\x1b[1;5P", CTRL | F1},
    {U"\x1b[1;6P", SHIFT | CTRL | F1},
    {U"\x1b[1;7P", CTRL | ALT | F1},
    {U"\x1b[1;8P", SHIFT | CTRL | ALT | F1},

    {U"\x1bOQ", F2},
    {U"\x1b[1;2Q", SHIFT | F2},
    {U"\x1b[1;3Q", ALT | F2},
    {U"\x1b[1;4Q", SHIFT | ALT | F2},
    {U"\x1b[1;5Q", CTRL | F2},
    {U"\x1b[1;6Q", SHIFT | CTRL | F2},
    {U"\x1b[1;7Q", CTRL | ALT | F2},
    {U"\x1b[1;8Q", SHIFT | CTRL | ALT | F2},

    {U"\x1bOR", F3},
    {U"\x1b[1;2R", SHIFT | F3},
    {U"\x1b[1;3R", ALT | F3},
    {U"\x1b[1;4R", SHIFT | ALT | F3},
    {U"\x1b[1;5R", CTRL | F3},
    {U"\x1b[1;6R", SHIFT | CTRL | F3},
    {U"\x1b[1;7R", CTRL | ALT | F3},
    {U"\x1b[1;8R", SHIFT | CTRL | ALT | F3},

    {U"\x1bOS", F4},
    {U"\x1b[1;2S", SHIFT | F4},
    {U"\x1b[1;3S", ALT | F4},
    {U"\x1b[1;4S", SHIFT | ALT | F4},
    {U"\x1b[1;5S", CTRL | F4},
    {U"\x1b[1;6S", SHIFT | CTRL | F4},
    {U"\x1b[1;7S", CTRL | ALT | F4},
    {U"\x1b[1;8S", SHIFT | CTRL | ALT | F4},

    {U"\x1b[15~", F5},
    {U"\x1b[15;2~", SHIFT | F5},
    {U"\x1b[15;3~", ALT | F5},
    {U"\x1b[15;4~", SHIFT | ALT | F5},
    {U"\x1b[15;5~", CTRL | F5},
    {U"\x1b[15;6~", SHIFT | CTRL | F5},
    {U"\x1b[15;7~", CTRL | ALT | F5},
    {U"\x1b[15;8~", SHIFT | CTRL | ALT | F5},

    {U"\x1b[17~", F6},
    {U"\x1b[17;2~", SHIFT | F6},
    {U"\x1b[17;3~", ALT | F6},
    {U"\x1b[17;4~", SHIFT | ALT | F6},
    {U"\x1b[17;5~", CTRL | F6},
    {U"\x1b[17;6~", SHIFT | CTRL | F6},
    {U"\x1b[17;7~", CTRL | ALT | F6},
    {U"\x1b[17;8~", SHIFT | CTRL | ALT | F6},

    {U"\x1b[18~", F7},
    {U"\x1b[18;2~", SHIFT | F7},
    {U"\x1b[18;3~", ALT | F7},
    {U"\x1b[18;4~", SHIFT | ALT | F7},
    {U"\x1b[18;5~", CTRL | F7},
    {U"\x1b[18;6~", SHIFT | CTRL | F7},
    {U"\x1b[18;7~", CTRL | ALT | F7},
    {U"\x1b[18;8~", SHIFT | CTRL | ALT | F7},

    {U"\x1b[19~", F8},
    {U"\x1b[19;2~", SHIFT | F8},
    {U"\x1b[19;3~", ALT | F8},
    {U"\x1b[19;4~", SHIFT | ALT | F8},
    {U"\x1b[19;5~", CTRL | F8},
    {U"\x1b[19;6~", SHIFT | CTRL | F8},
    {U"\x1b[19;7~", CTRL | ALT | F8},
    {U"\x1b[19;8~", SHIFT | CTRL | ALT | F8},

    {U"\x1b[20~", F9},
    {U"\x1b[20;2~", SHIFT | F9},
    {U"\x1b[20;3~", ALT | F9},
    {U"\x1b[20;4~", SHIFT | ALT | F9},
    {U"\x1b[20;5~", CTRL | F9},
    {U"\x1b[20;6~", SHIFT | CTRL | F9},
    {U"\x1b[20;7~", CTRL | ALT | F9},
    {U"\x1b[20;8~", SHIFT | CTRL | ALT | F9},

    {U"\x1b[21~", F10},
    {U"\x1b[21;2~", SHIFT | F10},
    {U"\x1b[21;3~", ALT | F10},
    {U"\x1b[21;4~", SHIFT | ALT | F10},
    {U"\x1b[21;5~", CTRL | F10},
    {U"\x1b[21;6~", SHIFT | CTRL | F10},
    {U"\x1b[21;7~", CTRL | ALT | F10},
    {U"\x1b[21;8~", SHIFT | CTRL | ALT | F10},

    {U"\x1b[23~", F11},
    {U"\x1b[23;2~", SHIFT | F11},
    {U"\x1b[23;3~", ALT | F11},
    {U"\x1b[23;4~", SHIFT | ALT | F11},
    {U"\x1b[23;5~", CTRL | F11},
    {U"\x1b[23;6~", SHIFT | CTRL | F11},
    {U"\x1b[23;7~", CTRL | ALT | F11},
    {U"\x1b[23;8~", SHIFT | CTRL | ALT | F11},

    {U"\x1b[24~", F12},
    {U"\x1b[24;2~", SHIFT | F12},
    {U"\x1b[24;3~", ALT | F12},
    {U"\x1b[24;4~", SHIFT | ALT | F12},
    {U"\x1b[24;5~", CTRL | F12},
    {U"\x1b[24;6~", SHIFT | CTRL | F12},
    {U"\x1b[24;7~", CTRL | ALT | F12},
    {U"\x1b[24;8~", SHIFT | CTRL | ALT | F12},

    {U"\x1b\x7f", ALT | BACKSPACE},
    {U"\x1b\x08", CTRL | ALT | BACKSPACE},

    {U"\x1b\x0d", ALT | ENTER},

    {U"\x1b[Z", SHIFT | TAB}
};

char32_t Term::read_key() {
    char32_t key{};
    while ((key = read_key0()) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return key;
}

char32_t Term::read_key0() {
    u32string seq = Term::Private::read_sequence0();
    return Term::Private::decode_sequence(seq);
}


u32string Term::Private::read_sequence() {
    u32string seq = read_sequence0();
    while (seq.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        seq = read_sequence0();
    }
    return seq;
}

u32string Term::Private::read_sequence0() {
    u32string seq;
    char32_t c;
    if (!Private::read_raw(&c))
        return seq;
    seq.push_back(c);
    if (c == U'\x1b') {
        if (!Private::read_raw(&c)) return seq;
        seq.push_back(c);
        if (c != U'[' && c != U'O') return seq;
        do {
            if (!Private::read_raw(&c)) return seq;
            seq.push_back(c);
        } while (c < 0x40);
    }
    return seq;
}

char32_t Term::Private::decode_sequence(const u32string& seq) {
    if (!seq.size())
        return 0;
    if (seq.size() == 1) {
        switch (seq[0]) {
            // Ctrl-Enter yields 0a on Windows, but on Linux 0d which is no
            // different from Enter w/o Ctrl, so we'll bring these two in line
            case U'\x0a':
                return ENTER;
            // Oddly enough, hitting BACKSPACE yields the ASCII code for DEL
            // (0x7f) while CTRL-BACKSPACE yields 0x08 (Backspace)
            case U'\x7f':
                return BACKSPACE;
            case U'\x08':
                return CTRL | BACKSPACE;
            default:
                return seq[0];
        }
    }
    if (sequences.count(seq)) {
        return sequences[seq];
    } else if (seq.size() == 2 || seq[0] == ESC) {
        return ALT | seq[1];
    }
    return Key::UNKNOWN;
}