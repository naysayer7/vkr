#pragma once
#include <chrono>
#include <format>
#include <string>

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

}  // namespace Utils