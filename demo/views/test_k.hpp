#pragma once
#include <format>
#include "imgui.h"

#include "controllers/test_k.hpp"
#include "state.hpp"
#include "views/test_results.hpp"

namespace Views {

void TestKSetup(TestKSetupState& state);
void TestKProgress(TestKProgressState& state);
void TestKResults();

inline void TestK(bool& running, TestKState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestKPhase::Setup:
      TestKSetup(state.setup);
      break;
    case TestKPhase::Progress:
      TestKProgress(state.progress);
      break;
    case TestKPhase::Results:
      TestKResults();
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestKSetup(TestKSetupState& state) {
  ImGui::Text("Тестирование kNN по параметру k");
  ImGui::Separator();

  ImGui::InputInt("M (maxEntries)", &state.maxEntries);
  ImGui::Spacing();
  ImGui::InputInt("k минимальное", &state.kMin);
  ImGui::InputInt("k максимальное", &state.kMax);
  ImGui::InputInt("Шаг k", &state.kStep);
  ImGui::InputInt("Эпох на измерение", &state.epochs);

  state.maxEntries = std::max(state.maxEntries, 2);
  state.kMin = std::max(state.kMin, 1);
  state.kMax = std::max(state.kMax, state.kMin);
  state.kStep = std::max(state.kStep, 1);
  state.epochs = std::max(state.epochs, 1);

  ImGui::Text(std::format("Измерений: {}  (k: {}..{} шаг {})",
                          state.CalculateMeasurements(), state.kMin, state.kMax,
                          state.kStep)
                  .c_str());

  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestK();
}

inline void TestKProgress(TestKProgressState& state) {
  const int done = state.done.load();
  const int total = state.total.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs = state.epochs.load();
  const int currentK = state.currentK.load();

  if (total == 0 || epochs == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  const float epochFraction =
      static_cast<float>(epochsDone) / static_cast<float>(epochs);
  const float outerFraction =
      (static_cast<float>(done) + epochFraction) / static_cast<float>(total);

  ImGui::ProgressBar(outerFraction, ImVec2(0.0f, 0.0f),
                     std::format("Измерение {}/{}  k={}",
                                 std::min(done + 1, total), total, currentK)
                         .c_str());

  ImGui::ProgressBar(epochFraction, ImVec2(0.0f, 0.0f),
                     std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

inline void TestKResults() {
  RenderTestResults(
      {"Расположение: results/k/", "Формат файлов: {n}_{k}_{M}.npy",
       "Каждый файл — 1D массив времён (нс) по эпохам."},
      [] { AppState::instance().m_TestKState.Reset(); });
}

}  // namespace Views
