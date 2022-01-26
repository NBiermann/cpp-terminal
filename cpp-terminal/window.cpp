#include "window.hpp"
// https://github.com/yhirose/cpp-unicodelib
// disable some GCC/clang warnings (long files with a ton of warnings)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wc++98-c++11-compat-binary-literal"
#pragma GCC diagnostic ignored "-Wc++98-c++11-compat-pedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif // defined
#include "unicodelib.h"
#include "unicodelib_encodings.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif // defined
#include <stdexcept>


using namespace std;

/***************
   Term::Color
 ***************/

Term::Color::Color(const Term::fg val)
    : rgb_mode(false), r{}, g{}, fg_val(val) {}

Term::Color::Color(const Term::bg val)
    : rgb_mode(false), r{}, g{}, bg_val(val) {}

Term::Color::Color(const uint8_t r_, const uint8_t g_, const uint8_t b_)
    : rgb_mode(true), r(r_), g(g_), b(b_)
{}

bool Term::Color::is_rgb() const {return rgb_mode;}
uint8_t Term::Color::get_r() const {return r;}
uint8_t Term::Color::get_g() const {return g;}
uint8_t Term::Color::get_b() const {
    return b;
}

/*****************
   Term::FgColor
 *****************/

Term::FgColor::FgColor() : Color(Term::fg::reset) {}
Term::FgColor::FgColor(fg c) : Color(c) {}
Term::FgColor::FgColor(const uint8_t r_, const uint8_t g_, const uint8_t b_)
        : Color(r_, g_, b_)
{}

Term::fg Term::FgColor::get_fg() const {
    return fg_val;
}

bool Term::FgColor::operator==(const FgColor& color) const {
    if (rgb_mode != color.rgb_mode) return false;
    if (rgb_mode) return (r == color.r && g == color.g && b == color.b);
    return (fg_val == color.fg_val);
}

bool Term::FgColor::operator!=(const FgColor& color) const {
    return !operator==(color);
}

std::string Term::FgColor::render() {
    if (rgb_mode) return Term::color24_fg(r, g, b);
    else return Term::color(fg_val);
}

bool Term::FgColor::is_reset() const {
    return (rgb_mode == false && fg_val == fg::reset);
}

bool Term::FgColor::is_unspecified() const {
    return (rgb_mode == false && fg_val == fg::unspecified);
}

/*****************
   Term::BgColor
 *****************/

Term::BgColor::BgColor() : Color(Term::bg::reset) {}
Term::BgColor::BgColor(bg c) : Color(c) {}
Term::BgColor::BgColor(const uint8_t r_, const uint8_t g_, const uint8_t b_)
    : Color(r_, g_, b_)
{}
Term::bg Term::BgColor::get_bg() const {
    return bg_val;
}

bool Term::BgColor::operator==(const BgColor& color) const {
    if (rgb_mode != color.rgb_mode) return false;
    if (rgb_mode) return (r == color.r && g == color.g && b == color.b);
    return (bg_val == color.bg_val);
}

bool Term::BgColor::operator!=(const BgColor& color) const {
    return !operator==(color);
}

std::string Term::BgColor::render () {
    if (rgb_mode) return Term::color24_bg(r, g, b);
    else return Term::color(bg_val);
}

bool Term::BgColor::is_reset() const {
    return (rgb_mode == false && bg_val == bg::reset);
}

bool Term::BgColor::is_unspecified() const {
    return (rgb_mode == false && bg_val == bg::unspecified);
}

/**************
 * Term::Cell
 **************
 */

Term::Cell::Cell()
    : cell_fg(fg::unspecified)
    , cell_bg(bg::unspecified)
    , cell_style(style::unspecified)
    , grapheme_length(0)
    , ch(U"")
{}

Term::Cell::Cell(char32_t c)
    : cell_fg(fg::unspecified)
    , cell_bg(bg::unspecified)
    , cell_style(style::unspecified)
    , grapheme_length(1)
    , ch{c}
{}

Term::Cell::Cell(const u32string& s){
    *this = Cell(s, fg::unspecified, bg::unspecified, style::unspecified);
}

