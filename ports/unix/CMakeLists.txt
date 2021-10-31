cmake_minimum_required(VERSION 3.12)

if(NOT MICROPY_VARIANT)
    set(MICROPY_VARIANT standard)
endif()

# Set build type to reduce firmware size
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

# Set main target and component locations
set(MICROPY_TARGET firmware)
get_filename_component(MICROPY_DIR "../.." ABSOLUTE)

# Set the location of this port's directory.
set(MICROPY_PORT_DIR ${CMAKE_SOURCE_DIR})


# Set the board directory and check that it exists.
if(NOT MICROPY_VARIANT_DIR)
    set(MICROPY_VARIANT_DIR ${MICROPY_PORT_DIR}/variants/${MICROPY_VARIANT})
endif()
if(NOT EXISTS ${MICROPY_VARIANT_DIR}/mpconfigvariant.cmake)
    message(FATAL_ERROR "Invalid MICROPY_VARIANT specified: ${MICROPY_VARIANT}")
endif()

# Include board config
include(${MICROPY_VARIANT_DIR}/mpconfigvariant.cmake) 

# Include component cmake fragments
include(${MICROPY_DIR}/py/py.cmake)
include(${MICROPY_DIR}/extmod/extmod.cmake)

# Define the top-level project
project(${MICROPY_TARGET})

include(${MICROPY_DIR}/py/usermod.cmake)

add_executable(${MICROPY_TARGET})

if(NOT SDL2_LIBRARIES OR NOT SDL2_INCLUDE_DIRS)
    find_package(SDL2 REQUIRED)
endif()

# If SDL2 was built using CMake, the generated configuration files define SDL2::* targets instead of the SDL2_* variables
if(TARGET SDL2::SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
# handle SDL2 built with only a static library
elseif(TARGET SDL2::SDL2-static)
    set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2-static)
else()
    target_include_directories(${MICROPY_TARGET}
        PRIVATE	${SDL2_INCLUDE_DIRS}
    )
endif()

target_include_directories(${MICROPY_TARGET} PRIVATE ${MICROPY_VARIANT_DIR})

target_compile_definitions(${MICROPY_TARGET} PRIVATE
    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
)

set(MICROPY_QSTRDEFS_PORT
    ${PROJECT_SOURCE_DIR}/qstrdefsport.h
)

set(MICROPY_SOURCE_LIB
    ${MICROPY_DIR}/shared/runtime/gchelper_generic.c
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
    ${MICROPY_DIR}/extmod/machine_pinbase.c
)

set_source_files_properties(
    ${MICROPY_DIR}/shared/runtime/gchelper_native.c
    PROPERTIES
    COMPILE_OPTIONS "-Wno-error=pointer-to-int-cast"
)

set(MICROPY_SOURCE_PORT
    main.c
    gccollect.c
    unix_mphal.c
    mpthreadport.c
    input.c
    modmachine.c
    modos.c
    moduos_vfs.c
    modtime.c
    moduselect.c
    alloc.c
    fatfs_port.c
    mpbthciport.c
    mpbtstackport_common.c
    mpbtstackport_h4.c
    mpbtstackport_usb.c
    mpnimbleport.c
)

set(MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_DIR}/shared/runtime/mpirq.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
    ${PROJECT_SOURCE_DIR}/main.c
    ${PROJECT_SOURCE_DIR}/gccollect.c
    ${PROJECT_SOURCE_DIR}/unix_mphal.c
    ${PROJECT_SOURCE_DIR}/mpthreadport.c
    ${PROJECT_SOURCE_DIR}/input.c
    ${PROJECT_SOURCE_DIR}/modmachine.c
    ${PROJECT_SOURCE_DIR}/modos.c
    ${PROJECT_SOURCE_DIR}/moduos_vfs.c
    ${PROJECT_SOURCE_DIR}/modtime.c
    ${PROJECT_SOURCE_DIR}/moduselect.c
    ${PROJECT_SOURCE_DIR}/alloc.c
    ${PROJECT_SOURCE_DIR}/fatfs_port.c
    ${PROJECT_SOURCE_DIR}/mpbthciport.c
    ${PROJECT_SOURCE_DIR}/mpbtstackport_common.c
    ${PROJECT_SOURCE_DIR}/mpbtstackport_h4.c
    ${PROJECT_SOURCE_DIR}/mpbtstackport_usb.c
    ${PROJECT_SOURCE_DIR}/mpnimbleport.c
)

if(MICROPY_PY_BLUETOOTH)
    list(APPEND MICROPY_SOURCE_PORT mpbthciport.c)
    target_compile_definitions(${MICROPY_TARGET} PRIVATE
        MICROPY_PY_BLUETOOTH=1
        MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE=1
        MICROPY_PY_BLUETOOTH_ENABLE_PAIRING_BONDING=1
        MICROPY_PY_BLUETOOTH_ENABLE_L2CAP_CHANNELS=1
    )
endif()

if(MICROPY_BLUETOOTH_NIMBLE)
    list(APPEND MICROPY_SOURCE_PORT mpnimbleport.c)
    target_compile_definitions(${MICROPY_TARGET} PRIVATE
        MICROPY_BLUETOOTH_NIMBLE=1
        MICROPY_BLUETOOTH_NIMBLE_BINDINGS_ONLY=0
    )
    target_compile_options(${MICROPY_TARGET} PRIVATE
    # TODO: This flag is currently needed to make nimble build.
    -Wno-unused-but-set-variable
    )
    include(${MICROPY_DIR}/extmod/nimble/nimble.cmake)
    target_link_libraries(${MICROPY_TARGET} micropy_extmod_nimble)
    get_target_property(NIMBLE_INCLUDE micropy_extmod_nimble INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND MICROPY_INC_CORE ${NIMBLE_INCLUDE})
endif()

# Define frozen manifest
set(MICROPY_CROSS_FLAGS -march=x86)
if (NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${PROJECT_SOURCE_DIR}/variants/manifest.py)
endif()
target_compile_definitions(${MICROPY_TARGET} PRIVATE
    MICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
    MPZ_DIG_SIZE=16
)

target_sources(${MICROPY_TARGET} PRIVATE
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_DRIVERS}
    ${MICROPY_SOURCE_PORT}
)

target_link_libraries(${MICROPY_TARGET} usermod m ${SDL2_LIBRARIES})

target_include_directories(${MICROPY_TARGET} PRIVATE
    ${MICROPY_INC_CORE}
    ${MICROPY_INC_USERMOD}
    ${MICROPY_BOARD_DIR}
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
)

target_compile_options(${MICROPY_TARGET} PRIVATE
    -Wall
    -Werror
)

# Include the main MicroPython cmake rules.
include(${MICROPY_DIR}/py/mkrules.cmake)
