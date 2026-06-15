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

// Читает NPY-файл с произвольным числовым типом и приводит данные к double.
inline npy::npy_data<double> ReadNpyAsDouble(const std::string& filePath) {
  // Сначала определяем реальный тип данных из заголовка.
  std::ifstream headerStream(filePath, std::ifstream::binary);
  if (!headerStream)
    throw std::runtime_error("io error: failed to open a file.");
  const npy::header_t header =
      npy::parse_header(npy::read_header(headerStream));
  headerStream.close();

  // Читаем данные исходным типом и конвертируем в double.
  auto convert = [&](auto sample) -> npy::npy_data<double> {
    using Scalar = decltype(sample);
    npy::npy_data<Scalar> raw = npy::read_npy<Scalar>(filePath);
    npy::npy_data<double> out;
    out.shape = raw.shape;
    out.fortran_order = raw.fortran_order;
    out.data.assign(raw.data.begin(), raw.data.end());
    return out;
  };

  const char kind = header.dtype.kind;
  const unsigned int size = header.dtype.itemsize;

  if (kind == 'f') {
    if (size == sizeof(float)) return convert(float{});
    if (size == sizeof(double)) return convert(double{});
    if (size == sizeof(long double)) return convert(static_cast<long double>(0));
  } else if (kind == 'i') {
    if (size == sizeof(int16_t)) return convert(int16_t{});
    if (size == sizeof(int32_t)) return convert(int32_t{});
    if (size == sizeof(int64_t)) return convert(int64_t{});
    if (size == sizeof(int8_t)) return convert(int8_t{});
  } else if (kind == 'u') {
    if (size == sizeof(uint16_t)) return convert(uint16_t{});
    if (size == sizeof(uint32_t)) return convert(uint32_t{});
    if (size == sizeof(uint64_t)) return convert(uint64_t{});
    if (size == sizeof(uint8_t)) return convert(uint8_t{});
  }

  throw std::runtime_error(
      "Неподдерживаемый тип данных NPY: '" + header.dtype.str() +
      "'. Ожидается числовой тип (float или integer).");
}

inline void LoadNpyFileThreadTarget(AppState& state, std::string filePath) {
  try {
    npy::npy_data<double> data = ReadNpyAsDouble(filePath);

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