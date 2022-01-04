#pragma once

#include "base.hpp"
#include <string>
#include <vector>

namespace Term {

/* Represents a rectangular window, as a 2D array of characters and their
 * attributes. The render method can convert this internal representation to a
 * string that when printed will show the Window on the screen.
 *
 * Note: the characters are represented by char32_t, representing their UTF-32
 * code point. The natural way to represent a character in a terminal would be
 * a "unicode grapheme cluster", but due to a lack of a good library for C++
 * that could handle those, we simply use a Unicode code point as a character.
 */

enum class border_type : uint8_t {
    no_border,
    blank,
    ascii,
    utf1,
    utf2
};

class Color {
    friend Window;
	bool rgb_mode;
	uint8_t r, g;
	union {
		uint8_t b;
		Term::fg fg_val;
		Term::bg bg_val;
	};
public :
    Color(const uint r_, const uint g_, const uint b_) : rgb_mode(true), r(r_), g(g_), b(b_)) {}c
    void set(const uint r_, const uint g_, const uint b_) {
        rgb_mode = true;
        r = r_; g = g_; b = b_;
    }
    bool is_rgb() const {return rgb_mode}
    virtual bool operator==(const Color&) = 0;
    virtual std::string render() = 0;
    virtual bool is_reset() = 0;
};

class fgColor : Color {
    friend Window;
public :
    fgColor(const Term::fg val) : rgb_mode(false), fg_val(val)) {}
    bool operator==(const fgColor &fg2) {
        if (rgb_mode != fg2.rgb_mode) return false;
        if (rgb_mode) return (r == fg2.r && g == fg2.g && b == fg2.b);
        return (fg_val == fg2.fg_val);
    }
    void set(const Term::fg val) {
        rgb_mode = false;
        fg_val = val;
    }
    std::string render() {
        if (rgb_mode) return Term::color24_fg(r, g, b);
        return Term::color(fg_val);
    }
    bool is_reset() {
        return (rgb_mode == false && fg_val == fg::reset);
    }
};

class bgColor : Color {
    friend Window;
public :
    bgColor(const Term::bg val) : rgb_mode(false), bg_val(val)) {}
    bool operator==(const fgColor &bg2) {
        if (rgb_mode != bg2.rgb_mode) return false;
        if (rgb_mode) return (r == bg2.r && g == bg2.g && b == bg2.b);
        return (bg_val == bg2.bg_val);
    }
    void set(const Term::bg val) {
        rgb_mode = false;
        bg_val = val;
    }
    std::string render() {
        if (rgb_mode) return Term::color24_bg(r, g, b);
        return Term::color(bg_val);
    }
    bool is_reset() {
        return (rgb_mode == false && bg_val == bg::reset);
    }
};

class Cell {
    friend Window;
	char32_t ch;
	fgColor cell_fg;
	bgColor cell_bg;
	style cell_style;
public :
    Cell() : ch(U' '), cell_fg(fg::reset), cell_bg(bg::reset), cell_style(style::reset);
    Cell(char32_t c) : ch(c), cell_fg(fg::reset), cell_bg(bg::reset), cell_style(style::reset);
}

class Window {
   private:
    size_t w, h;                  // width and height of the window
    bool is_main_window;
    bool auto_growing;
    size_t cursor_x, cursor_y;    // current cursor position
    size_t indent;
    bool auto_newline;
    fgColor active_fg;
    bgColor active_bg;
    style active_style;
    std::vector<std::vector<Cell>> grid;  // the cells (grid[0] is top row)
    std::vector<ChildWindow> children;

    void assure_pos(size_t x, size_t y);
    Window merge_children();

   public:
    Window(size_t w_, size_t h_, bool grow = false)
        : w{w_},
          h{h_},
          is_main_window(true);
          auto_growing(grow),
          cursor_x{0},
          cursor_y{0},
          indent(0);
          auto_newline(true);
          active_fg(fgColor(fg::reset)),
          active_bg(bgColor(bg::reset)),
          active_style(style::reset),
          grid(h_)
    {}

    size_t get_w();
    void set_w(size_t);

    size_t get_h();
    void set_h(size_t);


    size_t get_cursor_x();
    size_t get_cursor_y();
    void set_cursor(int, int);

    size_t get_indent();
    void set_indent(size_t);

    char32_t get_char(size_t, size_t);

    fgColor get_fg(size_t, size_t);
    bgColor get_bg(size_t, size_t);
    style get_style(size_t, size_t);
    Cell get_cell(size_t, size_t);

    void set_char(size_t, size_t, char32_t);
    void set_fg(size_t, size_t, fg);
    void set_fg(size_t, size_t, uint8_t, uint8_t, uint8_t);
    void set_bg(size_t, size_t, bg);
    void set_bg(size_t, size_t, uint8_t, uint8_t, uint8_t);
    void set_style(size_t, size_t, style);

    void choose_fg(fg);
    void choose_fg(uint8_t, uint8_t, uint8_t);
    void choose_bg(bg);
    void choose bg(uint8_t, uint8_t, uint8_t);
    void choose_style(style);


    void print_str(const std::u32string&)
    void print_str(const std::string);

    void fill_fg(int, int, int, int, fgColor);
    void fill_bg(int, int, int, int, bgColor);
    void fill_style(int, int, int, int, style);

    void print_rect(size_t, size_t, size_t, size_t, border_type = border_type::utf1, fgColor = fgColor(fg::reset));

    void clear();

    void clear_row(size_t);

    std::string render();
    Window cutout(size_t x0, size_t y0, size_t width, size_t height) const;
    size_t make_child(size_t o_x, size_t o_y, size_t w_, size_t h_, border_type b = 1, bool grow = false);
};

class ChildWindow : Window {
    friend Window;
    size_t offset_x, offset_y;
    enum {no_border, ascii_border, single_border, double_border} border;
    bool visible;

    ChildWindow(size_t o_x, size_t o_y, size_t w_, size_t h_, border_type b = 1, bool grow = false)
        : offset_x(o_x),
          offset_y(o_y),
          border(b),
          visible(false);
          w{w_},
          h{h_},
          is_main_window(false);
          auto_growing(grow),
          cursor_x{0},
          cursor_y{0},
          indent(0);
          auto_newline(true);
          active_fg(fgColor(fg::reset)),
          active_bg(bgColor(bg::reset)),
          active_style(style::reset),
          grid(h_)
    {}

public :
    void show() {visible = true;}
    void hide() {visible = false;}
    bool is_visible() {return visible;}
};


}  // namespace Term
