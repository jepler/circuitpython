#include "py/mpconfig.h"
#include "py/mpthread.h"
#include "py/runtime.h"
#include "modtime.h"
#include <math.h>
#include <sys/select.h>

mp_obj_t mp_time_sleep(mp_obj_t arg) {
    #if MICROPY_PY_BUILTINS_FLOAT
    uint64_t interval = (uint64_t)MICROPY_FLOAT_C_FUN(round)(mp_obj_get_float(arg) * 1e3);
    #else
    uint64_t interval = UINT64_C(1000) * mp_obj_get_int(arg);
    #endif

    for (; interval--;) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 1000};
        MP_THREAD_GIL_EXIT();
        (void)select(0, NULL, NULL, NULL, &tv);
        MP_THREAD_GIL_ENTER();
        RUN_BACKGROUND_TASKS;
        mp_handle_pending(true);
    }
    return mp_const_none;
}