Term::Cell::Cell(
        char32_t c,
        Term::FgColor a_fg,
        Term::BgColor a_bg,
        Term::style a_style)
    : cell_fg(a_fg)
    , cell_bg(a_bg)
    , cell_style(a_style)
    , grapheme_length(1)
    , ch{c}
{}

Term::Cell::Cell(const u32string& s, FgColor a_fg, BgColor a_bg, style a_style)
    : cell_fg(a_fg), cell_bg(a_bg), cell_style(a_style) {
    if (unicode::grapheme_count(s) > 1)
        throw runtime_error("Cell::Cell() string has more than 1 grapheme");
    if (s.size() > MAX_GRAPHEME_LENGTH)
        throw runtime_error(
            "Window::set_grapheme(): grapheme exceeds MAX_GRAPHEME_LENGTH");
    grapheme_length = s.size();
    s.copy(ch, s.size());
}

/****************
 * Term::Window
 ****************
 */

Term::Window::Window(size_t w_, size_t h_)
    : w{w_}
    , h{h_}
    , width_fixed(true)
    , height_fixed(true)
    , cursor_visibility(true)
    , cursor_x{0}
    , cursor_y{0}
    , tabsize(4)
    , default_fg(FgColor(fg::reset))
    , default_bg(BgColor(bg::reset))
    , default_style(style::reset)
    , grid(h_)
{}

Term::Window::~Window() {
    for (ChildWindow* cwin : children) {
        delete cwin;
    }
}

void Term::Window::assure_pos(size_t x, size_t y){
    if (y >= h) {
        if (height_fixed) throw std::runtime_error("y out of bounds");
        h = y + 1;
        grid.resize(h);
    }
    if (x >= w) {
        if (width_fixed) throw std::runtime_error("x out of bounds");
        w = x + 1;
    }
    if (x >= grid[y].size()) {
        grid[y].resize(x + 1);
    }
}

size_t Term::Window::simple_write(const std::u32string& s,
                                       FgColor a_fg,
                                       BgColor a_bg,
                                       style a_style) {
    using Term::Key;
    if (a_fg == fg::unspecified)
        a_fg = default_fg;
    if (a_bg == bg::unspecified)
        a_bg = default_bg;
    if (a_style == style::unspecified)
        a_style = default_style;
    if (cursor_y >= h && height_fixed) {
        // out of the window. Should not happen here: just to be safe.
        cursor_y = h - 1;
        if (cursor_x >= w)
            cursor_x = w - 1;
        return 0;
    }
    size_t x = cursor_x;
    size_t y = cursor_y;

    size_t i = 0;
    size_t sz = 0;
    for (; i != s.size(); i += sz) {
        sz = unicode::grapheme_length(s.data() + i);
        u32string grapheme = s.substr(i, sz);
        // The following two lines are a workaround for Windows where
        // combining characters still don't print out correctly.
        // Whenever a precomposed codepoint exists, the result will be
        // fine. Otherwise, broken output ...
        grapheme = unicode::to_nfc(grapheme);
        size_t sz_internal = unicode::grapheme_length(grapheme);
        if (sz_internal > MAX_GRAPHEME_LENGTH)
            throw runtime_error("Window::write(): too long grapheme cluster");

        bool newline =
            (grapheme[0] == CR || grapheme[0] == LF || (x >= w && width_fixed));
        if (newline) {
            ++y;
            if (y >= h) {
                if (height_fixed) {
                    // out of the window
                    y = h - 1;
                    if (x >= w)
                        x = w - 1;
                    break;
                }
                // adjust window height
                set_h(y + 1);
            }
            x = 0;
            if (grapheme[0] == CR || grapheme[0] == LF)
                continue;
        }

        // tab
        if (grapheme[0] == Key::TAB && tabsize) {
            size_t no_of_blanks = tabsize - (x % tabsize);
            if (x + no_of_blanks > w)
                no_of_blanks = w - x;
            for (size_t j = 0; j != no_of_blanks; ++j) {
                set_grapheme(x, y, U" ");
                set_fg(x, y, a_fg);
                set_bg(x, y, a_bg);
                set_style(x, y, a_style);
                ++x;
            }
        } else if (grapheme[0] >= U' ' && grapheme[0] <= UTF8_MAX) {
            if ((x >= w && width_fixed) || (y >= h && height_fixed)) {
                // out of the window
                break;
            }
            set_grapheme(x, y, grapheme);
            set_fg(x, y, a_fg);
            set_bg(x, y, a_bg);
            set_style(x, y, a_style);
            ++x;
        }
        if (x < w)
            continue;
        // Right margin exceeded. Ignore that if the next character in
        // the string is a newline character anyway:
        if (i + 1 != s.size() && (s[i + 1] == CR || s[i + 1] == LF))
            continue;
        // If allowed, adjust the width of the window
        if (!width_fixed) {
            set_w(x + 1);
            continue;
        }
        // Otherwise, start new line
        if (y < h - 1) {
            ++y;
            x = 0;
            continue;
        }
        // If at bottom line, either grow height if allowed ...
        if (!height_fixed) {
            ++y;
            set_h(y + 1);
            x = 0;
            continue;
        }
        // ... or keep cursor in bottom right corner and exit loop
        y = h - 1;
        x = w - 1;
        ++i;
        break;
    }
    // set cursor
    cursor_x = x;
    cursor_y = y;
    return i;
}

