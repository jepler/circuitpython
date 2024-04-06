#pragma once

#define CIRCUITPY_PYSTACK_SIZE 16384

#define MICROPY_PY_SYS_PLATFORM                     "posix"
#define MICROPY_HW_BOARD_NAME "posix"

#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_A)
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

// Configure which emitter to use for this target.
#if !defined(MICROPY_EMIT_X64) && defined(__x86_64__)
    #define MICROPY_EMIT_X64        (1)
#endif
#if !defined(MICROPY_EMIT_X86) && defined(__i386__)
    #define MICROPY_EMIT_X86        (1)
#endif
#if !defined(MICROPY_EMIT_THUMB) && defined(__thumb2__)
    #define MICROPY_EMIT_THUMB      (1)
    #define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))
#endif
// Some compilers define __thumb2__ and __arm__ at the same time, let
// autodetected thumb2 emitter have priority.
#if !defined(MICROPY_EMIT_ARM) && defined(__arm__) && !defined(__thumb2__)
    #define MICROPY_EMIT_ARM        (1)
#endif

#ifndef MICROPY_PY_SYS_PATH_DEFAULT
#define MICROPY_PY_SYS_PATH_DEFAULT ".frozen:~/.micropython/lib:/usr/lib/micropython"
#endif

#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_MEM_STATS (1)
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)

#include "py/circuitpy_mpconfig.h"
