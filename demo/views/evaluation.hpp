#pragma once
#include <chrono>
#include <format>
#include "imgui.h"

#include "contollers/evaluation.hpp"
#include "state.hpp"
#include "utils.hpp"

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
  auto& params = state.m_Params;
  auto& evState = state.m_EvaluationState;
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество запросов", &evState.numRuns);
  ImGui::InputInt("k для kNN", &evState.k);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
      ImGuiTableFlags_Hideable;
  ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 8);
  if (ImGui::BeginTable("table_params", 2, flags, outer_size)) {
    ImGui::TableSetupScrollFreeze(0, 1);  // Первая строка всегда видна
    ImGui::TableSetupColumn("min", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("max", ImGuiTableColumnFlags_None);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin((int)params.size());
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(std::format("{}", params[row].first).c_str(), 0, row);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(std::format("{}", params[row].second).c_str(), 1, row);
      }
    }
    ImGui::EndTable();
  }

  ImGui::InputInt2("R-tree parameters (maxEntries, minEntries)",
                   (int*)&state.m_RTreeParams);
  if (ImGui::Button("+")) {
    state.m_Params.emplace_back(state.m_RTreeParams.maxEntries,
                                state.m_RTreeParams.minEntries);
  }

  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate(state);
  }
}

void EvaluationProgress(AppState& state) {
  const auto& evState = state.m_EvaluationState;
  const auto& knnResult = evState.knnResult;
  const auto& lastMeasurement =
      knnResult.times.empty() ? 0.0 : knnResult.times.back().second.back();

  ImGui::ProgressBar(
      (float)evState.run / (float)evState.numRuns, ImVec2(0.0f, 0.0f),
      std::format("Тестирование... {}/{}", evState.run, evState.numRuns)
          .c_str());

  ImGui::Text(std::format("Последнее измерение: {}",
                          Utils::FormatDuration(lastMeasurement))
                  .c_str());
}

void EvaluationResults(AppState& state) {
  const auto& evState = state.m_EvaluationState;
  const auto& knnResult = evState.knnResult;
  ImGui::Text("Результаты тестирования сохранены");
  if (ImGui::Button("Назад в меню")) {
    state.m_EvaluationState.Reset();
    state.SetCurrentState(State::MainMenu);
  }
}
}  // namespace Views