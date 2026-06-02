#pragma once
#include <algorithm>
#include <execution>
#include <ranges>
#include <thread>
#include "npy.hpp"

#include "error.hpp"
#include "state.hpp"

namespace Controllers {

class IndexIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = int64_t;
  using difference_type = int64_t;
  using pointer = int64_t*;
  using reference = int64_t&;

  IndexIterator() = default;
  IndexIterator(value_type index) : index_(index) {}
  const value_type& operator*() const { return index_; }
  void operator++() { ++index_; }
  bool operator!=(const IndexIterator& lhs) const {
    return index_ != lhs.index_;
  }
  const value_type operator+(const IndexIterator& lhs) const {
    return index_ + lhs.index_;
  }
  value_type operator-(const IndexIterator& lhs) const {
    return index_ - lhs.index_;
  }

 private:
  value_type index_;
};

void LoadNpyFileThreadTarget(AppState& state, std::string filePath);
void LoadNpyFile() {
  AppState& state = AppState::instance();
  const char* selectedPath =
      tinyfd_openFileDialog("Select NPY file", "", 0, nullptr, nullptr, 0);
  if (!selectedPath || selectedPath[0] == '\0')
    return;

  state.SetStartFileReading();
  const std::string filePath(selectedPath);
  std::thread fileLoadingThread(LoadNpyFileThreadTarget, std::ref(state),
                                filePath);
  fileLoadingThread.detach();
}

void LoadNpyFileThreadTarget(AppState& state, std::string filePath) {
  try {
    npy::npy_data data = npy::read_npy<float>(filePath);

    if (data.shape.size() != 2 || data.shape[1] % 2 != 0)
      throw std::runtime_error(
          "Неверный формат NPY файла: ожидается 2D массив с чётным числом "
          "столбцов (пары координата + размер по каждой оси).");

    state.m_Objects.clear();
    state.m_Objects.reserve(data.shape[0]);

    std::for_each(std::execution::par, IndexIterator(0),
                  IndexIterator(data.shape[0]),
                  [&data, &state](IndexIterator iter) {
                    const std::size_t i = static_cast<std::size_t>(*iter);
                    const uint64_t id = static_cast<uint64_t>(i);
                    rtree::Rectangle<float> rect(data.shape[1] / 2);
                    for (size_t j = 0; j < data.shape[1]; ++j)
                      rect.size[j] = data.data[i * data.shape[1] + j];
                    const rtree::Object<float> obj(id, rect);
                    {
                      std::lock_guard<std::mutex> lock(state.m_Mutex);
                      state.m_Objects.push_back(std::move(obj));
                    }
                  });
    state.BuildRTree();
    state.SetCurrentState(State::MainMenu);
  } catch (const std::exception& e) {
    Error::Show(e.what());
    state.SetCurrentState(State::MainMenu);
  }
}
}  // namespace Controllers