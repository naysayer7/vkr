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

void TestKnn(bool& running, TestKnnState& state) {
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

void TestKnnSetup(TestKnnSetupState& state) {
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

void TestKnnProgress(TestKnnProgressState& state) {
  const int runsDone = state.runsDone.load();
  const int runs = state.runs.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs = state.epochs.load();
  const RTreeParameters params = state.currentParams.load();

  if (runs == 0 || epochs == 0) {
    ImGui::Text("Подготовка к тестированию...");
    return;
  }

  ImGui::ProgressBar(
      (float)runsDone / (float)runs, ImVec2(0.0f, 0.0f),
      std::format("Тестирование {}/{}", runsDone, runs).c_str());

  ImGui::Text(std::format("m: {}", params.minEntries).c_str());
  ImGui::Text(std::format("M: {}", params.maxEntries).c_str());

  ImGui::ProgressBar(
      (float)epochsDone / (float)epochs, ImVec2(0.0f, 0.0f),
      std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

void TestKnnResults(TestKnnResultState& state) {
  ImGui::Text("Результаты тестирования сохранены");
  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
    AppState::instance().m_TestKnnState.Reset();
  }
}
}  // namespace Views