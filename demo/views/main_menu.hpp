#pragma once
#include "../controllers/file_loading.hpp"
#include "imgui.h"
#include "state.hpp"

namespace Views {

inline void MainMenu(bool& running, AppState& state) {
#ifndef NDEBUG
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("ImGui Demo", nullptr, &state.m_ShowImGuiDemo);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
#endif

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  if (ImGui::Button("Загрузить NPY файл")) {
    Controllers::LoadNpyFile();
  }

  ImGui::BeginDisabled(!state.IsDemoAvaliable());
  if (ImGui::Button("Демонстрация"))
    state.SetCurrentState(State::DemoSetup);
  ImGui::EndDisabled();

  if (!state.m_Objects.empty()) {
    if (ImGui::Button("Тестирование KNN"))
      state.SetCurrentState(State::TestKnn);
    if (ImGui::Button("Тестирование использования памяти"))
      state.SetCurrentState(State::TestMemory);
    if (ImGui::Button("Тестирование KNN по N объектов"))
      state.SetCurrentState(State::TestN);
    if (ImGui::Button("Тестирование KNN по k"))
      state.SetCurrentState(State::TestK);

    ImGui::Separator();
    ImGui::Text("Объектов: %zu", state.GetObjectsCount());
    if (state.GetRTree()) {
      ImGui::Text("Дерево: M=%d  m=%d  %.2f MB", state.m_RTreeParams.maxEntries,
                  state.m_RTreeParams.minEntries,
                  state.GetRTreeMemorySize() / (1024.0f * 1024.0f));
    }
  }
  ImGui::End();
}

}  // namespace Views