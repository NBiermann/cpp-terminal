#include "base.hpp"
#include <iostream>
#include <string>
#include "private/conversion.hpp"
#include "private/platform.hpp"
#include "window.hpp"


std::string Term::color(style value) {
    return "\033[" + std::to_string(static_cast<unsigned int>(value)) + 'm';
}
std::string Term::color(fg value) {
    return "\033[" + std::to_string(static_cast<unsigned int>(value)) + 'm';
}
std::string Term::color(bg value) {
    return "\033[" + std::to_string(static_cast<unsigned int>(value)) + 'm';
}

std::string Term::color24_fg(unsigned int red,
                             unsigned int green,
                             unsigned int blue) {
    return "\033[38;2;" + std::to_string(red) + ';' + std::to_string(green) +
           ';' + std::to_string(blue) + 'm';
}

std::string Term::color24_bg(unsigned int red,
                             unsigned int green,
                             unsigned int blue) {
    return "\033[48;2;" + std::to_string(red) + ';' + std::to_string(green) +
           ';' + std::to_string(blue) + 'm';
}

void Term::write(const std::string& s) {
    std::cout << s << std::flush;
}

std::string Term::cursor_off() {
    return "\x1b[?25l";
}

std::string Term::cursor_on() {
    return "\x1b[?25h";
}

std::string Term::clear_screen() {
    return "\033[2J";
}

std::string Term::clear_screen_buffer() {
    return "\033[3J";
}

std::string Term::move_cursor(size_t col, size_t row) {
    return "\x1b[" + std::to_string(row + 1) + ';' + 
        std::to_string(col + 1) + 'H';
}

std::string Term::move_cursor_right(int col) {
    return "\x1b[" + std::to_string(col) + 'C';
}

std::string Term::move_cursor_down(int row) {
    return "\x1b[" + std::to_string(row) + 'B';
}

std::string Term::erase_to_eol() {
    return "\x1b[K";
}
bool Term::is_stdin_a_tty() {
    return Private::is_stdin_a_tty();
}
bool Term::is_stdout_a_tty() {
    return Private::is_stdout_a_tty();
}
bool Term::get_term_size(size_t& cols, size_t& rows) {
    return Private::get_term_size(cols, rows);
}

void Term::restore_screen() {
    write("\033[?1049l");  // restore screen
    write(
        "\033"
        "8");  // restore current cursor position
}

void Term::save_screen() {
    write(
        "\033"
        "7");              // save current cursor position
    write("\033[?1049h");  // save screen
}

void Term::get_cursor_position(int& rows, int& cols) {
    std::string buf;
    char32_t c;
    write("\x1b[6n"); // cursor position report
    for (unsigned int i = 0; i < 64; i++) {
        while (!Private::read_raw(&c))
            ;
        buf.push_back(char(c));
        if (buf[i] == 'R') {
            if (i < 5) {
                throw std::runtime_error(
                    "get_cursor_position(): too short response");
            } else {
                buf.pop_back();
            }
            break;
        }
    }
    // Find the result in the response, drop the rest:
    size_t i = buf.find("\x1b[");
    if (i == std::string::npos)
        throw std::runtime_error(
            "get_cursor_position(): result not found in the response");
    if (Private::convert_string_to_int(buf.substr(i + 2).c_str(),
                                       "%d;%d", &rows, &cols) != 2) {
        throw std::runtime_error(
            "get_cursor_position(): result could not be parsed");
    }
    --cols;
    --rows;
    return;
}

Term::Terminal::Terminal(unsigned options)
    : BaseTerminal(bool(options & CLEAR_SCREEN),
        bool(options & RAW_INPUT), 
        bool(options & DISABLE_CTRL_C))
    , w(0)
    , h(0)
{
    update_size();
}


Term::Terminal::~Terminal() {
}

bool Term::Terminal::update_size() {
    size_t old_w = w, old_h = h;
    bool ok = Term::get_term_size(w, h);
    if (!ok) throw ("Term::Terminal::update_size() failed");
    return (old_w != w || old_h != h);
}


size_t Term::Terminal::get_w() const {
    return w;
}

size_t Term::Terminal::get_h() const {
    return h;
}

void Term::Terminal::draw_window (Window& win,
                                  size_t offset_x, size_t offset_y) {
    size_t w, h;
    get_term_size(w, h);
    std::cout << Term::cursor_off();//<< Term::clear_screen();
    std::cout << Term::move_cursor(0, 0);
    std::cout << win.render(offset_x, offset_y, w, h) << std::flush;
    if (!win.is_cursor_visible()) return;
    std::cout << Term::move_cursor(win.get_cursor_x(), win.get_cursor_y());
    std::cout << cursor_on() << std::flush;

}
