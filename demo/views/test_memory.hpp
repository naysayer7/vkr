#pragma once
#include <format>
#include "imgui.h"

#include "controllers/test_memory.hpp"
#include "state.hpp"
#include "utils.hpp"

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
  ImGui::InputInt2("m (minEntries) диапазон", state.minObjects);

  // Ограничения: значения >= 1, min <= max
  state.maxObjects[0] = std::max(state.maxObjects[0], 2);
  state.maxObjects[1] = std::max(state.maxObjects[1], state.maxObjects[0]);
  state.minObjects[0] = std::max(state.minObjects[0], 1);
  state.minObjects[1] = std::max(state.minObjects[1], state.minObjects[0]);

  ImGui::Text(
      std::format("Количество тестов: {}",
                  Utils::CalculateRunsCount(state.minObjects, state.maxObjects))
          .c_str());

  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestMemory();
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

  ImGui::Text(std::format("M: {}  m: {}", params.maxEntries, params.minEntries)
                  .c_str());
}

inline void TestMemoryResults(TestMemoryResultState& state) {
  ImGui::Text(
      std::format("Результаты сохранены в {}", state.savedFilename).c_str());
  ImGui::Separator();

  if (ImGui::BeginTable("mem_results", 3,
                        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                            ImGuiTableFlags_BordersV)) {
    ImGui::TableSetupColumn("M");
    ImGui::TableSetupColumn("m");
    ImGui::TableSetupColumn("Память (байт)");
    ImGui::TableHeadersRow();

    for (const auto& [params, mem] : state.memorySizes) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text(std::to_string(params.maxEntries).c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text(std::to_string(params.minEntries).c_str());
      ImGui::TableSetColumnIndex(2);
      ImGui::Text(std::to_string(mem).c_str());
    }
    ImGui::EndTable();
  }

  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
    AppState::instance().m_TestMemoryState.Reset();
  }
}

}  // namespace Views
