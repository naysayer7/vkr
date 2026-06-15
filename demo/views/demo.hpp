#pragma once
#include "imgui.h"
#include "state.hpp"
#include "widgets.hpp"

#include "../controllers/file_loading.hpp"

namespace Views {

inline void Demo(bool& running, AppState& state) {
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
