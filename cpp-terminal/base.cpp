#include "base.hpp"
#include <iostream>
#include <string>
#include <algorithm>

#include "unicodelib_encodings.h"
#include "platform.hpp"
#include "window.hpp"

using namespace std;

string Term::color(style value) {
    return "\033[" + to_string(static_cast<unsigned int>(value)) + 'm';
}
string Term::color(fg value) {
    return "\033[" + to_string(static_cast<unsigned int>(value)) + 'm';
}
string Term::color(bg value) {
    return "\033[" + to_string(static_cast<unsigned int>(value)) + 'm';
}

string Term::color24_fg(unsigned int red,
                             unsigned int green,
                             unsigned int blue) {
    return "\033[38;2;" + to_string(red) + ';' + to_string(green) +
           ';' + to_string(blue) + 'm';
}

string Term::color24_bg(unsigned int red,
                             unsigned int green,
                             unsigned int blue) {
    return "\033[48;2;" + to_string(red) + ';' + to_string(green) +
           ';' + to_string(blue) + 'm';
}

void Term::write(const string& s) {
    std::cout << s << flush;
}

string Term::cursor_off() {
    return "\x1b[?25l";
}

string Term::cursor_on() {
    return "\x1b[?25h";
}

string Term::clear_screen() {
    return "\033[2J";
}

string Term::clear_screen_buffer() {
    return "\033[3J";
}

string Term::move_cursor(size_t col, size_t row) {
    return "\x1b[" + to_string(row + 1) + ';' +
        to_string(col + 1) + 'H';
}

string Term::move_cursor_right(int col) {
    return "\x1b[" + to_string(col) + 'C';
}

string Term::move_cursor_down(int row) {
    return "\x1b[" + to_string(row) + 'B';
}

string Term::erase_to_eol() {
    return "\x1b[K";
}
bool Term::is_stdin_a_tty() {
    return Private::is_stdin_a_tty();
}
bool Term::is_stdout_a_tty() {
    return Private::is_stdout_a_tty();
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

void Term::get_cursor_position(size_t& cols, size_t& rows) {
    string buf;
    char32_t c;
    write("\x1b[6n"); // cursor position report
    for (unsigned int i = 0; i < 64; i++) {
        while (!Private::read_raw(&c))
            ;
        buf.push_back(char(c));
        if (c == U'R') {
            if (i < 5) {
                throw runtime_error(
                    "get_cursor_position(): too short response");
            }
            break;
        }
    }
    // Expected response: <ESC>[{ROW};{COLUMN}R
    // Find the result in the response, drop the rest:
    size_t i = buf.find("\x1b[");
    bool ok = (i != string::npos);
    if (ok) {
        i = buf.find_first_of("0123456789", i + 2);
        ok = (i != string::npos);   
    }
    if (ok) {
        buf = buf.substr(i);
        rows = stoul(buf, &i) - 1;
        ok = (i < buf.size());
    }
    if (ok) {
        buf = buf.substr(i + 1);  // skip ';'
        cols = stoul(buf, &i) - 1;
    } else {
        throw runtime_error(
            "get_cursor_position(): result could not be parsed");
    }
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
    bool ok = get_term_size(w, h);
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
                                  size_t x0, size_t y0,
                                  size_t width, size_t height) {
    update_size();
    if (!width) width = w;
    if (!height) height = h;
    // inside win?
    if (x0 >= win.get_w() || y0 >= win.get_h()) return;
    // adjust the cut-out to fit both win and console window
    width = std::min({width, w, win.get_w() - x0});
    height = std::min({height, h, win.get_h() - y0});

    string out = Term::cursor_off() + Term::clear_screen_buffer() +
                 Term::move_cursor(0, 0);
    Window merged_win = win.merge_children();
    fgColor current_fg(fg::reset);
    bgColor current_bg(bg::reset);
    style current_style = style::reset;
    const size_t x1 = x0 + width;
    const size_t y1 = y0 + height;
    for (size_t j = y0; j < y1; j++) {
        if (j > y0) {
            // Resetting background color at the end of each line
            // is a workaround for the bug in Visual Studio Code
            // (https://github.com/jupyter-xeus/cpp-terminal/issues/95)
            if (!current_bg.is_reset()) {
                out.append(color(bg::reset));
                current_bg = bg::reset;
            }
            out.append("\n");
        }
        size_t i = x0;
        for (; i < x1; i++) {
            bool update_fg = false;
            bool update_bg = false;
            bool update_style = false;
            Cell current_cell = merged_win.get_cell(i, j);
            if (current_fg != current_cell.cell_fg) {
                current_fg = current_cell.cell_fg;
                update_fg = true;
            }
            if (current_bg != current_cell.cell_bg) {
                current_bg = current_cell.cell_bg;
                update_bg = true;
            }
            if (current_style != current_cell.cell_style) {
                current_style = current_cell.cell_style;
                update_style = true;
                if (current_style == style::reset) {
                    // style::reset resets fg and bg colors too, we have to
                    // set them again if they are non-default, but if fg or
                    // bg colors are reset, we do not update them, as
                    // style::reset already did that.
                    update_fg = !current_fg.is_reset();
                    update_bg = !current_bg.is_reset();
                }
            }
            // Set style first, as style::reset will reset colors too
            if (update_style)
                out.append(color(current_cell.cell_style));
            if (update_fg)
                out.append(current_cell.cell_fg.render());
            if (update_bg)
                out.append(current_cell.cell_bg.render());
            if (current_cell.grapheme_length == 0 ||
                current_cell.ch[0] == '\x0')  // unspecified cell: print blank
                out.push_back(' ');
            else
                unicode::utf8::encode(current_cell.ch,
                                      current_cell.grapheme_length, out);
        }
    }
    // reset colors and style at the end
    if (!current_fg.is_reset())
        out.append(color(fg::reset));
    if (!current_bg.is_reset())
        out.append(color(bg::reset));
    if (current_style != style::reset)
        out.append(color(style::reset));
    cout << out << flush;
    // place cursor
    if (!win.is_cursor_visible()) return;
    if (win.get_cursor_x() < x0) return;
    size_t cursor_x = win.get_cursor_x() - x0;
    if (cursor_x >= w || cursor_x >= win.get_w()) return;
    if (win.get_cursor_y() < y0) return;
    size_t cursor_y = (int)win.get_cursor_y() - y0;  
    if (cursor_y >= h || cursor_y >= win.get_h()) return;
    cout << Term::move_cursor(cursor_x, cursor_y);
    cout << cursor_on() << flush;
}
