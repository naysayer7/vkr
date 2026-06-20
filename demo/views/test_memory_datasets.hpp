#pragma once
#include <algorithm>
#include <filesystem>
#include <format>
#include "imgui.h"

#include "controllers/test_memory_datasets.hpp"
#include "state.hpp"
#include "views/test_results.hpp"

namespace Views {

void TestMemoryDatasetsSetup(TestMemoryDatasetsSetupState& state);
void TestMemoryDatasetsProgress(TestMemoryDatasetsProgressState& state);
void TestMemoryDatasetsResults(TestMemoryDatasetsResultState& state);

inline void TestMemoryDatasets(bool& running, TestMemoryDatasetsState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestMemoryDatasetsPhase::Setup:
      TestMemoryDatasetsSetup(state.setup);
      break;
    case TestMemoryDatasetsPhase::Progress:
      TestMemoryDatasetsProgress(state.progress);
      break;
    case TestMemoryDatasetsPhase::Results:
      TestMemoryDatasetsResults(state.result);
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestMemoryDatasetsSetup(TestMemoryDatasetsSetupState& state) {
  ImGui::Text("Тестирование использования памяти по наборам данных");
  ImGui::Separator();

  ImGui::InputInt("M (maxEntries)", &state.maxEntries);
  state.maxEntries = std::max(state.maxEntries, 2);

  ImGui::Spacing();
  ImGui::Text("Датасеты (%d файлов):", (int)state.selectedFiles.size());

  for (int i = 0; i < (int)state.selectedFiles.size(); ++i) {
    const std::string name =
        std::filesystem::path(state.selectedFiles[i]).filename().string();
    ImGui::Text("  %s", name.c_str());
    ImGui::SameLine();
    if (ImGui::Button(std::format("Удалить##f{}", i).c_str())) {
      state.selectedFiles.erase(state.selectedFiles.begin() + i);
      --i;
    }
  }

  if (ImGui::Button("Добавить файлы"))
    Controllers::SelectTestMemoryDatasetsFiles(state);

  if (!state.selectedFiles.empty()) {
    if (ImGui::Button("Очистить список"))
      state.selectedFiles.clear();
  }

  ImGui::Spacing();
  ImGui::Text("Измерений: %d", state.CalculateMeasurements());

  ImGui::Spacing();
  ImGui::BeginDisabled(state.selectedFiles.empty());
  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestMemoryDatasets();
  ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Назад"))
    AppState::instance().SetCurrentState(State::MainMenu);
}

inline void TestMemoryDatasetsProgress(TestMemoryDatasetsProgressState& state) {
  const int done = state.done.load();
  const int total = state.total.load();
  const std::string currentDataset = state.CurrentDataset();

  if (total == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  ImGui::ProgressBar(static_cast<float>(done) / static_cast<float>(total),
                     ImVec2(0.0f, 0.0f),
                     std::format("Датасет {}/{}  {}", std::min(done + 1, total),
                                 total, currentDataset)
                         .c_str());
}

inline void TestMemoryDatasetsResults(TestMemoryDatasetsResultState& state) {
  RenderTestResults(
      {"Расположение: " + state.savedFilename,
       "Формат: 2D массив, столбцы [N (объектов), память (байт)]"},
      [] { AppState::instance().m_TestMemoryDatasetsState.Reset(); },
      [&state] {
        if (ImGui::BeginTable("mem_datasets_results", 3,
                              ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_BordersV)) {
          ImGui::TableSetupColumn("Датасет");
          ImGui::TableSetupColumn("Объектов");
          ImGui::TableSetupColumn("Память (байт)");
          ImGui::TableHeadersRow();

          for (const auto& entry : state.entries) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(entry.name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", entry.indexedCount);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", entry.memory);
          }
          ImGui::EndTable();
        }
      });
}

}  // namespace Views
