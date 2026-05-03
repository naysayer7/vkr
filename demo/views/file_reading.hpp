#pragma once
#include "imgui.h"

#include "state.hpp"

namespace Views {

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

}  // namespace Views