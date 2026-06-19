#pragma once
#include <algorithm>
#include <execution>
#include <thread>
#include "npy.hpp"

#include "error.hpp"
#include "state.hpp"
#include "utils.hpp"

namespace Controllers {

void LoadNpyFileThreadTarget(AppState& state, std::string filePath);

inline void LoadNpyFile() {
  AppState& state = AppState::instance();
  const char* selectedPath =
      tinyfd_openFileDialog("Select NPY file", "", 0, nullptr, nullptr, 0);
  if (!selectedPath || selectedPath[0] == '\0')
    return;

  state.SetStartFileReading();
  const std::string filePath(selectedPath);
  std::thread fileLoadingThread(LoadNpyFileThreadTarget,
                                std::ref(state), filePath);
  fileLoadingThread.detach();
}

inline void LoadNpyFileThreadTarget(AppState& state, std::string filePath) {
  try {
    npy::npy_data<double> data = Utils::ReadNpyAsDouble(filePath);

    if (data.shape.size() != 2 || data.shape[1] % 2 != 0)
      throw std::runtime_error(
          "Неверный формат NPY файла: ожидается 2D массив с чётным числом "
          "столбцов (пары координата + размер по каждой оси).");

    state.m_Objects.clear();
    state.m_Objects.reserve(data.shape[0]);

    std::for_each(std::execution::par, Utils::IndexIterator(0),
                  Utils::IndexIterator(data.shape[0]), [&data, &state](int64_t idx) {
                    const std::size_t i = static_cast<std::size_t>(idx);
                    const uint64_t id = static_cast<uint64_t>(i);
                    rtree::Rectangle<double> rect(data.shape[1] / 2);
                    for (size_t j = 0; j < data.shape[1]; ++j)
                      rect.size[j] = data.data[i * data.shape[1] + j];
                    const rtree::Object<double> obj(id, rect);
                    {
                      std::lock_guard<std::mutex> lock(state.m_Mutex);
                      state.m_Objects.push_back(std::move(obj));
                    }
                  });
    state.SetCurrentState(State::MainMenu);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.SetCurrentState(State::MainMenu);
  }
}
}  // namespace Controllers