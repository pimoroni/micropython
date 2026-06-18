################################################################################
# Initial setup of Makefile environment.

# Select the variant to build for:
ifdef VARIANT_DIR
# Custom variant path - remove trailing slash and get the final component of
# the path as the variant name.
VARIANT ?= $(notdir $(VARIANT_DIR:/=))
else
# If not given on the command line, then default to standard.
VARIANT ?= standard
VARIANT_DIR ?= variants/$(VARIANT)
endif

ifeq ($(wildcard $(VARIANT_DIR)/.),)
$(error Invalid VARIANT specified: $(VARIANT_DIR))
endif

# If the build directory is not given, make it reflect the variant name.
BUILD ?= build-$(VARIANT)

include ../../py/mkenv.mk
include $(VARIANT_DIR)/mpconfigvariant.mk

# Use the default frozen manifest, variants may override this.
FROZEN_MANIFEST ?= variants/manifest.py

# Qstr definitions (must come before including py.mk).
QSTR_DEFS = qstrdefsport.h

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

################################################################################
# Project specific settings and compiler/linker flags.

CC = emcc
CXX = em++
LD = emcc
NODE ?= node
TERSER ?= npx terser

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(VARIANT_DIR)
INC += -I$(TOP)/ports/webassembly/picovector/lib/pngdec
INC += -I$(TOP)/ports/webassembly/picovector/lib/jpegdec

CXXFLAGS = $(CFLAGS) -DMICROPY_MALLOC_USES_ALLOCATED_SIZE=1 -fwasm-exceptions

CFLAGS += -std=gnu99 -Wall -Werror -Wdouble-promotion -Wfloat-conversion -Wno-deprecated-non-prototype -DMICROPY_MALLOC_USES_ALLOCATED_SIZE=1
CFLAGS += -O2 -DNDEBUG -fwasm-exceptions
CFLAGS += -gsource-map
CFLAGS += $(INC)

LDFLAGS += -specs=nano.specs -fwasm-exceptions
CXXFLAGS += $(INC) -std=c++17 -Wno-error -fms-extensions -fno-exceptions -fno-unwind-tables -fno-rtti -fno-use-cxa-atexit
CXXFLAGS += -DPV_DUAL_CORE=0 -DPV_PROFILE=0 -DTUFTY=1

EXPORTED_FUNCTIONS_EXTRA += ,\
	_mp_js_do_exec,\
	_mp_js_do_exec_async,\
	_mp_js_do_import,\
	_mp_js_register_js_module,\
	_proxy_c_free_obj,\
	_proxy_c_init,\
	_proxy_c_to_js_call,\
	_proxy_c_to_js_delete_attr,\
	_proxy_c_to_js_dir,\
	_proxy_c_to_js_get_array,\
	_proxy_c_to_js_get_dict,\
	_proxy_c_to_js_get_iter,\
	_proxy_c_to_js_get_type,\
	_proxy_c_to_js_has_attr,\
	_proxy_c_to_js_iternext,\
	_proxy_c_to_js_lookup_attr,\
	_proxy_c_to_js_resume,\
	_proxy_c_to_js_store_attr,\
	_proxy_convert_mp_to_js_obj_cside

EXPORTED_RUNTIME_METHODS_EXTRA += ,\
	PATH,\
	PATH_FS,\
	stringToNewUTF8,\
	UTF8ToString,\
	getValue,\
	lengthBytesUTF8,\
	setValue,\
	stringToUTF8,\
	HEAPU8

JSFLAGS += -s EXPORTED_FUNCTIONS="\
	_free,\
	_malloc,\
	_mp_js_init,\
	_mp_js_repl_init,\
	_mp_js_repl_process_char,\
	_mp_hal_get_interrupt_char,\
	_mp_sched_keyboard_interrupt$(EXPORTED_FUNCTIONS_EXTRA)"
JSFLAGS += -s EXPORTED_RUNTIME_METHODS="\
	ccall,\
	cwrap,\
	FS$(EXPORTED_RUNTIME_METHODS_EXTRA)"
JSFLAGS += --js-library library.js
JSFLAGS += -s SUPPORT_LONGJMP=wasm
JSFLAGS += -s MODULARIZE -s EXPORT_NAME=_createMicroPythonModule
JSFLAGS += -s ASYNCIFY=0
JSFLAGS += -s JSPI=1
JSFLAGS += -s JSPI_EXPORTS=["mp_js_do_exec","mp_js_do_exec_async","mp_js_do_import"]
JSFLAGS += -s ALLOW_TABLE_GROWTH=1
JSFLAGS += -s WASM_BIGINT=1
#JSFLAGS += -s ASYNCIFY_STACK_SIZE=131072
#JSFLAGS += -s STACK_SIZE=65536 -s ALLOW_MEMORY_GROWTH=1
#JSFLAGS += -s ASYNCIFY_ONLY=['mp_module_simulator_update']

