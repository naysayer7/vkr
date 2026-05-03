#pragma once

#include <format>
#include "imgui.h"

#include "state.hpp"

namespace Views {

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

}  // namespace Views