define check-configurable
ifneq "$(words $($1))" "1"
$$(error $1: Wrong format (more than one word))
endif

ifneq "$$(sort $($1))" "$($1)"
$$(error $1: Wrong format (whitespace? comment on same line?))
endif

endef
$(foreach c,$(configurables), $(eval $(call check-configurable,$(c))))

include $(TOP)/py/mkrules.mk
