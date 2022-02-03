#pragma once

#include "base.hpp"
#include "input.hpp"
#include <string>
#include <vector>

namespace Term {

enum { MAX_GRAPHEME_LENGTH = 4 };

enum class border_t {
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
	Color(const fg);
	Color(const bg);
	Color(const uint8_t, const uint8_t, const uint8_t);
public :
    bool is_rgb() const;
    uint8_t get_r() const;
    uint8_t get_g() const;
    uint8_t get_b() const;
    virtual std::string render() = 0;
    virtual bool is_reset() const = 0;
    virtual bool is_unspecified() const = 0;
};

// foreground color
class FgColor : public Color  {
    friend class Window;
public :
    FgColor();
    FgColor(fg c);
    FgColor(const uint8_t, const uint8_t, const uint8_t);
    fg get_fg() const;
    bool operator==(const FgColor&) const;
    bool operator!=(const FgColor&) const;
    std::string render() override;
    bool is_reset() const override;
    bool is_unspecified() const override;
};

// background color
class BgColor : public Color {
    friend class Window;
public :
    BgColor();
    BgColor(bg);
    BgColor(const uint8_t, const uint8_t, const uint8_t);
    bg get_bg() const;
    bool operator==(const BgColor&) const;
    bool operator!=(const BgColor&) const;
    std::string render () override;
    bool is_reset() const override;
    bool is_unspecified() const override;
};

/* Represents a cell in the terminal window, holding the character (i.e., the
 * Unicode grapheme cluster), the foreground and background colors and the 
 * style of this specific cell.
 */
struct Cell {
	FgColor cell_fg;
	BgColor cell_bg;
	style cell_style;
    uint8_t grapheme_length;
    char32_t ch[MAX_GRAPHEME_LENGTH];
    
    Cell();
    Cell(char32_t);
    Cell(const std::u32string&);
    Cell(char32_t, FgColor, BgColor, style);
    Cell(const std::u32string&, FgColor, BgColor, style);
};

struct Cursor {
    size_t x = 0;
    size_t y = 0;
    bool is_visible = false;
    Cursor() = default;
    Cursor(size_t x_, size_t y_, bool v_) : x(x_), y(y_), is_visible(v_) {}
};

class ChildWindow; // forward declaration

/* Represents a rectangular window, as a 2D array of characters and their 
 * attributes as defined in the "Cell" class. The draw_window() method of the
 * "Terminal" class converts this internal representation to a string which 
 * it prints to the console.
 */
class Window {
   protected :
    size_t w{}, h{};               // width and height of the window
    bool width_fixed{};            // if w may not grow automatically
    bool height_fixed{};           // same for h
    Cursor cursor;
    bool wordwrap{};
    size_t tabsize{};

    FgColor default_fg;
    BgColor default_bg;
    style default_style{};
    std::vector<std::vector<Cell>> grid; // the cells (grid[0] is top row)
    std::vector<ChildWindow*> children;
    Window* visual_cursor_holder{}; // default: this

    // word wrap control (these variables could be made publicly available
    // in a future version)
    // allow word wrap after whitespace and any of the following characters:
    std::u32string wrap_after = U"-.,;:/\\)]}>";
    // allow word wrap before whitespace and any of the following characters:
    std::u32string wrap_before = U"([{<";
    // if line is completely filled and whitespace follows, skip
    // one whitespace character:
    bool skip_whitespace_at_eol = true;

    // increases size of grid and/or grid[y] to include (x, y) unless forbidden 
    // by the fixation of width/height in which case an exception is thrown
    void assure_pos(size_t x, size_t y);

    // Writes the argument string starting at (cursor_x, cursor_y) into the
    // grid and moves the cursor to the position after the last printed
    // character. Returns the number of codepoints (char32_t) actually written.
    size_t simple_write(const std::u32string&,
                             FgColor = fg::unspecified,
                             BgColor = bg::unspecified,
                             style = style::unspecified);
    // Likewise, but returns the number of utf8 bytes actually written
    size_t simple_write(const std::string&,
                             FgColor = fg::unspecified,
                             BgColor = bg::unspecified,
                             style = style::unspecified);
    // Returns 1 if the character could be written, otherwise 0
    size_t simple_write(char32_t,
                             FgColor = fg::unspecified,
                             BgColor = bg::unspecified,
                             style = style::unspecified);

    size_t write_wordwrap(const std::u32string&,
                          FgColor = fg::unspecified,
                          BgColor = bg::unspecified,
                          style = style::unspecified);

    size_t write_wordwrap(const std::string&,
                          FgColor = fg::unspecified,
                          BgColor = bg::unspecified,
                          style = style::unspecified);

   public :
    Window(size_t width = 1, size_t height = 1);

    virtual ~Window();

    virtual bool is_base_window() {return true;}

    size_t get_w() const;
    // Decreasing w deletes all cells to the right of the new limit w.
    // If outside the new width, cursor.x will be set to w - 1
    virtual void set_w(size_t);
    // trim_w() deletes only cells with empty/unspecified content and will
    // not make w less or equal to cursor.x
    void trim_w(size_t minimum_width);

    size_t get_h() const;
    // Decreasing h deletes the bottom rows to fit the new height h.
    // If outside the new height, cursor.y will be set to h - 1
    virtual void set_h(size_t);
    // trim_h() deletes only cells with empty/unspecified content and will
    // not make h less or equal to cursor.y
    void trim_h(size_t minimum_height);

    virtual void resize(size_t, size_t);
    void trim(size_t, size_t);

    // true by default, but false if windows is initialized with w = 0
    bool is_width_fixed() const;
    void fix_width();
    void unfix_width();

