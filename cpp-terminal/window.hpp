#pragma once
#ifndef wIndOW_HeaDERguARd
#define wIndOW_HeaDERguARd

#include "base.hpp"
#include "input.hpp"
#include <string>
#include <vector>

namespace Term {

enum class border_type : uint8_t {
    NO_BORDER,
    BLANK,
    ASCII,
    LINE,
    DOUBLE_LINE
};

// Represents a color either as 8-bit ANSI color code or as 24-bit rgb
class Color {
protected :
 	bool rgb_mode;
	uint8_t r, g;
	union {
		uint8_t b;
		fg fg_val;
		bg bg_val;
	};
	Color(const Term::fg);
	Color(const Term::bg);
	Color(const uint8_t, const uint8_t, const uint8_t);
public :
    bool is_rgb() const;
    uint8_t get_r() const;
    uint8_t get_g() const;
    uint8_t get_b() const;
    virtual std::string render() = 0;
    virtual bool is_reset() = 0;
    virtual ~Color() = default;
};

// foreground color
class fgColor : public Color  {
    friend class Window;
public :
    fgColor();
    fgColor(fg c);
    fgColor(const uint8_t, const uint8_t, const uint8_t);
    fg get_fg() const;
    std::string render() override;
    bool is_reset() override;
};

bool operator==(const fgColor &, const fgColor &);
bool operator!=(const fgColor &, const fgColor &);

// background color
class bgColor : public Color {
    friend class Window;
public :
    bgColor();
    bgColor(bg);
    bgColor(const uint8_t, const uint8_t, const uint8_t);
    bg get_bg() const;
    std::string render () override;
    bool is_reset() override;
};

bool operator==(const bgColor &c1, const bgColor &c2);
bool operator!=(const bgColor &c1, const bgColor &c2);

/* Represents a position in the terminal window, holding the character,
 * foreground and background colors, and the style of this specific cell.
 */
struct Cell {
	fgColor cell_fg;
	bgColor cell_bg;
	style cell_style;
    char32_t ch;
    Cell()
        : cell_fg(fg::reset)
        , cell_bg(bg::reset)
        , cell_style(style::reset)
        , ch(U' ') {}
    Cell(char32_t c)
        : cell_fg(fg::reset)
        , cell_bg(bg::reset)
        , cell_style(style::reset)
        , ch(c) {}
    Cell(char32_t c, fgColor a_fg, bgColor a_bg, style a_style)
        : cell_fg(a_fg)
        , cell_bg(a_bg)
        , cell_style(a_style)
        , ch(c) {}
};


class ChildWindow; // forward declaration

/* Represents a rectangular window, as a 2D array of characters and their
 * attributes. The render method can convert this internal representation to a
 * string that when printed will show the Window on the screen.
 *
 * Note: the characters are represented by char32_t, representing their UTF-32
 * code point. The natural way to represent a character in a terminal would be
 * a "unicode grapheme cluster", but due to a lack of a good library for C++
 * that could handle those, we simply use a Unicode code point as a character.
 */
class Window {
   protected :
    size_t w{}, h{};               // width and height of the window
    bool fixed_width{};            // if w may not grow automatically
    bool fixed_height{};           // same for h
    bool cursor_visibility{};
    size_t cursor_x{}, cursor_y{}; // current cursor position
    size_t tabsize{};

    fgColor default_fg;
    bgColor default_bg;
    style default_style{};
    std::vector<std::vector<Cell>> grid; // the cells (grid[0] is top row)
    std::vector<ChildWindow> children;

    void assure_pos(size_t x, size_t y);
    Window merge_children() const;

   public :
    Window(size_t w_ = 0, size_t h_ = 0)
        : w{w_},
          h{h_},
          fixed_width(true),
          fixed_height(true),
          cursor_visibility(true),
          cursor_x{0},
          cursor_y{0},
          tabsize(0),

