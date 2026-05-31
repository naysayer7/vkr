#pragma once
#include <format>

#include "imgui.h"
#include "state.hpp"
#include "widgets.hpp"

#include "../controllers/file_loading.hpp"

namespace Views {

void TestScreen(bool& running, AppState& state) {
  auto& params = state.m_Params;

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  static ImGuiTableFlags flags =
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
      ImGuiTableFlags_Hideable;

  ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 8);
  if (ImGui::BeginTable("table_params", 2, flags, outer_size)) {
    ImGui::TableSetupScrollFreeze(0, 1); // Первая строка всегда видна
    ImGui::TableSetupColumn("min", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("max", ImGuiTableColumnFlags_None);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin((int)params.size());
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(std::format("{}", params[row].first).c_str(), 0, row);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(std::format("{}", params[row].second).c_str(), 1, row);
      }
    }
    ImGui::EndTable();
  }

  ImGui::InputInt2("R-tree parameters (maxEntries, minEntries)",
                   (int*)&state.m_RTreeParams);
  if (ImGui::Button("+")) {
    state.m_Params.emplace_back(state.m_RTreeParams.maxEntries,
                                state.m_RTreeParams.minEntries);
  }
  ImGui::End();
}

}  // namespace Views
