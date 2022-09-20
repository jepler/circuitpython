#
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

# Boards default to all modules enabled (with exceptions)
# Manually disable by overriding in #mpconfigboard.mk

# Smaller builds can be forced for resource constrained chips (typically SAMD21s
# without external flash) by setting CIRCUITPY_FULL_BUILD=0. Avoid using this
# for merely incomplete ports, as it changes settings in other files.
CIRCUITPY_FULL_BUILD ?= 1
CFLAGS += -DCIRCUITPY_FULL_BUILD=$(CIRCUITPY_FULL_BUILD)

# Reduce the size of in-flash properties. Requires support in the .ld linker
# file, so not enabled by default.
CIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE ?= 0
CFLAGS += -DCIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE=$(CIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE)

# async/await language keyword support
MICROPY_PY_ASYNC_AWAIT ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DMICROPY_PY_ASYNC_AWAIT=$(MICROPY_PY_ASYNC_AWAIT)

# uasyncio
# By default, include uasyncio if async/await are available.
MICROPY_PY_UASYNCIO ?= $(MICROPY_PY_ASYNC_AWAIT)
CFLAGS += -DMICROPY_PY_UASYNCIO=$(MICROPY_PY_UASYNCIO)

# uasyncio normally needs select
MICROPY_PY_USELECT ?= $(MICROPY_PY_UASYNCIO)
CFLAGS += -DMICROPY_PY_USELECT=$(MICROPY_PY_USELECT)

# enable select.select if select is enabled.
MICROPY_PY_USELECT_SELECT ?= $(MICROPY_PY_USELECT)
CFLAGS += -DMICROPY_PY_USELECT_SELECT=$(MICROPY_PY_USELECT_SELECT)

CIRCUITPY_AESIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AESIO=$(CIRCUITPY_AESIO)

# TODO: CIRCUITPY_ALARM will gradually be added to as many ports as possible
# so make this 1 or CIRCUITPY_FULL_BUILD eventually
CIRCUITPY_ALARM ?= 0
CFLAGS += -DCIRCUITPY_ALARM=$(CIRCUITPY_ALARM)

CIRCUITPY_ANALOGIO ?= 1
CFLAGS += -DCIRCUITPY_ANALOGIO=$(CIRCUITPY_ANALOGIO)

CIRCUITPY_ATEXIT ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ATEXIT=$(CIRCUITPY_ATEXIT)

CIRCUITPY_AUDIOBUSIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO=$(CIRCUITPY_AUDIOBUSIO)

# Some boards have PDMIn but do not implement I2SOut.
CIRCUITPY_AUDIOBUSIO_I2SOUT ?= $(CIRCUITPY_AUDIOBUSIO)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO_I2SOUT=$(CIRCUITPY_AUDIOBUSIO_I2SOUT)

# Likewise, some boards have I2SOut but do not implement PDMIn.
CIRCUITPY_AUDIOBUSIO_PDMIN ?= $(CIRCUITPY_AUDIOBUSIO)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO_PDMIN=$(CIRCUITPY_AUDIOBUSIO_PDMIN)

CIRCUITPY_AUDIOIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AUDIOIO=$(CIRCUITPY_AUDIOIO)

CIRCUITPY_AUDIOPWMIO ?= 0
CFLAGS += -DCIRCUITPY_AUDIOPWMIO=$(CIRCUITPY_AUDIOPWMIO)

ifndef CIRCUITPY_AUDIOCORE
ifeq ($(CIRCUITPY_AUDIOPWMIO),1)
CIRCUITPY_AUDIOCORE = $(CIRCUITPY_AUDIOPWMIO)
else
CIRCUITPY_AUDIOCORE = $(CIRCUITPY_AUDIOIO)
endif
endif
CFLAGS += -DCIRCUITPY_AUDIOCORE=$(CIRCUITPY_AUDIOCORE)

CIRCUITPY_AUDIOMIXER ?= $(CIRCUITPY_AUDIOIO)
CFLAGS += -DCIRCUITPY_AUDIOMIXER=$(CIRCUITPY_AUDIOMIXER)

