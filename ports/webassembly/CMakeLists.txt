# CMake build for the MicroPython webassembly port.
#
# This is an additive alternative to the Makefile in this directory: it reuses
# the shared MicroPython cmake fragments (py/py.cmake, extmod/extmod.cmake,
# py/usermod.cmake, py/mkrules.cmake) the hardware ports use, so the port can
# consume out-of-tree C/C++ user modules via USER_C_MODULES the same way the
# firmware does. Configure it with emscripten's cmake wrapper:
#
#   emcmake cmake -B build-standard -S ports/webassembly [-DMICROPY_VARIANT=asyncify]
#   cmake --build build-standard
#
# The variant is selected with -DMICROPY_VARIANT=<name> (default: standard), or
# an out-of-tree variant directory with -DMICROPY_VARIANT_DIR=<abs path> (the
# cmake analogue of the Makefile's VARIANT_DIR). The build output is
# micropython.mjs, byte-compatible with the Makefile build's output.

cmake_minimum_required(VERSION 3.13)

set(MICROPY_TARGET micropython)
get_filename_component(MICROPY_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(MICROPY_PORT_DIR ${CMAKE_CURRENT_LIST_DIR})

# --- Variant resolution (mirror of the Makefile VARIANT / VARIANT_DIR logic) --
if(MICROPY_VARIANT_DIR)
    # Out-of-tree variant: take the directory as given, derive the name from it.
    get_filename_component(MICROPY_VARIANT_DIR "${MICROPY_VARIANT_DIR}" ABSOLUTE)
    if(NOT MICROPY_VARIANT)
        get_filename_component(MICROPY_VARIANT "${MICROPY_VARIANT_DIR}" NAME)
    endif()
else()
    if(NOT MICROPY_VARIANT)
        set(MICROPY_VARIANT standard)
    endif()
    set(MICROPY_VARIANT_DIR ${MICROPY_PORT_DIR}/variants/${MICROPY_VARIANT})
endif()
if(NOT EXISTS ${MICROPY_VARIANT_DIR}/mpconfigvariant.cmake)
    message(FATAL_ERROR "Invalid MICROPY_VARIANT: ${MICROPY_VARIANT_DIR}/mpconfigvariant.cmake not found")
endif()
message("Building webassembly variant: ${MICROPY_VARIANT} (${MICROPY_VARIANT_DIR})")

# --- Per-variant contribution variables --------------------------------------
# A variant's mpconfigvariant.cmake appends to these; the base applies them to
# the target below. This mirrors how the Makefile variants add to CFLAGS /
# JSFLAGS / SRC_C / SRC_JS. All paths must be absolute (the variant may be
# out-of-tree).
#   MICROPY_PORT_SRC_C            extra C/C++ sources compiled into the firmware
#   MICROPY_PORT_SRC_JS           JS files appended to micropython.mjs after link
#   MICROPY_PORT_DEFS             extra compile definitions
#   MICROPY_PORT_COMPILE_OPTS     extra compile options (e.g. -fwasm-exceptions)
#   MICROPY_PORT_JSFLAGS          extra emscripten link settings (-sNAME[=VALUE])
#   MICROPY_PORT_LINK_OPTS        extra raw link options (e.g. -fwasm-exceptions)
#   MICROPY_PORT_JS_LIBRARY       extra --js-library entries (--js-library;<path>)
#   MICROPY_PORT_RUNTIME_METHODS  extra EXPORTED_RUNTIME_METHODS names
#   MICROPY_PORT_SUPPORT_LONGJMP  emscripten (default) | wasm
#   MICROPY_FROZEN_MANIFEST       overrides the default frozen manifest
set(MICROPY_PORT_SUPPORT_LONGJMP emscripten)
include(${MICROPY_VARIANT_DIR}/mpconfigvariant.cmake)

# --- Shared component fragments ----------------------------------------------
include(${MICROPY_DIR}/py/py.cmake)
include(${MICROPY_DIR}/extmod/extmod.cmake)

project(${MICROPY_TARGET} C CXX)

include(${MICROPY_DIR}/py/usermod.cmake)

add_executable(${MICROPY_TARGET})

# --- Sources -----------------------------------------------------------------
set(MICROPY_QSTRDEFS_PORT ${MICROPY_PORT_DIR}/qstrdefsport.h)

set(MICROPY_SOURCE_SHARED
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
)

set(MICROPY_SOURCE_PORT
    ${MICROPY_PORT_DIR}/lexer_dedent.c
    ${MICROPY_PORT_DIR}/main.c
    ${MICROPY_PORT_DIR}/modjs.c
    ${MICROPY_PORT_DIR}/modjsffi.c
    ${MICROPY_PORT_DIR}/mphalport.c
    ${MICROPY_PORT_DIR}/objjsproxy.c
    ${MICROPY_PORT_DIR}/proxy_c.c
    ${MICROPY_PORT_SRC_C}
)

set(MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_PORT}
)