size_t Term::Window::simple_write(const std::string& s,
                                       FgColor a_fg,
                                       BgColor a_bg,
                                       style a_style) {
    std::u32string s32 = unicode::utf8::decode(s);
    size_t i = simple_write(s32, a_fg, a_bg, a_style);
    return unicode::utf8::encode(s32.substr(0, i)).size();
}

size_t Term::Window::simple_write(char32_t ch,
                                       FgColor a_fg,
                                       BgColor a_bg,
                                       style a_style) {
    std::u32string s32(1, ch);
    return simple_write(s32, a_fg, a_bg, a_style);
}

size_t Term::Window::write_wordwrap(const std::u32string& s,
                                    FgColor a_fg,
                                    BgColor a_bg,
                                    style a_style) {
    size_t i = 0;
    size_t grapheme_total_count = 0;
    size_t written = 0;
    while (i != s.size()) {
        if (cursor_x >= w)
            throw runtime_error("write_wordwrap(): cursor out of window");
        size_t x = cursor_x;
        size_t j = i;
        
        bool print_so_far = false;
        bool insert_newline = false;
        bool skip_next = false;
        while (!print_so_far) {
            print_so_far = unicode::is_white_space(s[j]) ||
                           wrap_after.find(s[j]) != string::npos;
            // j to next grapheme
            j += unicode::grapheme_length(s.data() + j);
            ++x;
            if (j == s.size()) break;
            print_so_far |= unicode::is_white_space(s[j]) ||
                            wrap_before.find(s[j]) != string::npos;
            // end of line?
            if (x == w) {
                skip_next = unicode::is_white_space(s[j]) && 
                            skip_whitespace_at_eol;
                insert_newline = (cursor_x && !skip_next && !print_so_far);
                print_so_far = true;    
            }
        }
        if (insert_newline) {
            written = simple_write(Key::LF, a_fg, a_bg, a_style);
            if (!written) return grapheme_total_count;
        }
        written = simple_write(s.substr(i, j - i), a_fg, a_bg, a_style);
        grapheme_total_count += written;
        if (written != j - i) return grapheme_total_count;
        i = j;
        if (skip_next && i != s.size()) {
            i += unicode::grapheme_length(s.data() + i);
            ++grapheme_total_count;
        }
    }
    return grapheme_total_count;
}

size_t Term::Window::write_wordwrap(const std::string& s,
                                    FgColor a_fg,
                                    BgColor a_bg,
                                    style a_style) {
    std::u32string s32 = unicode::utf8::decode(s);
    size_t i = write_wordwrap(s32, a_fg, a_bg, a_style);
    return unicode::utf8::encode(s32.substr(0, i)).size();
}

Term::Window Term::Window::merge_children() const {
    Window res(w, h);
    res.copy_grid_from(*this);
    for (const ChildWindow* cwin : children) {
        // merge_into_grid() is recursive
        cwin->merge_into_grid(&res, 0, 0, w, h);
    }
    res.cursor_x = cursor_x;
    res.cursor_y = cursor_y;
    return res;
}

size_t Term::Window::get_w() const {
    return w;
}

