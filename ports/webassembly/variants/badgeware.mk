# Shared build fragment for the badgeware simulator variants
# (badgeware-tufty2350, and future badgeware-blinky2350 / badgeware-badger2350).
#
# It layers the picovector graphics library and the simulator C modules
# (machine, st7789, input, jsfetch) on top of the base webassembly port. Each
# badge variant's mpconfigvariant.mk includes this and adds its board-specific
# bits (board define, frozen manifest, etc.).

# --- C++ toolchain and flags (picovector is C++) -----------------------------
CXX = em++

# Picovector needs the allocated-size malloc API; we also build with source
# maps for debugging. These go in CFLAGS (not just CXXFLAGS) so the whole build
# is consistent, and are picked up by CXXFLAGS = $(CFLAGS) below.
CFLAGS += -DMICROPY_MALLOC_USES_ALLOCATED_SIZE=1
CFLAGS += -Wno-deprecated-non-prototype
CFLAGS += -gsource-map

# EM_ASM (used by the simulator C modules) requires a GNU C mode.
CSTD = gnu99

LDFLAGS += -specs=nano.specs

# --- async backend ------------------------------------------------------------
# How MicroPython suspends to the JS event loop (cooperative VM-hook yield,
# pause, and the blocking _jsfetch primitive). JSPI (Wasm stack switching) is
# lighter but isn't in mainline Safari yet, so default to Asyncify; switch with
# `make ... BADGEWARE_ASYNC=jspi`. Both feed the same api.js path (see invoke()).
BADGEWARE_ASYNC ?= asyncify

ifeq ($(BADGEWARE_ASYNC),jspi)
# JSPI suspension needs a pure-Wasm call stack, so setjmp/longjmp must use the
# Wasm backend (the default emscripten longjmp injects JS invoke_* frames that
# break the promising stack). -fwasm-exceptions selects it, at compile and link.
CFLAGS += -fwasm-exceptions
CFLAGS += -s SUPPORT_LONGJMP=wasm
LDFLAGS += -fwasm-exceptions
SUPPORT_LONGJMP = wasm
JSFLAGS += -s ASYNCIFY=0
JSFLAGS += -s JSPI=1
JSFLAGS += -s JSPI_EXPORTS=["mp_js_do_exec","mp_js_do_exec_async","mp_js_do_import"]
SRC_JS += async_jspi.js
else ifeq ($(BADGEWARE_ASYNC),asyncify)
# Asyncify instruments the whole program so any function can suspend via
# emscripten_sleep. The cooperative yield can fire at arbitrary Python recursion
# depth, so give the unwind buffer generous headroom (a deep suspend overflowing
# a small ASYNCIFY_STACK_SIZE is the main downside vs JSPI).
JSFLAGS += -s ASYNCIFY=1
JSFLAGS += -s ASYNCIFY_STACK_SIZE=131072
SRC_JS += async_asyncify.js
else
$(error BADGEWARE_ASYNC must be 'jspi' or 'asyncify')
endif

JSFLAGS += -s ALLOW_TABLE_GROWTH=1
JSFLAGS += -s WASM_BIGINT=1

CXXFLAGS = $(CFLAGS) -std=c++17 -Wno-error -fms-extensions -fno-exceptions -fno-unwind-tables -fno-rtti -fno-use-cxa-atexit

# stringToNewUTF8 is used by the _jsfetch primitive.
EXPORTED_RUNTIME_METHODS_EXTRA += ,stringToNewUTF8

# --- picovector library + simulator sources ----------------------------------
# Picovector decoder include paths.
INC += -I$(TOP)/ports/webassembly/picovector/lib/pngdec
INC += -I$(TOP)/ports/webassembly/picovector/lib/jpegdec

# Picovector build options (generic; board-specific defines like -DTUFTY=1 are
# set by the individual variant).
CFLAGS += -DPV_DUAL_CORE=0 -DPV_PROFILE=0

# JS glue for the fetch-backed networking primitive (_jsfetch).
JSFLAGS += --js-library simulator/jsfetch.js

SRC_C += \
	simulator/st7789.c \
	simulator/jsfetch.c \
	picovector/micropython/picovector_bindings.c \
	picovector/lib/pngdec/adler32.c \
	picovector/lib/pngdec/crc32.c \
	picovector/lib/pngdec/infback.c \
	picovector/lib/pngdec/inffast.c \
	picovector/lib/pngdec/inflate.c \
	picovector/lib/pngdec/inftrees.c \
	picovector/lib/pngdec/zutil.c

SRC_CXX += \
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
