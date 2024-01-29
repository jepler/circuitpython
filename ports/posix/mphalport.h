#pragma once
#include "supervisor/shared/tick.h"
void mp_hal_set_interrupt_char(int c);
#define mp_hal_ticks_ms()       ((mp_uint_t)supervisor_ticks_ms32())

#ifndef CHAR_CTRL_C
#define CHAR_CTRL_C (3)
#endif

void mp_hal_stdio_mode_raw(void);
void mp_hal_stdio_mode_orig(void);

// This macro is used to implement PEP 475 to retry specified syscalls on EINTR
#define MP_HAL_RETRY_SYSCALL(ret, syscall, raise) { \
        for (;;) { \
            MP_THREAD_GIL_EXIT(); \
            ret = syscall; \
            MP_THREAD_GIL_ENTER(); \
            if (ret == -1) { \
                int err = errno; \
                if (err == EINTR) { \
                    mp_handle_pending(true); \
                    continue; \
                } \
                raise; \
            } \
            break; \
        } \
}
