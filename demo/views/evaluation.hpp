#pragma once
#include "imgui.h"

#include "../contollers/evaluation.hpp"
#include "state.hpp"

namespace Views {

void EvaluationSetup(EvaluationState& state);
void EvaluationProgress(EvaluationState& state);
void EvaluationResults(EvaluationState& state);

void Evaluation(bool& running, AppState& state) {
  auto& evaluationState = state.m_EvaluationState;
  switch (evaluationState.phase) {
    case EvaluationPhase::Setup:
      EvaluationSetup(evaluationState);
      break;
    case EvaluationPhase::Progress:
      EvaluationProgress(evaluationState);
      break;
    case EvaluationPhase::Results:
      EvaluationResults(evaluationState);
      break;
    default:
      std::unreachable();
  }
}

void EvaluationSetup(EvaluationState& state) {
  ImGui::Begin("Evaluation setup");
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество запросов", &state.numRuns);
  ImGui::InputInt("k для kNN", &state.k);
  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate(state);
  }
  ImGui::End();
}

void EvaluationProgress(EvaluationState& state) {
  ImGui::Begin("Evaluation progress");
  ImGui::Text("Тестирование в процессе...");
  ImGui::End();
}

void EvaluationResults(EvaluationState& state) {
  ImGui::Begin("Evaluation results");
  ImGui::Text("Результаты тестирования:");
  // Display evaluation results
  ImGui::End();
}

}  // namespace Views
