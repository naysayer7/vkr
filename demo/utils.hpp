#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "npy.hpp"
#include "rtree.h"

namespace Utils {
inline std::string FormatMemorySize(size_t bytes) {
  const char* suffixes[] = {"B", "KB", "MB", "GB"};
  size_t suffixIndex = 0;
  double count = static_cast<double>(bytes);
  while (count >= 1024.0 && suffixIndex < 3) {
    count /= 1024.0;
    ++suffixIndex;
  }
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.2f %s", count, suffixes[suffixIndex]);
  return std::string(buffer);
}

inline std::string FormatDuration(const Measures::Duration& duration) {
  using namespace std::chrono;
  auto ms = duration_cast<milliseconds>(duration).count();
  auto us = duration_cast<microseconds>(duration).count() % 1000;
  auto ns = duration_cast<nanoseconds>(duration).count() % 1000;
  return std::format("{} ms {} us {} ns", ms, us, ns);
}

inline std::string FormatDuration(const double& duration) {
  return FormatDuration(Measures::Duration(duration));
}

inline int CalculateRunsCount(const int maxObjects[2]) {
  return std::max(0, maxObjects[1] - maxObjects[0] + 1);
}

class IndexIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = int64_t;
  using difference_type = int64_t;
  using pointer = const int64_t*;
  using reference = const int64_t&;

  IndexIterator() = default;
  explicit IndexIterator(value_type index) : index_(index) {}

  reference operator*() const { return index_; }
  pointer operator->() const { return &index_; }

  IndexIterator& operator++() { ++index_; return *this; }
  IndexIterator operator++(int) { IndexIterator tmp = *this; ++index_; return tmp; }
  IndexIterator& operator--() { --index_; return *this; }
  IndexIterator operator--(int) { IndexIterator tmp = *this; --index_; return tmp; }

  IndexIterator& operator+=(difference_type n) { index_ += n; return *this; }
  IndexIterator& operator-=(difference_type n) { index_ -= n; return *this; }

  IndexIterator operator+(difference_type n) const { return IndexIterator(index_ + n); }
  IndexIterator operator-(difference_type n) const { return IndexIterator(index_ - n); }
  difference_type operator-(const IndexIterator& other) const { return index_ - other.index_; }

  value_type operator[](difference_type n) const { return index_ + n; }

  bool operator==(const IndexIterator& other) const { return index_ == other.index_; }
  bool operator!=(const IndexIterator& other) const { return index_ != other.index_; }
  bool operator<(const IndexIterator& other) const { return index_ < other.index_; }
  bool operator>(const IndexIterator& other) const { return index_ > other.index_; }
  bool operator<=(const IndexIterator& other) const { return index_ <= other.index_; }
  bool operator>=(const IndexIterator& other) const { return index_ >= other.index_; }

  friend IndexIterator operator+(difference_type n, const IndexIterator& it) {
    return IndexIterator(n + it.index_);
  }

 private:
  value_type index_{0};
};

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

// Фиксированное зерно ГПСЧ для hold-out разбиения. Одинаковое во всех тестах,
// чтобы разбиение было воспроизводимым и совпадало между реализациями kNN.
constexpr std::uint32_t kQuerySeed = 12345;

// Точное число запросов для hold-out: round(percent% * n), ограниченное снизу 1 и
// сверху n-1 (хотя бы один запрос и один индексируемый объект). Для n < 2 → 0.
// Единый источник истины для контроллеров и UI.
inline std::size_t HoldoutQueryCount(std::size_t n, int percent) {
  if (n < 2)
    return 0;
  return std::clamp<std::size_t>(
      static_cast<std::size_t>(std::llround(percent / 100.0 * n)), 1, n - 1);
}

// Результат hold-out разбиения: указатели в исходный вектор объектов (без копий).
// indexed годится напрямую для RTree::BulkLoad.
template <typename T>
struct HoldoutSplit {
  std::vector<const rtree::Object<T>*> indexed;
  std::vector<const rtree::Object<T>*> queries;
};

// Случайно откладывает percent% объектов в роли запросов (hold-out), остальные —
// индексируемый набор; точное число вычисляется через HoldoutQueryCount.
// Фиксированный seed даёт одинаковое разбиение. objects не модифицируется
// (хранятся лишь указатели в него).
template <typename T>
inline HoldoutSplit<T> SplitHoldout(const std::vector<rtree::Object<T>>& objects,
                                    int percent, std::uint32_t seed) {
  const std::size_t n = objects.size();
  const std::size_t count = HoldoutQueryCount(n, percent);

  std::vector<std::size_t> indices(n);
  std::iota(indices.begin(), indices.end(), std::size_t{0});
  std::vector<std::size_t> chosen;
  chosen.reserve(count);
  std::sample(indices.begin(), indices.end(), std::back_inserter(chosen), count,
              std::mt19937(seed));

  std::vector<char> isQuery(n, 0);
  for (std::size_t c : chosen)
    isQuery[c] = 1;

  HoldoutSplit<T> out;
  out.indexed.reserve(n - count);
  out.queries.reserve(count);
  for (std::size_t j = 0; j < n; ++j)
    (isQuery[j] ? out.queries : out.indexed).push_back(&objects[j]);
  return out;
}

}  // namespace Utils