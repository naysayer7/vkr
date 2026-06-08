#pragma once
#include "../controllers/file_loading.hpp"
#include "imgui.h"
#include "state.hpp"

namespace Views {

void MainMenu(bool& running, AppState& state) {
  if (ImGui::BeginMainMenuBar()) {
    /* if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load npy file"))
        Controllers::LoadNpyFile();
      if (ImGui::MenuItem("Exit"))
        running = false;
      ImGui::EndMenu();
    } */
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
    Controllers::LoadNpyFile();
  }

  ImGui::BeginDisabled(!state.IsDemoAvaliable());
  if (ImGui::Button("Demo"))
    state.SetCurrentState(State::Demo);
  ImGui::EndDisabled();

  if (state.m_RTree) {
    if (ImGui::Button("Тестирование KNN")) {
      state.SetCurrentState(State::TestKnn);
    }
    if (ImGui::Button("Тестирование использования памяти")) {
      state.SetCurrentState(State::TestKnn);
    }

    ImGui::Text("Loaded RTree with %zu objects, memory size: %.2f MB (%.2f MB)",
                state.GetObjectsCount(),
                state.GetRTreeMemorySize() / (1024.0f * 1024.0f),
                state.m_ObjSize / (1024.0f * 1024.0f));
  } 
  ImGui::End();
}

}  // namespace Views