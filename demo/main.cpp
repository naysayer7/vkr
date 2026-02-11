#define SDL_MAIN_HANDLED

#include <thread>
#include <print>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <tinyfiledialogs.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#include "widgets.hpp"
#include "filetools.hpp"

extern unsigned char font_data[];
extern unsigned int font_data_len;
ImFont *loadFont();
void LoadCsvFile();

int main(int argc, char *argv[])
{
    // argc argv не используются
    (void)argc;
    (void)argv;

    SDL_SetMainReady();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        const char *err = SDL_GetError();
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", (err && err[0]) ? err : "<no SDL error message>");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("RTree demo", 1900, 900, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_SetRenderVSync(renderer, 1))
    {
        SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImFont *font = loadFont();
    if (font)
        io.FontDefault = font;

    ImGui::StyleColorsLight();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer))
    {
        SDL_Log("ImGui_ImplSDL3_InitForSDLRenderer failed");
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!ImGui_ImplSDLRenderer3_Init(renderer))
    {
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
    bool imguiDemoOpen = false;
    ViewportWindow viewport;
    AppState &state = AppState::instance();

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Main menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load csv file"))
                    LoadCsvFile();
                if (ImGui::MenuItem("Exit"))
                    running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("ImGui Demo", nullptr, &imguiDemoOpen);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Fullscreen host window
        const ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);

        if (ImGui::Begin("Main window", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            if (state.m_FileLoadingState.loading)
            {
                if (state.m_FileLoadingState.fileRead)
                {
                    ImGui::Text("Inserting objects into RTree... %zu / %zu", state.m_FileLoadingState.objectsInserted, state.m_Objects.size());
                    ImGui::ProgressBar(state.m_Objects.size() > 0 ? (float)state.m_FileLoadingState.objectsInserted / state.m_Objects.size() : 0.0f, ImVec2(-1, 0));
                    if (!state.m_FileLoadingState.errorMessage.empty())
                        ImGui::Text("Error: %s", state.m_FileLoadingState.errorMessage.c_str());
                }
                else
                {
                    ImGui::Text("Loading file... %zu / %zu lines read", state.m_FileLoadingState.linesRead, state.m_FileLoadingState.totalLines);
                    ImGui::ProgressBar(state.m_FileLoadingState.totalLines > 0 ? (float)state.m_FileLoadingState.linesRead / state.m_FileLoadingState.totalLines : 0.0f, ImVec2(-1, 0));
                    if (!state.m_FileLoadingState.errorMessage.empty())
                        ImGui::Text("Error: %s", state.m_FileLoadingState.errorMessage.c_str());
                }
            }
            else
            {
                viewport.Show();
            }
        }
        ImGui::End();

        if (imguiDemoOpen)
            ImGui::ShowDemoWindow(&imguiDemoOpen);

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

ImFont *loadFont()
{
    ImGuiIO &io = ImGui::GetIO();
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false; // Памятью шрифта управлет не ImGui
    return io.Fonts->AddFontFromMemoryTTF((void *)font_data, static_cast<int>(font_data_len), 0.0f, &fontConfig);
}

void ReadCsvFile(const std::string &filePath);

void LoadCsvFile()
{
    AppState &state = AppState::instance();
    const char *selectedPath = tinyfd_openFileDialog("Select CSV file", "", 0, nullptr, nullptr, 0);
    if (!selectedPath || selectedPath[0] == '\0')
        return;

    state.m_FileLoadingState.SetStartReading();
    state.m_Objects.clear();
    const std::string filePath(selectedPath);
    std::thread fileLoadingThread(ReadCsvFile, filePath);
    fileLoadingThread.detach();
}

void ReadCsvFile(const std::string &filePath)
{
    AppState &state = AppState::instance();
    try
    {
        std::ifstream fileStream(filePath);
        if (!fileStream.is_open())
            throw std::runtime_error("Failed to open file: " + filePath);

        std::size_t lineCount = Filetools::CountLines(fileStream);
        state.m_FileLoadingState.totalLines = lineCount;
        state.m_Objects.reserve(lineCount);

        Filetools::ReadObjectsFromCsvFile<float>(fileStream, state.m_FileLoadingState.linesRead, [&state](rtree::Object<float> &obj)
                                                 { state.m_Objects.push_back(obj); });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        state.m_FileLoadingState.fileRead = true;

        if (state.m_Objects.empty())
        {
            state.m_FileLoadingState.loading = false;
            return;
        }

        state.m_RTree = rtree::RTree<float>(4, 2, state.m_Objects.front().mbr.n);
        for (const auto &obj : state.m_Objects)
        {
            state.m_FileLoadingState.objectsInserted++;
            state.m_RTree.Insert(obj);
        }
        state.RecalculateMemorySize();
        state.m_FileLoadingState.loading = false;
    }
    catch (const std::exception &e)
    {
        state.m_FileLoadingState.errorMessage = e.what();
    }
}