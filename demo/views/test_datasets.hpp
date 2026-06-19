#pragma once
#include <algorithm>
#include <filesystem>
#include <format>
#include "imgui.h"

#include "controllers/test_datasets.hpp"
#include "state.hpp"
#include "views/test_results.hpp"

namespace Views {

void TestDatasetsSetup(TestDatasetsSetupState& state);
void TestDatasetsProgress(TestDatasetsProgressState& state);
void TestDatasetsResults();

inline void TestDatasets(bool& running, TestDatasetsState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestDatasetsPhase::Setup:
      TestDatasetsSetup(state.setup);
      break;
    case TestDatasetsPhase::Progress:
      TestDatasetsProgress(state.progress);
      break;
    case TestDatasetsPhase::Results:
      TestDatasetsResults();
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestDatasetsSetup(TestDatasetsSetupState& state) {
  ImGui::Text("Тестирование kNN по наборам данных");
  ImGui::Separator();

  ImGui::InputInt("M (maxEntries)", &state.maxEntries);
  ImGui::InputInt("m (minEntries)", &state.minEntries);
  ImGui::InputInt("Эпох на измерение", &state.epochs);
  ImGui::InputInt("k для kNN", &state.k);
  ImGui::SliderInt("Доля запросов (hold-out), %", &state.queryPercent, 1, 99);

  state.maxEntries = std::max(state.maxEntries, 2);
  state.minEntries =
      std::max(1, std::min(state.minEntries, (state.maxEntries + 1) / 2));
  state.epochs = std::max(state.epochs, 1);
  state.k = std::max(state.k, 1);
  state.queryPercent = std::clamp(state.queryPercent, 1, 99);

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
    Controllers::SelectTestDatasetsFiles(state);

  if (!state.selectedFiles.empty()) {
    if (ImGui::Button("Очистить список"))
      state.selectedFiles.clear();
  }

  ImGui::Spacing();
  ImGui::Text("Измерений: %d", state.CalculateMeasurements());

  ImGui::Spacing();
  ImGui::BeginDisabled(state.selectedFiles.empty());
  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestDatasets();
  ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Назад"))
    AppState::instance().SetCurrentState(State::MainMenu);
}

inline void TestDatasetsProgress(TestDatasetsProgressState& state) {
  const int done = state.done.load();
  const int total = state.total.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs = state.epochs.load();
  const std::string currentDataset = state.CurrentDataset();

  if (total == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  const float epochFraction =
      static_cast<float>(epochsDone) / static_cast<float>(epochs);
  const float outerFraction =
      (static_cast<float>(done) + epochFraction) / static_cast<float>(total);

  ImGui::ProgressBar(outerFraction, ImVec2(0.0f, 0.0f),
                     std::format("Датасет {}/{}  {}", std::min(done + 1, total),
                                 total, currentDataset)
                         .c_str());

  ImGui::ProgressBar(epochFraction, ImVec2(0.0f, 0.0f),
                     std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

inline void TestDatasetsResults() {
  RenderTestResults(
      {"Расположение: results/datasets/",
       "Формат файлов: {название_датасета}_results.npy",
       "Каждый файл — 1D массив времён (нс) по эпохам.",
       "Запросы: hold-out — часть объектов исключена из индекса."},
      [] { AppState::instance().m_TestDatasetsState.Reset(); });
}

}  // namespace Views
