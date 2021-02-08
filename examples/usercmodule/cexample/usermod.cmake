add_library(usermod_cexample INTERFACE)

target_sources(usermod_cexample INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/examplemodule.c
)

target_include_directories(usermod_cexample INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(usermod_cexample INTERFACE
    -DMODULE_CEXAMPLE_ENABLED=1
)

target_link_libraries(usermod INTERFACE usermod_cexample)