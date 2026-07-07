# Experimental JSPI variant (cmake). Mirrors variants/jspi/mpconfigvariant.mk.
#
# Builds with Emscripten's JSPI (Wasm stack switching) rather than Asyncify, so
# the MicroPython entry points (mp_js_do_exec / _async / _import) become
# suspending exports that can yield to the JS event loop mid-execution.
#
# NOTE: JSPI is still experimental in Emscripten and requires a browser/runtime
# with Wasm stack-switching support (and, under Node, an --experimental flag).

# Suspending the stack under JSPI requires a pure-Wasm call stack, so build
# setjmp/longjmp with the Wasm backend at both compile and link time (the
# default emscripten longjmp injects non-promising JS invoke_* frames).
set(MICROPY_PORT_SUPPORT_LONGJMP wasm)
list(APPEND MICROPY_PORT_COMPILE_OPTS -sSUPPORT_LONGJMP=wasm)

# emscripten_scan_registers() needs Asyncify; under JSPI the GC scans the C
# stack only (see main.c MICROPY_GC_SCAN_REGISTERS).
list(APPEND MICROPY_PORT_DEFS MICROPY_GC_SCAN_REGISTERS=0)

list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY=0)
list(APPEND MICROPY_PORT_JSFLAGS -sJSPI=1)
list(APPEND MICROPY_PORT_JSFLAGS "SHELL:-s JSPI_EXPORTS=[\"mp_js_do_exec\",\"mp_js_do_exec_async\",\"mp_js_do_import\"]")
list(APPEND MICROPY_PORT_JSFLAGS -sALLOW_TABLE_GROWTH=1)
list(APPEND MICROPY_PORT_JSFLAGS -sWASM_BIGINT=1)
list(APPEND MICROPY_PORT_JSFLAGS -sALLOW_MEMORY_GROWTH)

# Tell api.js to use the JSPI (direct promising call) invocation path.
list(APPEND MICROPY_PORT_SRC_JS ${MICROPY_PORT_DIR}/async_jspi.js)

# jsffi.run_sync(): block on a JS awaitable by suspending the stack (modjsffi.c).
list(APPEND MICROPY_PORT_DEFS MICROPY_PY_JS_RUN_SYNC=1)
