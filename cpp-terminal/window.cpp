#include "window.hpp"
#include <stdexcept>
#include "private/conversion.hpp"
#include "private/platform.hpp"

using namespace std;

bool Term::operator==(const Term::fgColor &c1, const Term::fgColor &c2) {
    if (c1.is_rgb() != c2.is_rgb()) return false;
    if (c1.is_rgb()) return (c1.get_r() == c2.get_r() && c1.get_g() == c2.get_g() && c1.get_b() == c2.get_b());
    return (c1.get_fg() == c2.get_fg());
}

bool Term::operator==(const Term::bgColor &c1, const Term::bgColor &c2) {
    if (c1.is_rgb() != c2.is_rgb()) return false;
    if (c1.is_rgb()) return (c1.get_r() == c2.get_r() && c1.get_g() == c2.get_g() && c1.get_b() == c2.get_b());
    return (c1.get_bg() == c2.get_bg());
}


void Term::Window::assure_pos(size_t x, size_t y){
    if (y >= h) {
        if (auto_growing) {
            h = y + 1;
            grid.resize(h);
        }
        else {
            throw std::runtime_error("y out of bounds");
        }
    }
    if (x >= w) {
        if (auto_growing) {
            w = x + 1;
        }
        else {
            throw std::runtime_error("x out of bounds");
        }
    }
    if (x >= grid[y].size()) {
        grid[y].resize(w, Cell());
    }
}

Term::Window Term::Window::merge_children() const {
    Window res(w, h);
    res.grid = grid;
    res.cursor_x = cursor_x;
    res.cursor_y = cursor_y;
    for (const ChildWindow &child : children) {
        if (!child.is_visible()) continue;
        // in case there are nested children:
        // Window mc = child.merge_children(); // TODO: activate that later
        // merge grid:
        for (size_t y = 0; y < child.h; ++y) {
            size_t pos_y = y + child.offset_y;
            if (pos_y >= h) break;
            for (size_t x = 0; y < child.w; ++x) {
                size_t pos_x = x + child.offset_x;
                if (pos_x >= w) break;
                res.set_cell(pos_x, pos_y, child.get_cell(x, y)); //later: mc.get_cell
            }<
        }
        // draw border:
        res.print_rect(
            child.offset_x ? child.offset_x - 1 : 0,
            child.offset_y ? child.offset_y - 1 : 0,
            child.w + 2,
            child.h + 2,
            child.border
        );
    }
    return res;
}

char32_t Term::Window::get_char(size_t x, size_t y) const {
    if (y < h && x < grid[y].size())
        return grid[y][x].ch;
    else return U' ';
}

Term::fgColor Term::Window::get_fg(size_t x, size_t y) const {
    if (y < h && x < grid[y].size())
        return grid[y][x].cell_fg;
    else return fg::reset;
}

Term::bgColor Term::Window::get_bg(size_t x, size_t y) const {
   if (y < h && x < grid[y].size())
        return grid[y][x].cell_bg;
    else return bg::reset;
}

Term::style Term::Window::get_style(size_t x, size_t y) const {
    if (y < h && x < grid[y].size())
        return grid[y][x].cell_style;
    else return style::reset;
}

Term::Cell Term::Window::get_cell(size_t x, size_t y) const {
    if (y < h && x < grid[y].size())
        return grid[y][x];
    else return Cell();
}

void Term::Window::set_cell(size_t x, size_t y, const Term::Cell &c) {
    assure_pos(x, y);
    grid[y][x] = c;
}

size_t Term::Window::get_w() const {
    return w;
}

size_t Term::Window::get_h() const {
    return h;
}

bool Term::Window::is_auto_growing() const {
    return auto_growing;
}

void Term::Window::set_auto_growing(bool grow) {
    auto_growing = grow;
}

bool Term::Window::is_auto_newline() const {
    return auto_newline;
}

void Term::Window::set_auto_newline(bool a) {
    auto_newline = a;
}


size_t Term::Window::get_cursor_x() const {
    return cursor_x;
}

size_t Term::Window::get_cursor_y() const {
    return cursor_y;
}

bool Term::Window::get_cursor_visibility() const {
    return cursor_visible;
}

void Term::Window::show_cursor() {
    cursor_visible = true;
}

void Term::Window::hide_cursor() {
    cursor_visible = false;
}



size_t Term::Window::get_indent() {
    return indent;
}

void Term::Window::set_char(size_t x, size_t y, char32_t c) {
    assure_pos(x, y);
    grid[y][x].ch = c;
}

void Term::Window::choose_fg(fg c) {
    active_fg = fgColor(c);
}

void Term::Window::choose_fg(uint8_t r, uint8_t g, uint8_t b) {
    active_fg = fgColor(r, g, b);
}

void Term::Window::set_fg(size_t x, size_t y, fgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_fg = c;
}

