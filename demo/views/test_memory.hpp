#pragma once
#include <format>
#include "imgui.h"

#include "controllers/test_memory.hpp"
#include "state.hpp"
#include "utils.hpp"
#include "views/test_results.hpp"

namespace Views {

void TestMemorySetup(TestMemorySetupState& state);
void TestMemoryProgress(TestMemoryProgressState& state);
void TestMemoryResults(TestMemoryResultState& state);

inline void TestMemory(bool& running, TestMemoryState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestMemoryPhase::Setup:
      TestMemorySetup(state.setup);
      break;
    case TestMemoryPhase::Progress:
      TestMemoryProgress(state.progress);
      break;
    case TestMemoryPhase::Results:
      TestMemoryResults(state.result);
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

inline void TestMemorySetup(TestMemorySetupState& state) {
  ImGui::Text("Тестирование использования памяти");
  ImGui::Separator();
  ImGui::InputInt2("M (maxEntries) диапазон", state.maxObjects);

  // Ограничения: значения >= 2, min <= max
  state.maxObjects[0] = std::max(state.maxObjects[0], 2);
  state.maxObjects[1] = std::max(state.maxObjects[1], state.maxObjects[0]);

  ImGui::Text(std::format("Количество тестов: {}",
                          Utils::CalculateRunsCount(state.maxObjects))
                  .c_str());

  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestMemory();

  ImGui::SameLine();
  if (ImGui::Button("Назад"))
    AppState::instance().SetCurrentState(State::MainMenu);
}

inline void TestMemoryProgress(TestMemoryProgressState& state) {
  const int done = state.done.load();
  const int total = state.total.load();
  const RTreeParameters params = state.currentParams.load();

  if (total == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  ImGui::ProgressBar(static_cast<float>(done) / static_cast<float>(total),
                     ImVec2(0.0f, 0.0f),
                     std::format("Тест {}/{}", done, total).c_str());

  ImGui::Text(std::format("M: {}", params.maxEntries).c_str());
}

inline void TestMemoryResults(TestMemoryResultState& state) {
  RenderTestResults(
      {"Расположение: " + state.savedFilename,
       "Формат: 2D массив, столбцы [M, память (байт)]"},
      [] { AppState::instance().m_TestMemoryState.Reset(); },
      [&state] {
        if (ImGui::BeginTable("mem_results", 2,
                              ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_BordersV)) {
          ImGui::TableSetupColumn("M");
          ImGui::TableSetupColumn("Память (байт)");
          ImGui::TableHeadersRow();

          for (const auto& [params, mem] : state.memorySizes) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(std::to_string(params.maxEntries).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(std::to_string(mem).c_str());
          }
          ImGui::EndTable();
        }
      });
}

}  // namespace Views
