#pragma once
#include <format>
#include <thread>

#include "error.hpp"
#include "imgui.h"
#include "state.hpp"

namespace Views {

inline void DemoSetup(bool& running, AppState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImGui::Text("Настройка демо");
  ImGui::Separator();

  ImGui::Text(
      std::format("Объектов: {}", AppState::instance().GetObjectsCount())
          .c_str());
  ImGui::Spacing();

  ImGui::InputInt("M (maxEntries)", &state.m_RTreeParams.maxEntries);
  ImGui::InputInt("m (minEntries)", &state.m_RTreeParams.minEntries);

  state.m_RTreeParams.maxEntries =
      std::max(state.m_RTreeParams.maxEntries, 2);
  state.m_RTreeParams.minEntries =
      std::max(1, std::min(state.m_RTreeParams.minEntries,
                           (state.m_RTreeParams.maxEntries + 1) / 2));

  ImGui::Spacing();

  if (ImGui::Button("Построить дерево и запустить")) {
    std::thread([&state]() {
      try {
        state.BuildRTree();
        // BuildRTree восстанавливает предыдущий стейт (DemoSetup),
        // поэтому явно переходим в Demo после постройки.
        state.SetCurrentState(State::Demo);
      } catch (const std::exception& e) {
        Error::Show(e.what());
        state.SetCurrentState(State::DemoSetup);
      }
    }).detach();
  }

  ImGui::SameLine();

  if (ImGui::Button("Назад"))
    state.SetCurrentState(State::MainMenu);

  ImGui::End();
}

}  // namespace Views
