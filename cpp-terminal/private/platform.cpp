#include "platform.hpp"
#include "../input.hpp"
#include "conversion.hpp"
#include "../base.hpp"
#include <iostream>
#include <ios>
#include <istream>
#include <iomanip>
#include <string>
#include <chrono>
#include <thread>


bool Term::Private::is_stdin_a_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdin));
#else
    return isatty(STDIN_FILENO);
#endif
}

bool Term::Private::is_stdout_a_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(STDOUT_FILENO);
#endif
}

bool Term::Private::get_term_size(int& rows, int& cols) {
#ifdef _WIN32
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("GetStdHandle(STD_OUTPUT_HANDLE) failed");
    }
    CONSOLE_SCREEN_BUFFER_INFO inf;
    if (GetConsoleScreenBufferInfo(hout, &inf)) {
        cols = inf.srWindow.Right - inf.srWindow.Left + 1;
        rows = inf.srWindow.Bottom - inf.srWindow.Top + 1;
        return true;
    } else {
        // This happens when we are not connected to a terminal
        return false;
    }
#else
    struct winsize ws {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // This happens when we are not connected to a terminal
        return false;
    } else {
        cols = ws.ws_col;
        rows = ws.ws_row;
        return true;
    }
#endif
}


bool Term::Private::read_raw(char32_t* s) {
    const bool debug = false;

#ifdef _WIN32
    if (!_kbhit()) return false;
    int i = _getwch();
    if (i == EOF) throw std::runtime_error("_getch() failed");
    if (debug)
        std::cout << "(raw: " << Private::to_hex((int) i);

    // A few special key combinations are rendered by Windows as 2
    // numbers, requiring another _getwch() call. The first number is always null:
    //    Ctrl - Num_Decimal_Point = 0 0x93
    //    Ctrl - Num_0 = 0, 0x92
    //    Ctrl - Num_1 = 0, 0x75
    //    Ctrl - Num_2 = 0, 0x91
    //    Ctrl - Num_3 = 0, 0x76
    //    Ctrl - Num_4 = 0, 0x73
    //    (Ctrl - Num_5 will not be recognized)
    //    Ctrl - Num_6 = 0, 0x74
    //    Ctrl - Num_7 = 0, 0x77
    //    Ctrl - Num_8 = 0, 0x8d
    //    Ctrl - Num_9 = 0, 0x84
    //    Ctrl - Alt - Enter = 0, 0x1c
    // On Linux, ctrl - Num_x is the same as Num_x
    // I decided to ignore the above (and possibly more such) Windows combinations
    // with the only exception of 0 1c (which will be rendered as Alt-Enter, because
    // Ctrl-Alt-Enter is not defined (apparently, no unique ANSI sequence for this).
    if (i == 0) {
        int j = _getwch();
        if (j == EOF) throw std::runtime_error("_getch() failed");
        if (debug)
            std::cout << " " << Private::to_hex((int) j) << ") ";
        *s = (j == 0x1c ? (Key::ALT | Key::ENTER) : Key::UNKNOWN);
        return true;
    }
    if (debug) std::cout << ") ";
    *s = static_cast<char32_t>(i);
    return true;

#else
    unsigned char c[3];
    int nread = read(STDIN_FILENO, c, 1);
    if (nread == -1 && errno != EAGAIN) {
        throw std::runtime_error("read() failed");
    }
    if (nread == 0) return false;
    //std::cout << "(raw: " << to_hex((int) c[0]) << ") ";
    char32_t u = static_cast<char32_t>(c[0]);
    if (debug)
        std::cout << "(raw: " << Private::to_hex((int) u);
    int bytes_to_follow;
    if (u < 0x80) {
        *s = u;
        if (debug) std::cout << ") ";
        return true;
    }
    else if (u >> 5 == 6) {
            bytes_to_follow = 1;
            u = u & 0x1f;
    }
    else if (u >> 4 == 0xe) {
        bytes_to_follow = 2;
        u = u & 0xf;
    }
    else if (u >> 3 == 0x1e) {
        bytes_to_follow = 3;
        u = u & 7;
    }
    else {
        *s = Key::UNKNOWN;
        return true;
    }
    nread = read(STDIN_FILENO, c, bytes_to_follow);
    if (nread == -1 && errno != EAGAIN) {
        throw std::runtime_error("read() failed");
    }
    if (nread != bytes_to_follow) {
        *s = Key::UNKNOWN;
        return true;
    }
    for (int i = 0; i != bytes_to_follow; ++i) {
        if (debug)
            std::cout << "," << Private::to_hex((int) c[i]);
        if (c[i] >> 6 != 2) {
            *s = Key::UNKNOWN;
            return true;
        }
        u = (u << 6) | (c[i] & 0x3f);
    }
    *s = u;
    if (debug) std::cout << " -> " << Private::to_hex(int(*s)) << ") ";
    return true;
#endif
}



