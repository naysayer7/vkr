#pragma once
#include <functional>
#include <string>
#include <vector>

#include "imgui.h"
#include "state.hpp"

namespace Views {

// Единый экран результатов для всех тестов: общий заголовок, разделители и
// кнопка возврата. Конкретный тест передаёт строки с описанием сохранённых
// данных и, при необходимости, дополнительный блок (например, таблицу).
//
//   lines        — строки описания (расположение, формат файлов и т.п.)
//   onBack       — сброс состояния конкретного теста при выходе в меню
//   extraContent — необязательный блок, отрисовываемый перед кнопкой
inline void RenderTestResults(
    const std::vector<std::string>& lines,
    const std::function<void()>& onBack,
    const std::function<void()>& extraContent = nullptr) {
  ImGui::Text("Результаты тестирования сохранены");
  ImGui::Separator();

  for (const auto& line : lines)
    ImGui::TextUnformatted(line.c_str());

  if (extraContent) {
    ImGui::Spacing();
    extraContent();
  }

  ImGui::Separator();
  if (ImGui::Button("Назад в меню")) {
    AppState::instance().SetCurrentState(State::MainMenu);
    if (onBack)
      onBack();
  }
}

}  // namespace Views
