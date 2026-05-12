#pragma once
#include <chrono>
#include <format>
#include "imgui.h"

#include "contollers/evaluation.hpp"
#include "utils.hpp"
#include "state.hpp"

namespace Views {

void EvaluationSetup(AppState& state);
void EvaluationProgress(AppState& state);
void EvaluationResults(AppState& state);

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
      EvaluationSetup(state);
      break;
    case EvaluationPhase::Progress:
      EvaluationProgress(state);
      break;
    case EvaluationPhase::Results:
      EvaluationResults(state);
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

void EvaluationSetup(AppState& state) {
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество запросов", &state.m_EvaluationState.numRuns);
  ImGui::InputInt("k для kNN", &state.m_EvaluationState.k);
  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate(state);
  }
}

void EvaluationProgress(AppState& state) {
  const auto& evState = state.m_EvaluationState;
  const auto& knnResult = evState.knnResult;
  const auto& lastMeasurement = knnResult.times.empty()
                                    ? Measures::Duration{0.0}
                                    : knnResult.times.back();

  ImGui::ProgressBar(
      (float)evState.run / (float)evState.numRuns, ImVec2(0.0f, 0.0f),
      std::format("Тестирование... {}/{}", evState.run, evState.numRuns)
          .c_str());

  ImGui::Text(
      std::format("AVG: {}", Utils::FormatDuration(knnResult.averageTime)).c_str());
  ImGui::Text(
      std::format("Последнее измерение: {}", Utils::FormatDuration(lastMeasurement))
          .c_str());
}

void EvaluationResults(AppState& state) {
  const auto& evState = state.m_EvaluationState;
  const auto& knnResult = evState.knnResult;
  ImGui::Text("Результаты тестирования:");
  ImGui::Text(
      std::format("AVG: {}", Utils::FormatDuration(knnResult.averageTime)).c_str());
  if (ImGui::Button("Сохранить результаты")) {
    Controllers::SaveEvaluationResults(state);
  }
}
}  // namespace Views