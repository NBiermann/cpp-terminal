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


const bool Term::Private::debug = false;

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

bool Term::Private::get_term_size(size_t& cols, size_t& rows) {
#ifdef _WIN32
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("GetStdHandle(STD_OUTPUT_HANDLE) failed");
    }
    CONSOLE_SCREEN_BUFFER_INFO inf;
    if (GetConsoleScreenBufferInfo(hout, &inf)) {
        cols = static_cast<size_t>(inf.srWindow.Right - inf.srWindow.Left) + 1;
        rows = static_cast<size_t>(inf.srWindow.Bottom - inf.srWindow.Top) + 1;
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
        cols = static_cast<size_t>(ws.ws_col);
        rows = static_cast<size_t>(ws.ws_row);
        return true;
    }
#endif
}


bool Term::Private::read_raw(char32_t* s) {
    if (!Term::Private::BaseTerminal::is_instantiated ||
        !Term::Private::BaseTerminal::raw_input) {
        return false;
    }
#ifdef _WIN32
    if (!_kbhit()) return false;
    int i = _getwch();
    if (i == EOF) throw std::runtime_error("_getwch() failed");
    if (debug) std::cout << "(raw: " << Private::to_hex((int) i);

    // Even in "virtual terminal input" mode, _getwch() appears to pass on
    // a few key combinations in Windows-specific manner rather than
    // as ANSI escape sequences. These are apparently 2 bytes, the first 
    // one being zero. Another _getwch() call is required.
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
    // On Linux, ctrl - Num_x yields the same ANSI sequence as Num_x.
    // I decided to ignore the above (and possibly more such) Windows
    // combinations, with the only exception of 0 1c which will be interpreted
    // as Alt-Enter.
    if (i == 0) {
        int j = _getwch();
        if (j == EOF) throw std::runtime_error("_getch() failed");
        if (debug) std::cout << " " << Private::to_hex((int) j) << ") ";
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
    char32_t u = static_cast<char32_t>(c[0]);
    if (debug) std::cout << "(raw: " << Private::to_hex((int) u);
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
        if (debug) std::cout << "," << Private::to_hex((int) c[i]);
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

void Term::Private::clean_up() {
    typedef BaseTerminal BT;
    if (!BT::is_instantiated) return;
    if (BT::clear_screen) {
        // restore screen
        std::cout << "\x1b[?1049l" << std::flush;
        // restore old cursor position
        std::cout << "\x1b"
                     "8"
                  << std::flush;
    }
#ifdef _WIN32
    if (BT::out_console) {
        SetConsoleOutputCP(BT::out_code_page);
        if (!SetConsoleMode(BT::hout, BT::dwOriginalOutMode)) {
            throw std::runtime_error("SetConsoleMode() failed in clean-up");
        }
    }
    if (BT::raw_input) {
        SetConsoleCP(BT::in_code_page);
        if (!SetConsoleMode(BT::hin, BT::dwOriginalInMode)) {
            throw std::runtime_error("SetConsoleMode() failed in clean-up");
        }
    }
#else
    if (BT::raw_input) {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &BT::orig_termios) == -1) {
            throw std::runtime_error("tcsetattr() failed in clean-up");
        }
    }
#endif
}


#ifdef _WIN32
// While Windows does support <csignal>, this way seems to be more idiomatic
BOOL WINAPI Term::Private::CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
            // Handle the CTRL-C signal.
        case CTRL_C_EVENT:
            Term::Private::clean_up();
            // pass the signal on
            return FALSE;
        default:
            return FALSE;
    }
}
#else
void Term::Private::SignalHandler(int signum) {
    Term::Private::clean_up();
    exit(signum);
}
#endif

Term::Private::BaseTerminal::BaseTerminal(bool a_clear_screen,
                                          bool a_raw_input,
                                          bool a_disable_ctrl_c) {
    if (is_instantiated) {
        throw std::runtime_error("Only one instance of BaseTerminal allowed");
    }
    is_instantiated = true;
    clear_screen = a_clear_screen;
    raw_input = a_raw_input;
    disable_ctrl_c = a_disable_ctrl_c;
    // Uncomment this to silently disable raw mode for non-tty
    // raw_input &= is_stdin_a_tty();
#ifdef _WIN32
    out_console = is_stdout_a_tty();
    if (out_console) {
        HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
        // Uncomment to hide vertical scroll bar:
        // ShowScrollBar(GetConsoleWindow(), SB_VERT, 0);

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

    if (raw_input) {
        hin = GetStdHandle(STD_INPUT_HANDLE);
        if (hin == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("GetStdHandle(STD_INPUT_HANDLE) failed");
        }
        in_code_page = GetConsoleCP();
        //        SetConsoleCP(65001);
        if (!GetConsoleMode(hin, &dwOriginalInMode)) {
            throw std::runtime_error("GetConsoleMode() failed");
        }
        DWORD flags = dwOriginalInMode;
        flags |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        flags &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        if (disable_ctrl_c) {
            // report ctrl-c as keyboard input rather than as a signal:
            flags &= ~ENABLE_PROCESSED_INPUT;
        }
        else {
            //std::signal(SIGINT, Term::Private::SignalHandler);
            if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
                throw std::runtime_error("Could not set control handler");
            }
        }
        if (!SetConsoleMode(hin, flags)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
    }
#else
    if (raw_input) {
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
        else {
            std::signal(SIGINT, Term::Private::SignalHandler);
        }
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw std::runtime_error("tcsetattr() failed");
        }
    }
#endif
    if (clear_screen) {
        // save current cursor position
        std::cout << "\x1b"
                     "7"
                  << std::flush;
        // save screen
        std::cout << "\033[?1049h" << std::flush;
    }
}

Term::Private::BaseTerminal::~BaseTerminal() noexcept(false) {
    Term::Private::clean_up();
    is_instantiated = false;
}

bool Term::Private::BaseTerminal::is_instantiated = false;
bool Term::Private::BaseTerminal::clear_screen = false;
bool Term::Private::BaseTerminal::raw_input = false;
bool Term::Private::BaseTerminal::disable_ctrl_c = false;
#ifdef _WIN32
HANDLE Term::Private::BaseTerminal::hout{};
DWORD Term::Private::BaseTerminal::dwOriginalOutMode{};
bool Term::Private::BaseTerminal::out_console{};
UINT Term::Private::BaseTerminal::out_code_page{};
HANDLE Term::Private::BaseTerminal::hin{};
DWORD Term::Private::BaseTerminal::dwOriginalInMode{};
UINT Term::Private::BaseTerminal::in_code_page{};
#else
struct termios Term::Private::BaseTerminal::orig_termios{};
#endif
