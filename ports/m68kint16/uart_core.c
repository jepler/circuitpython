#include <unistd.h>
#include "py/mpconfig.h"
#include <stdio.h>

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    int r = read(STDIN_FILENO, &c, 1);
    if (r <= 0) {
        return EOF;
    }
    return c;
}

// Send string of given length
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    mp_uint_t ret = len;
    int r = write(STDOUT_FILENO, str, len);
    if (r <= 0) {
        // in case of an error in the syscall, report no bytes written
        ret = 0;
    }
    return ret;
}
