# This is the default variant when you `make` the Unix port.

CFLAGS += \
	-I$(TOP)/lib/fenster \

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lgdi32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS += -framework Cocoa -framework AudioToolbox
	else
		LDFLAGS += -lX11
	endif
endif

FROZEN_MANIFEST ?= $(VARIANT_DIR)/manifest.py
SRC_C += $(TOP)/extmod/modfenster.c

GIT_SUBMODULES += lib/fenster