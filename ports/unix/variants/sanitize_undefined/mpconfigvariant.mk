include variants/coverage/mpconfigvariant.mk

SANITIZER ?= -fsanitize=undefined -DMICROPY_SANITIZE
CFLAGS += $(SANITIZER)
LDFLAGS += $(SANITIZER)
