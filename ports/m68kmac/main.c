#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Multiverse.h"

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"
#include "extmod/vfs.h"
#include "extmod/vfs_posix.h"

#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    mp_printf(&mp_plat_print, ">>> %s\n", src);
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MICROPY_HEAP_SIZE];
#endif

int main(int argc, char **argv) {
    mp_printf(&mp_plat_print, "Hello, world.\n");

    int stack_dummy;
    stack_top = (char *)&stack_dummy;

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();

    {
        // Mount the host FS at the root of our internal VFS
        mp_obj_t args[2] = {
            MP_OBJ_TYPE_GET_SLOT(&mp_type_vfs_mac, make_new)(&mp_type_vfs_mac, 0, 0, NULL),
            MP_OBJ_NEW_QSTR(MP_QSTR__slash_),
        };
        mp_vfs_mount(2, args, (mp_map_t *)&mp_const_empty_map);

        // Make sure the root that was just mounted is the current VFS (it's always at
        // the end of the linked list).  Can't use chdir('/') because that will change
        // the current path within the VfsPosix object.
        MP_STATE_VM(vfs_cur) = MP_STATE_VM(vfs_mount_table);
        while (MP_STATE_VM(vfs_cur)->next != NULL) {
            MP_STATE_VM(vfs_cur) = MP_STATE_VM(vfs_cur)->next;
        }
    }

    #if 0
    do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
    #endif

    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif
    // do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    // do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
    #else
    pyexec_frozen_module("frozentest.py", false);
    #endif
    mp_deinit();
    return 0;
}

#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info(&mp_plat_print);
}
#endif

void MP_NORETURN __fatal_error(const char *msg);

void nlr_jump_fail(void *val) {
    __fatal_error("nlr jump fail");
}

void MP_NORETURN __fatal_error(const char *msg) {
    mp_printf(&mp_plat_print, "Fatal: %s\n", msg);
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

#define TICK_TO_MS(x) ((x + 2) * 50 / 3)
#define TICK_TO_US(x) ((x + 2) * 50000 / 3)
#define MS_TO_TICK(x) ((x + 25) * 3 / 50)
#define US_TO_TICK(x) ((x + 25000) * 3 / 50000)

void mp_hal_delay_ms(mp_uint_t delay) {
    long unused;
    Delay(MS_TO_TICK(delay), &unused);
}

void mp_hal_delay_us(mp_uint_t delay) {
    long unused;
    Delay(US_TO_TICK(delay), &unused);
}

mp_uint_t mp_hal_ticks_ms(void) {
    return TICK_TO_MS(TickCount());
}

mp_uint_t mp_hal_ticks_us(void) {
    return TICK_TO_US(TickCount());
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return 0;
}