    // true by default, but false if windows is initialized with h = 0
    bool is_height_fixed() const;
    void fix_height();
    void unfix_height();

    bool is_size_fixed() const; // true only if both width and height fixed
    void fix_size(); // fix both width and height
    void unfix_size(); // unfix them
 
    const Cursor& get_cursor() const;
    void set_cursor(size_t, size_t);
    void set_cursor(size_t, size_t, bool);
    void set_cursor(Cursor);

    void show_cursor(); // by default, cursor is visible
    void hide_cursor();

    size_t get_tabsize() const; // default: 4
    void set_tabsize(size_t);    

    uint8_t get_grapheme_length(size_t, size_t) const;
    std::u32string get_grapheme(size_t, size_t) const;
    void set_grapheme(size_t, size_t, const std::u32string&);
    void set_char(size_t, size_t, char32_t);

    FgColor get_fg(size_t, size_t) const;
    void set_fg(size_t, size_t, FgColor);
    void set_fg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    BgColor get_bg(size_t, size_t) const;
    void set_bg(size_t, size_t, BgColor);
    void set_bg(size_t, size_t, uint8_t, uint8_t, uint8_t);

    style get_style(size_t, size_t) const;
    void set_style(size_t, size_t, style);

    Cell get_cell(size_t, size_t) const;
    void set_cell(size_t, size_t, const Cell&);

    std::vector<std::vector<Cell>> get_grid() const;
    void set_grid(const std::vector<std::vector<Cell>> &);
    void copy_grid_from(const Window&);

    FgColor get_default_fg() const; // default: fg::reset
    void set_default_fg(FgColor);
    void set_default_fg(uint8_t, uint8_t, uint8_t);

    BgColor get_default_bg() const; // default: bg::reset
    void set_default_bg(BgColor);
    void set_default_bg(uint8_t, uint8_t, uint8_t);

    style get_default_style() const; // default: style::reset
    void set_default_style(style);

    bool is_wordwrap() const; // default: false
    void set_wordwrap(bool = true);
    
    // writes a string from the cursor position on into the grid and moves
    // the cursor to the next free position. Returns the number of codepoints
    // (char32_t) actually written. Applies word wrap if and only if
    // width_fixed == true && and wordwrap == true, albeit not touching
    // nor reflecting any text already present in the grid.
    size_t write(const std::u32string&,
                 FgColor = fg::unspecified,
                 BgColor = bg::unspecified,
                 style = style::unspecified);

    // likewise, but returns the number of bytes (utf8) written
    size_t write(const std::string&,
                 FgColor = fg::unspecified,
                 BgColor = bg::unspecified,
                 style = style::unspecified);

    // when passing a single codepoint to write(), word wrap is never
    // applied. Returns 1 if success, otherwise 0.
    size_t write(char32_t ch,
                 FgColor = fg::unspecified,
                 BgColor = bg::unspecified,
                 style = style::unspecified);

    void fill_fg(size_t, size_t, size_t, size_t, FgColor);
    void fill_bg(size_t, size_t, size_t, size_t, BgColor);
    void fill_style(size_t, size_t, size_t, size_t, style);

    void print_rect(int, int, size_t, size_t,
                    border_t = border_t::LINE,
                    FgColor = fg::unspecified,
                    BgColor = bg::unspecified);

    void clear_row(size_t);
    void clear_grid();
    void clear();

    Window cutout(size_t x0, size_t y0, size_t width, size_t height) const;

    ChildWindow* new_child(size_t = 0, size_t = 0, size_t = 0, size_t = 0,
                           border_t b = border_t::LINE);

    ChildWindow* get_child(size_t);
    size_t get_child_index(ChildWindow*) const;
    size_t get_children_count() const;

    void child_to_foreground(ChildWindow*);
    void child_to_background(ChildWindow*);

    bool is_descendant(ChildWindow*) const;

    Window* get_visual_cursor_holder() const; // default: this
    void hand_over_visual_cursor(ChildWindow*);
    void take_over_visual_cursor();
    Cursor get_visual_cursor() const;

    Window merge_children() const;
};

// Represents a sub-window. Child windows may be nested.
class ChildWindow : public Window {
    friend Window;

   private:
    Window *parent;
    size_t offset_x{}, offset_y{};
    std::u32string title;
    border_t border;
    FgColor border_fg;
    BgColor border_bg;
    bool visible{};

    ChildWindow(Window* ptr, size_t off_x, size_t off_y,
                size_t w_, size_t h_, border_t b = border_t::LINE);
    ChildWindow(const ChildWindow&) = default;
    ChildWindow(ChildWindow&&) = default;
    void merge_into_grid(Window*, size_t, size_t, size_t, size_t) const;

   public :
    bool is_base_window() override {return false;}
    Window* get_parent() const;
    // is_inside_parent() returns true if the child, including its border,  is 
    // entirely inside the parent window. When strict == false, the 
    // borders of parent and child are allowed to (partially) coincide.
    bool is_inside_parent(bool strict = true) const;
    size_t get_offset_x() const;
    size_t get_offset_y() const;
    std::pair<size_t, size_t> move_to(size_t, size_t);
    std::u32string get_title() const;
    void set_title(const std::string&);
    void set_title(const std::u32string&);
    border_t get_border() const;
    void set_border(border_t, 
                    FgColor = fg::unspecified, 
                    BgColor = bg::unspecified);
    FgColor get_border_fg() const;
    void set_border_fg(FgColor);
    BgColor get_border_bg() const;
    void set_border_bg(BgColor);
    bool is_visible() const;
    void show();
    void hide();
    void to_foreground();
    void to_background();
};

}  // namespace Term
