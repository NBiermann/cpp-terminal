#include "../cpp-terminal/base.hpp"
#include "../cpp-terminal/window.hpp"
#include "../cpp-terminal/input.hpp"

#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace std;
using namespace Term;

int main()
{
    try {
        Terminal term(CLEAR_SCREEN | RAW_INPUT | DISABLE_CTRL_C);
        Window win(term.get_w(), term.get_h());
        win.set_wordwrap(true);
        u32string info = // will be filled into win in the main loop
            U"This demo illustrates several features of the \"Window\" "
            "class. Type some text into the green window, press F2 or "
            "F3 to place the cursor in the red or blue window, and F1 to go "
            "back to the green one. (Technically, those are subwindows and "
            "this text is in the main window.) Use backspace to modify your "
            "input. The arrow keys move the active window around. "
            "Ctrl-F1/F2/F3 hides resp. shows again the green/red/blue window. "
            "The green window has a fixed size, while the red one will "
            "expand its width according to the typed-in text "
            "and the blue one is able to grow in height. The blue "
            "and the main window have word wrap enabled. If your active "
            "window is obscured by another one, press Shift-F1/F2/F3 "
            "to bring it to the foreground. There is also "
            "a sub-subwindow located inside the green one. Press Ctrl-F4 to "
            "make it visible. You may also "
            "change the size of the console window with your mouse. --- ";
        vector<size_t>
            start_x{22, 5, 10, 4},
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
        ChildWindow *cwin;
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
            if(!fixed_w[i]) cwin->unfix_width();
            if (!fixed_h[i]) cwin->unfix_height();
            cwin->set_border_fg(border_col[i]);
            cwin->set_default_fg(fg_col[i]);
            cwin->set_default_bg(bg_col[i]);
            cwin->set_title(title[i]);
            cwin->show_cursor();
            // show #1..3, hide #4
            if (i < 3) cwin->show(); 
            child.push_back(cwin);
        }
        child[2]->set_wordwrap(true);
        // prepare exit dialog (shows up when ctrl-c pressed)
        ChildWindow *exit_dialog = win.new_child(0,0,0,0,border_t::LINE);
        exit_dialog->unfix_size();
        exit_dialog->set_border_fg(fg::magenta);
        exit_dialog->set_default_bg(bg::magenta);
        exit_dialog->write("Ctrl-C pressed.\nDo you want to exit? [Y/n]");
        // prepare main loop
        size_t active_child = 0;
        vector<u32string> input(4);
        bool update = true;
        bool accept_input = true;
        bool abort = false;
        bool refill_win = true;
        // main loop
        while (!abort) {
            cwin = child[active_child];
            cwin->set_border(border_t::DOUBLE_LINE);
            win.hand_over_visual_cursor(cwin);
            if (term.update_size()) {
                win.clear_grid();
                win.resize(term.get_w(), term.get_h());
                refill_win = true;
            }
            if (refill_win) {
                size_t written{};
                do {
                    written = win.write(info);
                } while(written == info.size());
                refill_win = false;
                update = true;
            }
            if (update) {
                term.draw_window(win);
                accept_input = win.get_visual_cursor().is_visible;
                update = false;
            }
            char32_t ch = read_key0();
            if (!ch) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            switch (ch) {
            case Key::ARROW_UP:
                if (cwin->get_offset_y()) {
                    cwin->move_to(cwin->get_offset_x(), 
                        cwin->get_offset_y() - 1);
                }
                update = true;
                continue;
            case Key::ARROW_DOWN:
                if (cwin->get_h() + cwin->get_offset_y() < win.get_h()) {
                    cwin->move_to(cwin->get_offset_x(), 
                        cwin->get_offset_y() + 1);
                }
                update = true;
                continue;
            case Key::ARROW_LEFT:
                if (cwin->get_offset_x() >
                    (cwin->get_border() == border_t::NO_BORDER ? 0 : 1)) {
                    cwin->move_to(cwin->get_offset_x() - 1,
                    cwin->get_offset_y());
                }
                update = true;
                continue;
            case Key::ARROW_RIGHT:
                if (cwin->get_w() + cwin->get_offset_x() < win.get_w()) {
                    cwin->move_to(cwin->get_offset_x() + 1,
                    cwin->get_offset_y());
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
                if (child[0]->is_visible()) child[0]->hide();
                else child[0]->show();
                update = true;
                continue;
            case Key::CTRL | Key::F2:
                if (child[1]->is_visible()) child[1]->hide();
                else child[1]->show();
                update = true;
                continue;
            case Key::CTRL | Key::F3:
                if (child[2]->is_visible()) child[2]->hide();
                else child[2]->show();
                update = true;
                continue;
            case Key::CTRL | Key::F4:
                cwin = child[3];
                if (child[3]->is_visible()) {
                    child[3]->hide();
                } else {
                    child[3]->show();
                    // make parental child visible too
                    child[0]->show(); 
                }
                update = true;
                continue;
            case Key::SHIFT | Key::F4:
                if (!child[3]->is_visible()) continue;
                [[fallthrough]];
            case Key::SHIFT | Key::F1:
                if (!child[0]->is_visible()) continue;
                child[0]->to_foreground();
                update = true;
                continue;
            case Key::SHIFT | Key::F2:
                if (!child[1]->is_visible()) continue;
                child[1]->to_foreground();
                update = true;
                continue;
            case Key::SHIFT | Key::F3:
                if (!child[2]->is_visible()) continue;
                child[2]->to_foreground();
                update = true;
                continue;
            case Key::ENTER:
                if (!accept_input) continue;
                input[active_child].push_back(ch);
                break;
            case Key::BACKSPACE:
                if (!accept_input) continue;
                if (input[active_child].size()) {
                    input[active_child].pop_back();
                    break;
                } else {
                    continue;
                }
            case Key::CTRL | Key::BACKSPACE:
                if (!accept_input) continue;
                input[active_child].clear();
                break;
            case Key::CTRL_C: {
                // place exit dialog in center
                int x = (win.get_w() - exit_dialog->get_w()) / 2;
                int y = (win.get_h() - exit_dialog->get_h()) / 2;
                exit_dialog->move_to(x < 0 ? 0 : x, y < 0 ? 0 : y);
                exit_dialog->to_foreground();
                exit_dialog->show();
                win.hand_over_visual_cursor(exit_dialog);
                term.draw_window(win);
                char32_t key;
                do {
                    key = read_key();
                } while (key != Key::ENTER && key != U'Y' && key != U'y' &&
                         key != U'N' && key != U'n');
                if (key == U'N' || key == U'n') {
                    exit_dialog->hide();
                    update = true;
                    continue;
                }
                abort = true;
                continue;
            }
            case Key::TAB:
                if (!accept_input) continue;
                input[active_child].push_back(ch);
                break;
            default:
                if (!accept_input) continue;
                if (ch < U' ' || ch == U'\x7f' || ch > UTF8_MAX) continue;
                input[active_child].push_back(ch);
            } // switch
            bool repeat = false;
            // to prevent auto-growing subwindows from exceeding the main 
            // window's size, trim the input string until the subwindow fits.
            do {
                cwin->clear_grid();
                size_t characters_printed = cwin->write(input[active_child]);
                input[active_child].resize(characters_printed);
                // make auto-shrinking
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
    cout << "program terminated by user." << endl;
    return 0;
}
