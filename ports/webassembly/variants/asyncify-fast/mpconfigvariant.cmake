# asyncify-fast variant (cmake). Mirrors variants/asyncify-fast/mpconfigvariant.mk.
#
# The asyncify variant, but with the Asyncify instrumentation cut from whole
# program down to a curated set of functions, for a much smaller and ~2.5x
# faster build. Reuse the asyncify configuration, then narrow it.
#
# EXPERIMENTAL: the ASYNCIFY_ADD list (asyncify_add.txt) must stay complete -- a
# suspend reachable through an indirect call that is not listed corrupts the
# Asyncify unwind/rewind silently. tests/ports/webassembly/asyncify_fast.mjs
# guards this by forcing a suspend through every dispatch path.
include(${MICROPY_PORT_DIR}/variants/asyncify/mpconfigvariant.cmake)

# Turn off the conservative "every indirect call may suspend" assumption and
# re-add just the indirect-dispatch sites that genuinely sit above a suspend
# (the VM call machinery + protocol slots, in asyncify_add.txt).
list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY_IGNORE_INDIRECT=1)
list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY_ADD=@${MICROPY_PORT_DIR}/asyncify_add.txt)

# With indirect auto-detection off, MicroPython's NLR setjmp/longjmp must use the
# Wasm exception/longjmp backend: the default emscripten longjmp emits invoke_*
# JS trampolines that sit on the stack during a suspend and abort once they are
# no longer auto-instrumented.
list(APPEND MICROPY_PORT_COMPILE_OPTS -fwasm-exceptions -sSUPPORT_LONGJMP=wasm)
list(APPEND MICROPY_PORT_LINK_OPTS -fwasm-exceptions)
set(MICROPY_PORT_SUPPORT_LONGJMP wasm)
