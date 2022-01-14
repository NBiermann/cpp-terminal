#pragma once

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#endif

#include <stdexcept>


namespace Term::Private {
extern const bool debug;

// Returns true if the standard input is attached to a terminal
bool is_stdin_a_tty();
// Returns true if the standard output is attached to a terminal
bool is_stdout_a_tty();

bool get_term_size(size_t& cols, size_t& rows);

// Returns true if a character is read, otherwise immediately returns false
// This can't be made inline
bool read_raw(char32_t* s);

void clean_up();

// handler for CTRL-C event
#ifdef _WIN32
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
#else
void SignalHandler(int);
#endif

/* Note: the code that uses Terminal must be inside try/catch block, otherwise
 * the destructors will not be called when an exception happens and the
 * terminal will not be left in a good state. Terminal uses exceptions when
 * something goes wrong.
 */
class BaseTerminal {
   friend bool read_raw(char32_t*);
   friend void clean_up();
   private:
#ifdef _WIN32
    static HANDLE hout;
    static DWORD dwOriginalOutMode;
    static bool out_console;
    static UINT out_code_page;

    static HANDLE hin;
    static DWORD dwOriginalInMode;
    static UINT in_code_page;
#else
    static struct termios orig_termios;
#endif

  protected:
    static bool is_instantiated;
    static bool clear_screen;
    static bool raw_input;
    static bool disable_ctrl_c;


  public:
    explicit BaseTerminal(bool a_clear_screen = true,
                          bool a_raw_input = false,
                          bool a_disable_ctrl_c = true);

    virtual ~BaseTerminal() noexcept(false);
};

}  // namespace Term::Private
