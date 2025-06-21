include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = m68k-elf-
STRIP = :

#CC = owcc
#LD = owcc
#CPP = $(PYTHON) owcc_stdin_hack.py -E

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

MODEL = -mshort

DEBUG=1
LD = $(CC)
CFLAGS += $(INC) -std=gnu99 $(COPT) $(MODEL) -ffunction-sections -fdata-sections -g 
LDFLAGS += $(MODEL) -Wl,-Map=$@.map
CSUPEROPT = -Os # save some code space

# Tune for Debugging or Optimization
# "target system does not support debug output"
ifeq ($(DEBUG), 1)
CFLAGS += -Og
else
CFLAGS += -Os -DNDEBUG
#CFLAGS += -fdata-sections -ffunction-sections
endif

# Flags for optional C++ source code
CXXFLAGS += $(filter-out -std=gnu99,$(CFLAGS))

LIBS =

SRC_C = \
	main.c \
	syscalls.c \
	uart_core.c \
	shared/libc/printf.c \
	shared/readline/readline.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	$(BUILD)/_frozen_mpy.c \

SRC_C += shared/libc/string0.c

SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))

all: $(BUILD)/firmware.elf

$(BUILD)/_frozen_mpy.c: $(TOP)/tests/frozen/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

test: $(BUILD)/firmware.elf
	$(Q)/usr/bin/printf "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\004" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
