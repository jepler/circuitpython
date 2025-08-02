#include <unistd.h>

#include "py/mpconfig.h"
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
#define USE_VIRTUAL_UART (1) // virtual UART at 0xc0006a

void mp_hal_stdin_init() {
#if USE_CONSOLE
    InitConsole();
#endif
}

static int pending_char = EOF;
// Receive single character
int mp_hal_stdin_rx_chr(void) {
    while (!mp_hal_stdin_available()) { // side-effect: sets pending_char to non-EOF if available
    }        
    int result = result = pending_char;
    pending_char = EOF;
    return result;
}

bool mp_hal_stdin_available(void) {
#if USE_CONSOLE
    if (pending_char == EOF) {
        if(Console::currentInstance->Available(1)) {
            pending_char = Console::currentInstance->WaitNextChar();
        }
    }
#endif
#if USE_VIRTUAL_UART
    if (pending_char == EOF) {
        pending_char = *(volatile int16_t*)0xc0006a;
    }
#endif
    return pending_char != EOF;
}

mp_uint_t debug_uart_tx_strn(const char *str, mp_uint_t len);
mp_uint_t debug_uart_tx_strn(const char *str, mp_uint_t len) {
#if USE_VIRTUAL_UART
    mp_uint_t result = len;
    // debug hack, needs patched umac
    while(len--) {
        *(char*)0xc0006a = *str++;
    }
    return result;
#endif
}

void debug_print_fn(void *data, const char *str, size_t len) {
    debug_uart_tx_strn(str, len);
}

mp_print_t debug_print = { NULL, debug_print_fn };

// Send string of given length
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
