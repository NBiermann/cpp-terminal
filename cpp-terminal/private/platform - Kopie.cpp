#include "platform.hpp"
#include "../input.hpp"
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

std::string to_hex(int i) {
    const std::string hexdigit = "0123456789abcdef";
    if (i == 0) return "0";
    int r;
    bool neg = false;
    std::string h;
    if (i < 0) {
        neg = true;
        i = -i;
    }
    while (i) {
        r = i % 16;
        i = i / 16;
        h = hexdigit.substr(r, 1) + h;
    }
    return (neg ? "-" : "") + h;
}

bool Term::Private::read_raw(char32_t* s) {
    // TODO: What if the keyboard is not initialized?
    if (false) {
        int c = getchar();
        if (c >= 0) {
            *s = c;
        } else if (c == EOF) {
            // In non-raw (blocking) mode this happens when the input file
            // ends. In such a case, return the End of Transmission (EOT)
            // character (Ctrl-D)
            *s = 0x04;
        } else {
            throw std::runtime_error("getchar() failed");
        }
        return true;
    }
#ifdef _WIN32
//SetConsoleCP(_In_ 65001 );
//    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
//    if (hin == INVALID_HANDLE_VALUE) {
//        throw std::runtime_error("GetStdHandle(STD_INPUT_HANDLE) failed");
//    }
//    char buf[64];
//    DWORD nread;
//    if (_kbhit()) {
//        if (!ReadConsole(hin, buf, 1, &nread, nullptr)) {
//            throw std::runtime_error("ReadFile() failed");
//        }
//
//            if (nread == sizeof(buf))
//                throw std::runtime_error("kbhit() and ReadFile() inconsistent");
//            std::cout << nread << " byte" << (nread == 1 ? ":  " : "s: ");
//            for (size_t i = 0; i != nread ; ++i) {
//                std::cout << (i ? "|" : "") << to_hex(buf[i]);
//            }
//            std::cout << "\n";
//            *s = buf[nread - 1];
//            return true;
////        }
//    } else {
//        return false;
//    }

// _getwch() ist das einzige, was mit Umlauten und € funktioniert! Leider werden F1-F12 und die Pfeiltasten anders
//

//SetConsoleCP(_In_ 65001 );
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    if (hin == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("GetStdHandle(STD_INPUT_HANDLE) failed");
    }
    std::cout << "!";
    LPDWORD lpMode;
    bool ok = GetConsoleMode(hin, lpMode);
    std::cout << "console mode = " << lpMode << " ";


    if (!_kbhit()) return false;
//    std::cout << "(CP:" << GetConsoleCP() << ") ";

    wint_t i = _getwch();

    if (i == WEOF) throw std::runtime_error("_getwch() failed");
    std::cout << "(raw: " << to_hex((int) i) << (_kbhit() ? "^" : "_") << std::cin.rdbuf()->in_avail() << ") ";
    if (i != 0 && i != 0xe0) {
        *s = static_cast<char32_t>(i);
        return true;
    }


    wint_t j = _getwch();
    if (j == WEOF) throw std::runtime_error("_getwch() failed");
    std::cout << "(raw: " << to_hex((int) j) << (_kbhit() ? "^" : "_") << std::cin.rdbuf()->in_avail() << ") ";
    if (i == 0) {
        switch (j) {
        case 0x98 : *s = Key::ALT_UP; return true;
        case 0xa0 : *s = Key::ALT_DOWN; return true;
        case 0x9d : *s = Key::ALT_RIGHT; return true;
        case 0x9b : *s = Key::ALT_LEFT; return true;
        case 0x3b : *s = Key::F1; return true;
        case 0x3c : *s = Key::F2; return true;
        case 0x3d : *s = Key::F3; return true;
        case 0x3e : *s = Key::F4; return true;
        case 0x3f : *s = Key::F5; return true;
        case 0x40 : *s = Key::F6; return true;
        case 0x41 : *s = Key::F7; return true;
        case 0x42 : *s = Key::F8; return true;
        case 0x43 : *s = Key::F9; return true;
        case 0x44 : *s = Key::F10; return true;
        default :
            j = _ungetwch(j);
            if (j == WEOF) throw std::runtime_error("_getwch() failed");
            return true;
        }
    }
    else { // i == 0xe0
        switch (j) {
        case 0x48 : *s = Key::ARROW_UP; return true;
        case 0x50 : *s = Key::ARROW_DOWN; return true;
        case 0x4d : *s = Key::ARROW_RIGHT; return true;
        case 0x4b : *s = Key::ARROW_LEFT; return true;
        case 0x8d : *s = Key::CTRL_UP; return true;
        case 0x91 : *s = Key::CTRL_DOWN; return true;
        case 0x74 : *s = Key::CTRL_RIGHT; return true;
        case 0x73 : *s = Key::CTRL_LEFT; return true;
        case 0x53 : *s = Key::DEL; return true;
        case 0x93 : *s = Key::CTRL_DEL; return true;
        case 0x47 : *s = Key::HOME; return true;
        case 0x77 : *s = Key::CTRL_HOME; return true;
        case 0x52 : *s = Key::INSERT; return true;
        case 0x4f : *s = Key::END; return true;
        case 0x75 : *s = Key::CTRL_END; return true;
        case 0x49 : *s = Key::PAGE_UP; return true;
        // case 0x86 : *s = Key::CTRL_PAGE_UP; return true; // duplicate with F12
        case 0x51 : *s = Key::PAGE_DOWN; return true;
        case 0x76 : *s = Key::CTRL_PAGE_DOWN; return true;
        case 0x85 : *s = Key::F11; return true;
        case 0x86 : *s = Key::F12; return true;
        default :
            j = _ungetwch(j);
            if (j == WEOF) throw std::runtime_error("_getwch() failed");
            *s = static_cast<char32_t>(i);
            return true;
        }
    }


//    if (!_kbhit()) return false;
//    int i;
//    do {
//        i = _getch();
//        std::cout << to_hex(i) << "|";
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//    } while (i == 0 || (i & 0xc0 == 0x80));
//    std::cout << " \n";
//    *s = static_cast<char>(i);
//    return true;
#else
    int nread = read(STDIN_FILENO, s, 1);
    if (nread == -1 && errno != EAGAIN) {
        throw std::runtime_error("read() failed");
    }
    return (nread == 1);
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
        SetConsoleCP(65001);
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