void Term::Window::set_w(size_t new_w) {
    if (new_w == w)
        return;
    w = new_w;
    for (size_t y = 0; y != grid.size(); ++y) {
        if (grid[y].size() > w)
            grid[y].resize(w);
    }
}

void Term::Window::trim_w(size_t minimal_width) {
    if (w <= minimal_width) {
        set_w(minimal_width);
        return;
    }
    size_t x = minimal_width;
    for (const auto& row : grid)
        x = max(x, row.size());
    // make sure the cursor remains in the window
    x = max(x, cursor_x + 1);
    set_w(x);
}

size_t Term::Window::get_h() const {
    return h;
}

void Term::Window::set_h(size_t new_h) {
    grid.resize(new_h);
    h = new_h;
}

void Term::Window::trim_h(size_t minimal_height) {
    if (h <= minimal_height || grid.size() <= minimal_height) {
        set_h(minimal_height);
        return;
    }
    size_t y = min(h, grid.size());
    for (; y > minimal_height; --y) {
        if (grid[y - 1].size())
            break;
    }
    // make sure the cursor remains in the window
    y = max(y, cursor_y + 1);
    set_h(y);
    // TODO take non-hidden sub-windows into account
}

void Term::Window::resize(size_t new_w, size_t new_h) {
    set_w(new_w);
    set_h(new_h);
}

void Term::Window::trim(size_t min_w, size_t min_h) {
    trim_h(min_h);
    trim_w(min_w);
}

bool Term::Window::is_width_fixed() const {
    return width_fixed;
}

void Term::Window::set_width_fixation(bool a) {
    width_fixed = a;
}

bool Term::Window::is_height_fixed() const {
    return height_fixed;
}

void Term::Window::set_height_fixation(bool a) {
    height_fixed = a;
}

size_t Term::Window::get_cursor_x() const {
    return cursor_x;
}

size_t Term::Window::get_cursor_y() const {
    return cursor_y;
}

void Term::Window::set_cursor(size_t x, size_t y) {
    assure_pos(x, y);
    cursor_x = x;
    cursor_y = y;
}

bool Term::Window::is_cursor_visible() const {
    return cursor_visibility;
}

void Term::Window::show_cursor() {
    cursor_visibility = true;
}

void Term::Window::hide_cursor() {
    cursor_visibility = false;
}

size_t Term::Window::get_tabsize() const {
    return tabsize;
}

void Term::Window::set_tabsize(size_t ts) {
    tabsize = ts;
}

char32_t Term::Window::get_char(size_t x, size_t y) const {
    // TODO test grapheme length first?!
    if (y >= grid.size() || x >= grid[y].size() ||
        grid[y][x].grapheme_length == 0) {
        return U' ';
    }
    return grid[y][x].ch[0];
}

void Term::Window::set_char(size_t x, size_t y, char32_t c) {
    assure_pos(x, y);
    grid[y][x].grapheme_length = 1;
    grid[y][x].ch[0] = c;
}

uint8_t Term::Window::get_grapheme_length(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x].grapheme_length;
    return 0;
}

u32string Term::Window::get_grapheme(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return std::u32string(grid[y][x].ch, grid[y][x].grapheme_length);
    return U"";
}

void Term::Window::set_grapheme(size_t x, size_t y, const u32string& s) {
    assure_pos(x, y);
    if (unicode::grapheme_count(s) > 1)
        throw runtime_error("Window::set_grapheme(): more than 1 grapheme");
    if (s.size() > MAX_GRAPHEME_LENGTH)
        throw runtime_error(
            "Window::set_grapheme(): grapheme cluster too long");
    grid[y][x].grapheme_length = s.size();
    s.copy(grid[y][x].ch, s.size());
}

Term::FgColor Term::Window::get_fg(size_t x, size_t y) const {
    if (y >= grid.size() || x >= grid[y].size() ||
        grid[y][x].cell_fg.is_unspecified())
        return default_fg;
    return grid[y][x].cell_fg;
}

void Term::Window::set_fg(size_t x, size_t y, FgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_fg = c;
}

void Term::Window::set_fg(size_t x, size_t y,
                          uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_fg = FgColor(r, g, b);
}


