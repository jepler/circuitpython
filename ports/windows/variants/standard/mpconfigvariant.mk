# This is the default variant when you `make` the Windows port.

ifneq ($(DISABLE_PLUGIN),1)
# Enable format string checking
include $(TOP)/py/fmtplugin.mk
endif
