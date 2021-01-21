add_library(usermod_cppexample INTERFACE)

target_sources(usermod_cppexample INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/example.cpp
    ${CMAKE_CURRENT_LIST_DIR}/examplemodule.c
)

target_include_directories(usermod_cppexample INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(usermod_cppexample INTERFACE
    -DMODULE_CPPEXAMPLE_ENABLED=1
)

target_link_libraries(usermod INTERFACE usermod_cppexample)