ifndef CIRCUITPY_AUDIOMP3
ifeq ($(CIRCUITPY_FULL_BUILD),1)
CIRCUITPY_AUDIOMP3 = $(CIRCUITPY_AUDIOCORE)
else
CIRCUITPY_AUDIOMP3 = 0
endif
endif
CFLAGS += -DCIRCUITPY_AUDIOMP3=$(CIRCUITPY_AUDIOMP3)

CIRCUITPY_BINASCII ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BINASCII=$(CIRCUITPY_BINASCII)

CIRCUITPY_BITBANG_APA102 ?= 0
CFLAGS += -DCIRCUITPY_BITBANG_APA102=$(CIRCUITPY_BITBANG_APA102)

CIRCUITPY_BITBANGIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BITBANGIO=$(CIRCUITPY_BITBANGIO)

CIRCUITPY_BITOPS ?= 0
CFLAGS += -DCIRCUITPY_BITOPS=$(CIRCUITPY_BITOPS)

# _bleio can be supported on most any board via HCI
CIRCUITPY_BLEIO_HCI ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BLEIO_HCI=$(CIRCUITPY_BLEIO_HCI)

# Explicitly enabled for boards that support _bleio.
CIRCUITPY_BLEIO ?= $(CIRCUITPY_BLEIO_HCI)
CFLAGS += -DCIRCUITPY_BLEIO=$(CIRCUITPY_BLEIO)

CIRCUITPY_BLE_FILE_SERVICE ?= 0
CFLAGS += -DCIRCUITPY_BLE_FILE_SERVICE=$(CIRCUITPY_BLE_FILE_SERVICE)

CIRCUITPY_BOARD ?= 1
CFLAGS += -DCIRCUITPY_BOARD=$(CIRCUITPY_BOARD)

CIRCUITPY_BUSDEVICE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BUSDEVICE=$(CIRCUITPY_BUSDEVICE)

CIRCUITPY_BUILTINS_POW3 ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BUILTINS_POW3=$(CIRCUITPY_BUILTINS_POW3)

CIRCUITPY_BUSIO ?= 1
CFLAGS += -DCIRCUITPY_BUSIO=$(CIRCUITPY_BUSIO)

# These two flags pretend to implement their class but raise a ValueError due to
# unsupported pins. This should be used sparingly on boards that don't break out
# generic IO but need parts of busio.
CIRCUITPY_BUSIO_SPI ?= 1
CFLAGS += -DCIRCUITPY_BUSIO_SPI=$(CIRCUITPY_BUSIO_SPI)

CIRCUITPY_BUSIO_UART ?= 1
CFLAGS += -DCIRCUITPY_BUSIO_UART=$(CIRCUITPY_BUSIO_UART)

CIRCUITPY_CAMERA ?= 0
CFLAGS += -DCIRCUITPY_CAMERA=$(CIRCUITPY_CAMERA)

CIRCUITPY_CANIO ?= 0
CFLAGS += -DCIRCUITPY_CANIO=$(CIRCUITPY_CANIO)

CIRCUITPY_DIGITALIO ?= 1
CFLAGS += -DCIRCUITPY_DIGITALIO=$(CIRCUITPY_DIGITALIO)

CIRCUITPY_COMPUTED_GOTO_SAVE_SPACE ?= 0
CFLAGS += -DCIRCUITPY_COMPUTED_GOTO_SAVE_SPACE=$(CIRCUITPY_COMPUTED_GOTO_SAVE_SPACE)

CIRCUITPY_OPT_LOAD_ATTR_FAST_PATH ?= 1
CFLAGS += -DCIRCUITPY_OPT_LOAD_ATTR_FAST_PATH=$(CIRCUITPY_OPT_LOAD_ATTR_FAST_PATH)

CIRCUITPY_OPT_MAP_LOOKUP_CACHE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_OPT_MAP_LOOKUP_CACHE=$(CIRCUITPY_OPT_MAP_LOOKUP_CACHE)

CIRCUITPY_COUNTIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_COUNTIO=$(CIRCUITPY_COUNTIO)

CIRCUITPY_DISPLAYIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_DISPLAYIO=$(CIRCUITPY_DISPLAYIO)

