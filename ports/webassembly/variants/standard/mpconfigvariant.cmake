# Standard webassembly variant (cmake).
# Mirrors variants/standard/mpconfigvariant.mk: an Asyncify build without the
# cooperative-yield feature set.
list(APPEND MICROPY_PORT_JSFLAGS -sASYNCIFY)
