/*



*/


#include "../cpp-terminal/base.hpp"
#include "../cpp-terminal/input.hpp"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wc++98-c++11-compat-binary-literal"
#pragma GCC diagnostic ignored "-Wc++98-c++11-compat-pedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif // defined
#include "unicodelib_encodings.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif // defined

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <set>

using namespace std;
using namespace Term;

std::map<int, std::string> keyname{
    {Key::BACKSPACE, "BACKSPACE"},
    {Key::ENTER, "ENTER"},
    {Key::TAB, "TAB"},
    {Key::ESC, "ESC"},
    {Key::SHIFT, "SHIFT"},
    {Key::CTRL, "CTRL"},
    {Key::ALT, "ALT"},
    {Key::ARROW_UP, "ARROW_UP"},
    {Key::ARROW_DOWN, "ARROW_DOWN"},
    {Key::ARROW_RIGHT, "ARROW_RIGHT"},
    {Key::ARROW_LEFT, "ARROW_LEFT"},
    {Key::PAGE_UP, "PAGE_UP"},
    {Key::PAGE_DOWN, "PAGE_DOWN"},
    {Key::HOME, "HOME"},
    {Key::END, "END"},
    {Key::DEL, "DEL"},
    {Key::INSERT, "INSERT"},
    {Key::NUMERIC_5, "NUMERIC_5"},
    {Key::F1, "F1"},
    {Key::F2, "F2"},
    {Key::F3, "F3"},
    {Key::F4, "F4"},
    {Key::F5, "F5"},
    {Key::F6, "F6"},
    {Key::F7, "F7"},
    {Key::F8, "F8"},
    {Key::F9, "F9"},
    {Key::F10, "F10"},
    {Key::F11, "F11"},
    {Key::F12, "F12"},
    {Key::UNKNOWN, "UNKNOWN"}
};

int main() {
    int ctrl_c_count = 2;
    try {
        Terminal term(RAW_INPUT | DISABLE_CTRL_C);
        while (true) {
            u32string s = Term::Private::read_sequence();     
            if (s.size() == 1) {
                cout << "codepoint " << hex << 
                    (int)s[0] << dec << " (hex)" << endl;
            } else {
                cout << "sequence =   " << hex;
                for (auto c : s) cout << (int)c << " ";
                cout << "(hex)" << dec << endl <<  "sequence = <ESC> ";
                for (auto it = s.begin() + 1; it != s.end(); ++it)
                    cout << unicode::utf8::encode_codepoint(*it) << "  ";
                cout << dec << endl;
            }
            cout << "recognized input = ";
            char32_t c = Term::Private::decode_sequence(s);
            if (c == U'\x3') --ctrl_c_count;
            else ctrl_c_count = 2;
            if (c & Key::SHIFT) {
                cout << "Shift-"; 
                c = c & ~Key::SHIFT;
            }
                if (c & Key::ALT) {
                cout << "Alt-";
                c = c & ~Key::ALT;
            }
            if (c & Key::CTRL) {
                cout << "Ctrl-";
                c = c & ~Key::CTRL;
            }
            if (keyname.count(c)) {
                cout << keyname[c] << endl;
            } else if (c < 27) {
                cout << "Ctrl-" << char('A' + c - 1) << endl;
            } else if (c < 32) {
                cout << "Ctrl-" << char('0' + c - 24) << endl;
            } else {
                cout << unicode::utf8::encode_codepoint(c) << endl;
            }
            if (c == U'\x3') {
                if (ctrl_c_count) {
                    cout << "press CTRL-C again to exit" << endl;
                } else {
                    cout << "aborting ..." << endl;
                    break;
                }
            }
            cout << endl;
        }
    }
    catch (const std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Unknown error." << std::endl;
        return 1;
    }
    return 0;


}