ifeq ($(CIRCUITPY_DISPLAYIO),1)
CIRCUITPY_PARALLELDISPLAY ?= $(CIRCUITPY_FULL_BUILD)
else
CIRCUITPY_PARALLELDISPLAY = 0
endif
CFLAGS += -DCIRCUITPY_PARALLELDISPLAY=$(CIRCUITPY_PARALLELDISPLAY)

# bitmaptools and framebufferio rely on displayio
ifeq ($(CIRCUITPY_DISPLAYIO),1)
CIRCUITPY_BITMAPTOOLS ?= $(CIRCUITPY_FULL_BUILD)
CIRCUITPY_FRAMEBUFFERIO ?= $(CIRCUITPY_FULL_BUILD)
CIRCUITPY_VECTORIO ?= 1
else
CIRCUITPY_BITMAPTOOLS ?= 0
CIRCUITPY_FRAMEBUFFERIO ?= 0
CIRCUITPY_VECTORIO ?= 0
endif
CFLAGS += -DCIRCUITPY_BITMAPTOOLS=$(CIRCUITPY_BITMAPTOOLS)
CFLAGS += -DCIRCUITPY_FRAMEBUFFERIO=$(CIRCUITPY_FRAMEBUFFERIO)
CFLAGS += -DCIRCUITPY_VECTORIO=$(CIRCUITPY_VECTORIO)

CIRCUITPY_DOTENV ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_DOTENV=$(CIRCUITPY_DOTENV)

CIRCUITPY_DUALBANK ?= 0
CFLAGS += -DCIRCUITPY_DUALBANK=$(CIRCUITPY_DUALBANK)

# Enabled micropython.native decorator (experimental)
CIRCUITPY_ENABLE_MPY_NATIVE ?= 0
CFLAGS += -DCIRCUITPY_ENABLE_MPY_NATIVE=$(CIRCUITPY_ENABLE_MPY_NATIVE)

CIRCUITPY_ERRNO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ERRNO=$(CIRCUITPY_ERRNO)

# CIRCUITPY_ESPIDF is handled in the espressif tree.
# Only for ESP32S chips.
# Assume not a ESP build.
CIRCUITPY_ESPIDF ?= 0
CFLAGS += -DCIRCUITPY_ESPIDF=$(CIRCUITPY_ESPIDF)

CIRCUITPY_ESP32_CAMERA ?= 0
CFLAGS += -DCIRCUITPY_ESP32_CAMERA=$(CIRCUITPY_ESP32_CAMERA)

CIRCUITPY__EVE ?= 0
CFLAGS += -DCIRCUITPY__EVE=$(CIRCUITPY__EVE)

CIRCUITPY_FLOPPYIO ?= 0
CFLAGS += -DCIRCUITPY_FLOPPYIO=$(CIRCUITPY_FLOPPYIO)

CIRCUITPY_FREQUENCYIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_FREQUENCYIO=$(CIRCUITPY_FREQUENCYIO)

CIRCUITPY_FUTURE ?= 1
CFLAGS += -DCIRCUITPY_FUTURE=$(CIRCUITPY_FUTURE)

CIRCUITPY_GETPASS ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_GETPASS=$(CIRCUITPY_GETPASS)

ifeq ($(CIRCUITPY_DISPLAYIO),1)
CIRCUITPY_GIFIO ?= $(CIRCUITPY_CAMERA)
else
CIRCUITPY_GIFIO ?= 0
endif
CFLAGS += -DCIRCUITPY_GIFIO=$(CIRCUITPY_GIFIO)

CIRCUITPY_GNSS ?= 0
CFLAGS += -DCIRCUITPY_GNSS=$(CIRCUITPY_GNSS)

CIRCUITPY_HASHLIB ?= $(CIRCUITPY_WEB_WORKFLOW)
CFLAGS += -DCIRCUITPY_HASHLIB=$(CIRCUITPY_HASHLIB)

CIRCUITPY_I2CTARGET ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_I2CTARGET=$(CIRCUITPY_I2CTARGET)

CIRCUITPY_IMAGECAPTURE ?= 0
CFLAGS += -DCIRCUITPY_IMAGECAPTURE=$(CIRCUITPY_IMAGECAPTURE)

CIRCUITPY_IPADDRESS ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_IPADDRESS=$(CIRCUITPY_IPADDRESS)

