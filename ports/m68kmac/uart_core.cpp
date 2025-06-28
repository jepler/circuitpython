#include <unistd.h>

#include "py/mpconfig.h"
extern "C" {
#include "py/mpprint.h"
}

#include "uart_core.h"

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

#define USE_CONSOLE (1)

// Receive single character
extern "C" 
int mp_hal_stdin_rx_chr(void);
int mp_hal_stdin_rx_chr(void) {
#if USE_CONSOLE
    if(!Console::currentInstance)
        InitConsole();
    if(Console::currentInstance == (Console*)-1)
        return EOF;
    int c = Console::currentInstance->WaitNextChar();
    if(c == 10) c = 13; // LF vs CR
#else
    int c = *(char*)0xc0006a;
#endif
    return c;
}

extern "C"
mp_uint_t debug_uart_tx_strn(const char *str, mp_uint_t len);
mp_uint_t debug_uart_tx_strn(const char *str, mp_uint_t len) {
    mp_uint_t result = len;
    // debug hack, needs patched umac
    while(len--) {
        *(char*)0xc0006a = *str++;
    }
    return result;
}

void debug_print_fn(void *data, const char *str, size_t len) {
    debug_uart_tx_strn(str, len);
}

mp_print_t debug_print = { NULL, debug_print_fn };

// Send string of given length
extern "C"
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    debug_uart_tx_strn(str, len);

#if USE_CONSOLE
    if(!Console::currentInstance)
        InitConsole();
    if(Console::currentInstance == (Console*)-1)
        return 0;

    Console::currentInstance->write(str, (size_t)len);
#endif

    return len;
}
