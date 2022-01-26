#pragma once

#include "platform.hpp"
#include <string>

namespace Term {

const char32_t UTF8_MAX = 0x0010ffff;

enum {
    // Option flags for Terminal constructor
    CLEAR_SCREEN = 1,
    RAW_INPUT = 2,
    DISABLE_CTRL_C = 4
};

enum class style : unsigned char {
    reset = 0,
    bold = 1,
    dim = 2,
    italic = 3,
    underline = 4,
    blink = 5,
    blink_rapid = 6,
    reversed = 7,
    conceal = 8,
    crossed = 9,
    overline = 53,
    unspecified = 253
};

enum class fg : unsigned char {
    black = 30,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    white = 37,
    reset = 39,
    gray = 90,
    bright_red = 91,
    bright_green = 92,
    bright_yellow = 93,
    bright_blue = 94,
    bright_magenta = 95,
    bright_cyan = 96,
    bright_white = 97,
    unspecified = 254
};

enum class bg : unsigned char {
    black = 40,
    red = 41,
    green = 42,
    yellow = 43,
    blue = 44,
    magenta = 45,
    cyan = 46,
    white = 47,
    reset = 49,
    gray = 100,
    bright_red = 101,
    bright_green = 102,
    bright_yellow = 103,
    bright_blue = 104,
    bright_magenta = 105,
    bright_cyan = 106,
    bright_white = 107,
    unspecified = 255
};

std::string color(style);
std::string color(fg);
std::string color(bg);

std::string color24_fg(unsigned int, unsigned int, unsigned int);

std::string color24_bg(unsigned int, unsigned int, unsigned int);

void write(const std::string&);

std::string cursor_off();

std::string cursor_on();

std::string clear_screen();

// clears screen + scroll back buffer
std::string clear_screen_buffer();

// If an attempt is made to move the cursor out of the window, the result is
// undefined.
std::string move_cursor(size_t, size_t);

// If an attempt is made to move the cursor to the right of the right margin,
// the cursor stops at the right margin.
std::string move_cursor_right(int);

// If an attempt is made to move the cursor below the bottom margin, the cursor
// stops at the bottom margin.
std::string move_cursor_down(int);

std::string erase_to_eol();

bool is_stdin_a_tty();
bool is_stdout_a_tty();

void restore_screen();

void save_screen();

void get_cursor_position(size_t&, size_t&);

class Window;

// initializes the terminal
class Terminal : public Private::BaseTerminal {
   private:
    size_t w{}, h{};

   public:
    // providing no parameters will disable the keyboard and ctrl+c
    // explicit Terminal(bool a_clear_screen);
    explicit Terminal(unsigned options = CLEAR_SCREEN);

    ~Terminal() override;

    // returns true if it has changed the values of w or h
    bool update_size();

    size_t get_w() const;
    size_t get_h() const;

    void draw_window (Window&, 
                      size_t x0 = 0, 
                      size_t y0 = 0, 
                      size_t width = std::string::npos, 
                      size_t height = std::string::npos);
};

}  // namespace Term
