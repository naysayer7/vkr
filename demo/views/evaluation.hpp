#pragma once
#include "imgui.h"

#include "state.hpp"
#include "../contollers/evaluation.hpp"

namespace Views {

void EvaluationSetup(EvaluationState& state);
void EvaluationProgress(EvaluationState& state);
void EvaluationResults(EvaluationState& state);

void Evaluation(bool& running, AppState& state) {
  auto& evaluationState = state.m_EvaluationState;
  if (evaluationState.isRunning()) {
    EvaluationProgress(evaluationState);
  } else if (evaluationState.hasResults()) {
    EvaluationResults(evaluationState);
  } else {
    EvaluationSetup(evaluationState);
  }
}

void EvaluationSetup(EvaluationState& state) {
  ImGui::Begin("Evaluation setup");
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество запросов", &state.numRuns);
  ImGui::InputInt("k для kNN", &state.k);
  ImGui::InputInt("Количество потоков", &state.numThreads);
  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate(state);
  }
  ImGui::End();
}

void EvaluationProgress(EvaluationState& state) {
    ImGui::Begin("Evaluation progress");
    ImGui::Text("Тестирование в процессе...");
    for (size_t i = 0; i < state.futures.size(); i++) {
      ImGui::Text("Поток %zu: %s", i,
                  (state.futures[i].valid() &&
                   state.futures[i].wait_for(std::chrono::seconds(0)) !=
                       std::future_status::ready)
                      ? "Выполняется"
                      : "Завершён");
    }
    ImGui::End();
}

void EvaluationResults(EvaluationState& state) {
    ImGui::Begin("Evaluation results");
    ImGui::Text("Результаты тестирования:");
    // Display evaluation results
    ImGui::End();
}

}  // namespace Views