void Term::Window::set_fg(size_t x, size_t y, fg c) {
    assure_pos(x, y);
    grid[y][x].cell_fg = fgColor(c);
}

void Term::Window::set_fg(size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_fg = fgColor(r, g, b);
}

void Term::Window::choose_bg(bg c) {
    active_bg = bgColor(c);
}

void Term::Window::choose_bg(uint8_t r, uint8_t g, uint8_t b) {
    active_bg = bgColor(r, g, b);
}

void Term::Window::set_bg(size_t x, size_t y, bgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_bg = c;
}

void Term::Window::set_bg(size_t x, size_t y, bg c) {
    assure_pos(x, y);
    grid[y][x].cell_bg = bgColor(c);
}

void Term::Window::set_bg(size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_bg = bgColor(r, g, b);
}

void Term::Window::choose_style(style c) {
    active_style = c;
}

void Term::Window::set_style(size_t x, size_t y, style c) {
    assure_pos(x, y);
    grid[y][x].cell_style = c;
}

void Term::Window::set_cursor(size_t x, size_t y) {
    assure_pos(x, y);
    cursor_x = x;
    cursor_y = y;
}

void Term::Window::set_h(size_t new_h) {
    grid.resize(new_h);
    h = new_h;
}

void Term::Window::set_w(size_t new_w) {
    if (new_w == w) return;
    w = new_w;
    for (size_t y = 0; y != grid.size(); ++y) {
        if (grid[y].size() > w) grid[y].resize(w, Cell());
    }
}

void Term::Window::resize(size_t new_w, size_t new_h) {
    set_w(new_w);
    set_h(new_h);
}

void Term::Window::set_indent(size_t i) {
    if (indent < w || auto_growing) indent = i;
    else indent = w - 1;
}

void Term::Window::print_str(const std::u32string& s)
{
    size_t x = cursor_x;
    size_t y = cursor_y;
    for (char32_t i : s) {
        bool newline = (i == U'\n' || (x >= w && auto_newline));
        if (newline) {
            x = indent;
            ++y;
            if ((x < w && y < h) || auto_growing) {
                for (size_t j = 0; j < indent; j++) {
                    set_char(x + j, y, U' ');
                    set_fg(x + j, y, active_fg);
                    set_bg(x + j, y, active_bg);
                    set_style(x + j, y, active_style);
                }
            } else {
                // String is out of the window
                return;
            }
        }
        if (i != U'\n') {
            if ((x < w && y < h) || auto_growing) {
                set_char(x, y, i);
                set_fg(x, y, active_fg);
                set_bg(x, y, active_bg);
                set_style(x, y, active_style);
            } else {
                // String is out of the window, but may
                // come back into it after next '\n'
                continue;
            }
            ++x;
        }
    }
    // set cursor
    cursor_x = x;
    cursor_y = y;
}

void Term::Window::print_str(const std::string& s)
{
    std::u32string s32 = Private::utf8_to_utf32(s);
    print_str(s32);
}


void Term::Window::fill_fg(int x1, int y1, int x2, int y2, fgColor color) {
    for (int j = y1; j < y2; j++) {
        for (int i = x1; i < x2; i++) {
            set_fg(i, j, color);
        }
    }
}

void Term::Window::fill_bg(int x1, int y1, int x2, int y2, bgColor color) {
    for (int j = y1; j < y2; j++) {
        for (int i = x1; i < x2; i++) {
            set_bg(i, j, color);
        }
    }
}

void Term::Window::fill_style(int x1, int y1, int x2, int y2, style st) {
    for (int j = y1; j < y2; j++) {
        for (int i = x1; i < x2; i++) {
            set_style(i, j, st);
        }
    }
}

void Term::Window::print_rect(
    size_t x1,
    size_t y1,
    size_t width,
    size_t height,
    border_type b,
    fgColor color
)
// A rectangle will not auto-grow the window, it just disappears
// outside its borders
{
    if (x1 >= w || y1 >= h) return;
    std::u32string border;
    switch (b) {
    case border_type::no_border : return;
    case border_type::blank : border = U"      "; break;
    case border_type::ascii : border = U"|-++++"; break;
    case border_type::utf1 : border = U"│─┌┐└┘"; break;
    case border_type::utf2 : border = U"║═╔╗╚╝"; break;
    default: throw runtime_error ("undefined border value");
    }
    size_t x2 = x1 + width, y2 = y1 + height;
    for (size_t y = y1 + 1; y < y2 && y < h; ++y) {
        set_char(x1, y, border[0]);
        set_fg(x1, y, color);
        if (x2 < w) {
            set_char(x2, y, border[0]);
            set_fg(x2, y, color);
        }
    }
    for (size_t x = x1 + 1; x < x2 && x < w; ++x) {
        set_char(x, y1, border[1]);
        set_fg(x, y1, color);
        if (y2 < h) {
            set_char(x, y2, border[1]);
            set_fg(x, y2, color);
        }
    }
    set_char(x1, y1, border[2]);
    set_fg(x1, y1, color);
    if (x2 < w) {
        set_char(x2, y1, border[3]);
        set_fg(x2, y1, color);
    }
    if (y2 < h) {
        set_char(x1, y2, border[4]);
        set_fg(x1, y2, color);
        if (x2 < w) {
            set_char(x2, y2, border[5]);
            set_fg(x2, y2, color);
        }
    }
}