CIRCUITPY_IS31FL3741 ?= 0
CFLAGS += -DCIRCUITPY_IS31FL3741=$(CIRCUITPY_IS31FL3741)

CIRCUITPY_JSON ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_JSON=$(CIRCUITPY_JSON)

CIRCUITPY_KEYPAD ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_KEYPAD=$(CIRCUITPY_KEYPAD)

CIRCUITPY_MATH ?= 1
CFLAGS += -DCIRCUITPY_MATH=$(CIRCUITPY_MATH)

CIRCUITPY_MEMORYMONITOR ?= 0
CFLAGS += -DCIRCUITPY_MEMORYMONITOR=$(CIRCUITPY_MEMORYMONITOR)

CIRCUITPY_MICROCONTROLLER ?= 1
CFLAGS += -DCIRCUITPY_MICROCONTROLLER=$(CIRCUITPY_MICROCONTROLLER)

CIRCUITPY_MDNS ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_MDNS=$(CIRCUITPY_MDNS)

CIRCUITPY_MSGPACK ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_MSGPACK=$(CIRCUITPY_MSGPACK)

CIRCUITPY_NEOPIXEL_WRITE ?= 1
CFLAGS += -DCIRCUITPY_NEOPIXEL_WRITE=$(CIRCUITPY_NEOPIXEL_WRITE)

CIRCUITPY_AULITECH_NEUTONML ?= 1
CFLAGS += -DAULITECH_NEUTONML=$(AULITECH_NEUTONML)

CIRCUITPY_NVM ?= 1
CFLAGS += -DCIRCUITPY_NVM=$(CIRCUITPY_NVM)

CIRCUITPY_ONEWIREIO ?= $(CIRCUITPY_BUSIO)
CFLAGS += -DCIRCUITPY_ONEWIREIO=$(CIRCUITPY_ONEWIREIO)

CIRCUITPY_OS ?= 1
CFLAGS += -DCIRCUITPY_OS=$(CIRCUITPY_OS)

CIRCUITPY_PEW ?= 0
CFLAGS += -DCIRCUITPY_PEW=$(CIRCUITPY_PEW)

CIRCUITPY_PIXELBUF ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_PIXELBUF=$(CIRCUITPY_PIXELBUF)

# Only for SAMD boards for the moment
CIRCUITPY_PS2IO ?= 0
CFLAGS += -DCIRCUITPY_PS2IO=$(CIRCUITPY_PS2IO)

CIRCUITPY_PULSEIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_PULSEIO=$(CIRCUITPY_PULSEIO)

CIRCUITPY_PWMIO ?= 1
CFLAGS += -DCIRCUITPY_PWMIO=$(CIRCUITPY_PWMIO)

CIRCUITPY_QRIO ?= $(CIRCUITPY_IMAGECAPTURE)
CFLAGS += -DCIRCUITPY_QRIO=$(CIRCUITPY_QRIO)

CIRCUITPY_RAINBOWIO ?= 1
CFLAGS += -DCIRCUITPY_RAINBOWIO=$(CIRCUITPY_RAINBOWIO)

CIRCUITPY_RANDOM ?= 1
CFLAGS += -DCIRCUITPY_RANDOM=$(CIRCUITPY_RANDOM)

CIRCUITPY_RE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_RE=$(CIRCUITPY_RE)

# Should busio.I2C() check for pullups?
# Some boards in combination with certain peripherals may not want this.
CIRCUITPY_REQUIRE_I2C_PULLUPS ?= 1
CFLAGS += -DCIRCUITPY_REQUIRE_I2C_PULLUPS=$(CIRCUITPY_REQUIRE_I2C_PULLUPS)

# CIRCUITPY_RP2PIO is handled in the raspberrypi tree.
# Only for rp2 chips.
# Assume not a rp2 build.
CIRCUITPY_RP2PIO ?= 0
CFLAGS += -DCIRCUITPY_RP2PIO=$(CIRCUITPY_RP2PIO)

CIRCUITPY_RGBMATRIX ?= 0
CFLAGS += -DCIRCUITPY_RGBMATRIX=$(CIRCUITPY_RGBMATRIX)

