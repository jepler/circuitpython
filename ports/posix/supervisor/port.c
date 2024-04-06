#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "common-hal/supervisor/Runtime.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Processor.h"
#include "shared-bindings/microcontroller/ResetReason.h"
#if CIRCUITPY_RTC
#include "shared-bindings/rtc/__init__.h"
#endif
#include "shared-bindings/supervisor/Runtime.h"
#include "shared/runtime/interrupt_char.h"
#include "supervisor/flash.h"
#include "supervisor/port.h"
#include "supervisor/serial.h"
#include "mphalport.h"
#include "main.h"

static struct termios orig_termios;

void mp_hal_stdio_mode_raw(void) {
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

void mp_hal_stdio_mode_orig(void) {
    // restore terminal settings
    tcsetattr(0, TCSAFLUSH, &orig_termios);
}


uint64_t port_get_raw_ticks(uint8_t *subticks_out) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t subticks = ((uint64_t)ts.tv_nsec * 140737) >> 32;
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

static uint32_t *_stack_top, *_stack_limit;
#if defined(__has_include) && __has_include(<pthread.h>)
#include <pthread.h>
int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr);
safe_mode_t port_init(void) {
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);
    void *stackaddr;
    size_t stacksize;
    pthread_attr_getstack(&attr, &stackaddr, &stacksize);
    _stack_top = (uint32_t *)((char *)stackaddr + stacksize);
    _stack_limit = (uint32_t *)stackaddr + 4096;
    pthread_attr_destroy(&attr);
    return SAFE_MODE_NONE;
}
#else
safe_mode_t port_init(void) {
    intptr_t pagesize = (intptr_t)sysconf(_SC_PAGESIZE);
    _stack_top = (uint32_t *)(((intptr_t)address_of_argc + pagesize - 1) & ~(pagesize - 1));
    struct rlimit lim;
    getrlimit(RLIMIT_STACK, &lim);
    _stack_limit = _stack_top - lim.rlim_cur / sizeof(uint32_t);
    return SAFE_MODE_NONE;
}
#endif

uint32_t *port_stack_get_limit(void) {
    return _stack_limit;
}
uint32_t *port_stack_get_top(void) {
    return _stack_top;
}

void reset_port(void) {
    #if CIRCUITPY_RTC
    rtc_reset();
    #endif
}
void port_idle_until_interrupt(void) {
}
void port_background_task(void) {
}


static void *flash_ptr;
void supervisor_flash_init(void) {
    int ffd = open("CIRCUITPY.IMG", O_RDWR | O_CREAT, 0666 /* minus bits in umask */);
    size_t flash_size = supervisor_flash_get_block_size() * supervisor_flash_get_block_count();
    posix_fallocate(ffd, 0, flash_size);
    flash_ptr = mmap(NULL, flash_size, PROT_READ | PROT_WRITE, MAP_SHARED, ffd, 0);
}
uint32_t supervisor_flash_get_block_size(void) {
    return 512;
}
uint32_t supervisor_flash_get_block_count(void) {
    return 1024;
}
mp_uint_t supervisor_flash_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {
    if (!flash_ptr) {
        return 1; /* error */
    }
    if (block_num >= supervisor_flash_get_block_count()) {
        return 1; /* error */
    }
    memcpy(dest, flash_ptr + block_num * supervisor_flash_get_block_size(), supervisor_flash_get_block_size() * num_blocks);
    return 0;
}
mp_uint_t supervisor_flash_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
    if (!flash_ptr) {
        return 1; /* error */
    }
    if (block_num >= supervisor_flash_get_block_count()) {
        return 1; /* error */
    }
    memcpy(flash_ptr + block_num * supervisor_flash_get_block_size(), src, supervisor_flash_get_block_size() * num_blocks);
    return 0;
}


void port_internal_flash_flush(void) {
    size_t flash_size = supervisor_flash_get_block_size() * supervisor_flash_get_block_count();
    if (flash_ptr) {
        msync(flash_ptr, flash_size, MS_ASYNC | MS_INVALIDATE);
    }
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
    } else if (c == '\x1c' && isatty(0)) {
        raise(SIGQUIT);
    } else if (c == '\n') {
        c = '\r';
    }

    return (int)c;
}
bool port_serial_connected(void) {
    return true;
}
