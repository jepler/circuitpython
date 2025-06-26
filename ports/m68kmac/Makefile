include ../../py/mkenv.mk

CROSS_COMPILE=m68k-apple-macos-
SIZE=:

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

UNAME_S := $(shell uname -s)
LD = $(CXX)
CFLAGS += $(INC) -Wall -Wdouble-promotion -Wfloat-conversion -std=c99 $(COPT)
CSUPEROPT = -Os # save some code space

# Tune for Debugging or Optimization
#CFLAGS += -g  # always include debug info in the ELF
ifeq ($(DEBUG), 1)
CFLAGS += -O0
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

# Flags for optional C++ source code
CXXFLAGS += $(filter-out -std=c99,$(CFLAGS))

LIBS = 

SRC_C = \
	main.c \

SRC_C += \
	$(BUILD)/_frozen_mpy.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_native.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \

SRC_CXX += \
	uart_core.cpp \
	retro/Console.cpp \
	retro/ConsoleWindow.cpp \
	retro/InitConsole.cpp \

SRC_S += \
	shared/runtime/gchelper_m68k.S \

SRC_QSTR += \
	shared/readline/readline.c \
	shared/runtime/pyexec.c \

OBJ += $(PY_CORE_O) $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.S=.o))

all: $(BUILD)/micropython.bin

$(BUILD)/_frozen_mpy.c: $(TOP)/tests/frozen/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=mpz $< > $@

$(BUILD)/micropython.code.bin: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

REZ=Rez
RINCLUDES=/Retro68-build/toolchain/RIncludes
REZFLAGS=-I$(RINCLUDES)

$(BUILD)/micropython.bin $(BUILD)/micropython.APPL $(BUILD)/micropython.dsk: $(BUILD)/micropython.code.bin
	$(REZ) $(REZFLAGS) \
		--copy "$(BUILD)/micropython.code.bin" \
		"$(RINCLUDES)/Retro68APPL.r" \
		-t "APPL" -c "mupy" \
		-o $(BUILD)/micropython.bin --cc $(BUILD)/micropython.APPL --cc $(BUILD)/micropython.dsk

include $(TOP)/py/mkrules.mk
