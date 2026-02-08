set(FONT_TTF "${CMAKE_CURRENT_SOURCE_DIR}/assets/Segui UI Variable Display.ttf")
set(FONT_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated_font.cpp")

add_custom_command(
  OUTPUT ${FONT_CPP}
  COMMAND xxd -n font_data -i ${FONT_TTF} ${FONT_CPP}
  DEPENDS "${FONT_TTF}"
  COMMENT "Generating C++ source embedding the font"
)

add_executable(rtree_demo main.cpp ${FONT_CPP})
target_link_libraries(rtree_demo PRIVATE rtree_lib SDL3::SDL3-static imgui)
