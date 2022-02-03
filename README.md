**Warning**: At the moment, cpp-terminal-utf is under development and <u>should not be used</u> before it reaches version 1.0.0-beta. From that point on, the project will follow [semantic versioning](https://semver.org/).

-----

# Terminal-UTF

### Introduction

This is a fork of https://github.com/jupyter-xeus/cpp-terminal. While my first motivation was to make cpp-terminal Unicode capable, the project underwent several major modifications which made it rather incompatible to the base project. The main differences are (or are planned to be):

- All coordinate arguments are now specified in order `(column, row)`, in all methods and functions.
- All coordinates count from zero (where row 0 is the top row and col 0 the left-most column). The translation into and from ANSI sequences which actually count from 1 is done under the hood.
- All arguments for rectangular areas have the format `(x0, y0, width, height)`, where (x0, y0) is the top left corner.
- **Unicode support** (with the limitations described below). For this, `read_char()` and `read_char0()` now return `char32_t` values. 
- For conversion from utf8 to utf32 and vice versa, the identification of grapheme clusters and for normalization, cpp-terminal-utf relies on the header-only [cpp-unicodelib](https://github.com/yhirose/cpp-unicodelib) library. 
- The special keys have been assigned new internal values above the Unicode range. The modifier keys CTRL, ALT and SHIFT are now bit flags. For example, CTRL-F1 is internally represented as  
  `Key::CTRL | Key::F1`
- Support for more ANSI sequences (especially for combinations with SHIFT, ALT and CTRL).
- Redesigned Window class, allowing sub-windows (menus are planned, too).
- Improved handling of CTRL-C events.

-----

### Unicode support limitations

#### Windows and Linux:

- The `Window` class holds a `char32_t` array of fixed size for the Unicode grapheme cluster (i.e., the displayed character) for each cell. Trying to fill a cell with a grapheme cluster longer than `MAX_GRAPHEME_LENGTH` throws an exception. You may of course adjust this value in `window.hpp` to your needs.
- As for the grapheme clusters, I have not yet considered multi-width letters nor the `ZERO WIDTH JOINER` (`U+200D`). In fact, anything other than combining character sequences might lead to unpredictable behavior, depending on the built-in abilities of the console.

#### Windows only:

- AFAIK, only Unicode characters in BMP (i.e., below `U'\x10000'`) are supported in a Windows console.
- Windows consoles do not yet correctly display combining characters. The `Window::set_grapheme()` method performs a NFC normalization on the passed string as a workaround. Which means that only if there is a matching composed character, a combining sequence will display correctly. I hope that this restriction will eventually be fixed in the new [Windows Terminal](https://github.com/Microsoft/Terminal).

-----

### API Documentation

Private/protected class members and members of the namespace Term::Private are not given here and may change without notice.

#### class Terminal

```
namespace Term{
enum {
    CLEAR_SCREEN = 1,
    RAW_INPUT = 2,
    DISABLE_CTRL_C = 4
};

Terminal(unsigned options = CLEAR_SCREEN);

bool update_size();
size_t get_w(); // get width (in columns)
size_t get_h(); // get height (in rows)
void draw_window (const Window &win, 
				  size_t x0 = 0, 
				  size_t y0 = 0, 
				  size_t width = string::npos, 
				  size_t height = string::npos);
				  
} // namespace Term
```

Terminal allows for only one instance. Options can be combined using bitwise OR, e.g. `Terminal term(CLEAR_SCREEN | RAW_INPUT)`.

It is strongly advised to create an instance of Terminal before using any of the methods provided by cpp-terminal-utf. It is also recommended to put this and any operations on the console in a try-catch block to ensure that the Terminal destructor gets called and the console continues to work properly if something in the program goes wrong.

`CLEAR_SCREEN`: Saves the state of the console and clears the screen. The destructor of Terminal will restore the original state and content of the console.

`RAW_INPUT`: disables echoing and line input and gives you the ability to capture special keys like F1-F12, arrow keys and so on. Use `read_char()` and `read_char0()` to process the user input (see below).

`DISABLE_CTRL_C`: As the name implies, CTRL-C will be processed as a normal key stroke rather than send a SIGINT signal to the application.

`update_size()` returns true if the dimensions of the console have changed since the last call.

`get_w()` and `get_h()` return the saved values of the last-performed `update_size()` call. They do not call `update_size()` themselves. (Note: the `Terminal` constructor and `draw_window()` do call `update_size()`. Apart from that, it is up to the programmer to check or not check the actual size of the console.)

`draw_window()` renders the content of a Window object into the appropriate ANSI sequences and prints the result to the console. You may specify a cut-out by the arguments (x0, y0, width, height) which for example allows for a simple scrolling mechanism. Parts of the window respectively of the cut-out which exceed the actual console size will be ignored.

#### Basic enumerations and functions (taken over from cpp-terminal)

```
namespace Term{
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
    unspecified = 253 // only for internal purpose, don't use
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
    unspecified = 254 // only for internal purpose, don't use
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
    unspecified = 255 // only for internal purpose, don't use
};

std::string color(style);
std::string color(fg);
std::string color(bg);
std::string color24_fg(unsigned int, unsigned int, unsigned int);
std::string color24_bg(unsigned int, unsigned int, unsigned int);

void write(const std::string& s) {std::cout << s << std::flush;}

std::string cursor_off();
std::string cursor_on();
std::string clear_screen();

// clears screen + scroll back buffer
std::string clear_screen_buffer();

// If an attempt is made to move the cursor out of the window, the result is
// undefined. In contrast to cpp-terminal, counts from zero. 
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

// In contrast to cpp-terminal, counts from zero:
void get_cursor_position(size_t&, size_t&);

} // namespace Term
```

For simple tasks like just colorizing certain words, you don't need the Window class. Just make use of the above functions, e. g. `write("normal text " + color(fg::red) + "red text " + color(fg::reset) + "normal again");`

All these work exactly as in cpp-terminal, except `move_cursor()` and `get_cursor_position()` which expect and return values that count from zero which IMHO suits better a programmer's point of view.

#### Raw input

```
namespace Term {
enum Key : char32_t {
    // Note that for A-Z in combination with CTRL,
    // the below values are used instead of CTRL | 'a' etc.
    // E.g., the key combination Alt-Control-P yields
    // ALT | CTRL_P and not CTRL | ALT | 'P'
    CTRL_A = 1,
    CTRL_B,
    CTRL_C,
    CTRL_D,
    CTRL_E,
    CTRL_F,
    CTRL_G,
    CTRL_H = 8,
    CTRL_I = 9,
    CTRL_J = 0xa,
    CTRL_K,
    CTRL_L,
    CTRL_M = 0xd,
    CTRL_N,
    CTRL_O,
    CTRL_P,
    CTRL_Q,
    CTRL_R,
    CTRL_S,
    CTRL_T,
    CTRL_U,
    CTRL_V,
    CTRL_W,
    CTRL_X,
    CTRL_Y,
    CTRL_Z, // = 0x1a
    CTRL_4 = 0x1c,
    CTRL_5,
    CTRL_6,
    CTRL_7,

    BACKSPACE = 8, // same as CTRL_H
    TAB = 9,       // same as CTRL_I
    LF = 0xa,      // same as CTRL_J
    CR = 0xd,      // same as CTRL_M
    ENTER = 0xd,   // same as CTRL_M and CR, for compatibility
    ESC = 0x1b,
    DEL = 0x7f,
    
    // non-printables
    ARROW_UP = 0x1000000u,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    INSERT,
    NUMERIC_5, // "5" on the numeric block with "Num" disabled. Linux-only.
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // modifiers (flags)
    SHIFT =   0x2000000u,
    ALT =     0x4000000u,
    CTRL =    0x8000000u, // for CTRL-A - CTRL-Z see above

    UNKNOWN = 0xfffffffu
};

// Waits for a key press, translates escape codes, decodes utf8
char32_t read_key();

// If there was a key press, returns the translated key from escape codes
// resp. the decoded UTF codepoint, otherwise returns 0.
// If the escape code is not supported, returns Key::UNKNOWN
char32_t read_key0();

} // namespace Term
```

#### Windows and sub-windows

```
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
class Cell {
   public:
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
    Cursor(size_t, size_t, bool);
};

/* Represents a rectangular window, as a 2D array of characters and their 
 * attributes as defined in the "Cell" class. The draw_window() method of the
 * "Terminal" class converts this internal representation to a string which 
 * it prints to the console.
 */
class Window {
   public :
    Window(size_t width = 0, size_t height = 0);
    virtual ~Window();

    virtual bool is_base_window(); //returns true

    size_t get_w() const;
    virtual void set_w(size_t);
    void trim_w(size_t);

    size_t get_h() const;
    virtual void set_h(size_t);
    void trim_h(size_t);

    virtual void resize(size_t, size_t);
    void trim(size_t, size_t);

    bool is_width_fixed() const; // default: true
    void set_width_fixation (bool);

    bool is_height_fixed() const; // default: true
    void set_height_fixation (bool);

    bool is_size_fixed() const; // both width and height fixed?
    void fix_size(); // fix width and height
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
    // (char32_t) actually written. Applies a simple word wrap algorithm if
    // and only if is_width_fixed() and is_wordwrap() both yield true, albeit 
    // not touching nor reflecting any text already present in the grid.
    size_t write(const std::u32string&,
                 FgColor = fg::unspecified,
                 BgColor = bg::unspecified,
                 style = style::unspecified);

    // likewise, but returning the number of bytes (utf8) written
    size_t write(const std::string&,
                 FgColor = fg::unspecified,
                 BgColor = bg::unspecified,
                 style = style::unspecified);

    // when passing a single codepoint to write(), word wrap is never
    // applied. Returns 1 if successful.
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
    ChildWindow* new_child(size_t o_x, size_t o_y, size_t w_, size_t h_,
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
   public :
    bool is_base_window() override; // returns false
    Window* get_parent() const;
    bool is_inside_parent() const;
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
```

A child window object is always associated with exactly one parent window object. There is no public constructor. A new instance can only be generated by the parent window's `new_child()` method.

The parent window stores pointers to its children in a vector. A child's index in this vector determines if it obscures another child window, index 0 indicating the background (which still obscures the parent window, of course). The indexes may be changed by methods like `to_foreground()` or `to_background()`, which is why the only reliable way of referencing a child window is a pointer to it.

The destructor of a Window object also destroys any associated child.