################################################################################
# Source files and libraries.

SRC_SHARED = $(addprefix shared/,\
	runtime/interrupt_char.c \
	runtime/stdout_helpers.c \
	runtime/pyexec.c \
	readline/readline.c \
	timeutils/timeutils.c \
	)

SRC_C += \
	lexer_dedent.c \
	main.c \
	modjs.c \
	modjsffi.c \
	mphalport.c \
	objjsproxy.c \
	proxy_c.c \
	simulator/st7789.c \
	picovector/micropython/picovector_bindings.c \
    picovector/lib/pngdec/adler32.c \
    picovector/lib/pngdec/crc32.c \
    picovector/lib/pngdec/infback.c \
    picovector/lib/pngdec/inffast.c \
    picovector/lib/pngdec/inflate.c \
    picovector/lib/pngdec/inftrees.c \
    picovector/lib/pngdec/zutil.c

SRC_CXX = \
	picovector/micropython/picovector.cpp \
	picovector/picovector.cpp \
	picovector/shape.cpp \
	picovector/font.cpp \
	picovector/pixel_font.cpp \
	picovector/image.cpp \
	picovector/brush.cpp \
	picovector/color.cpp \
	picovector/primitive.cpp \
	picovector/algorithms/geometry.cpp \
	picovector/algorithms/dda.cpp \
	picovector/brushes/pattern.cpp \
	picovector/brushes/color.cpp \
	picovector/brushes/image.cpp \
	picovector/brushes/gradient.cpp \
	picovector/brushes/pixelate.cpp \
	picovector/brushes/blur.cpp \
	picovector/brushes/brightness.cpp \
	picovector/filters/blur.cpp \
	picovector/filters/dither.cpp \
	picovector/filters/monochrome.cpp \
	picovector/filters/onebit.cpp \
	picovector/micropython/brush.cpp \
	picovector/micropython/color.cpp \
	picovector/micropython/font.cpp \
	picovector/micropython/image_jpeg.cpp \
	picovector/micropython/image_png.cpp \
	picovector/micropython/image.cpp \
	picovector/micropython/mat3.cpp \
	picovector/micropython/pixel_font.cpp \
	picovector/micropython/shape.cpp \
	picovector/micropython/rect.cpp \
	picovector/micropython/vec2.cpp \
	picovector/micropython/algorithm.cpp \
	simulator/machine.cpp \
	simulator/input.cpp \
	picovector/lib/pngdec/PNGdec.cpp \
	picovector/lib/jpegdec/JPEGdec.cpp

# List of sources for qstr extraction.
SRC_QSTR += $(SRC_C) $(SRC_CXX) $(SRC_SHARED)

SRC_JS += \
	api.js \
	objpyproxy.js \
	proxy_js.js \

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_SHARED:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))

################################################################################
# Main targets.

.PHONY: all repl min test test//% test_min

all: $(BUILD)/micropython.mjs

$(BUILD)/micropython.mjs: $(OBJ) library.js $(SRC_JS)
	$(ECHO) "LINK $@"
	$(Q)emcc $(LDFLAGS) -o $@ $(OBJ) $(JSFLAGS)
	$(Q)cat $(SRC_JS) >> $@

$(BUILD)/micropython.min.mjs: $(BUILD)/micropython.mjs
	$(TERSER) $< --compress --module -o $@

repl: $(BUILD)/micropython.mjs
	$(NODE) $<

min: $(BUILD)/micropython.min.mjs

test: $(BUILD)/micropython.mjs $(TOP)/tests/run-tests.py
	cd $(TOP)/tests && MICROPY_MICROPYTHON_MJS=../ports/webassembly/$< ./run-tests.py -t webassembly

test//%: $(BUILD)/micropython.mjs $(TOP)/tests/run-tests.py
	cd $(TOP)/tests && MICROPY_MICROPYTHON_MJS=../ports/webassembly/$< ./run-tests.py -t webassembly -i "$*"

test_min: $(BUILD)/micropython.min.mjs $(TOP)/tests/run-tests.py
	cd $(TOP)/tests && MICROPY_MICROPYTHON_MJS=../ports/webassembly/$< ./run-tests.py -t webassembly

################################################################################
# Remaining make rules.

include $(TOP)/py/mkrules.mk
