# PyScript variant (cmake). Mirrors variants/pyscript/mpconfigvariant.mk.
list(APPEND MICROPY_PORT_JSFLAGS -sALLOW_MEMORY_GROWTH)

set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/variants/pyscript/manifest.py)