Term::BgColor Term::Window::get_bg(size_t x, size_t y) const {
    if (y >= grid.size() || x >= grid[y].size() ||
        grid[y][x].cell_bg.is_unspecified())
        return default_bg;
    return grid[y][x].cell_bg;
}

void Term::Window::set_bg(size_t x, size_t y, BgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_bg = c;
}

void Term::Window::set_bg(size_t x, size_t y,
                          uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_bg = BgColor(r, g, b);
}

Term::style Term::Window::get_style(size_t x, size_t y) const {
    if (y >= grid.size() || x >= grid[y].size() ||
        grid[y][x].cell_style == style::unspecified)
        return default_style;
    return grid[y][x].cell_style;
}

void Term::Window::set_style(size_t x, size_t y, style c) {
    assure_pos(x, y);
    grid[y][x].cell_style = c;
}

Term::Cell Term::Window::get_cell(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x];
    else return Cell(U' ', default_fg, default_bg, default_style);
}

void Term::Window::set_cell(size_t x, size_t y, const Term::Cell &c) {
    assure_pos(x, y);
    grid[y][x] = c;
}

vector<vector<Term::Cell>> Term::Window::get_grid() const {
    return grid;
}

void Term::Window::set_grid(const vector<vector<Term::Cell>> &new_grid) {
    grid = new_grid;
    if (grid.size() > h) {
        if (height_fixed) grid.resize(h);
        else h = grid.size();
    }
    for (vector<Cell> &row : grid) {
        if (row.size() > w) {
            if (width_fixed) row.resize(w);
            else w = row.size();
        }
    }
}

void Term::Window::copy_grid_from(const Term::Window & win) {
    set_grid(win.get_grid());
}

Term::FgColor Term::Window::get_default_fg() const {
    return default_fg;
}

void Term::Window::set_default_fg(FgColor c) {
    default_fg = c;
}

void Term::Window::set_default_fg(uint8_t r, uint8_t g, uint8_t b) {
    default_fg = FgColor(r, g, b);
}

Term::BgColor Term::Window::get_default_bg() const {
    return default_bg;
}


void Term::Window::set_default_bg(BgColor c) {
    default_bg = c;
}

void Term::Window::set_default_bg(uint8_t r, uint8_t g, uint8_t b) {
    default_bg = BgColor(r, g, b);
}

Term::style Term::Window::get_default_style() const {
    return default_style;
}


void Term::Window::set_default_style(style c) {
    default_style = c;
}

bool Term::Window::is_wordwrap() const {
    return wordwrap;
}

void Term::Window::set_wordwrap(bool ww) {
    wordwrap = ww;
}

size_t Term::Window::write(const u32string& s,
                           FgColor a_fg,
                           BgColor a_bg,
                           style a_style) {
    if (wordwrap) return write_wordwrap(s, a_fg, a_bg, a_style);
    return simple_write(s, a_fg, a_bg, a_style);
}

size_t Term::Window::write(const string& s, 
                           FgColor a_fg, 
                           BgColor a_bg, 
                           style a_style) {
    if (wordwrap) return write_wordwrap(s, a_fg, a_bg, a_style);
    return simple_write(s, a_fg, a_bg, a_style);
}

size_t Term::Window::write(char32_t ch,
                           FgColor a_fg,
                           BgColor a_bg,
                           style a_style) {
    return simple_write(ch, a_fg, a_bg, a_style);
}

void Term::Window::fill_fg(size_t x1, size_t y1,
                           size_t width, size_t height, FgColor color) {
    if (color == fg::unspecified) color = default_fg;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_fg(i, j, color);
        }
    }
}

void Term::Window::fill_bg(size_t x1, size_t y1,
                           size_t width, size_t height, BgColor color) {
    if (color == bg::unspecified) color = default_bg;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_bg(i, j, color);
        }
    }
}

void Term::Window::fill_style(size_t x1, size_t y1,
                              size_t width, size_t height, style st) {
    if (st == style::unspecified) st = default_style;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_style(i, j, st);
        }
    }
}

