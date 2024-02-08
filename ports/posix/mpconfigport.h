#pragma once

#define CIRCUITPY_PYSTACK_SIZE 16384

#define MICROPY_PY_SYS_PLATFORM                     "posix"
#define MICROPY_HW_BOARD_NAME "posix"

#define MICROPY_USE_READLINE (1)
#define MICROPY_GCREGS_SETJMP (0)

#if defined(__i386__)
    #define MICROPY_HW_MCU_NAME "x86"
#elif defined(__x86_64__)
#define MICROPY_HW_MCU_NAME "amd64"
#elif defined(__thumb2__) || defined(__thumb__) || defined(__arm__)
#define MICROPY_HW_MCU_NAME "arm32"
#elif defined(__aarch64__)
#define MICROPY_HW_MCU_NAME "aarch64"
#elif defined(__xtensa__)
#define MICROPY_HW_MCU_NAME "xtensa"
#elif defined(__powerpc__)
#define MICROPY_HW_MCU_NAME "powerpc"
#elif defined(__mips__)
#define MICROPY_HW_MCU_NAME "mips"
#else
#define MICROPY_HW_MCU_NAME "idk"
// #warning "No native NLR support for this arch, using setjmp implementation"
#endif

#ifndef MICROPY_PY_SYS_PATH_DEFAULT
#define MICROPY_PY_SYS_PATH_DEFAULT ".frozen:~/.micropython/lib:/usr/lib/micropython"
#endif

#include "py/circuitpy_mpconfig.h"