CIRCUITPY_ROTARYIO ?= 1
CFLAGS += -DCIRCUITPY_ROTARYIO=$(CIRCUITPY_ROTARYIO)

CIRCUITPY_ROTARYIO_SOFTENCODER ?= 0
CFLAGS += -DCIRCUITPY_ROTARYIO_SOFTENCODER=$(CIRCUITPY_ROTARYIO_SOFTENCODER)

CIRCUITPY_RTC ?= 1
CFLAGS += -DCIRCUITPY_RTC=$(CIRCUITPY_RTC)

# CIRCUITPY_SAMD is handled in the atmel-samd tree.
# Only for SAMD chips.
# Assume not a SAMD build.
CIRCUITPY_SAMD ?= 0
CFLAGS += -DCIRCUITPY_SAMD=$(CIRCUITPY_SAMD)

CIRCUITPY_SDCARDIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_SDCARDIO=$(CIRCUITPY_SDCARDIO)

CIRCUITPY_SDIOIO ?= 0
CFLAGS += -DCIRCUITPY_SDIOIO=$(CIRCUITPY_SDIOIO)

CIRCUITPY_SERIAL_BLE ?= 0
CFLAGS += -DCIRCUITPY_SERIAL_BLE=$(CIRCUITPY_SERIAL_BLE)

CIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY?= 0
CFLAGS += -DCIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY=$(CIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY)

CIRCUITPY_SHARPDISPLAY ?= $(CIRCUITPY_FRAMEBUFFERIO)
CFLAGS += -DCIRCUITPY_SHARPDISPLAY=$(CIRCUITPY_SHARPDISPLAY)

CIRCUITPY_SOCKETPOOL ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_SOCKETPOOL=$(CIRCUITPY_SOCKETPOOL)

CIRCUITPY_SSL ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_SSL=$(CIRCUITPY_SSL)

# Currently always off.
CIRCUITPY_STAGE ?= 0
CFLAGS += -DCIRCUITPY_STAGE=$(CIRCUITPY_STAGE)

CIRCUITPY_STATUS_BAR ?= 1
CFLAGS += -DCIRCUITPY_STATUS_BAR=$(CIRCUITPY_STATUS_BAR)

CIRCUITPY_STORAGE ?= 1
CFLAGS += -DCIRCUITPY_STORAGE=$(CIRCUITPY_STORAGE)

CIRCUITPY_STRUCT ?= 1
CFLAGS += -DCIRCUITPY_STRUCT=$(CIRCUITPY_STRUCT)

CIRCUITPY_SUPERVISOR ?= 1
CFLAGS += -DCIRCUITPY_SUPERVISOR=$(CIRCUITPY_SUPERVISOR)

CIRCUITPY_SYNTHIO ?= $(CIRCUITPY_AUDIOCORE)
CFLAGS += -DCIRCUITPY_SYNTHIO=$(CIRCUITPY_SYNTHIO)

CIRCUITPY_TERMINALIO ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_TERMINALIO=$(CIRCUITPY_TERMINALIO)

ifeq ($(CIRCUITPY_DISPLAYIO),1)
CIRCUITPY_FONTIO ?= $(CIRCUITPY_TERMINALIO)
endif
CFLAGS += -DCIRCUITPY_FONTIO=$(CIRCUITPY_FONTIO)

CIRCUITPY_TIME ?= 1
CFLAGS += -DCIRCUITPY_TIME=$(CIRCUITPY_TIME)

# touchio might be native or generic. See circuitpy_defns.mk.
CIRCUITPY_TOUCHIO_USE_NATIVE ?= 0
CFLAGS += -DCIRCUITPY_TOUCHIO_USE_NATIVE=$(CIRCUITPY_TOUCHIO_USE_NATIVE)

CIRCUITPY_TOUCHIO ?= 1
CFLAGS += -DCIRCUITPY_TOUCHIO=$(CIRCUITPY_TOUCHIO)

CIRCUITPY_TRACEBACK ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_TRACEBACK=$(CIRCUITPY_TRACEBACK)

# For debugging.
CIRCUITPY_UHEAP ?= 0
CFLAGS += -DCIRCUITPY_UHEAP=$(CIRCUITPY_UHEAP)