void Term::Window::print_rect(
    int x1, // exceptionally, integer here. Negative values allow the
    int y1, // rectangle to be partially out of the window.
    size_t width,
    size_t height,
    border_t b,
    FgColor fgcol,
    BgColor bgcol
)
// A rectangle does not auto-grow the window.
// Only the in-window parts are drawn.
{
    if (fgcol == fg::unspecified) fgcol = default_fg;
    if (bgcol == bg::unspecified) bgcol = default_bg;
    if (x1 >= (int) w || y1 >= (int) h)
        return;
    int x2 = x1 + width - 1, y2 = y1 + height - 1;
    if (x2 < 0 || y2 < 0)
        return;
    size_t x2u = (unsigned)x2, y2u = (unsigned)y2;
    std::u32string border;
    switch (b) {
    case border_t::NO_BORDER : return;
    case border_t::BLANK : border = U"      "; break;
    case border_t::ASCII : border = U"|-++++"; break;
    case border_t::LINE : border = U"│─┌┐└┘"; break;
    case border_t::DOUBLE_LINE : border = U"║═╔╗╚╝"; break;
    default: throw runtime_error ("undefined border value");
    }
    for (size_t y = max(y1 + 1, 0); y < y2u && y < h; ++y) {
        if (x1 >= 0) {
            set_char(x1, y, border[0]);
            set_fg(x1, y, fgcol);
            set_bg(x1, y, bgcol);
        }
        if (x2u < w) {
            set_char(x2u, y, border[0]);
            set_fg(x2u, y, fgcol);
            set_bg(x2u, y, bgcol);
        }
    }
    for (size_t x = max(x1 + 1, 0); x < x2u && x < w; ++x) {
        if (y1 >= 0) {
            set_char(x, y1, border[1]);
            set_fg(x, y1, fgcol);
            set_bg(x, y1, bgcol);
        }
        if (y2u < h) {
            set_char(x, y2u, border[1]);
            set_fg(x, y2u, fgcol);
            set_bg(x, y2u, bgcol);
        }
    }
    if (x1 >= 0 && y1 >= 0) {
        set_char(x1, y1, border[2]);
        set_fg(x1, y1, fgcol);
        set_bg(x1, y1, bgcol);
    }
    if (x2u < w && y1 >= 0) {
        set_char(x2, y1, border[3]);
        set_fg(x2, y1, fgcol);
        set_bg(x2, y1, bgcol);
    }
    if (x1 >= 0 && y2u < h) {
        set_char(x1, y2, border[4]);
        set_fg(x1, y2, fgcol);
        set_bg(x1, y2, bgcol);
    }
    if (x2u < w && y2u < h) {
        set_char(x2, y2, border[5]);
        set_fg(x2, y2, fgcol);
        set_bg(x2, y2, bgcol);
    }
}

void Term::Window::clear_row(size_t y) {
    if (y < grid.size()) {
        grid[y].clear();
    }
}

void Term::Window::clear_grid() {
    grid.clear();
    grid.resize(h);
    cursor_x = 0;
    cursor_y = 0;
}

void Term::Window::clear() {
    clear_grid();
    children.clear();
}

Term::Window Term::Window::cutout(size_t x0, size_t y0,
                                  size_t width, size_t height) const {
    Window cropped(width, height);
    for (size_t y = y0; y != y0 + height && y < h; ++y) {
        for (size_t x = x0; x != x0 + width && x < grid[y].size(); ++x) {
            cropped.grid[y].push_back(grid[y][x]);
        }
    }
    // preserve cursor if within cut-out
    if (cursor_x < x0 || cursor_x >= x0 + width || cursor_y < y0
        || cursor_y >= y0 + height) {
        cropped.set_cursor(0, 0);
    }
    else {
        cropped.set_cursor(cursor_x - x0, cursor_y - y0);
    }
    return cropped;
}

Term::ChildWindow* Term::Window::new_child(size_t o_x, size_t o_y,
                                    size_t w_, size_t h_, border_t b) {
    ChildWindow* cwin = new ChildWindow(this, o_x, o_y, w_, h_, b);
    children.push_back(cwin);
    return cwin;
}

Term::ChildWindow* Term::Window::get_child_ptr(size_t i) {
    if (i < children.size()) return children[i];
    throw runtime_error("get_child(): child index out of bounds");
}

