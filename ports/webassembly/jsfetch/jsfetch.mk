# _jsfetch: a fetch-backed blocking HTTP primitive for the webassembly port.
#
# It performs HTTP(S) requests via the browser's fetch() and suspends the WASM
# stack (emscripten_sleep) until they complete, so it only works in a build that
# can suspend — i.e. a JSPI or Asyncify variant. Enable it by including this
# fragment from such a variant's mpconfigvariant.mk:
#
#     include jsfetch/jsfetch.mk
#
# which compiles the `_jsfetch` C module (gated on MICROPY_PY_JSFETCH) and wires
# in its JS half.
CFLAGS += -DMICROPY_PY_JSFETCH=1
SRC_C += jsfetch/jsfetch.c
JSFLAGS += --js-library jsfetch/jsfetch.js

# mp_js_fetch_str() returns a malloc'd UTF-8 string via stringToNewUTF8.
EXPORTED_RUNTIME_METHODS_EXTRA += ,stringToNewUTF8
