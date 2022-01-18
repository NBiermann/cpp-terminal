#include "../cpp-terminal-utf/cpp-terminal/base.hpp"
#include "../cpp-terminal-utf/cpp-terminal/window.hpp"
#include "../cpp-terminal-utf/cpp-terminal/input.hpp"

#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;
using namespace Term;

int main()
{   
    try {
        Terminal term(CLEAR_SCREEN | RAW_INPUT | DISABLE_CTRL_C);
        Window win (term.get_w(), term.get_h());
        u32string info =
            U"This demo illustrates several features of the \"Window\" "
            "class. Type some text into the green window, press F2 or "
            "F3 to place the cursor in the red or blue window, and F1 to go "
            "back to the green one. (Technically, these are subwindows and "
            "this text is in the main window.) Use backspace to modify your "
            "input. The arrow keys move the active window around. "
            "Ctrl-F1/F2/F3 hides resp. shows again the green/red/blue window. "
            "The green window has a fixed size, while the red one will "
            "expand its width according to the typed-in text, "
            "and the blue one is able to grow in height. The blue "
            "and the main window have word-wrap activated. If your active "
            "window is obscured by another one, press Shift-F1/F2/F3 "
            "to bring it to the foreground. There is also "
            "a sub-subwindow located inside the green one. Press Ctrl-F4 to "
            "make it visible. You may also try "
            "and change the size of the console window with your mouse. --- ";
        size_t written{};
        do {
            written = win.write_wordwrap(info);
        } while(written == info.size());
        vector<size_t>
            start_x{5, 22, 37, 4},
            start_y{7, 14, 8, 3},
            default_w{14, 8, 10, 5},
            default_h{8, 2, 4, 3};
        vector<bool>
            fixed_w{true, false, true, true},
            fixed_h{true, true, false, true};
        vector<fg>
            fg_col{fg::green, fg::red, fg::bright_blue, fg::yellow};
        vector<bg>
            bg_col{bg::reset, bg::reset, bg::reset, bg::reset};
        vector<fg>
            border_col{fg::green, fg::red, fg::bright_blue, fg::yellow};
        vector<string>
            title{ "1", "2", "3", "4" };
        ChildWindow* cwin;
        vector<ChildWindow*> child;
        for (size_t i = 0; i != 4; ++i) {
            if (i < 3) {
                cwin = win.new_child(start_x[i], start_y[i], default_w[i],
                    default_h[i], border_t::LINE);
            }
            else {
                // Create Window #4 as sub-window of #1 
                cwin = child[0]->new_child(start_x[3], start_y[3], 
                        default_w[3], default_h[3], border_t::LINE);
            }           
            cwin->set_width_fixation(fixed_w[i]);
            cwin->set_height_fixation(fixed_h[i]);
            cwin->set_border_fg(border_col[i]);
            cwin->set_default_fg(fg_col[i]);
            cwin->set_default_bg(bg_col[i]);
            cwin->set_title(title[i]);
            cwin->show_cursor();
            if (i < 3) cwin->show(); // initially, hide #4
            child.push_back(cwin);
        }
        size_t active_child = 0;
        vector<u32string> input(4);
        bool update = true;
        bool accept_input = true;
        bool abort = false;
        while (!abort) {
            cwin = child[active_child];
            cwin->set_border(border_t::DOUBLE_LINE);
            if (term.update_size()) {
                win.clear_grid();
                win.resize(term.get_w(), term.get_h());
                do {
                    written = win.write_wordwrap(info);
                } while(written == info.size());
                update = true;
            }
            if (update) {
                win.take_cursor_from_child(cwin);
                term.draw_window(win);
                accept_input = win.is_cursor_visible();
                update = false;
            }
            char32_t ch = read_key0();
            if (!ch) continue;
            switch (ch) {
            case Key::ARROW_UP:
                if (cwin->get_offset_y()) {
                    cwin->move_to(cwin->get_offset_x(), cwin->get_offset_y() - 1);
                }
                update = true;
                continue;
            case Key::ARROW_DOWN:
                if (cwin->get_h() + cwin->get_offset_y() < win.get_h()) {
                    cwin->move_to(cwin->get_offset_x(), cwin->get_offset_y() + 1);
                }
                update = true;
                continue;
            case Key::ARROW_LEFT:
                if (cwin->get_offset_x() > 
                    (cwin->get_border() == border_t::NO_BORDER ? 0u : 1u)) {
                    cwin->move_to(cwin->get_offset_x() - 1, cwin->get_offset_y());
                }
                update = true;
                continue;
            case Key::ARROW_RIGHT:
                if (cwin->get_w() + cwin->get_offset_x() < win.get_w()) {
                    cwin->move_to(cwin->get_offset_x() + 1, cwin->get_offset_y());
                }
                update = true;
                continue;
            case Key::CTRL | Key::ARROW_UP:
            case Key::PAGE_UP: {
                int b = (cwin->get_border() == border_t::NO_BORDER ? 0 : 1);
                int new_y = (int)cwin->get_offset_y() - cwin->get_h() - 1;
                if (new_y - b < 0) new_y = b;
                cwin->move_to(cwin->get_offset_x(), new_y);
                update = true;
                continue;
            }   
            case Key::CTRL | Key::ARROW_DOWN:
            case Key::PAGE_DOWN: {
                int b = (cwin->get_border() == border_t::NO_BORDER ? 0 : 1);
                int new_y = cwin->get_offset_y() + cwin->get_h() + 1;
                if (new_y + cwin->get_h() + b >= win.get_h()) {
                    new_y = (int)win.get_h() - b - cwin->get_h();
                    if (new_y < b) new_y = b;
                }
                cwin->move_to(cwin->get_offset_x(), new_y);
                update = true;
                continue;
            }
            case Key::CTRL | Key::ARROW_LEFT: {
                int b = (cwin->get_border() == border_t::NO_BORDER ? 0 : 1);
                int new_x = (int)cwin->get_offset_x() - cwin->get_w() - 1;
                if (new_x - b < 0) new_x = b;
                cwin->move_to(new_x, cwin->get_offset_y());
                update = true;
                continue;
            }
            case Key::CTRL | Key::ARROW_RIGHT: {
                int b = (cwin->get_border() == border_t::NO_BORDER ? 0 : 1);
                int new_x = cwin->get_offset_x() + cwin->get_w() + 1;
                if (new_x + cwin->get_w() + b >= win.get_w()) {
                    new_x = (int)win.get_w() - b - cwin->get_w();
                    if (new_x < b) new_x = b;
                }
                cwin->move_to(new_x, cwin->get_offset_y());
                update = true;
                continue;
            }
            case Key::F1:
                cwin->set_border(border_t::LINE);
                active_child = 0;
                update = true;
                continue;
            case Key::F2:
                cwin->set_border(border_t::LINE);
                active_child = 1;
                update = true;
                continue;
            case Key::F3:
                cwin->set_border(border_t::LINE);
                active_child = 2;
                update = true;
                continue;
            case Key::F4:
                cwin->set_border(border_t::LINE);
                active_child = 3;
                update = true;
                continue;
            case Key::CTRL | Key::F1:
                cwin = child[0];
                if (cwin->is_visible()) cwin->hide();
                else cwin->show();
                update = true;
                continue;
            case Key::CTRL | Key::F2:
                cwin = child[1];
                if (cwin->is_visible()) cwin->hide();
                else cwin->show();
                update = true;
                continue;
            case Key::CTRL | Key::F3:
                cwin = child[2];
                if (cwin->is_visible()) cwin->hide();
                else cwin->show();
                update = true;
                continue;
            case Key::CTRL | Key::F4:
                cwin = child[3];
                if (cwin->is_visible()) cwin->hide();
                else cwin->show();
                update = true;
                continue;
            case Key::SHIFT | Key::F1:
            case Key::SHIFT | Key::F4:
                child[0]->to_foreground();
                update = true;
                continue;
            case Key::SHIFT | Key::F2:
                child[1]->to_foreground();
                update = true;
                continue;
            case Key::SHIFT | Key::F3:
                child[2]->to_foreground();
                update = true;
                continue;
            case Key::ENTER:
                if (!accept_input) continue;
                input[active_child].push_back(ch);
                break;
            case Key::BACKSPACE:
                if (!accept_input) continue;
                if (input[active_child].size()) input[active_child].pop_back();
                break;
            case Key::CTRL_C:
                abort = true;
                continue;
            default:
                if (!accept_input) continue;
                if (ch < U' ' || ch > UTF8_MAX) continue;
                input[active_child].push_back(ch);
            } // switch
            bool repeat = false;
            do {
                cwin->clear_grid();
                size_t characters_printed = (active_child == 2 ?
                    cwin->write_wordwrap(input[active_child]) :
                    cwin->write(input[active_child]));
                input[active_child].resize(characters_printed);
                cwin->trim(default_w[active_child], default_h[active_child]);
                repeat = !cwin->is_inside_parent();
                if (repeat) input[active_child].pop_back();
            } while (repeat);
            update = true;
        }
    } 
    catch (runtime_error& e) {
        cout << e.what() << endl;
        return 1;
    } 
    catch (...) {
        cout << "unknown error." << endl;
        return 2;
    }
    cout << "ctrl-c pressed" << endl;
    return 0;
}