size_t Term::Window::get_child_index(ChildWindow *cwin) const {
    for (size_t i = 0; i != children.size(); ++i) {
        if (children[i] == cwin) return i;
    }
    throw runtime_error("get_child_index(): argument is not a child of *this");
}

size_t Term::Window::get_children_count() const {
    return children.size();
}

void Term::Window::child_to_foreground(ChildWindow* cwin) {
    size_t i = get_child_index(cwin);
    if (i == children.size() - 1) return;
    ChildWindow* temp = children[i];
    for (; i != children.size() - 1; ++i) {
        children[i] = children[i + 1];
    }
    children[i] = temp;
}

void Term::Window::child_to_background(ChildWindow* cwin) {
    size_t i = get_child_index(cwin);
    if (i == 0) return;
    ChildWindow* temp = children[i];
    for (; i; --i) {
        children[i] = children[i - 1];
    }
    children[0] = temp;
}

void Term::Window::take_cursor_from_child(ChildWindow* cwin) {
    if (cwin == this)
        return;
    if (cwin->is_base_window())
        throw runtime_error(
            "take_cursor_from_child(): argument is not a child");
    if (!cwin->is_visible() || !cwin->cursor_visibility) {
        cursor_visibility = false;
        return;
    }
    Window* pwin = cwin->get_parent_ptr();
    size_t x = cwin->cursor_x;
    size_t y = cwin->cursor_y;
    // Cursor obscured by child's own child?
    for (ChildWindow* cwin2 : cwin->children) {
        if (!cwin2->visible) {
            continue;
        }
        size_t b = (cwin2->border == border_t::NO_BORDER ? 0 : 1);
        if (x + b >= cwin2->offset_x && x < cwin2->offset_x + cwin2->w + b &&
            y + b >= cwin2->offset_y && y < cwin2->offset_y + cwin2->h + b) {
            cursor_visibility = false;
            return;
        }
    }
    x += cwin->offset_x;
    y += cwin->offset_y;
    if (x >= pwin->w || y >= pwin->h) {
        cursor_visibility = false;
        return;
    }
    // pass cursor on to base window
    while (!pwin->is_base_window()) {
        cwin = dynamic_cast<ChildWindow*>(pwin);
        pwin = cwin->get_parent_ptr();
        x += cwin->offset_x;
        y += cwin->offset_y;
        if (!cwin->is_visible() || x >= pwin->w || y >= pwin->h) {
            cursor_visibility = false;
            return;
        }
    }
    if (pwin != this)
        throw runtime_error(
            "take_cursor_from_child(): argument does not belong to *this");
    cursor_x = x;
    cursor_y = y;
    // is cursor obscured by another window?
    bool is_more_to_the_fore = false;
    for (ChildWindow *cwin2 : children) {
        if (cwin2 == cwin) {
            is_more_to_the_fore = true;
            continue;
        }
        if (!is_more_to_the_fore || !cwin2->visible) {
            continue;
        }
        size_t b = (cwin2->border == border_t::NO_BORDER ? 0 : 1);
        if (x + b >= cwin2->offset_x &&
            x < cwin2->offset_x + cwin2->w + b &&
            y + b >= cwin2->offset_y &&
            y < cwin2->offset_y + cwin2->h + b) {
            cursor_visibility = false;
            return;
        }
    }
    cursor_visibility = true;
    return;
}


/*********************
 * Term::ChildWindow
 *********************
 */

Term::ChildWindow::ChildWindow(Window* ptr,
                               size_t off_x, size_t off_y,
                               size_t w_, size_t h_, border_t b)
    : parent_ptr(ptr)
    , Window(w_, h_)
    , offset_x(off_x)
    , offset_y(off_y)
    , border(b)
    , visible(false)
{}