CIRCUITPY_USB ?= 1
CFLAGS += -DCIRCUITPY_USB=$(CIRCUITPY_USB)

# Compute these value once, so the shell command is not reinvoked many times.
USB_NUM_ENDPOINT_PAIRS_5_OR_GREATER := $(shell expr $(USB_NUM_ENDPOINT_PAIRS) '>=' 5)
USB_NUM_ENDPOINT_PAIRS_8_OR_GREATER := $(shell expr $(USB_NUM_ENDPOINT_PAIRS) '>=' 8)

# Some chips may not support the same number of IN or OUT endpoints as pairs.
# For instance, the ESP32-S2 only supports 5 IN endpoints at once, even though
# it has 7 endpoint pairs.
USB_NUM_IN_ENDPOINTS ?= $(USB_NUM_ENDPOINT_PAIRS)
CFLAGS += -DUSB_NUM_IN_ENDPOINTS=$(USB_NUM_IN_ENDPOINTS)

USB_NUM_OUT_ENDPOINTS ?= $(USB_NUM_ENDPOINT_PAIRS)
CFLAGS += -DUSB_NUM_OUT_ENDPOINTS=$(USB_NUM_OUT_ENDPOINTS)

CIRCUITPY_USB_CDC ?= $(CIRCUITPY_USB)
CFLAGS += -DCIRCUITPY_USB_CDC=$(CIRCUITPY_USB_CDC)
CIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT ?= 1
CFLAGS += -DCIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT=$(CIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT)
CIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT ?= 0
CFLAGS += -DCIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT=$(CIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT)

# HID is available by default, but is not turned on if there are fewer than 5 endpoints.
CIRCUITPY_USB_HID ?= $(CIRCUITPY_USB)
CFLAGS += -DCIRCUITPY_USB_HID=$(CIRCUITPY_USB_HID)
CIRCUITPY_USB_HID_ENABLED_DEFAULT ?= $(USB_NUM_ENDPOINT_PAIRS_5_OR_GREATER)
CFLAGS += -DCIRCUITPY_USB_HID_ENABLED_DEFAULT=$(CIRCUITPY_USB_HID_ENABLED_DEFAULT)

CIRCUITPY_USB_HOST ?= 0
CFLAGS += -DCIRCUITPY_USB_HOST=$(CIRCUITPY_USB_HOST)

# MIDI is available by default, but is not turned on if there are fewer than 8 endpoints.
CIRCUITPY_USB_MIDI ?= $(CIRCUITPY_USB)
CFLAGS += -DCIRCUITPY_USB_MIDI=$(CIRCUITPY_USB_MIDI)
CIRCUITPY_USB_MIDI_ENABLED_DEFAULT ?= $(USB_NUM_ENDPOINT_PAIRS_8_OR_GREATER)
CFLAGS += -DCIRCUITPY_USB_MIDI_ENABLED_DEFAULT=$(CIRCUITPY_USB_MIDI_ENABLED_DEFAULT)

CIRCUITPY_USB_MSC ?= $(CIRCUITPY_USB)
CFLAGS += -DCIRCUITPY_USB_MSC=$(CIRCUITPY_USB_MSC)
CIRCUITPY_USB_MSC_ENABLED_DEFAULT ?= $(CIRCUITPY_USB_MSC)
CFLAGS += -DCIRCUITPY_USB_MSC_ENABLED_DEFAULT=$(CIRCUITPY_USB_MSC_ENABLED_DEFAULT)

# Defaulting this to OFF initially because it has only been tested on a
# limited number of platforms, and the other platforms do not have this
# setting in their mpconfigport.mk and/or mpconfigboard.mk files yet.
CIRCUITPY_USB_VENDOR ?= 0
CFLAGS += -DCIRCUITPY_USB_VENDOR=$(CIRCUITPY_USB_VENDOR)

ifndef USB_NUM_ENDPOINT_PAIRS
$(error "USB_NUM_ENDPOINT_PAIRS (number of USB endpoint pairs)must be defined")
endif
CFLAGS += -DUSB_NUM_ENDPOINT_PAIRS=$(USB_NUM_ENDPOINT_PAIRS)