Term::Private::BaseTerminal::~BaseTerminal() noexcept(false) {
#ifdef _WIN32
    if (out_console) {
        SetConsoleOutputCP(out_code_page);
        if (!SetConsoleMode(hout, dwOriginalOutMode)) {
            throw std::runtime_error("SetConsoleMode() failed in destructor");
        }
    }

    if (keyboard_enabled) {
        SetConsoleCP(in_code_page);
        if (!SetConsoleMode(hin, dwOriginalInMode)) {
            throw std::runtime_error("SetConsoleMode() failed in destructor");
        }
    }
#else
    if (keyboard_enabled) {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
            throw std::runtime_error("tcsetattr() failed in destructor");
        }
    }
#endif
}

#ifdef _WIN32
Term::Private::BaseTerminal::BaseTerminal(bool enable_keyboard,
                                          bool /*disable_ctrl_c*/)
    : keyboard_enabled{enable_keyboard} {
    // Uncomment this to silently disable raw mode for non-tty
    // if (keyboard_enabled) keyboard_enabled = is_stdin_a_tty();
    out_console = is_stdout_a_tty();
    if (out_console) {
        hout = GetStdHandle(STD_OUTPUT_HANDLE);
        out_code_page = GetConsoleOutputCP();
        SetConsoleOutputCP(65001);
        if (hout == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("GetStdHandle(STD_OUTPUT_HANDLE) failed");
        }
        if (!GetConsoleMode(hout, &dwOriginalOutMode)) {
            throw std::runtime_error("GetConsoleMode() failed");
        }
        DWORD flags = dwOriginalOutMode;
        flags |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        flags |= DISABLE_NEWLINE_AUTO_RETURN;
        if (!SetConsoleMode(hout, flags)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
    }

    if (keyboard_enabled) {
        hin = GetStdHandle(STD_INPUT_HANDLE);
        in_code_page = GetConsoleCP();
//        SetConsoleCP(65001);
        if (hin == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("GetStdHandle(STD_INPUT_HANDLE) failed");
        }
        if (!GetConsoleMode(hin, &dwOriginalInMode)) {
            throw std::runtime_error("GetConsoleMode() failed");
        }
        DWORD flags = dwOriginalInMode;
        flags |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        flags &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        if (!SetConsoleMode(hin, flags)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
    }
#else
Term::Private::BaseTerminal::BaseTerminal(bool enable_keyboard,
                                          bool disable_ctrl_c)
    : keyboard_enabled{enable_keyboard} {
    // Uncomment this to silently disable raw mode for non-tty
    // if (keyboard_enabled) keyboard_enabled = is_stdin_a_tty();
    if (keyboard_enabled) {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
            throw std::runtime_error("tcgetattr() failed");
        }

        // Put terminal in raw mode
        struct termios raw = orig_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        // This disables output post-processing, requiring explicit \r\n. We
        // keep it enabled, so that in C++, one can still just use std::endl
        // for EOL instead of "\r\n".
        // raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        if (disable_ctrl_c) {
            raw.c_lflag &= ~(ISIG);
        }
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw std::runtime_error("tcsetattr() failed");
        }
    }
#endif
}
