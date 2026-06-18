#pragma once
#include <algorithm>
#include <filesystem>
#include <format>
#include "imgui.h"

#include "controllers/test_n_naive.hpp"
#include "state.hpp"
#include "views/test_results.hpp"

namespace Views {

void TestNNaiveSetup(TestNNaiveSetupState& state);
void TestNNaiveProgress(TestNNaiveProgressState& state);
void TestNNaiveResults();

inline void TestNNaive(bool& running, TestNNaiveState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestNNaivePhase::Setup:
      TestNNaiveSetup(state.setup);
      break;
    case TestNNaivePhase::Progress:
      TestNNaiveProgress(state.progress);
      break;
    case TestNNaivePhase::Results:
      TestNNaiveResults();
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestNNaiveSetup(TestNNaiveSetupState& state) {
  ImGui::Text("Тестирование наивного kNN (перебор) по количеству объектов");
  ImGui::Separator();

  ImGui::InputInt("Эпох на измерение", &state.epochs);
  ImGui::InputInt("k для kNN", &state.k);

  state.epochs = std::max(state.epochs, 1);
  state.k = std::max(state.k, 1);

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
    Controllers::SelectTestNNaiveFiles(state);

  if (!state.selectedFiles.empty()) {
    if (ImGui::Button("Очистить список"))
      state.selectedFiles.clear();
  }

  ImGui::Spacing();
  ImGui::Text("Измерений: %d", state.CalculateMeasurements());

  ImGui::Spacing();
  ImGui::BeginDisabled(state.selectedFiles.empty());
  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestNNaive();
  ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Назад"))
    AppState::instance().SetCurrentState(State::MainMenu);
}

inline void TestNNaiveProgress(TestNNaiveProgressState& state) {
  const int done = state.done.load();
  const int total = state.total.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs = state.epochs.load();
  const int currentN = state.currentN.load();

  if (total == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  const float epochFraction =
      static_cast<float>(epochsDone) / static_cast<float>(epochs);
  const float outerFraction =
      (static_cast<float>(done) + epochFraction) / static_cast<float>(total);

  ImGui::ProgressBar(outerFraction, ImVec2(0.0f, 0.0f),
                     std::format("Датасет {}/{}  N={}",
                                 std::min(done + 1, total), total, currentN)
                         .c_str());

  ImGui::ProgressBar(epochFraction, ImVec2(0.0f, 0.0f),
                     std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

inline void TestNNaiveResults() {
  RenderTestResults(
      {"Расположение: results/n_naive/", "Формат файлов: {n}_{k}.npy",
       "Каждый файл — 1D массив времён (нс) по эпохам."},
      [] { AppState::instance().m_TestNNaiveState.Reset(); });
}

}  // namespace Views
