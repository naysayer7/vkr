#define SDL_MAIN_HANDLED

#include <algorithm>
#include <execution>
#include <npy.hpp>
#include <print>
#include <ranges>
#include <thread>

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "tinyfiledialogs.h"

#include "widgets.hpp"

class IndexIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = int64_t;
  using difference_type = int64_t;
  using pointer = int64_t*;
  using reference = int64_t&;

  IndexIterator() = default;
  IndexIterator(value_type index) : index_(index) {}
  const value_type& operator*() const { return index_; }
  const void operator++() { ++index_; }
  bool operator!=(const IndexIterator& lhs) const {
    return index_ != lhs.index_;
  }
  const value_type operator+(const IndexIterator& lhs) const {
    return index_ + lhs.index_;
  }
  value_type operator-(const IndexIterator& lhs) const {
    return index_ - lhs.index_;
  }

 private:
  value_type index_;
};

extern unsigned char font_data[];
extern unsigned int font_data_len;
ImFont* loadFont();
void MainMenu(bool& running, AppState& state);
void Demo(bool& running, AppState& state);
void BuildingRTree(bool& running, AppState& state);
void FileReading(bool& running, AppState& state);
void Evaluation(bool& running, AppState& state);
void LoadNpyFile();

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

    switch (state.GetCurrentState()) {
      case State::MainMenu:
        MainMenu(running, state);
        break;
      case State::Demo:
        Demo(running, state);
        break;
      case State::BuildingRTree:
        BuildingRTree(running, state);
        break;
      case State::FileReading:
        FileReading(running, state);
        break;
      case State::Testing:
        Evaluation(running, state);
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

void MainMenu(bool& running, AppState& state) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load npy file"))
        LoadNpyFile();
      if (ImGui::MenuItem("Exit"))
        running = false;
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("ImGui Demo", nullptr, &state.m_ShowImGuiDemo);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  // Fullscreen host window
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImGui::Text("Main menu");
  ImGui::Separator();

  if (ImGui::Button("Load NPY file")) {
    LoadNpyFile();
  }

  ImGui::BeginDisabled(!state.m_RTree || state.m_RTree->GetN() != 2 ||
                       state.m_Objects.size() == 0 ||
                       state.m_Objects.size() > 15000);
  if (ImGui::Button("Demo"))
    state.SetCurrentState(State::Demo);
  ImGui::EndDisabled();

  if (state.m_RTree) {
    if (ImGui::Button("Free tree")) {
      state.m_RTree.reset();
      state.m_Objects.clear();
      state.RecalculateMemorySize();
    }
    ImGui::Text("Loaded RTree with %zu objects, memory size: %.2f MB (%.2f MB)",
                state.GetObjectsCount(),
                state.GetRTreeMemorySize() / (1024.0f * 1024.0f),
                state.m_ObjSize / (1024.0f * 1024.0f));
  }
  ImGui::End();
}

void FileReading(bool& running, AppState& state) {
  // Fullscreen host window
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(0.0f, 0.0f),
                     "Загрузка файла...");
  ImGui::End();
}

void BuildingRTree(bool& running, AppState& state) {
  // Fullscreen host window
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImGui::ProgressBar(
      state.m_BuildingRTreeState.GetProgress(), ImVec2(0.0f, 0.0f),
      std::format("Построение дерева {:.2f}%",
                  state.m_BuildingRTreeState.GetProgress() * 100.0)
          .c_str());
  ImGui::SameLine();
  ImGui::Text("%zu / %zu", state.m_BuildingRTreeState.handledObjects,
              state.m_BuildingRTreeState.totalObjects);
  ImGui::End();
}

void Demo(bool& running, AppState& state) {
  // Main menu
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load npy file"))
        LoadNpyFile();
      if (ImGui::MenuItem("Exit"))
        running = false;
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("ImGui Demo", nullptr, &state.m_ShowImGuiDemo);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  // Fullscreen host window
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  if (ImGui::Begin("Main window", nullptr,
                   ImGuiWindowFlags_NoDecoration |
                       ImGuiWindowFlags_NoBringToFrontOnFocus)) {
    Widgets::Viewport();
  }
  ImGui::End();
}

void Evaluation(bool& running, AppState& state) {
  ImGui::Text("Evaluation");
  ImGui::Separator();
  if (ImGui::Button("Back to Main Menu")) {
    state.SetCurrentState(State::MainMenu);
  }
}

void LoadNpyFileThreadTarget(const std::string& filePath);
void LoadNpyFile() {
  AppState& state = AppState::instance();
  const char* selectedPath =
      tinyfd_openFileDialog("Select NPY file", "", 0, nullptr, nullptr, 0);
  if (!selectedPath || selectedPath[0] == '\0')
    return;

  state.SetStartFileReading();
  const std::string filePath(selectedPath);
  std::thread fileLoadingThread(LoadNpyFileThreadTarget, filePath);
  fileLoadingThread.detach();
}

void LoadNpyFileThreadTarget(const std::string& filePath) {
  AppState& state = AppState::instance();
  npy::npy_data data = npy::read_npy<float>(filePath);

  std::print("Shape: ");
  for (size_t i = 0; i < data.shape.size(); ++i) {
    std::print("{}", data.shape[i]);
    if (i < data.shape.size() - 1)
      std::print("x");
  }

  if (data.shape.size() != 2 || data.shape[1] % 2 != 0) {
    // TODO: handle error
    return;
  }

  state.m_Objects.clear();
  state.m_Objects.reserve(data.shape[0]);

  std::for_each(std::execution::par, IndexIterator(0),
                IndexIterator(data.shape[0]),
                [&data, &state](IndexIterator iter) {
                  const std::size_t i = static_cast<std::size_t>(*iter);
                  const uint64_t id = static_cast<uint64_t>(i);
                  rtree::Rectangle<float> rect(data.shape[1] / 2);
                  for (size_t j = 0; j < data.shape[1]; ++j)
                    rect.size[j] = data.data[i * data.shape[1] + j];
                  const rtree::Object<float> obj(id, rect);
                  {
                    // Синхронизация доступа к общему вектору объектов
                    std::lock_guard<std::mutex> lock(state.m_Mutex);
                    state.m_Objects.push_back(std::move(obj));
                  }
                });

  state.m_BuildingRTreeState.totalObjects = state.m_Objects.size();
  state.SetCurrentState(State::BuildingRTree);
  state.m_RTree =
      std::make_unique<rtree::RTree<float>>(4, 2, data.shape[1] / 2);
  for (const auto& obj : state.m_Objects) {
    state.m_RTree->Insert(&obj);
    state.m_BuildingRTreeState.handledObjects++;
  }

  state.RecalculateMemorySize();

  state.SetCurrentState(State::MainMenu);
}