# Default frozen manifest, overridable by the variant (set before this include).
if(NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/variants/manifest.py)
endif()

target_sources(${MICROPY_TARGET} PRIVATE
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_PORT}
)

target_link_libraries(${MICROPY_TARGET} usermod)

target_include_directories(${MICROPY_TARGET} PRIVATE
    ${MICROPY_INC_CORE}
    ${MICROPY_INC_USERMOD}
    ${MICROPY_PORT_DIR}
    ${MICROPY_VARIANT_DIR}
    ${CMAKE_BINARY_DIR}
)

target_compile_definitions(${MICROPY_TARGET} PRIVATE
    NDEBUG
    # extmod/modvfs.c includes extmod/vfs_fat.h -> lib/oofatfs/ff.h
    # unconditionally, so FatFS's config header must resolve even though this
    # port builds with MICROPY_VFS_FAT off (matches extmod.mk CFLAGS_THIRDPARTY).
    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
    ${MICROPY_PORT_DEFS}
)

# Matches the Makefile CFLAGS (-std=c99 only for C so C++ user modules keep
# their own standard).
target_compile_options(${MICROPY_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=c99>
    -Wall
    -Werror
    -Wdouble-promotion
    -Wfloat-conversion
    -Os
    ${MICROPY_PORT_COMPILE_OPTS}
)

# --- Emscripten link layer (translation of the Makefile JSFLAGS) -------------
set(MICROPY_EXPORTED_FUNCTIONS
    "_free,_malloc,_mp_js_init,_mp_js_repl_init,_mp_js_repl_process_char,_mp_hal_get_interrupt_char,_mp_sched_keyboard_interrupt,_mp_js_do_exec,_mp_js_do_exec_async,_mp_js_do_import,_mp_js_register_js_module,_proxy_c_free_obj,_proxy_c_init,_proxy_c_to_js_call,_proxy_c_to_js_delete_attr,_proxy_c_to_js_dir,_proxy_c_to_js_get_iter,_proxy_c_to_js_get_type_and_data,_proxy_c_to_js_has_attr,_proxy_c_to_js_iternext,_proxy_c_to_js_lookup_attr,_proxy_c_to_js_resume,_proxy_c_to_js_store_attr,_proxy_convert_mp_to_js_obj_cside")

set(MICROPY_RUNTIME_METHODS
    "ccall,cwrap,FS,HEAPU8,PATH,PATH_FS,UTF8ToString,getValue,lengthBytesUTF8,setValue,stringToUTF8")
foreach(_m ${MICROPY_PORT_RUNTIME_METHODS})
    string(APPEND MICROPY_RUNTIME_METHODS ",${_m}")
endforeach()

target_link_options(${MICROPY_TARGET} PRIVATE
    "SHELL:-s EXPORTED_FUNCTIONS=${MICROPY_EXPORTED_FUNCTIONS}"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=${MICROPY_RUNTIME_METHODS}"
    "SHELL:-s SUPPORT_LONGJMP=${MICROPY_PORT_SUPPORT_LONGJMP}"
    "SHELL:-s MODULARIZE"
    "SHELL:-s EXPORT_NAME=_createMicroPythonModule"
    # SHELL: keeps each --js-library grouped with its path; without it CMake
    # deduplicates the repeated --js-library flag and the extra libraries get
    # passed to wasm-ld as bare input files.
    "SHELL:--js-library ${MICROPY_PORT_DIR}/library.js"
    ${MICROPY_PORT_JS_LIBRARY}
    ${MICROPY_PORT_JSFLAGS}
    ${MICROPY_PORT_LINK_OPTS}
)

# Output an ES module (micropython.mjs); the .mjs suffix makes emscripten emit
# EXPORT_ES6 automatically, as the Makefile relies on.
set_target_properties(${MICROPY_TARGET} PROPERTIES
    OUTPUT_NAME micropython
    SUFFIX ".mjs"
)

# --- Frozen manifest / qstr / moduledefs generation --------------------------
# (mkrules.cmake reads the target's include dirs + defines set above.)
include(${MICROPY_DIR}/py/mkrules.cmake)

# --- Post-link: append the port/variant JS glue to micropython.mjs -----------
# Reproduces the Makefile's `cat $(SRC_JS) >> micropython.mjs`. The variant JS
# (async_asyncify.js / async_jspi.js) comes first, matching the Makefile order
# (variant mpconfigvariant.mk is included before the base SRC_JS additions).
set(MICROPY_SRC_JS
    ${MICROPY_PORT_SRC_JS}
    ${MICROPY_PORT_DIR}/api.js
    ${MICROPY_PORT_DIR}/objpyproxy.js
    ${MICROPY_PORT_DIR}/proxy_js.js
)
add_custom_command(TARGET ${MICROPY_TARGET} POST_BUILD
    COMMAND cat ${MICROPY_SRC_JS} >> $<TARGET_FILE:${MICROPY_TARGET}>
    VERBATIM
    COMMAND_EXPAND_LISTS
)
