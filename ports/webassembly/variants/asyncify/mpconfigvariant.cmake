# Asyncify variant (cmake). Mirrors variants/asyncify/mpconfigvariant.mk.
#
# Asyncify instruments the whole program so any function can suspend via
# emscripten_sleep. The cooperative yield (main.c mp_js_yield) can fire at
# arbitrary Python recursion depth, so give the unwind buffer generous headroom.
list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY)
list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY_STACK_SIZE=131072)

# The auto split heap (mpconfigvariant.h) grows the wasm memory on demand, so it
# must be allowed to grow at runtime.
list(APPEND MICROPY_PORT_JSFLAGS -sALLOW_MEMORY_GROWTH)

# Drive the entry points via ccall({async: true}) (see api.js invoke());
# async_asyncify.js announces the backend to api.js.
list(APPEND MICROPY_PORT_SRC_JS ${MICROPY_PORT_DIR}/async_asyncify.js)

# jsffi.run_sync(): block on a JS awaitable by suspending the stack (modjsffi.c).
list(APPEND MICROPY_PORT_DEFS MICROPY_PY_JS_RUN_SYNC=1)
