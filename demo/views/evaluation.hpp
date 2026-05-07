#pragma once
#include <format>
#include "imgui.h"

#include "../contollers/evaluation.hpp"
#include "state.hpp"

namespace Views {

void EvaluationSetup(EvaluationState& state);
void EvaluationProgress(EvaluationState& state);
void EvaluationResults(EvaluationState& state);

void Evaluation(bool& running, AppState& state) {
  auto& evaluationState = state.m_EvaluationState;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
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
  ImGui::End();
}

void EvaluationSetup(EvaluationState& state) {
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество запросов", &state.numRuns);
  ImGui::InputInt("k для kNN", &state.k);
  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate(state);
  }
}

void EvaluationProgress(EvaluationState& state) {
    ImGui::ProgressBar((float)state.run / (float)state.numRuns, ImVec2(0.0f, 0.0f),
                     std::format("Тестирование... {}/{}", state.run, state.numRuns).c_str());
}

void EvaluationResults(EvaluationState& state) {
  ImGui::Text("Результаты тестирования:");
  ImGui::Text(std::format("AVG: {} ms", state.knnResult.averangeTime).c_str());
}

}  // namespace Views