void Term::ChildWindow::merge_into_grid(Window *win,
                                        size_t parent_offset_x,
                                        size_t parent_offset_y,
                                        size_t parent_w,
                                        size_t parent_h) const {
    if (!visible) return;
    size_t acc_offset_x = parent_offset_x + offset_x;
    size_t acc_offset_y = parent_offset_y + offset_y;
    for (size_t y = 0; y < h; ++y) {
        size_t pos_y = acc_offset_y + y;
        // subwindow outside the (parental) window do not throw an
        // an exception. Just the in-window parts are copied into the grid.
        if (pos_y >= parent_offset_y + parent_h || pos_y >= win->get_h())
            break;
        for (size_t x = 0; x < w; ++x) {
            size_t pos_x = acc_offset_x + x;
            if (pos_x >= parent_offset_x + parent_w || pos_x >= win->get_w())
                break;
            win->set_cell(pos_x, pos_y, get_cell(x,y));
        }
    }
    // draw border:
    if (border == border_t::NO_BORDER) return;
    //
    win->print_rect(
        // TODO include title right away?!
        (int)acc_offset_x - 1, (int)acc_offset_y - 1, w + 2, h + 2,
        border, border_fg, border_bg);
    // process title
    if (!title.size()) return;
    // title vertically out of window?
    if (acc_offset_y == 0 || acc_offset_y > win->get_h()) {
        return;
    }
    u32string title_cp = title.substr(0, w);
    // if enough space, surround with blanks
    if (title_cp.size() <= w - 2) {
        title_cp = U" " + title_cp + U" ";
    }
    size_t x0 = acc_offset_x + (w - title_cp.size()) / 2;
    for (size_t i = 0; i != title_cp.size() && x0 + i < win->get_w(); ++i) {
        // color has already been set by printing border
        win->set_char(x0 + i, acc_offset_y - 1, title_cp[i]);
    }
    // recursively with own children
    for (const auto child : children) {
        child->merge_into_grid(win, acc_offset_x, acc_offset_y,
            min(w, win->get_w() - acc_offset_x),
            min(h, win->get_h() - acc_offset_y));
    }
}

Term::Window* Term::ChildWindow::get_parent_ptr() const {
    return parent_ptr;
}

bool Term::ChildWindow::is_inside_parent() const {
    size_t b = (border == border_t::NO_BORDER ? 0 : 1);
    if (offset_x < b || offset_y < b)
        return false;
    if (offset_x + w + b > parent_ptr->get_w())
        return false;
    if (offset_y + h + b > parent_ptr->get_h())
        return false;
    return true;
}

size_t Term::ChildWindow::get_offset_x() const {
    return offset_x;
}

size_t Term::ChildWindow::get_offset_y() const {
    return offset_y;
}

pair<size_t, size_t> Term::ChildWindow::move_to(size_t x, size_t y) {
    size_t border_width = (border == border_t::NO_BORDER ? 0 : 1);
    if (x < border_width) {
        offset_x = border_width;
    } else {
        size_t max_x = parent_ptr->get_w() - w - border_width;
        offset_x = min(x, max_x);
    }
    if (y < border_width) {
        offset_y = border_width;
    } else {
        size_t max_y = parent_ptr->get_h() - h - border_width;
        offset_y = min(y, max_y);
    }
    return make_pair(offset_x, offset_y);
}

std::u32string Term::ChildWindow::get_title() const {
    return title;
}

void Term::ChildWindow::set_title(const std::string& s) {
    set_title(unicode::utf8::decode(s));
}

void Term::ChildWindow::set_title(const std::u32string& s) {
    title = s;
}

Term::border_t Term::ChildWindow::get_border() const {
    return border;
}

void Term::ChildWindow::set_border(Term::border_t b,
                                   FgColor fgcol, BgColor bgcol) {
    border = b;
    if (fgcol != fg::unspecified) border_fg = fgcol;
    if (bgcol != bg::unspecified) border_bg = bgcol;
}

Term::FgColor Term::ChildWindow::get_border_fg() const {
    return border_fg;
}

void Term::ChildWindow::set_border_fg(FgColor fgcol) {
    border_fg = fgcol;
}

Term::BgColor Term::ChildWindow::get_border_bg() const {
    return border_bg;
}

void Term::ChildWindow::set_border_bg(BgColor bgcol) {
    border_bg = bgcol;
}

bool Term::ChildWindow::is_visible() const {
    return visible;
}

void Term::ChildWindow::show() {
    visible = true;
}

void Term::ChildWindow::hide() {
    visible = false;
}

void Term::ChildWindow::to_foreground() {
    parent_ptr->child_to_foreground(this);
}

void Term::ChildWindow::to_background() {
    parent_ptr->child_to_background(this);
}

