# This is for fuzzing with AFLplusplus

# Disable optimisations and enable assert() on coverage builds.
DEBUG ?= 1

CC=afl-cc

CFLAGS += \
	-Wformat -Wmissing-declarations -Wmissing-prototypes \
	-fwrapv \
	-Wold-style-definition -Wpointer-arith -Wshadow -Wuninitialized -Wunused-parameter \

MICROPY_PY_SSL = 0
MICROPY_PY_FFI = 0

export AFL_UBSAN_VERBOSE
export AFL_USE_UBSAN
export AFL_QUIET
AFL_USE_UBSAN=1
AFL_QUIET=1
AFL_UBSAN_VERBOSE=1

MPY_TOOL_FLAGS = -mlongint-impl longlong

# https://github.com/micropython/micropython/issues/17718
MICROPY_PY_BTREE=0
