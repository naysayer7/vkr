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
  if (state.m_Objects.empty()) {
    ImGui::Text(
        "Загрузите NPY файл с точками (<= 15000) в 2D, чтобы начать демонстрацию.");
  }

  if (ImGui::Button("Тестирование наивного KNN по наборам данных"))
    state.SetCurrentState(State::TestDatasetsNaive);
  if (ImGui::Button("Тестирование KNN по наборам данных"))
    state.SetCurrentState(State::TestDatasets);
  if (ImGui::Button("Тестирование памяти по наборам данных"))
    state.SetCurrentState(State::TestMemoryDatasets);

  ImGui::BeginDisabled(state.m_Objects.empty());
  if (ImGui::Button("Тестирование KNN по M"))
    state.SetCurrentState(State::TestKnn);
  if (ImGui::Button("Тестирование KNN по k"))
    state.SetCurrentState(State::TestK);
  if (ImGui::Button("Тестирование памяти по M"))
    state.SetCurrentState(State::TestMemory);
  ImGui::EndDisabled();

  ImGui::Separator();
  if (state.m_Objects.empty()) {
    ImGui::Text("Загрузите NPY файл, чтобы начать тестирование.");
  } else {
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