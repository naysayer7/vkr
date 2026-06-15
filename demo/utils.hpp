#pragma once
#include <chrono>
#include <format>
#include <string>

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

inline int CalculateRunsCount(const int minObjects[2],
                              const int maxObjects[2]) {
  int runs = 0;
  for (int M = maxObjects[0]; M <= maxObjects[1]; ++M)
    for (int m = minObjects[0]; m <= std::min(minObjects[1], (M + 1) / 2); ++m)
      runs++;
  return runs;
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

}  // namespace Utils