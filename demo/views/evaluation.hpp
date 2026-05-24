#pragma once
#include <chrono>
#include <format>
#include <stdexcept>
#include "imgui.h"

#include "contollers/evaluation.hpp"
#include "state.hpp"
#include "utils.hpp"

namespace Views {

void EvaluationSetup(EvaluationSetupState& state);
void EvaluationProgress(EvaluationProgressState& state);
void EvaluationResults(EvaluationResultState& state);

void Evaluation(bool& running, EvaluationState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase) {
    case EvaluationPhase::Setup:
      EvaluationSetup(state.setup);
      break;
    case EvaluationPhase::Progress:
      EvaluationProgress(state.progress);
      break;
    case EvaluationPhase::Results:
      EvaluationResults(state.result);
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

void EvaluationSetup(EvaluationSetupState& state) {
  auto& params = state.params;
  ImGui::Text("Настройки для тестирования");
  ImGui::InputInt("Количество эпох", &state.epochs);
  ImGui::InputInt("k для kNN", &state.k);

  /* static ImGuiTableFlags flags =
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
        ImGui::Text(std::format("{}", params[row].minEntries).c_str(), 0, row);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(std::format("{}", params[row].maxEntries).c_str(), 1, row);
      }
    }
    ImGui::EndTable();
  }

  ImGui::InputInt2("Параметры R-дерева", (int*)&state.paramsInput);
  state.paramsInput[0] = std::max(state.paramsInput[0], 1);
  state.paramsInput[1] = std::max(state.paramsInput[1], 2 *
  state.paramsInput[0]);

  if (ImGui::Button("+")) {
    state.params.emplace_back(
        RTreeParameters{state.paramsInput[0], state.paramsInput[1]});
  } */

  ImGui::InputInt2("MIN", (int*)&state.minObjects);
  ImGui::InputInt2("MAX", (int*)&state.maxObjects);
  ImGui::Text(
      std::format("Количество тестов: {}", state.CalculateRunsCount()).c_str());

  if (ImGui::Button("Начать тестирование")) {
    Controllers::Evaluate();
  }
}

void EvaluationProgress(EvaluationProgressState& state) {
  ImGui::ProgressBar(
      (float)state.runsDone / (float)state.runs, ImVec2(0.0f, 0.0f),
      std::format("Тестирование {}/{}", state.runsDone, state.runs).c_str());


  
  ImGui::Text(
    std::format("m: {}", state.currentParams.minEntries).c_str()
  );
  ImGui::Text(
    std::format("M: {}", state.currentParams.maxEntries).c_str()
  );

  ImGui::ProgressBar(
      (float)state.epochsDone / (float)state.epochs, ImVec2(0.0f, 0.0f),
      std::format("Эпоха {}/{}", state.epochsDone, state.epochs).c_str());
}

void EvaluationResults(EvaluationResultState& state) {
  ImGui::Text("Результаты тестирования сохранены");
  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
  }
}
}  // namespace Views