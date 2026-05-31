#pragma once
#include "imgui.h"
#include "state.hpp"
#include "widgets.hpp"

#include "../controllers/file_loading.hpp"

namespace Views {

void Demo(bool& running, AppState& state) {
  // Main menu
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load npy file"))
        Controllers::LoadNpyFile();
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

}  // namespace Views