# For debugging.
CIRCUITPY_USTACK ?= 0
CFLAGS += -DCIRCUITPY_USTACK=$(CIRCUITPY_USTACK)

# for decompressing utlities
CIRCUITPY_ZLIB ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ZLIB=$(CIRCUITPY_ZLIB)

# ulab numerics library
CIRCUITPY_ULAB ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ULAB=$(CIRCUITPY_ULAB)

# CIRCUITPY_VIDEOCORE is handled in the broadcom tree.
# Only for Broadcom chips.
# Assume not a Broadcom build.
CIRCUITPY_VIDEOCORE ?= 0
CFLAGS += -DCIRCUITPY_VIDEOCORE=$(CIRCUITPY_VIDEOCORE)

# watchdog hardware support
CIRCUITPY_WATCHDOG ?= 0
CFLAGS += -DCIRCUITPY_WATCHDOG=$(CIRCUITPY_WATCHDOG)

CIRCUITPY_WIFI ?= 0
CFLAGS += -DCIRCUITPY_WIFI=$(CIRCUITPY_WIFI)

CIRCUITPY_WEB_WORKFLOW ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_WEB_WORKFLOW=$(CIRCUITPY_WEB_WORKFLOW)

# tinyusb port tailored configuration
CIRCUITPY_TUSB_MEM_ALIGN ?= 4
CFLAGS += -DCIRCUITPY_TUSB_MEM_ALIGN=$(CIRCUITPY_TUSB_MEM_ALIGN)

CIRCUITPY_TUSB_ATTR_USBRAM ?= ".bss.usbram"
CFLAGS += -DCIRCUITPY_TUSB_ATTR_USBRAM=$(CIRCUITPY_TUSB_ATTR_USBRAM)

# Define an equivalent for MICROPY_LONGINT_IMPL, to pass to $(MPY-TOOL) in py/mkrules.mk
# $(MPY-TOOL) needs to know what kind of longint to use (if any) to freeze long integers.
# This should correspond to the MICROPY_LONGINT_IMPL definition in mpconfigport.h.
#
# Also propagate longint choice from .mk to C. There's no easy string comparison
# in cpp conditionals, so we #define separate names for each.

ifeq ($(LONGINT_IMPL),NONE)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=none
CFLAGS += -DLONGINT_IMPL_NONE
else ifeq ($(LONGINT_IMPL),MPZ)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=mpz
CFLAGS += -DLONGINT_IMPL_MPZ
else ifeq ($(LONGINT_IMPL),LONGLONG)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=longlong
CFLAGS += -DLONGINT_IMPL_LONGLONG
else
$(error LONGINT_IMPL set to surprising value: "$(LONGINT_IMPL)")
endif
MPY_TOOL_FLAGS += $(MPY_TOOL_LONGINT_IMPL)

###
ifeq ($(LONGINT_IMPL),NONE)
else ifeq ($(LONGINT_IMPL),MPZ)
else ifeq ($(LONGINT_IMPL),LONGLONG)
else
$(error LONGINT_IMPL set to surprising value: "$(LONGINT_IMPL)")
endif

PREPROCESS_FROZEN_MODULES = PYTHONPATH=$(TOP)/tools/python-semver $(TOP)/tools/preprocess_frozen_modules.py
ifneq ($(FROZEN_MPY_DIRS),)
$(BUILD)/frozen_mpy: $(FROZEN_MPY_DIRS)
	$(ECHO) FREEZE $(FROZEN_MPY_DIRS)
	$(Q)$(MKDIR) -p $@
	$(Q)$(PREPROCESS_FROZEN_MODULES) -o $@ $(FROZEN_MPY_DIRS)

$(BUILD)/manifest.py: $(BUILD)/frozen_mpy | $(TOP)/py/circuitpy_mpconfig.mk mpconfigport.mk boards/$(BOARD)/mpconfigboard.mk
	$(ECHO) MKMANIFEST $(FROZEN_MPY_DIRS)
	(cd $(BUILD)/frozen_mpy && find * -name \*.py -exec printf 'freeze_as_mpy("frozen_mpy", "%s")\n' {} \; )> $@.tmp && mv -f $@.tmp $@
FROZEN_MANIFEST=$(BUILD)/manifest.py
endif
