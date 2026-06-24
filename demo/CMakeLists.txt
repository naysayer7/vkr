
set(IMGUI_PATH  "./external/imgui")
set(SDL3_VERSION "3.4.0")

# --- SDL3 ---
# Build SDL3 as a static library
set(SDL_SHARED OFF CACHE BOOL "Build SDL3 shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 static library" FORCE)
FetchContent_Declare(
  SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.zip
)
FetchContent_MakeAvailable(SDL3)

# --- ImGui (with SDL3 backends) ---
set(IMGUI_SOURCES
  ${IMGUI_PATH}/imgui.cpp
  ${IMGUI_PATH}/imgui_demo.cpp
  ${IMGUI_PATH}/imgui_draw.cpp
  ${IMGUI_PATH}/imgui_tables.cpp
  ${IMGUI_PATH}/imgui_widgets.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdlrenderer3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_PATH} ${IMGUI_PATH}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# --- tinyfiledialogs ---
add_library(tinyfiledialogs STATIC external/tinyfiledialogs/tinyfiledialogs.c)
target_include_directories(tinyfiledialogs PUBLIC external/tinyfiledialogs)


# --- Demo executable ---
set(FONT_TTF "${CMAKE_CURRENT_SOURCE_DIR}/assets/Segui UI Variable Display.ttf")
set(FONT_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated_font.cpp")
add_custom_command(
  OUTPUT ${FONT_CPP}
  COMMAND xxd -n font_data -i ${FONT_TTF} ${FONT_CPP}
  DEPENDS "${FONT_TTF}"
  COMMENT "Generating C++ source embedding the font"
)

add_executable(rtree_demo main.cpp ${FONT_CPP})
target_include_directories(rtree_demo PRIVATE external/libnpy/include) # libnpy
target_link_libraries(rtree_demo PRIVATE rtree_lib SDL3::SDL3-static imgui tinyfiledialogs)

# Собирать как GUI-приложение Windows, чтобы не открывалась консоль
set_target_properties(rtree_demo PROPERTIES WIN32_EXECUTABLE TRUE)
if(MSVC)
  target_link_options(rtree_demo PRIVATE /ENTRY:mainCRTStartup)
endif()
