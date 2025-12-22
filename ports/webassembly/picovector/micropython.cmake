add_library(usermod_picovector INTERFACE)

find_package(PNGDEC CONFIG REQUIRED PATHS ${LIB_DIR})

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
  ${CMAKE_CURRENT_LIST_DIR}/rasteriser.cpp
  ${CMAKE_CURRENT_LIST_DIR}/algorithms/geometry.cpp
  ${CMAKE_CURRENT_LIST_DIR}/algorithms/dda.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/color.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/image_png.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/input.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/mat3.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/rect.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/point.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/algorithm.cpp
)

target_include_directories(usermod_picovector INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_picovector pngdec)

set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/micropython/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/picovector.cpp
  ${CMAKE_CURRENT_LIST_DIR}/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/primitive.cpp
  ${CMAKE_CURRENT_LIST_DIR}/rasteriser.cpp
  ${CMAKE_CURRENT_LIST_DIR}/algorithms/geometry.cpp
  ${CMAKE_CURRENT_LIST_DIR}/algorithms/dda.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/brush.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/color.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/image_png.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/input.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/mat3.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/pixel_font.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/shape.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/rect.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/point.cpp
  ${CMAKE_CURRENT_LIST_DIR}/micropython/algorithm.cpp
  PROPERTIES COMPILE_FLAGS
  "-Wno-unused-variable"
)

target_compile_options(usermod_picovector INTERFACE
    -Wall
    -Werror
)

