#pragma once
#include <filesystem>
#include <format>
#include "imgui.h"

#include "controllers/test_n.hpp"
#include "state.hpp"

namespace Views {

void TestNSetup(TestNSetupState& state);
void TestNProgress(TestNProgressState& state);
void TestNResults();

void TestN(bool& running, TestNState& state) {
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  ImGui::Begin(
      "Main window", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
  switch (state.phase.load()) {
    case TestNPhase::Setup:
      TestNSetup(state.setup);
      break;
    case TestNPhase::Progress:
      TestNProgress(state.progress);
      break;
    case TestNPhase::Results:
      TestNResults();
      break;
    default:
      std::unreachable();
  }
  ImGui::End();
}

void TestNSetup(TestNSetupState& state) {
  ImGui::Text("Тестирование kNN по количеству объектов");
  ImGui::Separator();

  ImGui::InputInt("M (maxEntries)", &state.maxEntries);
  ImGui::InputInt("m (minEntries)", &state.minEntries);
  ImGui::InputInt("Эпох на измерение", &state.epochs);
  ImGui::InputInt("k для kNN", &state.k);

  state.maxEntries = std::max(state.maxEntries, 2);
  state.minEntries = std::max(1, std::min(state.minEntries, (state.maxEntries + 1) / 2));
  state.epochs     = std::max(state.epochs, 1);
  state.k          = std::max(state.k, 1);

  ImGui::Spacing();
  ImGui::Text("Датасеты (%d файлов):", (int)state.selectedFiles.size());

  for (int i = 0; i < (int)state.selectedFiles.size(); ++i) {
    const std::string name =
        std::filesystem::path(state.selectedFiles[i]).filename().string();
    ImGui::Text("  %s", name.c_str());
    ImGui::SameLine();
    if (ImGui::Button(std::format("Удалить##f{}", i).c_str())) {
      state.selectedFiles.erase(state.selectedFiles.begin() + i);
      --i;
    }
  }

  if (ImGui::Button("Добавить файлы"))
    Controllers::SelectTestNFiles(state);

  if (!state.selectedFiles.empty()) {
    if (ImGui::Button("Очистить список"))
      state.selectedFiles.clear();
  }

  ImGui::Spacing();
  ImGui::Text("Измерений: %d", state.CalculateMeasurements());

  ImGui::Spacing();
  ImGui::BeginDisabled(state.selectedFiles.empty());
  if (ImGui::Button("Начать тестирование"))
    Controllers::StartTestN();
  ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Назад"))
    AppState::instance().SetCurrentState(State::MainMenu);
}

void TestNProgress(TestNProgressState& state) {
  const int done       = state.done.load();
  const int total      = state.total.load();
  const int epochsDone = state.epochsDone.load();
  const int epochs     = state.epochs.load();
  const int currentN   = state.currentN.load();

  if (total == 0) {
    ImGui::Text("Подготовка...");
    return;
  }

  const float epochFraction =
      static_cast<float>(epochsDone) / static_cast<float>(epochs);
  const float outerFraction =
      (static_cast<float>(done) + epochFraction) / static_cast<float>(total);

  ImGui::ProgressBar(
      outerFraction, ImVec2(0.0f, 0.0f),
      std::format("Датасет {}/{}  N={}",
                  std::min(done + 1, total), total, currentN).c_str());

  ImGui::ProgressBar(
      epochFraction, ImVec2(0.0f, 0.0f),
      std::format("Эпоха {}/{}", epochsDone, epochs).c_str());
}

void TestNResults() {
  ImGui::Text("Результаты сохранены в results/n/");
  ImGui::Separator();
  ImGui::Text("Формат файлов: {n}_{k}_{M}_{m}.npy");
  ImGui::Text("Каждый файл — 1D массив времён (нс) по эпохам.");

  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
    AppState::instance().m_TestNState.Reset();
  }
}

}  // namespace Views
