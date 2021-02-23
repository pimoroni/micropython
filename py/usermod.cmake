# Create a target for all user modules to link against.
add_library(usermod INTERFACE)

function(usermod_gather_sources VARNAME INCLUDED_VARNAME LIB)
    if (NOT ${LIB} IN_LIST ${INCLUDED_VARNAME})
        list(APPEND ${INCLUDED_VARNAME} ${LIB})
        set(${INCLUDED_VARNAME} ${${INCLUDED_VARNAME}} PARENT_SCOPE)
    endif()
    get_target_property(new_sources ${LIB} INTERFACE_SOURCES)
    if (new_sources)
        list(APPEND ${VARNAME} ${new_sources})
    endif()
    get_target_property(new_sources ${LIB} INTERFACE_LIBRARIES)
    get_target_property(trans_depend ${LIB} INTERFACE_LINK_LIBRARIES)
    if (trans_depend)
        foreach(SUB_LIB ${trans_depend})
            usermod_gather_sources(${VARNAME} ${INCLUDED_VARNAME} ${SUB_LIB})
        endforeach()
    endif()
    set(${VARNAME} ${${VARNAME}} PARENT_SCOPE)
    set(${INCLUDED_VARNAME} ${${INCLUDED_VARNAME}} PARENT_SCOPE)
endfunction()

# Include CMake files for user modules.
if (USER_C_MODULES)
    foreach(USER_C_MODULE_PATH ${USER_C_MODULES})
        message("Including User C Module(s) from ${USER_C_MODULE_PATH}/usermod.cmake")
        include(${USER_C_MODULE_PATH}/usermod.cmake)
    endforeach()
endif()

# Recursively gather sources for QSTR scanning - doesn't support generator expressions.
usermod_gather_sources(SOURCE_USERMOD found_modules usermod)

# Report found modules.
list(REMOVE_ITEM found_modules "usermod")
list(JOIN found_modules ", " found_modules)
message("Found User C Module(s): ${found_modules}")
