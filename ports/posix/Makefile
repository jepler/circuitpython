# This file is part of the MicroPython project, http://micropython.org/
#
# The MIT License (MIT)
#
# SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

CIRCUITPY_PORT_NO_BOARD = 1

include ../../py/circuitpy_mkenv.mk

CROSS_COMPILE =

INC += -I. \
       -I../.. \
       -I../lib/mp-readline \
       -I../shared/timeutils \
       -I$(BUILD)

CFLAGS += -ggdb3 -Og

CFLAGS += $(INC) -Wall -Werror -std=gnu11 $(BASE_CFLAGS) $(CFLAGS_MOD) $(COPT) -Werror=missing-prototypes

SRC_COMMON_HAL_EXPANDED = $(addprefix shared-bindings/, $(SRC_COMMON_HAL)) \
                          $(addprefix shared-bindings/, $(SRC_BINDINGS_ENUMS)) \
                          $(addprefix common-hal/, $(SRC_COMMON_HAL))

SRC_SHARED_MODULE_EXPANDED = $(addprefix shared-bindings/, $(SRC_SHARED_MODULE)) \
                             $(addprefix shared-module/, $(SRC_SHARED_MODULE)) \
                             $(addprefix shared-module/, $(SRC_SHARED_MODULE_INTERNAL))

# There may be duplicates between SRC_COMMON_HAL_EXPANDED and SRC_SHARED_MODULE_EXPANDED,
# because a few modules have files both in common-hal/ and shared-module/.
# Doing a $(sort ...) removes duplicates as part of sorting.
SRC_COMMON_HAL_SHARED_MODULE_EXPANDED = $(sort $(SRC_COMMON_HAL_EXPANDED) $(SRC_SHARED_MODULE_EXPANDED))

SRC_C += shared/runtime/gchelper_generic.c
SRC_C += input.c

OBJ = $(PY_O) $(SUPERVISOR_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_COMMON_HAL_SHARED_MODULE_EXPANDED:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CIRCUITPY_COMMON:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))

SRC_QSTR += $(SRC_C) $(SRC_SUPERVISOR) $(SRC_COMMON_HAL_EXPANDED) $(SRC_SHARED_MODULE_EXPANDED) $(SRC_CIRCUITPY_COMMON)
# Sources that only hold QSTRs after pre-processing.
SRC_QSTR_PREPROCESSOR +=

all: circuitpython

circuitpython: $(OBJ)
	$(STEPECHO) "LINK $@"
	$(Q)echo $(OBJ) > $(BUILD)/firmware.objs
	$(Q)$(CC) -o $@ $(LDFLAGS) @$(BUILD)/firmware.objs -Wl,--start-group $(LIBS) -Wl,--end-group -lm

include $(TOP)/py/mkrules.mk

TEST_SHOW_FAILURES ?= false
TEST_ARGS ?=
.PHONY: test
test: circuitpython
	$(STEPECHO) "TEST $(TEST_ARGS)"
	$(eval DIRNAME=ports/$(notdir $(CURDIR)))
	$(Q)cd $(TOP)/tests; \
	./run-tests.py --clean-failures; \
	if ! MICROPY_MICROPYTHON=../$(DIRNAME)/circuitpython ./run-tests.py $(TEST_ARGS); then \
		exitstatus=$?; \
		if $(TEST_SHOW_FAILURES); then \
			./run-tests.py --print-failures; \
		fi; \
		echo $?; \
	fi

.PHONY: vtest% vtest
vtest:
	$(Q)$(MAKE) TEST_SHOW_FAILURES=true "test"
vtest%:
	$(Q)$(MAKE) TEST_SHOW_FAILURES=true "test$*"

.PHONY: testd-%
testd-%:
	$(Q)$(MAKE) TEST_ARGS="-d $*" test

.PHONY: test-%
test-%:
	$(Q)$(MAKE) TEST_ARGS="*/$*.py" test


.PHONY: print-failures clean-failures
print-failures clean-failures:
	../../tests/run-tests.py --$@
