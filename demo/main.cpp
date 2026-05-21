#define SDL_MAIN_HANDLED

#include <print>

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui.h"

#include "measures.hpp"
#include "views/demo.hpp"
#include "views/evaluation.hpp"
#include "views/main_menu.hpp"
#include "views/file_reading.hpp"
#include "views/building_rtree.hpp"
/* #include "views/test.hpp" */
#include "widgets.hpp"

extern unsigned char font_data[];
extern unsigned int font_data_len;
ImFont* loadFont();
void BuildingRTree(bool& running, AppState& state);

int main(int argc, char* argv[]) {
  // argc argv не используются
  (void)argc;
  (void)argv;

  SDL_SetMainReady();

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    const char* err = SDL_GetError();
    SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s",
            (err && err[0]) ? err : "<no SDL error message>");
    return 1;
  }

  SDL_Window* window =
      SDL_CreateWindow("RTree demo", 1900, 900, SDL_WINDOW_RESIZABLE);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (!SDL_SetRenderVSync(renderer, 1)) {
    SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigWindowsMoveFromTitleBarOnly = true;

  ImFont* font = loadFont();
  if (font)
    io.FontDefault = font;

  ImGui::StyleColorsLight();

  if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
    SDL_Log("ImGui_ImplSDL3_InitForSDLRenderer failed");
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
    SDL_Log("ImGui_ImplSDLRenderer3_Init failed");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Масштабирование интерфейса в зависимости от DPI дисплея
  auto sdl_display = SDL_GetPrimaryDisplay();
  auto sdl_display_scale = SDL_GetDisplayContentScale(sdl_display);
  ImGui::GetStyle().ScaleAllSizes(sdl_display_scale);
  io.FontGlobalScale = sdl_display_scale;

  bool running = true;
  AppState& state = AppState::instance();

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL3_ProcessEvent(&e);
      if (e.type == SDL_EVENT_QUIT)
        running = false;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (state.m_ShowImGuiDemo)
      ImGui::ShowDemoWindow(&state.m_ShowImGuiDemo);

    // Views::TestScreen(running, state);

    switch (state.GetCurrentState()) {
      case State::MainMenu:
        Views::MainMenu(running, state);
        break;
      case State::Demo:
        Views::Demo(running, state);
        break;
      case State::BuildingRTree:
        Views::BuildingRTree(running, state);
        break;
      case State::FileReading:
        Views::FileReading(running, state);
        break;
      case State::Evaluation:
        Views::Evaluation(running, state.m_EvaluationState);
        break;
      default:
        break;
    }
    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

ImFont* loadFont() {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig fontConfig;
  fontConfig.FontDataOwnedByAtlas = false;  // Памятью шрифта управлет не ImGui
  return io.Fonts->AddFontFromMemoryTTF(
      (void*)font_data, static_cast<int>(font_data_len), 0.0f, &fontConfig);
}
