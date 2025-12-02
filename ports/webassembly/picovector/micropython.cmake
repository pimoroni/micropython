add_library(usermod_picovector INTERFACE)

include(modules/c/pngdec/pngdec)

target_sources(usermod_picovector INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/micropython/picovector_bindings.c
  ${CMAKE_CURRENT_LIST_DIR}/micropython/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/primitive.cpp  
)

target_include_directories(usermod_picovector INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(usermod_picovector INTERFACE PICO=1)

target_link_libraries(usermod INTERFACE usermod_picovector pngdec hardware_interp)

set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/micropython/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/primitive.cpp
  PROPERTIES COMPILE_FLAGS
  "-Wno-unused-variable -Wno-unused"
)

set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/micropython/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/primitive.cpp
  PROPERTIES COMPILE_OPTIONS
  "-O2;-fgcse-after-reload;-floop-interchange;-fpeel-loops;-fpredictive-commoning;-fsplit-paths;-ftree-loop-distribute-patterns;-ftree-loop-distribution;-ftree-vectorize;-ftree-partial-pre;-funswitch-loops"
)