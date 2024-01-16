#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common-hal/supervisor/Runtime.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Processor.h"
#include "shared-bindings/microcontroller/ResetReason.h"
#include "shared-bindings/supervisor/Runtime.h"
#include "supervisor/flash.h"
#include "supervisor/port.h"
#include "supervisor/serial.h"

static struct termios orig_termios;

STATIC void mp_hal_stdio_mode_raw(void) {
    // save and set terminal settings
    tcgetattr(0, &orig_termios);
    static struct termios termios;
    termios = orig_termios;
    termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    termios.c_cflag = (termios.c_cflag & ~(CSIZE | PARENB)) | CS8;
    termios.c_lflag = 0;
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &termios);
}

STATIC void mp_hal_stdio_mode_orig(void) {
    // restore terminal settings
    tcsetattr(0, TCSAFLUSH, &orig_termios);
}


uint64_t port_get_raw_ticks(uint8_t *subticks_out) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t subticks = (uint64_t)ts.tv_nsec * 30517 /*30517.578*/;
    int result = ts.tv_sec * 1024 + subticks / 32;
    if (subticks_out) {
        *subticks_out = subticks % 32;
    }
    return result;
}


void common_hal_mcu_disable_interrupts(void) {
}
void common_hal_mcu_enable_interrupts(void) {
}

static uint32_t *_stack_top;
uint32_t *port_stack_get_limit(void) {
    return _stack_top - 40000 * sizeof(void *) / 4;
}
uint32_t *port_stack_get_top(void) {
    return _stack_top;
}

safe_mode_t port_init(void) {
    uint32_t dummy;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdangling-pointer"
    _stack_top = &dummy;
    #pragma GCC diagnostic pop
    mp_hal_stdio_mode_raw();
    atexit(mp_hal_stdio_mode_orig);
    return SAFE_MODE_NONE;
}

void reset_port(void) {
}
void port_idle_until_interrupt(void) {
}
void port_background_task(void) {
}


void supervisor_flash_init(void) {
}
uint32_t supervisor_flash_get_block_size(void) {
    return 512;
}
uint32_t supervisor_flash_get_block_count(void) {
    return 0;
}
mp_uint_t supervisor_flash_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {
    return 1;                                                                                              /* error */
}
mp_uint_t supervisor_flash_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
    return 1;                                                                                                    /* error */
}


void port_internal_flash_flush(void) {
}
void supervisor_flash_release_cache(void) {
}


mcu_reset_reason_t common_hal_mcu_processor_get_reset_reason(void) {
    return RESET_REASON_UNKNOWN;
}

uint32_t safe_word;
void port_set_saved_word(uint32_t value) {
    safe_word = value;
}

uint32_t port_get_saved_word(void) {
    return safe_word;
}

void reset_cpu(void) {
    exit(0);
}

void port_start_background_tick(void) {
}
void port_finish_background_tick(void) {
}
void port_interrupt_after_ticks(uint32_t ticks) {
}
void port_enable_tick(void) {
}
void port_disable_tick(void) {
}

STATIC const mp_rom_map_elem_t board_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_board) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);

bool common_hal_supervisor_runtime_get_serial_connected(void) {
    return true;
}
bool common_hal_supervisor_runtime_get_serial_bytes_available(void) {
    fd_set f;
    struct timeval no_timeout = {0, 0};
    FD_ZERO(&f);
    FD_SET(0, &f);
    int result = select(1, &f, NULL, NULL, &no_timeout);
    return result > 0;
}

const super_runtime_obj_t common_hal_supervisor_runtime_obj = {
    .base = {
        .type = &supervisor_runtime_type,
    },
};

void common_hal_mcu_reset(void) {
    exit(0);
}

static uint32_t mp_heap[1024 * 1024];

// Get heap bottom address
uint32_t *port_heap_get_bottom(void) {
    return &mp_heap[0];
}

// Get heap top address
uint32_t *port_heap_get_top(void) {
    return &mp_heap[MP_ARRAY_SIZE(mp_heap)];
}

void port_background_tick(void) {
}

void port_serial_write_substring(const char *text, uint32_t length) {
    write(1, text, length);
}

bool port_serial_bytes_available(void) {
    return common_hal_supervisor_runtime_get_serial_bytes_available();
}

int port_serial_read(void) {
    if (!common_hal_supervisor_runtime_get_serial_bytes_available()) {
        return EOF;
    }
    unsigned char c;
    int ret = read(0, &c, 1);
    if (ret == 0) {
        c = 4; // EOF, ctrl-D
    } else if (c == '\n') {
        c = '\r';
    }

    return (int)c;
}
bool port_serial_connected(void) {
    return true;
}
