#include <unistd.h>

#include "py/mpconfig.h"
extern "C" {
#include "py/mpprint.h"
}

#include "retro/Console.h"
#include "retro/ConsoleWindow.h"
/*
 * Core UART functions to implement for a port
 */

using namespace retro;
namespace retro
{
    void InitConsole();
}

// Receive single character
extern "C" 
int mp_hal_stdin_rx_chr(void);
int mp_hal_stdin_rx_chr(void) {
    if(!Console::currentInstance)
        InitConsole();
    if(Console::currentInstance == (Console*)-1)
        return EOF;
    int c = Console::currentInstance->WaitNextChar();
    if(c == 10) c = 13; // LF vs CR
    return c;
}

// Send string of given length
extern "C"
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    if(!Console::currentInstance)
        InitConsole();
    if(Console::currentInstance == (Console*)-1)
        return 0;

    Console::currentInstance->write(str, (size_t)len);
    return len;
}
