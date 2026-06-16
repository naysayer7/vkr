#pragma once
#include <chrono>
#include <format>
#include <stdexcept>
#include "imgui.h"

#include "controllers/test_knn.hpp"
#include "state.hpp"
#include "utils.hpp"

namespace Views {

void TestKnnSetup(TestKnnSetupState& state);
void TestKnnProgress(TestKnnProgressState& state);
void TestKnnResults(TestKnnResultState& state);

inline void TestKnn(bool& running, TestKnnState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestKnnPhase::Setup:
      TestKnnSetup(state.setup);
      break;
    case TestKnnPhase::Progress:
      TestKnnProgress(state.progress);
      break;
    case TestKnnPhase::Results:
      TestKnnResults(state.result);
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestKnnSetup(TestKnnSetupState& state) {
  auto& params = state.params;
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество эпох", &state.epochs);
  ImGui::InputInt("k для kNN", &state.k);

  ImGui::InputInt2("Диапазон M (мин, макс)", (int*)&state.maxObjects);
  ImGui::Text(std::format("Количество тестов: {}",
                          Utils::CalculateRunsCount(state.maxObjects))
                  .c_str());

  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate();
  }
}

inline void TestKnnProgress(TestKnnProgressState& state) {
  const int runsDone = state.runsDone.load();
  const int runs = state.runs.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs = state.epochs.load();
  const RTreeParameters params = state.currentParams.load();

  if (runs == 0 || epochs == 0) {
    ImGui::Text("Подготовка к тестированию...");
    return;
  }

  ImGui::ProgressBar((float)runsDone / (float)runs, ImVec2(0.0f, 0.0f),
                     std::format("Тестирование {}/{}", runsDone, runs).c_str());

  ImGui::Text(std::format("M: {}", params.maxEntries).c_str());

  ImGui::ProgressBar((float)epochsDone / (float)epochs, ImVec2(0.0f, 0.0f),
                     std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

inline void TestKnnResults(TestKnnResultState& state) {
  ImGui::Text("Результаты тестирования сохранены");
  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
    AppState::instance().m_TestKnnState.Reset();
  }
}
}  // namespace Views