          default_fg(fgColor(fg::reset)),
          default_bg(bgColor(bg::reset)),
          default_style(style::reset),
          grid(h_)
    {}
    virtual ~Window() = default;

    virtual bool is_main_window() {return true;}

    size_t get_w() const;
    void set_w(size_t);

    size_t get_h() const;
    void set_h(size_t);

    void resize(size_t, size_t);

    bool is_fixed_width() const;
    void set_fixed_width (bool);

    bool is_fixed_height() const;
    void set_fixed_height (bool);

    size_t get_cursor_x() const;
    size_t get_cursor_y() const;
    void set_cursor(size_t, size_t);

    bool is_cursor_visible() const;
    void show_cursor();
    void hide_cursor();

    size_t get_tabsize() const;
    void set_tabsize(size_t);

    char32_t get_char(size_t, size_t) const;
    void set_char(size_t, size_t, char32_t);

    fgColor get_fg(size_t, size_t) const;
    void set_fg(size_t, size_t, fgColor);
    void set_fg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    bgColor get_bg(size_t, size_t) const;
    void set_bg(size_t, size_t, bgColor);
    void set_bg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    style get_style(size_t, size_t) const;
    void set_style(size_t, size_t, style);

    Cell get_cell(size_t, size_t) const;
    void set_cell(size_t, size_t, const Cell&);

    std::vector<std::vector<Cell>> get_grid() const;
    void set_grid(const std::vector<std::vector<Cell>> &);
    void copy_grid_from_window(const Window&);
    void clear_grid();


    void set_default_fg(fgColor);
    void set_default_fg(uint8_t, uint8_t, uint8_t);
    void set_default_bg(bgColor);
    void set_default_bg(uint8_t, uint8_t, uint8_t);
    void set_default_style(style);

    // Returns the number of characters actually printed
    size_t print_str(const std::u32string&,
                     fgColor = fg::unspecified,
                     bgColor = bg::unspecified,
                     style = style::unspecified);
    size_t print_str(const std::string&,
                     fgColor = fg::unspecified,
                     bgColor = bg::unspecified,
                     style = style::unspecified);

    void fill_fg(size_t, size_t, size_t, size_t, fgColor);
    void fill_bg(size_t, size_t, size_t, size_t, bgColor);
    void fill_style(size_t, size_t, size_t, size_t, style);

    void print_rect(size_t, size_t, size_t, size_t,
                    border_type = border_type::LINE,
                    fgColor = fgColor(fg::reset),
                    bgColor = bgColor(bg::reset));

    void clear();

    void clear_row(size_t);

    std::string render(size_t = 0, size_t = 0,
                       size_t = std::string::npos, size_t = std::string::npos);
    Window cutout(size_t x0, size_t y0, size_t width, size_t height) const;
    size_t new_child(size_t o_x, size_t o_y, size_t w_, size_t h_,
                     border_type b = border_type::LINE);
    ChildWindow * get_child(size_t);
    size_t get_children_count() const;
    void get_cursor_from_child(size_t);
    void get_cursor_from_child(const std::vector<size_t>&);
};

// Represents a sub-window. Child windows may be nested.
class ChildWindow : public Window {
    friend Window;
    size_t offset_x{}, offset_y{};
    border_type border;
    fgColor border_fg;
    bgColor border_bg;
    bool visible{};

    ChildWindow(size_t o_x, size_t o_y, size_t w_, size_t h_,
                border_type b = border_type::LINE)
        : Window(w_, h_),
          offset_x(o_x),
          offset_y(o_y),
          border(b),
          visible(false)
{}

public :
    bool is_main_window() override {return false;}
    size_t get_offset_x() const;
    size_t get_offset_y() const;
    void move_to(size_t, size_t);
    border_type get_border() const;
    void set_border(border_type, fgColor = fg::reset, bgColor = bg::reset);
    void set_border_fg(fgColor);
    void set_border_bg(bgColor);
    void show();
    void hide();
    bool is_visible() const;
};


}  // namespace Term
#endif // wIndOW_HeaDERguARd

