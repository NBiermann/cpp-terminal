#pragma once
#ifndef wIndOW_HeaDERguARd
#define wIndOW_HeaDERguARd

#include "base.hpp"
#include "input.hpp"
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
protected :
 	bool rgb_mode;
	uint8_t r, g;
	union {
		uint8_t b;
		Term::fg fg_val;
		Term::bg bg_val;
	};
	Color(const Term::fg val) : rgb_mode(false), r(0), g(0), fg_val(val) {}
	Color(const Term::bg val) : rgb_mode(false), r(0), g(0), bg_val(val) {}
public :
    Color(const uint8_t r_, const uint8_t g_, const uint8_t b_) : rgb_mode(true), r(r_), g(g_), b(b_) {}
    bool is_rgb() const {return rgb_mode;}
    uint8_t get_r() const {return r;}
    uint8_t get_g() const {return g;}
    uint8_t get_b() const {return b;}
    virtual std::string render() = 0;
    virtual bool is_reset() = 0;
};

class fgColor : public Color  {
    friend class Window;
public :
    fgColor() : Color(fg::reset) {}
    fgColor(fg c) : Color(c) {}
    fgColor(const uint8_t r_, const uint8_t g_, const uint8_t b_) : Color(r_, g_, b_) {}
    fg get_fg() const {return fg_val;}
    std::string render() {
        if (rgb_mode) return Term::color24_fg(r, g, b);
        else return Term::color(fg_val);
    }
    bool is_reset() {
        return (rgb_mode == false && fg_val == fg::reset);
    }
};

bool operator==(const fgColor &, const fgColor &);
bool operator!=(const fgColor &, const fgColor &);

class bgColor : public Color {
    friend class Window;
public :
    bgColor() : Color(bg::reset) {}
    bgColor(bg c) : Color(c) {}
    bgColor(const uint8_t r_, const uint8_t g_, const uint8_t b_) : Color(r_, g_, b_) {}
    bg get_bg() const {return bg_val;}
    std::string render() {
        if (rgb_mode) return Term::color24_bg(r, g, b);
        else return Term::color(bg_val);
    }
    bool is_reset() {
        return (rgb_mode == false && bg_val == bg::reset);
    }
};

bool operator==(const bgColor &c1, const bgColor &c2);
bool operator!=(const bgColor &c1, const bgColor &c2);

class Cell {
    friend class Window;
	char32_t ch;
	fgColor cell_fg;
	bgColor cell_bg;
	style cell_style;
public :
    Cell() : ch(U' '), cell_fg(fg::reset), cell_bg(bg::reset), cell_style(style::reset) {}
    Cell(char32_t c) : ch(c), cell_fg(fg::reset), cell_bg(bg::reset), cell_style(style::reset) {}
};

class ChildWindow; // forward declaration

class Window {
   protected :
    size_t w{}, h{};                  // width and height of the window
    bool is_main_window{};
    bool auto_growing{};
    bool auto_newline{};
    size_t cursor_x{}, cursor_y{};    // current cursor position
    bool cursor_visible{};
    size_t indent{}, tabsize{};

    fgColor active_fg;
    bgColor active_bg;
    style active_style{};
    std::vector<std::vector<Cell>> grid;  // the cells (grid[0] is top row)
    std::vector<ChildWindow> children;

    void assure_pos(size_t x, size_t y);
    Window merge_children() const;

   public :
    Window(size_t w_, size_t h_)
        : w{w_},
          h{h_},
          is_main_window(true),
          auto_growing(false),
          auto_newline(true),
          cursor_x{0},
          cursor_y{0},
          cursor_visible(true),
          indent(0),
          tabsize(0),

          active_fg(fgColor(fg::reset)),
          active_bg(bgColor(bg::reset)),
          active_style(style::reset),
          grid(h_)
    {}

    size_t get_w() const;
    void set_w(size_t);

    size_t get_h() const;
    void set_h(size_t);
    void resize(size_t, size_t);

    bool is_auto_growing() const;
    void set_auto_growing(bool);

    bool is_auto_newline() const;
    void set_auto_newline(bool);

    size_t get_cursor_x() const;
    size_t get_cursor_y() const;
    void set_cursor(size_t, size_t);

    bool get_cursor_visibility() const;
    void show_cursor();
    void hide_cursor();

    size_t get_indent() const;
    void set_indent(size_t);

    size_t get_tabsize() const;
    void set_tabsize(size_t);

    char32_t get_char(size_t, size_t) const;
    void set_char(size_t, size_t, char32_t);

    fgColor get_fg(size_t, size_t) const;
    void set_fg(size_t, size_t, fgColor);
    void set_fg(size_t, size_t, fg);
    void set_fg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    bgColor get_bg(size_t, size_t) const;
    void set_bg(size_t, size_t, bgColor);
    void set_bg(size_t, size_t, bg);
    void set_bg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    style get_style(size_t, size_t) const;
    void set_style(size_t, size_t, style);

    Cell get_cell(size_t, size_t) const;
    void set_cell(size_t, size_t, const Cell&);

    std::vector<std::vector<Cell>> get_grid() const;
    void set_grid(const std::vector<std::vector<Cell>> &);
    void copy_grid_from_window(const Window&);
    void clear_grid();


    void choose_fg(fg);
    void choose_fg(uint8_t, uint8_t, uint8_t);
    void choose_bg(bg);
    void choose_bg(uint8_t, uint8_t, uint8_t);
    void choose_style(style);

    void print_str(const std::u32string&);
    void print_str(const std::string&);

    void fill_fg(int, int, int, int, fgColor);
    void fill_bg(int, int, int, int, bgColor);
    void fill_style(int, int, int, int, style);

    void print_rect(size_t, size_t, size_t, size_t, border_type = border_type::utf1, fgColor = fgColor(fg::reset), bgColor = bgColor(bg::reset));

    void clear();

    void clear_row(size_t);

    std::string render();
    Window cutout(size_t x0, size_t y0, size_t width, size_t height) const;
    size_t new_child(size_t o_x, size_t o_y, size_t w_, size_t h_, border_type b = border_type::utf1);
    ChildWindow * get_child(size_t);
    size_t get_children_count() const;
    void get_cursor_from_child(size_t);
    void get_cursor_from_child(const std::vector<size_t>&);
};

class ChildWindow : public Window {
    friend Window;
    size_t offset_x{}, offset_y{};
    border_type border;
    fgColor border_fg;
    bgColor border_bg;
    bool visible;

    ChildWindow(size_t o_x, size_t o_y, size_t w_, size_t h_, border_type b = border_type::utf1)
        : Window(w_, h_),
          offset_x(o_x),
          offset_y(o_y),
          border(b),
          visible(false)
{
    is_main_window = false;
}

public :
    size_t get_offset_x() const;
    size_t get_offset_y() const;
    void move_to(size_t, size_t);
    border_type get_border() const;
    void set_border(border_type, fgColor = fgColor(fg::reset), bgColor = bgColor(bg::reset));
    void set_border_fg(fgColor);
    void set_border_bg(bgColor);
    void show();
    void hide();
    bool is_visible() const;
};

class TerminalWindow : public Terminal {
public:
    Window win;
    TerminalWindow(int options = CLEAR_SCREEN | RAW_INPUT);
};

}  // namespace Term
#endif // wIndOW_HeaDERguARd

