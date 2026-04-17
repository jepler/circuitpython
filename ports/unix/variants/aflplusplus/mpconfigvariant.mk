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

MPY_TOOL_FLAGS = -mlongint-impl longlong

# https://github.com/micropython/micropython/issues/17718
MICROPY_PY_BTREE=0