void Term::Window::clear() {
    grid.clear();
    grid.resize(h);
    cursor_x = 0;
    cursor_y = 0;
}

void Term::Window::clear_row(size_t y) {
    if (y < grid.size()) {
        grid[y].clear();
    }
}

std::string Term::Window::render() {
    if (children.size()) return merge_children().render();
    std::string out;
    if (is_main_window) out = cursor_off() + clear_screen();
    fgColor current_fg(fg::reset);
    bgColor current_bg(bg::reset);
    style current_style = style::reset;
    for (size_t j = 0; j < h; j++) {
        if (j)
            out.append("\n");
        for (size_t i = 0; i < w; i++) {
            bool update_fg = false;
            bool update_bg = false;
            bool update_style = false;
            if (current_fg != get_fg(i, j)) {
                current_fg = get_fg(i, j);
                update_fg = true;
            }
            if (current_bg != get_bg(i, j)) {
                current_bg = get_bg(i, j);
                update_bg = true;
            }
            if (current_style != get_style(i, j)) {
                current_style = get_style(i, j);
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
                out.append(color(get_style(i, j)));
            if (update_fg)
                out.append(get_fg(i, j).render());
            if (update_bg)
                out.append(get_bg(i, j).render());
            Private::codepoint_to_utf8(out, get_char(i, j));
        }
    }
    // reset colors and style at the end
    if (!current_fg.is_reset())
        out.append(color(fg::reset));
    if (!current_bg.is_reset())
        out.append(color(bg::reset));
    if (current_style != style::reset)
        out.append(color(style::reset));
    if (is_main_window) {
        out.append(move_cursor(cursor_y + 1, cursor_x + 1));
        if (cursor_visible) out.append(cursor_on());
    }
    return out;
};

Term::Window Term::Window::cutout(size_t x0, size_t y0, size_t width, size_t height) const {
    Window cropped(width, height);
    for (size_t y = y0; y != y0 + height && y < h; ++y) {
        for (size_t x = x0; x != x0 + width && x < grid[y].size(); ++x) {
            cropped.grid[y].push_back(grid[y][x]);
        }
    }
    // preserve cursor if within cut-out
    if (cursor_x < x0 || cursor_x >= x0 + width || cursor_y < y0 || cursor_y >= y0 + height) {
        cropped.set_cursor(0, 0);
    }
    else {
        cropped.set_cursor(cursor_x - x0, cursor_y - y0);
    }
    return cropped;
}

size_t Term::Window::new_child(size_t o_x, size_t o_y, size_t w_, size_t h_, border_type b) {
    size_t n = children.size();
    ChildWindow cwin(o_x, o_y, w_, h_, b);
    children.push_back(cwin);
    return n;
}

Term::ChildWindow * Term::Window::get_child(size_t i) {
    if (i < children.size()) return &children[i];
    throw runtime_error("Term::Window::get_child(): bad child index");
}

size_t Term::Window::get_children_count() const {
    return children.size();
}

void Term::Window::get_cursor_from_child(size_t i) {
    if (i >= children.size() || !children[i].is_visible()) return;
    size_t x = cursor_x, y = cursor_y;
    x += children[i].get_offset_x() + children[i].get_cursor_x();
    y += children[i].get_offset_y() + children[i].get_offset_y();
    if (x >= w || y >= h) return;
    cursor_x = x;
    cursor_y = y;
}

//TODO:

void Term::Window::get_cursor_from_child(std::vector<size_t>) {

}

/*********************
 * Term::ChildWindow
 *********************
 */

size_t Term::ChildWindow::get_offset_x() const {
    return offset_x;
}

size_t Term::ChildWindow::get_offset_y() const {
    return offset_y;
}

void Term::ChildWindow::move_to(size_t x, size_t y) {
    offset_x = x;
    offset_y = y;
}

Term::border_type Term::ChildWindow::get_border() const {
    return border;
}

void Term::ChildWindow::set_border(Term::border_type b) {
    border = b;
}

void Term::ChildWindow::show() {
    visible = true;
}

void Term::ChildWindow::hide() {
    visible = false;
}

bool Term::ChildWindow::is_visible() const {
    return visible;
}


Term::TerminalWindow::TerminalWindow(bool disable_ctrl_c)
:   Terminal(true, true, disable_ctrl_c),
    win(80,30) {
    int h, w;
    Private::get_term_size(h, w);
    win.resize(w, h);
}
