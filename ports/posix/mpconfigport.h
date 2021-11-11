
#pragma once

#define CIRCUITPY_INTERNAL_NVM_SIZE (0)

#define DISABLE_FILESYSTEM (1)

#define MICROPY_NLR_THUMB (0)

#define CIRCUITPY_DEFAULT_STACK_SIZE (8192)

#define CIRCUITPY_PROCESSOR_COUNT (0)

#define MICROPY_HW_BOARD_NAME "posix"
#define MICROPY_HW_MCU_NAME "posix"

#include "py/circuitpy_mpconfig.h"

#define MICROPY_PORT_ROOT_POINTERS \
    CIRCUITPY_COMMON_ROOT_POINTERS
